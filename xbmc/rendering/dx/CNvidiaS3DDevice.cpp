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
#include "CNvidiaS3DDevice.h"
#include "threads/SingleLock.h"
#include "guilib/GUIWindowManager.h"
#include "settings/Settings.h"

CNvidiaS3DDevice::CNvidiaS3DDevice(IDirect3D9Ex* pD3D) : IS3DDevice(pD3D),
    m_restoreFFScreen(false)
{
    m_supported = PreInit();
}

CNvidiaS3DDevice::~CNvidiaS3DDevice() 
{
  UnInit();
}

void CNvidiaS3DDevice::UnInit(void) 
{
  m_initialized  = false;
}

// 
bool CNvidiaS3DDevice::CorrectPresentParams(D3DPRESENT_PARAMETERS *pD3DPP, bool stereo)
{
  return false;
}

// Returns true if S3D is supported by the platform and exposes supported display modes 
bool CNvidiaS3DDevice::GetS3DCaps(S3D_CAPS *pCaps)
{
  return false;
}

// 
bool CNvidiaS3DDevice::PreInit()
{
  return false;
}

bool CNvidiaS3DDevice::OnDeviceCreated(IDirect3DDevice9Ex* pD3DDevice)
{
  return false;
}

// Switch the monitor to 3D mode
// Call with NULL to use current display mode
bool CNvidiaS3DDevice::SwitchTo3D(S3D_DISPLAY_MODE *pMode)
{
  if (g_graphicsContext.IsFullScreenRoot() && CSettings::Get().GetBool("videoscreen.fakefullscreen"))
  {
    CSettings::Get().SetBool("videoscreen.fakefullscreen", false);
    g_graphicsContext.LockFakeFullScreen(true);
    m_restoreFFScreen = true;
  }

  return false;
}

// Switch the monitor back to 2D mode
// Call with NULL to use current display mode
bool CNvidiaS3DDevice::SwitchTo2D(S3D_DISPLAY_MODE *pMode)
{
  if (m_restoreFFScreen)
  {
    m_restoreFFScreen = false;
    g_graphicsContext.LockFakeFullScreen(false);
    CSettings::Get().SetBool("videoscreen.fakefullscreen", true);
  }

  return false;
}
    
// Activate left view, requires device to be set
bool CNvidiaS3DDevice::SelectLeftView()
{
  return false;
}

// Activates right view, requires device to be set
bool CNvidiaS3DDevice::SelectRightView()
{
  return false;
}

// Activates right view, requires device to be set
bool CNvidiaS3DDevice::PresentFrame()
{
  return false;
}

#endif // HAS_DX
