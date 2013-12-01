/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#ifdef HAS_DX

#include <d3d9.h>
#include <dxva2api.h>
#include "utils/log.h"
#include "system.h"
#include "CIntelS3DDevice.h"
#include "threads/SingleLock.h"
#include "guilib/GUIWindowManager.h"
#include "settings/Settings.h"

// Dynamic loading of intel_s3d method.
typedef IGFXS3DControl* (__stdcall *CreateIGFXS3DControlExPtr)(void);
static CreateIGFXS3DControlExPtr g_CreateIGFXS3DControlEx;

// Dynamic loading of DXVA2 methods.
typedef HRESULT (__stdcall *DXVA2CreateDirect3DDeviceManager9Ptr)(UINT *pResetToken, IDirect3DDeviceManager9 **ppDXVAManager);
static DXVA2CreateDirect3DDeviceManager9Ptr g_DXVA2CreateDirect3DDeviceManager9;
typedef HRESULT (__stdcall *DXVA2CreateVideoServicePtr)(IDirect3DDevice9* pDD, REFIID riid, void** ppService);
static DXVA2CreateVideoServicePtr g_DXVA2CreateVideoService;

static bool LoadS3D()
{
  static CCriticalSection section;
  static HMODULE          hDXVA2Dll;
  static HMODULE          hS3DDll;

  CSingleLock lock(section);
  if(hDXVA2Dll == NULL)
    hDXVA2Dll = LoadLibraryEx("dxva2.dll", NULL, 0);
  if(hDXVA2Dll == NULL)
    return false;

  g_DXVA2CreateDirect3DDeviceManager9 = (DXVA2CreateDirect3DDeviceManager9Ptr)GetProcAddress(hDXVA2Dll, "DXVA2CreateDirect3DDeviceManager9");
  if(g_DXVA2CreateDirect3DDeviceManager9 == NULL)
    return false;

  g_DXVA2CreateVideoService = (DXVA2CreateVideoServicePtr)GetProcAddress(hDXVA2Dll, "DXVA2CreateVideoService");
  if(g_DXVA2CreateVideoService == NULL)
    return false;

  if(hS3DDll == NULL)
    hS3DDll = LoadLibraryEx("intel_s3d.dll", NULL, 0);
  if(hS3DDll == NULL)
    return false;

  g_CreateIGFXS3DControlEx = (CreateIGFXS3DControlExPtr)GetProcAddress(hS3DDll, "CreateIGFXS3DControlEx");
  if(g_CreateIGFXS3DControlEx == NULL)
    return false;

  return true;
}

CIntelS3DDevice::CIntelS3DDevice(IDirect3D9Ex* pD3D) :IS3DDevice(pD3D),
    m_resetToken(0),
    m_restoreFFScreen(false),
    m_pS3DControl(NULL),
    m_pDeviceManager9(NULL),
    m_pProcessService(NULL),
    m_pProcessLeft(NULL),
    m_pProcessRight(NULL),
    m_pRenderSurface(NULL)
{
    m_supported = PreInit();
}

CIntelS3DDevice::~CIntelS3DDevice() 
{
  UnInit();

  SAFE_RELEASE(m_pRenderSurface);
  SAFE_RELEASE(m_pDeviceManager9);
  SAFE_DELETE(m_pS3DControl);
}

void CIntelS3DDevice::UnInit(void) 
{
  SAFE_RELEASE(m_Sample.SrcSurface);
  SAFE_RELEASE(m_pProcessService);
  SAFE_RELEASE(m_pProcessRight);
  SAFE_RELEASE(m_pProcessLeft);

  m_initialized  = false;
}

// 
bool CIntelS3DDevice::CorrectPresentParams(D3DPRESENT_PARAMETERS *pD3DPP, bool stereo)
{
  if (stereo)
  {
    // NOTE overlay may be used only in windowed mode
    pD3DPP->Windowed = true;
    pD3DPP->SwapEffect = D3DSWAPEFFECT_OVERLAY;

    // Mark the back buffer lockable if software DXVA2 could be used.
    // This is because software DXVA2 device requires a lockable render target
    // for the optimal performance.
    pD3DPP->Flags |= D3DPRESENTFLAG_LOCKABLE_BACKBUFFER;
  }

  return true;
}

