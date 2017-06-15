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


#include "WinSystemWin10DX.h"
#include "ServiceBroker.h"
#include "guilib/gui3d.h"
#include "guilib/GraphicContext.h"
#include "rendering/dx/DeviceResources.h"
#include "settings/DisplaySettings.h"
#include "settings/Settings.h"
#include "threads/SingleLock.h"
#include "utils/CharsetConverter.h"
#include "utils/log.h"
#include "windowing/WindowingFactory.h"
#include <ppltasks.h>

using namespace concurrency;
using namespace Windows::UI::Core;
using namespace Windows::UI::ViewManagement;

#ifdef HAS_DX

CWinSystemWin10DX::CWinSystemWin10DX()
: CRenderSystemDX()
{

}

CWinSystemWin10DX::~CWinSystemWin10DX()
{

}

void CWinSystemWin10DX::PresentRender(bool rendered, bool videoLayer)
{
  if (rendered)
    PresentRenderImpl(rendered);

  if (m_delayDispReset && m_dispResetTimer.IsTimePast())
  {
    m_delayDispReset = false;
    CWinSystemWin10::OnDisplayReset();
  }
  if (!rendered)
    Sleep(40);
}

bool CWinSystemWin10DX::UseWindowedDX(bool fullScreen)
{
  return (CServiceBroker::GetSettings().GetBool(CSettings::SETTING_VIDEOSCREEN_FAKEFULLSCREEN) || !fullScreen);
}

bool CWinSystemWin10DX::CreateNewWindow(std::string name, bool fullScreen, RESOLUTION_INFO& res)
{
  if(!CWinSystemWin10::CreateNewWindow(name, fullScreen, res))
    return false;

#if 0
  SetFocusWnd(m_hWnd);
  SetDeviceWnd(m_hWnd);
  CRenderSystemDX::m_interlaced = ((res.dwFlags & D3DPRESENTFLAG_INTERLACED) != 0);
  CRenderSystemDX::m_useWindowedDX = UseWindowedDX(fullScreen);
  SetRenderParams(m_nWidth, m_nHeight, fullScreen, res.fRefreshRate);
  const MONITOR_DETAILS* monitor = GetMonitor(res.iScreen);
  if (!monitor)
    return false;

  SetMonitor(monitor->hMonitor);
#endif
  return true;
}

void CWinSystemWin10DX::UpdateMonitor()
{
  CLog::Log(LOGERROR, "%s is not implemented", __FUNCTION__);
#if 0
  const MONITOR_DETAILS* monitor = GetMonitor(m_nScreen);
  if (monitor)
    SetMonitor(monitor->hMonitor);
#endif
}

bool CWinSystemWin10DX::ResizeWindow(int newWidth, int newHeight, int newLeft, int newTop)
{
  CWinSystemWin10::ResizeWindow(newWidth, newHeight, newLeft, newTop);
  CRenderSystemDX::OnResize(newWidth, newHeight);

  return true;
}

void CWinSystemWin10DX::OnMove(int x, int y)
{
  CRenderSystemDX::OnMove();
}

bool CWinSystemWin10DX::SetFullScreen(bool fullScreen, RESOLUTION_INFO& res, bool blankOtherDisplays)
{
  DX::DeviceResources::Get()->SetFullScreen(fullScreen);
  return CWinSystemWin10::SetFullScreen(fullScreen, res, blankOtherDisplays);
}

std::string CWinSystemWin10DX::GetClipboardText(void)
{
  std::wstring unicode_text;
  std::string utf8_text;

  auto contentView = Windows::ApplicationModel::DataTransfer::Clipboard::GetContent();
  if (contentView->Contains(Windows::ApplicationModel::DataTransfer::StandardDataFormats::Text))
  {
    Concurrency::create_task(contentView->GetTextAsync()).then([&unicode_text](Platform::String^ str)
    {
      unicode_text.append(str->Data());
    }).wait();
  }

  g_charsetConverter.wToUTF8(unicode_text, utf8_text);
  return utf8_text;
}

void CWinSystemWin10DX::NotifyAppFocusChange(bool bGaining)
{
  CWinSystemWin10::NotifyAppFocusChange(bGaining);

#if 0
  // if true fullscreen we need switch render system to/from ff manually like dx9 does
  if (!UseWindowedDX(m_bFullScreen) && CRenderSystemDX::m_bRenderCreated)
  {
    CRenderSystemDX::m_useWindowedDX = !bGaining;
    CRenderSystemDX::SetFullScreenInternal();
    if (bGaining)
      CRenderSystemDX::CreateWindowSizeDependentResources();

    // minimize window on lost focus
    if (!bGaining)
      ShowWindow(m_hWnd, SW_FORCEMINIMIZE);
  }
#endif
}

#endif
