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

#pragma once

#include <d3d9.h>
#include <dxva2api.h>
#include "IS3DDevice.h"

class IS3DDevice;

class CAmdS3DDevice: public IS3DDevice
{
public:
  CAmdS3DDevice(IDirect3D9Ex* pD3D);
 ~CAmdS3DDevice();

  // correct present params for correct stereo rendering some implementations need it
  bool CorrectPresentParams(D3DPRESENT_PARAMETERS *pD3DPP);

  // Returns true if S3D is supported by the platform and exposes supported display modes 
  // !! m.b. not needed
  bool GetS3DCaps(S3D_CAPS *pCaps);

  // create devices for stereoscopic rendering
  bool OnDeviceCreated(IDirect3DDevice9Ex* pD3DDevice);

  // Switch the monitor to 3D mode
  // Call with NULL to use current display mode
  bool SwitchTo3D(S3D_DISPLAY_MODE *pMode);

  // Switch the monitor back to 2D mode
  // Call with NULL to use current display mode
  bool SwitchTo2D(S3D_DISPLAY_MODE *pMode);
    
  // Activate left view, requires device to be set
  bool SelectLeftView(void);

  // Activates right view, requires device to be set
  bool SelectRightView(void);

  // 
  bool PresentFrame(void);

  void UnInit(void);

protected:
  bool PreInit(void);

  bool                            m_restoreFFScreen;
};

#endif // HAS_DX