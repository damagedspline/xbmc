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

#define MEMCPY_VAR(dstVarName, src, count) memcpy_s(&(dstVarName), sizeof(dstVarName), (src), (count))

#include <d3d9.h>
#include <dxva2api.h>
#include "IS3DDevice.h"
#include "win32/igfx_s3dcontrol/igfx_s3dcontrol.h"

class IS3DDevice;
class IGFXS3DControl;

class CIntelS3DDevice: public IS3DDevice
{
public:
  CIntelS3DDevice(IDirect3D9Ex* pD3D);
 ~CIntelS3DDevice();

  // correct present params for correct stereo rendering some implementations need it
  bool CorrectPresentParams(D3DPRESENT_PARAMETERS *pD3DPP, bool stereo);

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
  bool Less(const IGFX_DISPLAY_MODE &l, const IGFX_DISPLAY_MODE& r);
  bool CheckOverlaySupport(int iWidth, int iHeight, D3DFORMAT dFormat);

  // various structures for S3D and DXVA2 calls

  bool                            m_restoreFFScreen;
  unsigned int                    m_resetToken;

  IDirect3DDevice9Ex*             m_pD3DDevice;
  IGFX_S3DCAPS                    m_S3DCaps;
  IGFX_DISPLAY_MODE               m_S3DPrefMode;       // Prefered S3D display mode
  DXVA2_VideoDesc                 m_VideoDesc;
  DXVA2_VideoProcessBltParams     m_BltParams; 
  DXVA2_VideoSample               m_Sample;            // Simple sample :)
   
  IGFXS3DControl*                 m_pS3DControl;
  IDirect3DDeviceManager9*        m_pDeviceManager9;   
  IDirectXVideoProcessorService*  m_pProcessService;   // Service required to create video processors
  IDirectXVideoProcessor*         m_pProcessLeft;      // Left channel processor
  IDirectXVideoProcessor*         m_pProcessRight;     // Right channel processor
  IDirect3DSurface9*              m_pRenderSurface;    // The surface for L+R results

};

#endif // HAS_DX