// Returns true if S3D is supported by the platform and exposes supported display modes 
bool CIntelS3DDevice::GetS3DCaps(S3D_CAPS *pCaps)
{
  IGFX_S3DCAPS S3DCaps;

  if (FAILED(m_pS3DControl->GetS3DCaps(&S3DCaps)) && S3DCaps.ulNumEntries > 0)
    return false;

  pCaps->uiNumEntries = S3DCaps.ulNumEntries;
  pCaps->SupportedModes = new S3D_DISPLAY_MODE[S3DCaps.ulNumEntries];

  unsigned long pref_idx = 0;
  for (unsigned long  i = 0; i < S3DCaps.ulNumEntries; i++)
  {
    S3D_DISPLAY_MODE* mode = &pCaps->SupportedModes[i];
    mode->uiWidth = S3DCaps.S3DSupportedModes[i].ulResWidth;
    mode->uiHeight = S3DCaps.S3DSupportedModes[i].ulResHeight;
    mode->uiRefreshRate = S3DCaps.S3DSupportedModes[i].ulRefreshRate;
  }

  return true;
}

// 
bool CIntelS3DDevice::PreInit()
{
  if (!LoadS3D())
  {
    CLog::Log(LOGWARNING, __FUNCTION__" - Loading s3d failure");
    return false;
  }

  CLog::Log(LOGDEBUG, __FUNCTION__" - trying create GFXS3D control...");

  m_pS3DControl = g_CreateIGFXS3DControlEx();

  if (FAILED(m_pS3DControl != NULL))
  {
    CLog::Log(LOGWARNING, __FUNCTION__" - failed creating S3D control");
    return false;
  }

  if (FAILED(m_pS3DControl->GetS3DCaps(&m_S3DCaps)) && m_S3DCaps.ulNumEntries > 0)
  {
    CLog::Log(LOGWARNING, __FUNCTION__" - failed get S3D caps. Platform not support S3D");
    return false;
  }

  CLog::Log(LOGDEBUG, __FUNCTION__" - Intel S3D is supported");

  unsigned long pref_idx = 0;
  for (unsigned long  i = 0; i < m_S3DCaps.ulNumEntries; i++)
  {
    CLog::Log(LOGDEBUG, __FUNCTION__" - Supported stereo mode: %lux%lu @ %lu", m_S3DCaps.S3DSupportedModes[i].ulResWidth, 
                                                                                m_S3DCaps.S3DSupportedModes[i].ulResHeight, 
                                                                                m_S3DCaps.S3DSupportedModes[i].ulRefreshRate);
    // TODO select prefered mode with better match to current screen resoulution
    if (Less(m_S3DCaps.S3DSupportedModes[pref_idx], m_S3DCaps.S3DSupportedModes[i])) 
      pref_idx = i;
  }

  m_S3DPrefMode = m_S3DCaps.S3DSupportedModes[pref_idx];

  // GetS3DCaps returns all supported modes and max is 1920x1080 @ 60
  // for 1080p needs override to 24 fps
  // TODO find best solution
  if (m_S3DPrefMode.ulResHeight == 1080)
    m_S3DPrefMode.ulRefreshRate = 24L;

  CLog::Log(LOGDEBUG, __FUNCTION__" - Select prefered stereoscopic mode: %lux%lu @ %lu", m_S3DPrefMode.ulResWidth, m_S3DPrefMode.ulResHeight, m_S3DPrefMode.ulRefreshRate);

  if (!CheckOverlaySupport(m_S3DPrefMode.ulResWidth, m_S3DPrefMode.ulResHeight, D3DFMT_X8R8G8B8))
  {
    CLog::Log(LOGWARNING, __FUNCTION__" - Overlay not supported. 3D is not possible.");
    return false;
  }

  if (FAILED(g_DXVA2CreateDirect3DDeviceManager9(&m_resetToken, &m_pDeviceManager9))) 
  {
    CLog::Log(LOGWARNING, __FUNCTION__" - creating D3D device manager failed");
    return false;
  }

  return true;
}

