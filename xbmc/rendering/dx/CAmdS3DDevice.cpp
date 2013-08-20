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
#include "CAmdS3DDevice.h"
#include "threads/SingleLock.h"
#include "guilib/GUIWindowManager.h"
#include "settings/Settings.h"
#include "windowing/WindowingFactory.h"
#include "win32/igfx_s3dcontrol/AtiDx9Stereo.h"

CAmdS3DDevice::CAmdS3DDevice(IDirect3D9Ex* pD3D) : IS3DDevice(pD3D),
    m_restoreFFScreen(false),
    m_backBufferWidth(0),
    m_backBufferHeight(0),
    m_enableStereo(false),
    m_pBackBufferSurface(NULL),
    m_pBackBufferDepthStencil(NULL),
    m_pDriverComSurface(NULL),
    m_pRightEyeRT(NULL),
    m_pLeftEyeRT(NULL),
    m_pRightEyeDS(NULL),
    m_pLeftEyeDS(NULL),
    m_pD3DDevice(NULL)
{
    m_supported = PreInit();
}

CAmdS3DDevice::~CAmdS3DDevice() 
{
  UnInit();
}

void CAmdS3DDevice::UnInit(void) 
{
  m_enableStereo = false;
  m_initialized  = false;
  ReleaseCommSurface();
  SAFE_RELEASE(m_pBackBufferSurface);
  SAFE_RELEASE(m_pBackBufferDepthStencil);
  SAFE_RELEASE(m_pDriverComSurface);
  SAFE_RELEASE(m_pRightEyeRT);
  SAFE_RELEASE(m_pLeftEyeRT);
  SAFE_RELEASE(m_pRightEyeDS);
  SAFE_RELEASE(m_pLeftEyeDS);
}

// 
bool CAmdS3DDevice::CorrectPresentParams(D3DPRESENT_PARAMETERS *pD3DPP)
{
  return true;
}

// Returns true if S3D is supported by the platform and exposes supported display modes 
bool CAmdS3DDevice::GetS3DCaps(S3D_CAPS *pCaps)
{
  return true;
}

// 
bool CAmdS3DDevice::PreInit()
{
  // TODO check for support
  return true;
}

void CAmdS3DDevice::OnDestroyDevice(void)
{
  UnInit();
}

void CAmdS3DDevice::OnLostDevice(void)
{
  UnInit();
}

void CAmdS3DDevice::OnResetDevice(void)
{
}

