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
#include "guilib/GUIWindowManager.h"
#include "settings/lib/Setting.h"
#include "settings/Settings.h"
#include "threads/SingleLock.h"
#include "windowing/WindowingFactory.h"
#include "CNvidiaS3DDevice.h"

#define CHECK(a) \
do { \
  HRESULT res = a; \
  if(FAILED(res)) \
  { \
    CLog::Log(LOGERROR, __FUNCTION__" - failed executing "#a" at line %d with error %x", __LINE__, res); \
    return false; \
  } \
} while(0);

#define NVSTEREO_IMAGE_SIGNATURE 0x4433564E //NV3D

// ORedflags in the dwFlagsfielsof the _Nv_Stereo_Image_Headerstructure above
#define SIH_SWAP_EYES 0x00000001
#define SIH_SCALE_TO_FIT 0x00000002

typedef struct _NVSTEREOIMAGEHEADER
{
  unsigned int dwSignature;
  unsigned int dwWidth;
  unsigned int dwHeight;
  unsigned int dwBPP;
  unsigned int dwFlags;

} NVSTEREOIMAGEHEADER, *LPNVSTEREOIMAGEHEADER;


CNvidiaS3DDevice::CNvidiaS3DDevice(IDirect3D9Ex* pD3D) : IS3DDevice(pD3D),
    m_restoreFFScreen(false),
    m_inStereo(false),
    m_uiScreenWidth(0),
    m_uiScreenHeight(0),
    m_pRenderSurface(NULL)
{
  m_supported = PreInit();

  if (m_supported)
  {
    g_Windowing.Register(this);

    // register ISettingCallback 
    std::set<std::string> settingSet;
    settingSet.insert("videoscreen.fakefullscreen");
    CSettings::Get().RegisterCallback(this, settingSet);
  }
}

CNvidiaS3DDevice::~CNvidiaS3DDevice() 
{
  if (m_supported)
  {
    g_Windowing.Unregister(this);
    CSettings::Get().UnregisterCallback(this);
  }

  UnInit();
  SAFE_RELEASE(m_pD3DDevice);
}

void CNvidiaS3DDevice::UnInit(void) 
{
  SAFE_RELEASE(m_pRenderSurface);
  m_initialized  = false;
}

// 
bool CNvidiaS3DDevice::CorrectPresentParams(D3DPRESENT_PARAMETERS *pD3DPP)
{
  if (m_inStereo)
  {
    // NOTE 3D Vision can be in full screen only
    pD3DPP->Windowed = false;
  }

  return true;
}

// Returns true if S3D is supported by the platform and exposes supported display modes 
bool CNvidiaS3DDevice::GetS3DCaps(S3D_CAPS *pCaps)
{
  return true;
}

// 
bool CNvidiaS3DDevice::PreInit()
{
  // TODO check 3D VISION is present
  return true;
}

void CNvidiaS3DDevice::OnDestroyDevice()
{
  UnInit();
}

void CNvidiaS3DDevice::OnLostDevice()
{
  UnInit();
}

void CNvidiaS3DDevice::OnResetDevice()
{
}

void CNvidiaS3DDevice::OnCreateDevice()
{
  IDirect3DDevice9Ex* pD3DDevice = (IDirect3DDevice9Ex*)g_Windowing.Get3DDevice();

  D3DDISPLAYMODE d3dmodeTemp;
  pD3DDevice->GetDisplayMode(0, &d3dmodeTemp);

  m_uiScreenWidth = d3dmodeTemp.Width;
  m_uiScreenHeight = d3dmodeTemp.Height;

  // 3D VISION uses a single surface 2x images wide and image high
  if (FAILED(pD3DDevice->CreateRenderTarget(m_uiScreenWidth*2, m_uiScreenHeight, 
                                       D3DFMT_X8R8G8B8, D3DMULTISAMPLE_NONE, 0, TRUE, &m_pRenderSurface, NULL)))
  {
    return;
  }

  m_pD3DDevice = pD3DDevice;
  pD3DDevice->AddRef();

  m_initialized = true;
}

bool CNvidiaS3DDevice::OnSettingChanging(const CSetting *setting)
{
  if (setting == NULL)
    return true;

  const std::string &settingId = setting->GetId();
  return (settingId == "videoscreen.fakefullscreen" && m_inStereo) ? false : true;
}

// Switch the monitor to 3D mode
// Call with NULL to use current display mode
bool CNvidiaS3DDevice::SwitchTo3D(S3D_DISPLAY_MODE *pMode)
{
  if (g_graphicsContext.IsFullScreenRoot() && CSettings::Get().GetBool("videoscreen.fakefullscreen"))
  {
    CSettings::Get().SetBool("videoscreen.fakefullscreen", false);
    m_restoreFFScreen = true;
  }

  // do nothing, auto switch to 3d with first signed frame
  m_inStereo = true;

  return m_inStereo && m_initialized;
}

// Switch the monitor back to 2D mode
// Call with NULL to use current display mode
bool CNvidiaS3DDevice::SwitchTo2D(S3D_DISPLAY_MODE *pMode)
{
  // do nothing, auto switch to 2d with first unsigned frame
  m_inStereo = false;

  if (m_restoreFFScreen)
  {
    m_restoreFFScreen = false;
    CSettings::Get().SetBool("videoscreen.fakefullscreen", true);
  }

  return true;
}
    
// Activate left view, requires device to be set
bool CNvidiaS3DDevice::SelectLeftView()
{
  if (!m_inStereo)
    return false;

  return true;
}

// Activates right view, requires device to be set
bool CNvidiaS3DDevice::SelectRightView()
{
  if (!m_inStereo)
    return false;

  IDirect3DSurface9* pTarget;
  CHECK(m_pD3DDevice->GetRenderTarget(0, &pTarget));

  // copy left channel to result
  RECT dstRect = {0, 0, m_uiScreenWidth, m_uiScreenHeight};
  HRESULT hr = m_pD3DDevice->StretchRect(pTarget, NULL, m_pRenderSurface, &dstRect, D3DTEXF_NONE);

  if (FAILED(hr))
  {
    pTarget->Release();
    CLog::Log(LOGERROR, __FUNCTION__" - failed executing IDirect3DDevice9::StretchRect method with error %x", hr);
    return false;
  }

  pTarget->Release();
  return true;
}

// Activates right view, requires device to be set
bool CNvidiaS3DDevice::PresentFrame()
{
  if (!m_inStereo)
    return false;

  HRESULT hr;

  IDirect3DSurface9* pTarget;
  CHECK(m_pD3DDevice->GetRenderTarget(0, &pTarget));

  // copy right channel to result
  RECT dstRect = {m_uiScreenWidth, 0, m_uiScreenWidth*2, m_uiScreenHeight};
  hr = m_pD3DDevice->StretchRect(pTarget, NULL, m_pRenderSurface, &dstRect, D3DTEXF_NONE);

  if (FAILED(hr))
  {
    pTarget->Release();
    CLog::Log(LOGERROR, __FUNCTION__" - failed executing IDirect3DDevice9::StretchRect method with error %x", hr);
    return false;
  }

  Add3DSignature();

  // stretch result to render target
  // with 3d signature drivers show this surface in stereo mode.
  hr = m_pD3DDevice->StretchRect(m_pRenderSurface, NULL, pTarget, NULL, D3DTEXF_NONE);
  
  if (FAILED(hr))
  {
    pTarget->Release();
    CLog::Log(LOGERROR, __FUNCTION__" - failed executing IDirect3DDevice9::StretchRect method with error %x", hr);
    return false;
  }

  pTarget->Release();
  return true;
}

// put 3d signature in image in the last row of the stereo surface
void CNvidiaS3DDevice::Add3DSignature()
{
  HRESULT hr;

  // Lock the stereo surface
  D3DLOCKED_RECT lock;
  hr = m_pRenderSurface->LockRect(&lock, NULL, 0);
  if(FAILED(hr))
  {
    CLog::Log(LOGERROR, __FUNCTION__" - failed executing IDirect3DSurface9::LockRect method with error %x", hr);
    return;
  }

  try {
    // write stereo signature in the last raw of the stereo surface
    LPNVSTEREOIMAGEHEADER pSIH = (LPNVSTEREOIMAGEHEADER)(((unsigned char *) lock.pBits) + (lock.Pitch * (m_uiScreenHeight-1)));

    // Update the signature header values
    pSIH->dwSignature = NVSTEREO_IMAGE_SIGNATURE;
    pSIH->dwBPP = 32;
    pSIH->dwFlags = SIH_SCALE_TO_FIT /*| SIH_SWAP_EYES*/; // Src image has left on left and right on right, thats why this flag is not needed.
    pSIH->dwWidth = m_uiScreenWidth * 2;
    pSIH->dwHeight = m_uiScreenHeight;
  }
  catch (...)
  {
    // on some systems (probably without 3D VISION) it may fails with access violation
    // don't spam this
    //CLog::Log(LOGERROR, __FUNCTION__" - signing stereo image failed with access violation");
  }

  // unlock stereo surface
  hr = m_pRenderSurface->UnlockRect();
  if(FAILED(hr))
    CLog::Log(LOGERROR, __FUNCTION__" - failed executing IDirect3DSurface9::UnlockRect with error %x", hr);
}

#endif // HAS_DX