bool CIntelS3DDevice::OnDeviceCreated(IDirect3DDevice9Ex* pD3DDevice)
{
  D3DDISPLAYMODE d3dmodeTemp;
  pD3DDevice->GetDisplayMode(0, &d3dmodeTemp);

  unsigned int uiWidth = d3dmodeTemp.Width;
  unsigned int uiHeight = d3dmodeTemp.Height;
  unsigned int uiRefreshRate = d3dmodeTemp.RefreshRate;

  DXVA2_AYUVSample16 color = {0x8000, 0x8000, 0x1000, 0xffff};

  DXVA2_ExtendedFormat format = { DXVA2_SampleProgressiveFrame,           // SampleFormat
                                  DXVA2_VideoChromaSubsampling_MPEG2,     // VideoChromaSubsampling
                                  DXVA2_NominalRange_Normal,              // NominalRange
                                  DXVA2_VideoTransferMatrix_BT709,        // VideoTransferMatrix
                                  DXVA2_VideoLighting_dim,                // VideoLighting
                                  DXVA2_VideoPrimaries_BT709,             // VideoPrimaries
                                  DXVA2_VideoTransFunc_709                // VideoTransferFunction            
                                };

  ZeroMemory(&m_VideoDesc, sizeof(m_VideoDesc));
  ZeroMemory(&m_BltParams, sizeof(m_BltParams));
  ZeroMemory(&m_Sample, sizeof(m_Sample));

  // init m_VideoDesc structure
  MEMCPY_VAR(m_VideoDesc.SampleFormat, &format, sizeof(DXVA2_ExtendedFormat));
  m_VideoDesc.SampleWidth                  = uiWidth;
  m_VideoDesc.SampleHeight                 = uiHeight;
  m_VideoDesc.InputSampleFreq.Numerator    = uiRefreshRate;
  m_VideoDesc.InputSampleFreq.Denominator  = 1;
  m_VideoDesc.OutputFrameFreq.Numerator    = uiRefreshRate;
  m_VideoDesc.OutputFrameFreq.Denominator  = 1;

  // init m_BltParams structure
  MEMCPY_VAR(m_BltParams.DestFormat, &format, sizeof(DXVA2_ExtendedFormat));
  MEMCPY_VAR(m_BltParams.BackgroundColor, &color, sizeof(DXVA2_AYUVSample16));

  RECT rect = {0, 0, uiWidth, uiHeight};

  m_BltParams.BackgroundColor = color;
  m_BltParams.DestFormat      = format;
  m_BltParams.TargetRect      = rect;
  m_BltParams.Alpha = DXVA2_Fixed32OpaqueAlpha();

  // init m_Sample structure
  m_Sample.Start = 0;
  m_Sample.End = 1;
  m_Sample.SampleFormat = format;
  m_Sample.PlanarAlpha.Fraction = 0;
  m_Sample.PlanarAlpha.Value = 1;   
  m_Sample.SrcRect = m_Sample.DstRect = rect;

  // Reset the D3DDeviceManager with the new device 
  m_pDeviceManager9->ResetDevice(((IDirect3DDevice9Ex*)pD3DDevice), m_resetToken);

  m_pS3DControl->SetDevice(m_pDeviceManager9);

  // Create DXVA2 Video Processor Service.
  g_DXVA2CreateVideoService(pD3DDevice, IID_IDirectXVideoProcessorService, (void**)&m_pProcessService); 

  // Activate L channel
  m_pS3DControl->SelectLeftView();
  // Create VPP device for the L channel
  m_pProcessService->CreateVideoProcessor(DXVA2_VideoProcProgressiveDevice, &m_VideoDesc, D3DFMT_X8R8G8B8, 1, &m_pProcessLeft);

  // Activate R channel
  m_pS3DControl->SelectRightView();
  // Create VPP device for the R channel
  m_pProcessService->CreateVideoProcessor(DXVA2_VideoProcProgressiveDevice, &m_VideoDesc, D3DFMT_X8R8G8B8, 1, &m_pProcessRight);

  // create render target for channels
  pD3DDevice->CreateRenderTarget(uiWidth, uiHeight, D3DFMT_X8R8G8B8, D3DMULTISAMPLE_NONE, 0, TRUE, &m_Sample.SrcSurface, NULL);

  // back to L channel (for possible issue with dxva renderer)
  m_pS3DControl->SelectLeftView();

  m_pD3DDevice = pD3DDevice;
  pD3DDevice->AddRef();

  m_initialized = true;
  return true;
}