void CAmdS3DDevice::OnCreateDevice(void)
{
  if (!m_enableStereo)
    return;

  HRESULT hr;
  IDirect3DDevice9* pd3dDevice = (IDirect3DDevice9*)g_Windowing.Get3DDevice();

  D3DPRESENT_PARAMETERS pParams;
  //pd3dDevice->GetCreationParameters(&pParams);

  if(FAILED(CreateCommSurface()))
  {
    CLog::Log(LOGERROR, __FUNCTION__" - Create communication surface failed. Stereo isn't possible");
    m_enableStereo = false;
    m_supported = false;
    return;
  }

  //Send the command to the driver 
  if(FAILED(SendStereoCommand(ATI_STEREO_ENABLESTEREO, NULL, 0, 0, 0)))
  {
    CLog::Log(LOGERROR, __FUNCTION__" - Enable stereo failed. Stereo isn't possible");
    m_pDriverComSurface->Release();
    m_pDriverComSurface = NULL;
    m_enableStereo = false;
    return;
  }

  // See what stereo modes are available
  ATIDX9GETDISPLAYMODES displayModeParams;
  displayModeParams.dwNumModes    = 0;
  displayModeParams.pStereoModes = NULL;

  //Send stereo command to get the number of available stereo modes.
  hr = SendStereoCommand(ATI_STEREO_GETDISPLAYMODES, (BYTE *)(&displayModeParams),
                         sizeof(ATIDX9GETDISPLAYMODES), 0, 0);  
  if(FAILED(hr))
  {
    m_pDriverComSurface->Release();
    m_pDriverComSurface = NULL;
    m_enableStereo = false;
    return;
  }

  if(displayModeParams.dwNumModes != 0)
  {
    //Allocating memory to get the list of modes.
    displayModeParams.pStereoModes = new D3DDISPLAYMODE[displayModeParams.dwNumModes];
	   
    //Send stereo command to get the list of stereo modes
    hr = SendStereoCommand(ATI_STEREO_GETDISPLAYMODES, (BYTE *)(&displayModeParams), 
                           sizeof(ATIDX9GETDISPLAYMODES), 0, 0);
    if(FAILED(hr))
    {
      m_pDriverComSurface->Release();
      m_pDriverComSurface = NULL;
      m_enableStereo = false;
      delete[] displayModeParams.pStereoModes;
      return;
    }
  }

  D3DDISPLAYMODE currentMode;
  pd3dDevice->GetDisplayMode(0, &currentMode);

  int refreshMatch = -1;
  int resFormatMatch = -1;
  for (int i = 0; i < (int)displayModeParams.dwNumModes; i++)
  {
    if (displayModeParams.pStereoModes[i].Width != pParams.BackBufferWidth)
      continue;
    if (displayModeParams.pStereoModes[i].Height != pParams.BackBufferHeight)
      continue;
    if ((displayModeParams.pStereoModes[i].Format != pParams.BackBufferFormat)
    && ((displayModeParams.pStereoModes[i].Format != D3DFMT_X8R8G8B8) || (pParams.BackBufferFormat != D3DFMT_A8R8G8B8)))
      continue;

    // if it made it this far, the selected screen resolution and format match one of the possible stereo modes
    resFormatMatch = i;
	if (displayModeParams.pStereoModes[i].RefreshRate == currentMode.RefreshRate)
    {
      refreshMatch = i; // found a match with the current refresh
      break;
    }
  }

  ReleaseCommSurface();

  if (resFormatMatch < 0 )
  {
    m_enableStereo = false;
    delete[] displayModeParams.pStereoModes;
    return;
  }

  int displayModeIndex = refreshMatch >= 0 ? refreshMatch : resFormatMatch;

  //A valid multisample value other then 0 or 1 must be set for stereo. (ex 2)
  pParams.MultiSampleType = D3DMULTISAMPLE_2_SAMPLES;
  pParams.Flags = 0; // can't lock the back buffer
  pParams.EnableAutoDepthStencil = false; // need to create a special depth buffer

  pParams.FullScreen_RefreshRateInHz = displayModeParams.pStereoModes[displayModeIndex].RefreshRate;
  pParams.BackBufferFormat = displayModeParams.pStereoModes[displayModeIndex].Format;
  delete[] displayModeParams.pStereoModes;

  hr = pd3dDevice->Reset(&pParams); // Call reset to create the full screen stereo quad buffers
  if(FAILED(hr))
  {
    m_enableStereo = false;
    return;
  }

  m_backBufferWidth = pParams.BackBufferWidth;
  m_backBufferHeight = pParams.BackBufferHeight;
  m_zFormat = pParams.AutoDepthStencilFormat;

  m_pD3DDevice = pd3dDevice;
  pd3dDevice->AddRef();

  m_initialized = CreateResources();
}

