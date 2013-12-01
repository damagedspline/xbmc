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
#include "system.h"

typedef struct _S3D_DISPLAY_MODE
{
  unsigned int uiWidth;
  unsigned int uiHeight;
  unsigned int uiRefreshRate;

} S3D_DISPLAY_MODE;

typedef struct _S3D_CAPS
{
  unsigned int uiNumEntries;
  S3D_DISPLAY_MODE * SupportedModes; 

} S3D_CAPS;

class IS3DDevice
{
public:
  static IS3DDevice* CreateDevice(IDirect3D9Ex* pD3D, UINT uAdapterID);

  IS3DDevice(IDirect3D9Ex* pD3D) 
          :m_pD3D(NULL),
           m_initialized(false),
           m_supported(false)
  { 
    m_pD3D = pD3D; 
    pD3D->AddRef(); 
  }

  ~IS3DDevice() { SAFE_RELEASE(m_pD3D); };

  // 
  virtual bool CorrectPresentParams(D3DPRESENT_PARAMETERS *pD3DPP, bool stereo) { return false; }

  // Returns true if S3D is supported by the platform and exposes supported display modes 
  virtual bool GetS3DCaps(S3D_CAPS *pCaps) { return false; }

  // create devices for stereoscopic rendering
  virtual bool OnDeviceCreated(IDirect3DDevice9Ex* pD3DDevice) { return false; }

  // Switch the monitor to 3D mode
  // Call with NULL to use current display mode
  virtual bool SwitchTo3D(S3D_DISPLAY_MODE *pMode) { return false; } 

  // Switch the monitor back to 2D mode
  // Call with NULL to use current display mode
  virtual bool SwitchTo2D(S3D_DISPLAY_MODE *pMode) { return false; }
    
  // Activate left view, requires device to be set
  virtual bool SelectLeftView() { return false; }

  // Activates right view, requires device to be set
  virtual bool SelectRightView() { return false; }

  // Activates right view, requires device to be set
  virtual bool PresentFrame() { return false; }

  virtual void UnInit() { m_initialized = false; }

  bool IsInitialized() { return m_initialized; }

  bool IsSupported() { return m_supported; }

protected:
  // pre init device
  virtual bool PreInit() { return false; }

  bool                            m_initialized;
  bool                            m_supported;
  IDirect3D9Ex*                   m_pD3D;
};

#endif // HAS_DX