// Switch the monitor to 3D mode
// Call with NULL to use current display mode
bool CIntelS3DDevice::SwitchTo3D(S3D_DISPLAY_MODE *pMode)
{
  if (g_graphicsContext.IsFullScreenRoot() && !CSettings::Get().GetBool("videoscreen.fakefullscreen"))
  {
    CSettings::Get().SetBool("videoscreen.fakefullscreen", true);
    g_graphicsContext.LockFakeFullScreen(true);
    m_restoreFFScreen = true;
  }

  if (pMode)
  {
    IGFX_DISPLAY_MODE mode = { pMode->uiWidth, pMode->uiHeight, pMode->uiRefreshRate };
    return !FAILED(m_pS3DControl->SwitchTo3D(&mode));
  }

  return !FAILED(m_pS3DControl->SwitchTo3D(&m_S3DPrefMode));
}

// Switch the monitor back to 2D mode
// Call with NULL to use current display mode
bool CIntelS3DDevice::SwitchTo2D(S3D_DISPLAY_MODE *pMode)
{
  HRESULT hr;
  if (pMode)
  {
    IGFX_DISPLAY_MODE mode = { pMode->uiWidth, pMode->uiHeight, pMode->uiRefreshRate };
    hr = m_pS3DControl->SwitchTo2D(&mode);
  }
  else
    hr = m_pS3DControl->SwitchTo2D(NULL);

  if (m_restoreFFScreen)
  {
    m_restoreFFScreen = false;
    g_graphicsContext.LockFakeFullScreen(false);
    CSettings::Get().SetBool("videoscreen.fakefullscreen", false);
  }

  return !FAILED(hr);
}
    
// Activate left view, requires device to be set
bool CIntelS3DDevice::SelectLeftView()
{
  // switch to  fake render target
  if (FAILED(m_pD3DDevice->GetRenderTarget(0, &m_pRenderSurface)))
    return false;

  return !FAILED(m_pD3DDevice->SetRenderTarget(0, m_Sample.SrcSurface));
}

// Activates right view, requires device to be set
bool CIntelS3DDevice::SelectRightView()
{
  // channel L completed render it to result surface
  return !FAILED(m_pProcessLeft->VideoProcessBlt(m_pRenderSurface, &m_BltParams, &m_Sample, 1, NULL));
}

// Activates right view, requires device to be set
bool CIntelS3DDevice::PresentFrame()
{
  // channel R completed render it and back true render traget
  if(FAILED(m_pProcessRight->VideoProcessBlt(m_pRenderSurface, &m_BltParams, &m_Sample, 1, NULL)))
    return false;

  HRESULT hr = m_pD3DDevice->SetRenderTarget(0, m_pRenderSurface);
  m_pRenderSurface->Release();

  return !FAILED(hr);
}

bool CIntelS3DDevice::Less(const IGFX_DISPLAY_MODE &l, const IGFX_DISPLAY_MODE& r)
{
    if (r.ulResWidth >= 0xFFFF || r.ulResHeight >= 0xFFFF || r.ulRefreshRate >= 0xFFFF)
        return false;

    if (l.ulResWidth < r.ulResWidth)
      return true;
    else if (l.ulResHeight < r.ulResHeight)
      return true;
    else if (l.ulRefreshRate < r.ulRefreshRate)
      return true;    
        
    return false;
}

bool CIntelS3DDevice::CheckOverlaySupport(int iWidth, int iHeight, D3DFORMAT dFormat)
{
    D3DCAPS9                      d3d9caps;
    D3DOVERLAYCAPS                d3doverlaycaps = {0};
    IDirect3D9ExOverlayExtension *d3d9overlay    = NULL;

    bool overlaySupported = false;

    memset(&d3d9caps, 0, sizeof(d3d9caps));
    if (FAILED(m_pD3D->GetDeviceCaps(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, &d3d9caps)) 
      || !(d3d9caps.Caps & D3DCAPS_OVERLAY))
    {
        overlaySupported = false;            
    }
    else
    {
      if (FAILED(m_pD3D->QueryInterface(IID_PPV_ARGS(&d3d9overlay))) || (d3d9overlay == NULL))
      {
        overlaySupported = false;
      }
      else
      {
        HRESULT hr = d3d9overlay->CheckDeviceOverlayType(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL,
                     iWidth,
                     iHeight,
                     dFormat, 
                     NULL,
                     D3DDISPLAYROTATION_IDENTITY, &d3doverlaycaps);

        overlaySupported = !FAILED(hr);
        SAFE_RELEASE(d3d9overlay);
      }
    }

    return overlaySupported;
}

#endif // HAS_DX