bool CAmdS3DDevice::CreateResources()
{
  HRESULT hr;

  if (!m_enableStereo)
    return false;

  hr = m_pD3DDevice->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &m_pBackBufferSurface);
  if(FAILED(hr))
  {
    CLog::Log(LOGWARNING, __FUNCTION__" - Getting back buffer failed.");
    m_enableStereo = false;
    return false;
  }

  hr = CreateCommSurface();
  if(FAILED(hr))
  {
    CLog::Log(LOGWARNING, __FUNCTION__" - Create communication surface failed.");
    m_enableStereo = false;
    return false;
  }

  //Retrieve the line offset
  hr = SendStereoCommand(ATI_STEREO_GETLINEOFFSET, (BYTE *)(&m_lineOffset), sizeof(DWORD), 0, 0);
  if(FAILED(hr))
  {
    m_enableStereo = false;
    return false;
  }

  // see if lineOffset is valid
  if (m_lineOffset == 0)
  { 
    CLog::Log(LOGWARNING, __FUNCTION__" - lineOffset isn't valid.");

    // Something went wrong with the device reset. Allocate a depth buffer and disable stereo.
    m_enableStereo = false;
    hr = m_pD3DDevice->CreateDepthStencilSurface(m_backBufferWidth, m_backBufferHeight, m_zFormat,
                                               D3DMULTISAMPLE_NONE, 0, false, &m_pBackBufferDepthStencil, NULL);
    m_pD3DDevice->SetRenderState(D3DRS_ZENABLE, D3DZB_TRUE);
    if(FAILED(hr))
    {
      CLog::Log(LOGWARNING, __FUNCTION__" - Set render state failed.");
      return false;
    }

    hr = m_pD3DDevice->SetDepthStencilSurface(m_pBackBufferDepthStencil);
    if(FAILED(hr))
    {
      CLog::Log(LOGWARNING, __FUNCTION__" - Set depthstencil surface failed.");
      return false;
	}
    return false;
  }

  // create an extra large depth buffer to handle both left and right buffers
  hr = m_pD3DDevice->CreateDepthStencilSurface(m_backBufferWidth, 2 * m_lineOffset, m_zFormat,
		                                       D3DMULTISAMPLE_2_SAMPLES, 0, false, &m_pBackBufferDepthStencil, NULL);
  if(FAILED(hr))
  {
    CLog::Log(LOGWARNING, __FUNCTION__" - Create depthstencil surface failed.");
    m_enableStereo = false;
    return false;
  }

  hr = m_pD3DDevice->SetDepthStencilSurface(m_pBackBufferDepthStencil);
  if(FAILED(hr))
  {
    CLog::Log(LOGWARNING, __FUNCTION__" - Set depthstencil surface failed.");
    m_enableStereo = false;
    return false;
  }

  // Create the right eye texture
  hr = m_pD3DDevice->CreateRenderTarget(m_backBufferWidth,
                                        m_backBufferHeight,
                                        D3DFMT_X8R8G8B8,
                                        D3DMULTISAMPLE_NONE,
                                        0,
                                        false, &m_pRightEyeRT, NULL);

  if(FAILED(hr))
  {
    CLog::Log(LOGWARNING, __FUNCTION__" - Create the right eye texture failed.");
    m_enableStereo = false;
    return false;
  }

  // Create the right eye depth buffer
  hr = m_pD3DDevice->CreateDepthStencilSurface(m_backBufferWidth,
                                               m_backBufferHeight,
                                               D3DFMT_D24S8,
                                               D3DMULTISAMPLE_NONE,
                                               0,
                                               false, &m_pRightEyeDS, NULL);
  if(FAILED(hr))
  {
    CLog::Log(LOGWARNING, __FUNCTION__" - Create the right eye depth buffer failed.");
    m_enableStereo = false;
    return false;
  }

  // Create the left eye texture
  hr = m_pD3DDevice->CreateRenderTarget(m_backBufferWidth,
                                        m_backBufferHeight,
                                        D3DFMT_X8R8G8B8,
                                        D3DMULTISAMPLE_NONE,
                                        0,
                                        false, &m_pLeftEyeRT, NULL );

  if( FAILED( hr ) )
  {
    CLog::Log(LOGWARNING, __FUNCTION__" - Create the left eye depth buffer failed.");
    m_enableStereo = false;
    return false;
  }

  // Create the left eye depth buffer
  hr = m_pD3DDevice->CreateDepthStencilSurface(m_backBufferWidth,
                                               m_backBufferHeight,
                                               D3DFMT_D24S8,
                                               D3DMULTISAMPLE_NONE,
                                               0,
                                               false, &m_pLeftEyeDS, NULL);
  if(FAILED(hr))
  {
    CLog::Log(LOGWARNING, __FUNCTION__" - Create the left eye depth buffer failed.");
    m_enableStereo = false;
    return false;
  }

  m_pD3DDevice->SetRenderState(D3DRS_ZENABLE, D3DZB_TRUE);

  return true;
}

// Switch the monitor to 3D mode
// Call with NULL to use current display mode
bool CAmdS3DDevice::SwitchTo3D(S3D_DISPLAY_MODE *pMode)
{
  if (g_graphicsContext.IsFullScreenRoot() && !CSettings::Get().GetBool("videoscreen.fakefullscreen"))
  {
    CSettings::Get().SetBool("videoscreen.fakefullscreen", true);
    g_graphicsContext.LockFakeFullScreen(true);
    m_restoreFFScreen = true;
  }

  m_enableStereo = true;
  return false;
}

