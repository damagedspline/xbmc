/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#ifndef WIN_SYSTEM_WIN10_DX_H
#define WIN_SYSTEM_WIN10_DX_H

#ifdef HAS_DX

#pragma once

#include "windowing/win10/WinSystemWin10.h"
#include "rendering/dx/RenderSystemDXWin10.h"
#include "utils/GlobalsHandling.h"


class CWinSystemWin10DX : public CWinSystemWin10, public CRenderSystemDX
{
public:
  CWinSystemWin10DX();
  ~CWinSystemWin10DX();

  virtual bool CreateNewWindow(std::string name, bool fullScreen, RESOLUTION_INFO& res, PHANDLE_EVENT_FUNC userFunction);
  virtual bool ResizeWindow(int newWidth, int newHeight, int newLeft, int newTop);
  virtual void OnMove(int x, int y);
  virtual bool SetFullScreen(bool fullScreen, RESOLUTION_INFO& res, bool blankOtherDisplays);
  virtual bool WindowedMode() { return CRenderSystemDX::m_useWindowedDX; }
  virtual void NotifyAppFocusChange(bool bGaining);
  virtual void PresentRender(bool rendered, bool videoLayer);

  std::string GetClipboardText(void);

  /*!
   \brief Register as a dependent of the DirectX Render System
   Resources should call this on construction if they're dependent on the Render System
   for survival. Any resources that registers will get callbacks on loss and reset of
   device. In addition, callbacks for destruction and creation of the device are also called,
   where any resources dependent on the DirectX device should be destroyed and recreated.
   \sa Unregister, ID3DResource
  */
  void Register(ID3DResource *resource) override { CRenderSystemDX::Register(resource); };
  /*!
   \brief Unregister as a dependent of the DirectX Render System
   Resources should call this on destruction if they're a dependent on the Render System
   \sa Register, ID3DResource
  */
  void Unregister(ID3DResource *resource) override { CRenderSystemDX::Unregister(resource); };

  void Register(IDispResource *resource) override { CWinSystemWin10::Register(resource); };
  void Unregister(IDispResource *resource) override { CWinSystemWin10::Unregister(resource); };

protected:
  bool UseWindowedDX(bool fullScreen);
  void UpdateMonitor() override;
  void OnDisplayLost() override { CWinSystemWin10::OnDisplayLost(); };
  void OnDisplayReset() override { CWinSystemWin10::OnDisplayReset(); };
  void OnDisplayBack() override { CWinSystemWin10::OnDisplayBack(); };
};

XBMC_GLOBAL_REF(CWinSystemWin10DX,g_Windowing);
#define g_Windowing XBMC_GLOBAL_USE(CWinSystemWin10DX)

#endif

#endif // WIN_SYSTEM_WIN10_DX_H