// Switch the monitor back to 2D mode
// Call with NULL to use current display mode
bool CAmdS3DDevice::SwitchTo2D(S3D_DISPLAY_MODE *pMode)
{
  if (m_restoreFFScreen)
  {
    m_restoreFFScreen = false;
    g_graphicsContext.LockFakeFullScreen(false);
    CSettings::Get().SetBool("videoscreen.fakefullscreen", false);
  }

  m_enableStereo = false;
  return false;
}
    
// Activate left view, requires device to be set
bool CAmdS3DDevice::SelectLeftView()
{
  m_pD3DDevice->SetRenderTarget(0, m_pLeftEyeRT);
  m_pD3DDevice->SetDepthStencilSurface(m_pLeftEyeDS);
  return true;
}

// Activates right view, requires device to be set
bool CAmdS3DDevice::SelectRightView()
{
  m_pD3DDevice->SetRenderTarget(0, m_pRightEyeRT);
  m_pD3DDevice->SetDepthStencilSurface(m_pRightEyeDS);
  return true;
}

// Activates right view, requires device to be set
bool CAmdS3DDevice::PresentFrame()
{
	// Use the quad buffer render target
  m_pD3DDevice->SetRenderTarget(0, m_pBackBufferSurface);

  // update the quad buffer with the right render target
  D3DVIEWPORT9 viewPort;
  viewPort.X = 0;
  viewPort.Y = m_lineOffset;
  viewPort.Width = m_backBufferWidth;
  viewPort.Height = m_backBufferHeight;
  viewPort.MinZ = 0;
  viewPort.MaxZ = 1;

  m_pD3DDevice->SetViewport(&viewPort);

  // set the right quad buffer as the destination for StretchRect
  DWORD dwEye = ATI_STEREO_RIGHTEYE;
  SendStereoCommand(ATI_STEREO_SETDSTEYE, NULL, 0, (BYTE *)&dwEye, sizeof(dwEye));

  m_pD3DDevice->StretchRect(m_pRightEyeRT, NULL, m_pBackBufferSurface, NULL, D3DTEXF_LINEAR);

  // set the left quad buffer as the destination for StretchRect
  viewPort.Y = 0;
  m_pD3DDevice->SetViewport(&viewPort);

  dwEye = ATI_STEREO_LEFTEYE;
  SendStereoCommand(ATI_STEREO_SETDSTEYE, NULL, 0, (BYTE *)&dwEye, sizeof(dwEye));

  m_pD3DDevice->StretchRect(m_pLeftEyeRT, NULL, m_pBackBufferSurface, NULL, D3DTEXF_LINEAR);
  return true;
}

// Create a surface to be used to communicate with the driver
HRESULT CAmdS3DDevice::CreateCommSurface()
{
  return m_pD3DDevice->CreateOffscreenPlainSurface(10, 10, (D3DFORMAT)FOURCC_AQBS, D3DPOOL_DEFAULT, &m_pDriverComSurface, NULL);
}

//release comm surface
void CAmdS3DDevice::ReleaseCommSurface()
{
  SAFE_RELEASE(m_pDriverComSurface);
}

bool CAmdS3DDevice::SendStereoCommand(ATIDX9STEREOCOMMAND stereoCommand, BYTE *pOutBuffer, 
                                      DWORD dwOutBufferSize, BYTE *pInBuffer, DWORD dwInBufferSize)
{
  HRESULT hr;
  ATIDX9STEREOCOMMPACKET *pCommPacket;
  D3DLOCKED_RECT lockedRect;

  hr = m_pDriverComSurface->LockRect(&lockedRect, 0, 0);
  if(FAILED(hr))
  {
    CLog::Log(LOGWARNING, __FUNCTION__" - lock CommSurface failed");
    return false;
  }

  pCommPacket = (ATIDX9STEREOCOMMPACKET *)(lockedRect.pBits);
  pCommPacket->dwSignature = 'STER';
  pCommPacket->pResult = &hr;
  pCommPacket->stereoCommand = stereoCommand;
  if (pOutBuffer && !dwOutBufferSize)
  {
    return true;
  }

  pCommPacket->pOutBuffer = pOutBuffer;
  pCommPacket->dwOutBufferSize = dwOutBufferSize;
  if (pInBuffer && !dwInBufferSize)
  {
    return true;
  }

  pCommPacket->pInBuffer = pInBuffer;
  pCommPacket->dwInBufferSize = dwInBufferSize;
  m_pDriverComSurface->UnlockRect();

  return true;
}

#endif // HAS_DX
