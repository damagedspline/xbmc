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

#include "WinSystemWin10.h"
#include "WinEventsWin10.h"
#include "resource.h"
#include "ServiceBroker.h"
#include "guilib/gui3d.h"
#include "messaging/ApplicationMessenger.h"
#include "settings/AdvancedSettings.h"
#include "settings/DisplaySettings.h"
#include "settings/Settings.h"
#include "threads/SingleLock.h"
#include "utils/log.h"
#include "utils/CharsetConverter.h"
#include "utils/SystemInfo.h"
#include "rendering/dx/DeviceResources.h"

#ifdef TARGET_WIN10
#pragma pack(push,8)

#include <tpcshrd.h>
#include <ppltasks.h>

using namespace Windows::UI::Core;
using namespace Windows::Graphics::Display;

CWinSystemWin10::CWinSystemWin10()
: CWinSystemBase()
{
  m_eWindowSystem = WINDOW_SYSTEM_WIN10;
  m_nPrimary = 0;
  m_ValidWindowedPosition = false;
  m_IsAlteringWindow = false;
}

CWinSystemWin10::~CWinSystemWin10()
{

};

bool CWinSystemWin10::InitWindowSystem()
{
  if(!CWinSystemBase::InitWindowSystem())
    return false;

  if(m_MonitorsInfo.empty())
  {
    CLog::Log(LOGERROR, "%s - no suitable monitor found, aborting...", __FUNCTION__);
    return false;
  }

  return true;
}

bool CWinSystemWin10::DestroyWindowSystem()
{
  RestoreDesktopResolution(m_nScreen);
  return true;
}

bool CWinSystemWin10::CreateNewWindow(const std::string& name, bool fullScreen, RESOLUTION_INFO& res, PHANDLE_EVENT_FUNC userFunction)
{

  m_nWidth  = res.iWidth;
  m_nHeight = res.iHeight;
  m_bFullScreen = fullScreen;
  m_nScreen = res.iScreen;


  m_bWindowCreated = true;

  CreateBlankWindows();

  ResizeInternal(true);

  // Show the window

  return true;
}

bool CWinSystemWin10::CreateBlankWindows()
{
  CLog::Log(LOGERROR, "%s is not implemented", __FUNCTION__);
  return true;
}

bool CWinSystemWin10::BlankNonActiveMonitors(bool bBlank)
{
  CLog::Log(LOGERROR, "%s is not implemented", __FUNCTION__);
  return true;
}

bool CWinSystemWin10::CenterWindow()
{
  RESOLUTION_INFO DesktopRes = CDisplaySettings::GetInstance().GetResolutionInfo(RES_DESKTOP);

  m_nLeft = (DesktopRes.iWidth / 2) - (m_nWidth / 2);
  m_nTop = (DesktopRes.iHeight / 2) - (m_nHeight / 2);

  RECT rc;
  rc.left = m_nLeft;
  rc.top = m_nTop;
  rc.right = rc.left + m_nWidth;
  rc.bottom = rc.top + m_nHeight;

  // center the window

  return true;
}

bool CWinSystemWin10::ResizeWindow(int newWidth, int newHeight, int newLeft, int newTop)
{
  m_nWidth = newWidth;
  m_nHeight = newHeight;

  if(newLeft > 0)
    m_nLeft = newLeft;

  if(newTop > 0)
    m_nTop = newTop;

  ResizeInternal();

  return true;
}

void CWinSystemWin10::NotifyAppFocusChange(bool bGaining)
{
  if (m_bFullScreen && bGaining)
  {
    //SetWindowPos(m_hWnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOREDRAW);
  }
}

bool CWinSystemWin10::SetFullScreen(bool fullScreen, RESOLUTION_INFO& res, bool blankOtherDisplays)
{
  return SetFullScreenEx(fullScreen, res, blankOtherDisplays, false);
}

bool CWinSystemWin10::SetFullScreenEx(bool fullScreen, RESOLUTION_INFO& res, bool blankOtherDisplays, bool forceResChange)
{
  CLog::Log(LOGERROR, "%s is not implemented", __FUNCTION__);

  m_bFullScreen = fullScreen;
  m_nScreen = res.iScreen;
  m_nWidth = res.iWidth;
  m_nHeight = res.iHeight;
  m_bBlankOtherDisplay = blankOtherDisplays;

  ResizeInternal(false);

#if 0
  m_IsAlteringWindow = true;

  CLog::Log(LOGDEBUG, "%s (%s) on screen %d with size %dx%d, refresh %f%s", __FUNCTION__, !fullScreen ? "windowed" : (CServiceBroker::GetSettings().GetBool(CSettings::SETTING_VIDEOSCREEN_FAKEFULLSCREEN) ? "windowed fullscreen" : "true fullscreen"), res.iScreen, res.iWidth, res.iHeight, res.fRefreshRate, (res.dwFlags & D3DPRESENTFLAG_INTERLACED) ? "i" : "");

  bool forceResize = false;

  if (m_nScreen != res.iScreen)
  {
    forceResize = true;
    RestoreDesktopResolution(m_nScreen);
  }

  if(m_hWnd && !m_bFullScreen && fullScreen)
  {
    // save position of windowed mode
    WINDOWINFO wi;
    wi.cbSize = sizeof(WINDOWINFO);
    if(GetWindowInfo(m_hWnd, &wi))
    {
      m_nLeft = wi.rcClient.left;
      m_nTop = wi.rcClient.top;
      m_ValidWindowedPosition = true;
    }
    else
      CLog::Log(LOGERROR, "%s : GetWindowInfo failed with %d", __FUNCTION__, GetLastError());
  }

  m_bFullScreen = fullScreen;
  m_nScreen = res.iScreen;
  m_nWidth  = res.iWidth;
  m_nHeight = res.iHeight;
  m_bBlankOtherDisplay = blankOtherDisplays;

  if (fullScreen && CServiceBroker::GetSettings().GetBool(CSettings::SETTING_VIDEOSCREEN_FAKEFULLSCREEN))
    ChangeResolution(res, forceResChange);

  ResizeInternal(forceResize);

  BlankNonActiveMonitors(m_bBlankOtherDisplay);

  m_IsAlteringWindow = false;
#endif
  return true;
}

bool CWinSystemWin10::DPIChanged(WORD dpi, RECT windowRect)
{
  (void)dpi;
  RECT resizeRect = windowRect;

  CLog::Log(LOGERROR, "%s is not implemented", __FUNCTION__);

#if 0
  HMONITOR hMon = MonitorFromRect(&resizeRect, MONITOR_DEFAULTTONULL);
  if (hMon == NULL)
  {
    hMon = MonitorFromWindow(m_hWnd, MONITOR_DEFAULTTOPRIMARY);
  }

  if (hMon)
  {
    MONITORINFOEX monitorInfo;
    monitorInfo.cbSize = sizeof(MONITORINFOEX);
    GetMonitorInfo(hMon, &monitorInfo);
    RECT wr = monitorInfo.rcWork;
    long wrWidth = wr.right - wr.left;
    long wrHeight = wr.bottom - wr.top;
    long resizeWidth = resizeRect.right - resizeRect.left;
    long resizeHeight = resizeRect.bottom - resizeRect.top;

    if (resizeWidth > wrWidth)
    {
      resizeRect.right = resizeRect.left + wrWidth;
    }

    // make sure suggested windows size is not taller or wider than working area of new monitor (considers the toolbar)
    if (resizeHeight > wrHeight)
    {
      resizeRect.bottom = resizeRect.top + wrHeight;
    }
  }

  // resize the window to the suggested size. Will generate a WM_SIZE event
  SetWindowPos(m_hWnd,
    NULL,
    resizeRect.left,
    resizeRect.top,
    resizeRect.right - resizeRect.left,
    resizeRect.bottom - resizeRect.top,
    SWP_NOZORDER | SWP_NOACTIVATE);
#endif
  return true;
}


void CWinSystemWin10::RestoreDesktopResolution(int screen)
{
  int resIdx = RES_DESKTOP;
  for (int idx = RES_DESKTOP; idx < RES_DESKTOP + GetNumScreens(); idx++)
  {
    if (CDisplaySettings::GetInstance().GetResolutionInfo(idx).iScreen == screen)
    {
      resIdx = idx;
      break;
    }
  }
  ChangeResolution(CDisplaySettings::GetInstance().GetResolutionInfo(resIdx));
}

const MONITOR_DETAILS* CWinSystemWin10::GetMonitor(int screen) const
{
  for (unsigned int monitor = 0; monitor < m_MonitorsInfo.size(); monitor++)
    if (m_MonitorsInfo[monitor].ScreenNumber == screen)
      return &m_MonitorsInfo[monitor];

  // What to do if monitor is not found? Not sure... use the primary screen as a default value.
  if (m_nPrimary >= 0 && static_cast<size_t>(m_nPrimary) < m_MonitorsInfo.size())
  {
    CLog::LogFunction(LOGDEBUG, __FUNCTION__, "no monitor found for screen %i, "
                      "will use primary screen %i", screen, m_nPrimary);
    return &m_MonitorsInfo[m_nPrimary];
  }
  else
  {
    CLog::LogFunction(LOGERROR, __FUNCTION__, "no monitor found for screen %i", screen);
    return nullptr;
  }
}

int CWinSystemWin10::GetCurrentScreen()
{
  CLog::Log(LOGERROR, "%s is not implemented", __FUNCTION__);
#if 0
  HMONITOR hMonitor = MonitorFromWindow(m_hWnd, MONITOR_DEFAULTTOPRIMARY);
  for (unsigned int monitor = 0; monitor < m_MonitorsInfo.size(); monitor++)
    if (m_MonitorsInfo[monitor].hMonitor == hMonitor)
      return m_MonitorsInfo[monitor].ScreenNumber;
  // primary as fallback - v. strange if this ever happens
#endif
  return 0;
}

RECT CWinSystemWin10::ScreenRect(int screen)
{
  RECT rc{ 0,0, 640, 480 };

  CLog::Log(LOGERROR, "%s is not implemented", __FUNCTION__);
#if 0
  const MONITOR_DETAILS* details = GetMonitor(screen);

  if (!details)
  {
    CLog::LogFunction(LOGERROR, __FUNCTION__, "no monitor found for screen %i", screen);
  }

  DEVMODEW sDevMode;
  ZeroMemory(&sDevMode, sizeof(sDevMode));
  sDevMode.dmSize = sizeof(sDevMode);
  if(!EnumDisplaySettingsW(details->DeviceNameW.c_str(), ENUM_CURRENT_SETTINGS, &sDevMode))
    CLog::Log(LOGERROR, "%s : EnumDisplaySettings failed with %d", __FUNCTION__, GetLastError());

  rc.left = sDevMode.dmPosition.x;
  rc.right = sDevMode.dmPosition.x + sDevMode.dmPelsWidth;
  rc.top = sDevMode.dmPosition.y;
  rc.bottom = sDevMode.dmPosition.y + sDevMode.dmPelsHeight;
#endif
  return rc;
}

bool CWinSystemWin10::ResizeInternal(bool forceRefresh)
{
  CLog::Log(LOGERROR, "%s is not implemented", __FUNCTION__);


#if 0
  if (m_hWnd == NULL)
    return false;
  DWORD dwStyle = WS_CLIPCHILDREN;
  HWND windowAfter;
  RECT rc;

  if(m_bFullScreen)
  {
    dwStyle |= WS_POPUP;
    windowAfter = HWND_TOP;
    rc = ScreenRect(m_nScreen);
  }
  else
  {
    dwStyle |= WS_OVERLAPPEDWINDOW;
    windowAfter = g_advancedSettings.m_alwaysOnTop ? HWND_TOPMOST : HWND_NOTOPMOST;

    rc.left = m_nLeft;
    rc.right = m_nLeft + m_nWidth;
    rc.top = m_nTop;
    rc.bottom = m_nTop + m_nHeight;

    HMONITOR hMon = MonitorFromRect(&rc, MONITOR_DEFAULTTONULL);
    HMONITOR hMon2 = MonitorFromWindow(m_hWnd, MONITOR_DEFAULTTOPRIMARY);

    // hasn't been windowed yet, or windowed position would not fullscreen to the same screen we were fullscreen on?
    // -> center on the screen that we were fullscreen on
    if(!m_ValidWindowedPosition || hMon == NULL || hMon != hMon2)
    {
      RECT newScreenRect = ScreenRect(GetCurrentScreen());
      rc.left = m_nLeft = newScreenRect.left + ((newScreenRect.right - newScreenRect.left) / 2) - (m_nWidth / 2);
      rc.top  = m_nTop  =  newScreenRect.top + ((newScreenRect.bottom - newScreenRect.top) / 2) - (m_nHeight / 2);
      rc.right = m_nLeft + m_nWidth;
      rc.bottom = m_nTop + m_nHeight;
      m_ValidWindowedPosition = true;
    }

    AdjustWindowRect( &rc, WS_OVERLAPPEDWINDOW, false );
  }

  WINDOWINFO wi;
  wi.cbSize = sizeof (WINDOWINFO);
  if(!GetWindowInfo(m_hWnd, &wi))
  {
    CLog::Log(LOGERROR, "%s : GetWindowInfo failed with %d", __FUNCTION__, GetLastError());
    return false;
  }
  RECT wr = wi.rcWindow;

  if (forceRefresh || wr.bottom  - wr.top != rc.bottom - rc.top || wr.right - wr.left != rc.right - rc.left ||
                     (wi.dwStyle & WS_CAPTION) != (dwStyle & WS_CAPTION))
  {
    CLog::Log(LOGDEBUG, "%s - resizing due to size change (%d,%d,%d,%d%s)->(%d,%d,%d,%d%s)",__FUNCTION__,wr.left, wr.top, wr.right, wr.bottom, (wi.dwStyle & WS_CAPTION) ? "" : " fullscreen",
                                                                                                         rc.left, rc.top, rc.right, rc.bottom, (dwStyle & WS_CAPTION) ? "" : " fullscreen");
    SetWindowRgn(m_hWnd, 0, false);
    SetWindowLong(m_hWnd, GWL_STYLE, dwStyle);

    // The SWP_DRAWFRAME is here because, perversely, without it win7 draws a
    // white frame plus titlebar around the xbmc splash
    SetWindowPos(m_hWnd, windowAfter, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, SWP_SHOWWINDOW|SWP_DRAWFRAME);

    //! @todo Probably only need this if switching screens
    ValidateRect(NULL, NULL);
  }
#endif
  return true;
}

bool CWinSystemWin10::ChangeResolution(const RESOLUTION_INFO& res, bool forceChange /*= false*/)
{
  CLog::Log(LOGERROR, "%s is not implemented", __FUNCTION__);

#if 0
  const MONITOR_DETAILS* details = GetMonitor(res.iScreen);

  if (!details)
    return false;

  DEVMODEW sDevMode;
  ZeroMemory(&sDevMode, sizeof(sDevMode));
  sDevMode.dmSize = sizeof(sDevMode);

  // If we can't read the current resolution or any detail of the resolution is different than res
  if (!EnumDisplaySettingsW(details->DeviceNameW.c_str(), ENUM_CURRENT_SETTINGS, &sDevMode) ||
      sDevMode.dmPelsWidth != res.iWidth || sDevMode.dmPelsHeight != res.iHeight ||
      sDevMode.dmDisplayFrequency != (int)res.fRefreshRate ||
      ((sDevMode.dmDisplayFlags & DM_INTERLACED) && !(res.dwFlags & D3DPRESENTFLAG_INTERLACED)) ||
      (!(sDevMode.dmDisplayFlags & DM_INTERLACED) && (res.dwFlags & D3DPRESENTFLAG_INTERLACED)) 
      || forceChange)
  {
    ZeroMemory(&sDevMode, sizeof(sDevMode));
    sDevMode.dmSize = sizeof(sDevMode);
    sDevMode.dmDriverExtra = 0;
    sDevMode.dmPelsWidth = res.iWidth;
    sDevMode.dmPelsHeight = res.iHeight;
    sDevMode.dmDisplayFrequency = (int)res.fRefreshRate;
    sDevMode.dmDisplayFlags = (res.dwFlags & D3DPRESENTFLAG_INTERLACED) ? DM_INTERLACED : 0;
    sDevMode.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT | DM_DISPLAYFREQUENCY | DM_DISPLAYFLAGS;

    LONG rc = DISP_CHANGE_SUCCESSFUL;
    bool bResChanged = false;

    // Windows 8 refresh rate workaround for 24.0, 48.0 and 60.0 Hz
    if (CSysInfo::IsWindowsVersionAtLeast(CSysInfo::WindowsVersionWin8) && (res.fRefreshRate == 24.0 || res.fRefreshRate == 48.0 || res.fRefreshRate == 60.0))
    {
      CLog::Log(LOGDEBUG, "%s : Using Windows 8+ workaround for refresh rate %d Hz", __FUNCTION__, (int)res.fRefreshRate);

      // Get current resolution stored in registry
      DEVMODEW sDevModeRegistry;
      ZeroMemory(&sDevModeRegistry, sizeof(sDevModeRegistry));
      sDevModeRegistry.dmSize = sizeof(sDevModeRegistry);
      if (EnumDisplaySettingsW(details->DeviceNameW.c_str(), ENUM_REGISTRY_SETTINGS, &sDevModeRegistry))
      {
        // Set requested mode in registry without actually changing resolution
        rc = ChangeDisplaySettingsExW(details->DeviceNameW.c_str(), &sDevMode, NULL, CDS_UPDATEREGISTRY | CDS_NORESET, NULL);
        if (rc == DISP_CHANGE_SUCCESSFUL)
        {
          // Change resolution based on registry setting
          rc = ChangeDisplaySettingsExW(details->DeviceNameW.c_str(), NULL, NULL, CDS_FULLSCREEN, NULL);
          if (rc == DISP_CHANGE_SUCCESSFUL)
            bResChanged = true;
          else
            CLog::Log(LOGERROR, "%s : ChangeDisplaySettingsEx (W8+ change resolution) failed with %d, using fallback", __FUNCTION__, rc);

          // Restore registry with original values
          sDevModeRegistry.dmSize = sizeof(sDevModeRegistry);
          sDevModeRegistry.dmDriverExtra = 0;
          sDevModeRegistry.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT | DM_DISPLAYFREQUENCY | DM_DISPLAYFLAGS;
          rc = ChangeDisplaySettingsExW(details->DeviceNameW.c_str(), &sDevModeRegistry, NULL, CDS_UPDATEREGISTRY | CDS_NORESET, NULL);
          if (rc != DISP_CHANGE_SUCCESSFUL)
            CLog::Log(LOGERROR, "%s : ChangeDisplaySettingsEx (W8+ restore registry) failed with %d", __FUNCTION__, rc);
        }
        else
          CLog::Log(LOGERROR, "%s : ChangeDisplaySettingsEx (W8+ set registry) failed with %d, using fallback", __FUNCTION__, rc);
      }
      else
        CLog::Log(LOGERROR, "%s : Unable to retrieve registry settings for Windows 8+ workaround, using fallback", __FUNCTION__);
    }

    // Standard resolution change/fallback for Windows 8+ workaround
    if (!bResChanged)
    {
      // CDS_FULLSCREEN is for temporary fullscreen mode and prevents icons and windows from moving
      // to fit within the new dimensions of the desktop
      rc = ChangeDisplaySettingsExW(details->DeviceNameW.c_str(), &sDevMode, NULL, CDS_FULLSCREEN, NULL);
      if (rc == DISP_CHANGE_SUCCESSFUL)
        bResChanged = true;
      else
        CLog::Log(LOGERROR, "%s : ChangeDisplaySettingsEx failed with %d", __FUNCTION__, rc);
    }
    
    if (bResChanged) 
      ResolutionChanged();

    return bResChanged;
  }
#endif
  // nothing to do, return success
  return true;
}


void CWinSystemWin10::UpdateResolutions()
{
  CLog::Log(LOGERROR, "%s is not implemented", __FUNCTION__);

  m_MonitorsInfo.clear();
  CWinSystemBase::UpdateResolutions();
  UpdateResolutionsInternal();

  if(m_MonitorsInfo.empty())
    return;

  float refreshRate = 0;
  int w = 0;
  int h = 0;
  uint32_t dwFlags;

  // Primary
  m_MonitorsInfo[m_nPrimary].ScreenNumber = 0;
  w = m_MonitorsInfo[m_nPrimary].ScreenWidth;
  h = m_MonitorsInfo[m_nPrimary].ScreenHeight;
  if( (m_MonitorsInfo[m_nPrimary].RefreshRate == 59) || (m_MonitorsInfo[m_nPrimary].RefreshRate == 29) || (m_MonitorsInfo[m_nPrimary].RefreshRate == 23) )
    refreshRate = (float)(m_MonitorsInfo[m_nPrimary].RefreshRate + 1) / 1.001f;
  else
    refreshRate = (float)m_MonitorsInfo[m_nPrimary].RefreshRate;
  dwFlags = m_MonitorsInfo[m_nPrimary].Interlaced ? D3DPRESENTFLAG_INTERLACED : 0;

  UpdateDesktopResolution(CDisplaySettings::GetInstance().GetResolutionInfo(RES_DESKTOP), 0, w, h, refreshRate, dwFlags);
  CLog::Log(LOGNOTICE, "Primary mode: %s", CDisplaySettings::GetInstance().GetResolutionInfo(RES_DESKTOP).strMode.c_str());

  // Desktop resolution of the other screens
  if(m_MonitorsInfo.size() >= 2)
  {
    int xbmcmonitor = 1;  // The screen number+1 showed in the GUI display settings

    for (unsigned int monitor = 0; monitor < m_MonitorsInfo.size(); monitor++)
    {
      if (monitor != m_nPrimary)
      {
        m_MonitorsInfo[monitor].ScreenNumber = xbmcmonitor;
        w = m_MonitorsInfo[monitor].ScreenWidth;
        h = m_MonitorsInfo[monitor].ScreenHeight;
        if( (m_MonitorsInfo[monitor].RefreshRate == 59) || (m_MonitorsInfo[monitor].RefreshRate == 29) || (m_MonitorsInfo[monitor].RefreshRate == 23) )
          refreshRate = (float)(m_MonitorsInfo[monitor].RefreshRate + 1) / 1.001f;
        else
          refreshRate = (float)m_MonitorsInfo[monitor].RefreshRate;
        dwFlags = m_MonitorsInfo[monitor].Interlaced ? D3DPRESENTFLAG_INTERLACED : 0;

        RESOLUTION_INFO res;
        UpdateDesktopResolution(res, xbmcmonitor++, w, h, refreshRate, dwFlags);
        CDisplaySettings::GetInstance().AddResolutionInfo(res);
        CLog::Log(LOGNOTICE, "Secondary mode: %s", res.strMode.c_str());
      }
    }
  }

#if 0
  // The rest of the resolutions. The order is not important.
  for (unsigned int monitor = 0; monitor < m_MonitorsInfo.size(); monitor++)
  {
    for(int mode = 0;; mode++)
    {
      DEVMODEW devmode;
      ZeroMemory(&devmode, sizeof(devmode));
      devmode.dmSize = sizeof(devmode);
      if(EnumDisplaySettingsW(m_MonitorsInfo[monitor].DeviceNameW.c_str(), mode, &devmode) == 0)
        break;
      if(devmode.dmBitsPerPel != 32)
        continue;

      float refreshRate;
      if(devmode.dmDisplayFrequency == 59 || devmode.dmDisplayFrequency == 29 || devmode.dmDisplayFrequency == 23)
        refreshRate = (float)(devmode.dmDisplayFrequency + 1) / 1.001f;
      else
        refreshRate = (float)(devmode.dmDisplayFrequency);
      dwFlags = (devmode.dmDisplayFlags & DM_INTERLACED) ? D3DPRESENTFLAG_INTERLACED : 0;

      RESOLUTION_INFO res;
      UpdateDesktopResolution(res, m_MonitorsInfo[monitor].ScreenNumber, devmode.dmPelsWidth, devmode.dmPelsHeight, refreshRate, dwFlags);
      AddResolution(res);
      CLog::Log(LOGNOTICE, "Additional mode: %s", res.strMode.c_str());
    }
  }
#endif
}

void CWinSystemWin10::AddResolution(const RESOLUTION_INFO &res)
{
  for (unsigned int i = 0; i < CDisplaySettings::GetInstance().ResolutionInfoSize(); i++)
  {
    if (CDisplaySettings::GetInstance().GetResolutionInfo(i).iScreen      == res.iScreen &&
        CDisplaySettings::GetInstance().GetResolutionInfo(i).iWidth       == res.iWidth &&
        CDisplaySettings::GetInstance().GetResolutionInfo(i).iHeight      == res.iHeight &&
        CDisplaySettings::GetInstance().GetResolutionInfo(i).iScreenWidth == res.iScreenWidth &&
        CDisplaySettings::GetInstance().GetResolutionInfo(i).iScreenHeight== res.iScreenHeight &&
        CDisplaySettings::GetInstance().GetResolutionInfo(i).fRefreshRate == res.fRefreshRate &&
        CDisplaySettings::GetInstance().GetResolutionInfo(i).dwFlags      == res.dwFlags)
      return; // already have this resolution
  }

  CDisplaySettings::GetInstance().AddResolutionInfo(res);
}

bool CWinSystemWin10::UpdateResolutionsInternal()
{
  CLog::Log(LOGNOTICE, "Win10 UWP Found screen. Need to update code to use DirectX!");

  auto devRes = DX::DeviceResources::Get();
  auto dispatcher = devRes->GetDispatcher();

  auto handler = ref new DispatchedHandler([this]()
  {
    MONITOR_DETAILS md = {};

    auto displayInfo = Windows::Graphics::Display::DisplayInformation::GetForCurrentView();
    bool flipResolution = false;
    switch (displayInfo->NativeOrientation)
    {
      case DisplayOrientations::Landscape:
        switch (displayInfo->CurrentOrientation)
        {
        case DisplayOrientations::Portrait:
        case DisplayOrientations::PortraitFlipped:
          flipResolution = true;
          break;
        }
      case DisplayOrientations::Portrait:
        switch (displayInfo->CurrentOrientation)
        {
        case DisplayOrientations::Landscape:
        case DisplayOrientations::LandscapeFlipped:
          flipResolution = true;
          break;
        }
    }
    md.ScreenWidth = flipResolution ? displayInfo->ScreenHeightInRawPixels : displayInfo->ScreenWidthInRawPixels;
    md.ScreenHeight = flipResolution ? displayInfo->ScreenWidthInRawPixels : displayInfo->ScreenHeightInRawPixels;

    // note that refresh rate information is not available on Win10 UWP
    md.RefreshRate = 60;
    md.Interlaced = false;

    m_MonitorsInfo.push_back(md);
  });

  if (dispatcher->HasThreadAccess)
    handler->Invoke();
  else
    Concurrency::create_task(dispatcher->RunAsync(CoreDispatcherPriority::High, handler)).wait();

  return true;
}

using namespace Windows::UI::Core;
using namespace Windows::UI::Xaml;
#include "windowing/WindowingFactory.h"

void CWinSystemWin10::ShowOSMouse(bool show)
{
  DX::DeviceResources::Get()->ShowCursor(show);
}

bool CWinSystemWin10::Minimize()
{
  CLog::Log(LOGERROR, "%s is not implemented", __FUNCTION__);
  // ShowWindow(m_hWnd, SW_MINIMIZE);
  return true;
}
bool CWinSystemWin10::Restore()
{
  CLog::Log(LOGERROR, "%s is not implemented", __FUNCTION__);
  // ShowWindow(m_hWnd, SW_RESTORE);
  return true;
}
bool CWinSystemWin10::Hide()
{
  CLog::Log(LOGERROR, "%s is not implemented", __FUNCTION__);
  // ShowWindow(m_hWnd, SW_HIDE);
  return true;
}
bool CWinSystemWin10::Show(bool raise)
{
  CLog::Log(LOGERROR, "%s is not implemented", __FUNCTION__);
#if 0
  HWND windowAfter = HWND_BOTTOM;
  if (raise)
  {
    if (m_bFullScreen)
      windowAfter = HWND_TOP;
    else
      windowAfter = g_advancedSettings.m_alwaysOnTop ? HWND_TOPMOST : HWND_NOTOPMOST;
  }

  SetWindowPos(m_hWnd, windowAfter, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE|SWP_SHOWWINDOW);
  UpdateWindow(m_hWnd);
  if (raise)
  {
    SetForegroundWindow(m_hWnd);
    SetFocus(m_hWnd);
  }
#endif
  return true;
}

void CWinSystemWin10::Register(IDispResource *resource)
{
  CSingleLock lock(m_resourceSection);
  m_resources.push_back(resource);
}

void CWinSystemWin10::Unregister(IDispResource* resource)
{
  CSingleLock lock(m_resourceSection);
  std::vector<IDispResource*>::iterator i = find(m_resources.begin(), m_resources.end(), resource);
  if (i != m_resources.end())
    m_resources.erase(i);
}

void CWinSystemWin10::OnDisplayLost()
{
  CLog::Log(LOGDEBUG, "%s - notify display lost event", __FUNCTION__);

  // make sure renderer has no invalid references
  KODI::MESSAGING::CApplicationMessenger::GetInstance().SendMsg(TMSG_RENDERER_FLUSH);

  {
    CSingleLock lock(m_resourceSection);
    for (std::vector<IDispResource *>::iterator i = m_resources.begin(); i != m_resources.end(); ++i)
      (*i)->OnLostDisplay();
  }
}

void CWinSystemWin10::OnDisplayReset()
{
  if (!m_delayDispReset)
  {
    CLog::Log(LOGDEBUG, "%s - notify display reset event", __FUNCTION__);
    CSingleLock lock(m_resourceSection);
    for (std::vector<IDispResource *>::iterator i = m_resources.begin(); i != m_resources.end(); ++i)
      (*i)->OnResetDisplay();
  }
}

void CWinSystemWin10::OnDisplayBack()
{
  int delay = CServiceBroker::GetSettings().GetInt("videoscreen.delayrefreshchange");
  if (delay > 0)
  {
    m_delayDispReset = true;
    m_dispResetTimer.Set(delay * 100);
  }
  OnDisplayReset();
}

void CWinSystemWin10::ResolutionChanged()
{
  OnDisplayLost();
  OnDisplayBack();
}

void CWinSystemWin10::SetForegroundWindowInternal(HWND hWnd)
{
  CLog::Log(LOGERROR, "%s is not implemented", __FUNCTION__);
#if 0
  if (!IsWindow(hWnd)) return;

  // if the window isn't focused, bring it to front or SetFullScreen will fail
  BYTE keyState[256] = { 0 };
  // to unlock SetForegroundWindow we need to imitate Alt pressing
  if (GetKeyboardState((LPBYTE)&keyState) && !(keyState[VK_MENU] & 0x80))
    keybd_event(VK_MENU, 0, KEYEVENTF_EXTENDEDKEY | 0, 0);

  BOOL res = SetForegroundWindow(hWnd);

  if (GetKeyboardState((LPBYTE)&keyState) && !(keyState[VK_MENU] & 0x80))
    keybd_event(VK_MENU, 0, KEYEVENTF_EXTENDEDKEY | KEYEVENTF_KEYUP, 0);

  if (!res)
  {
    //relation time of SetForegroundWindow lock
    DWORD lockTimeOut = 0;
    HWND  hCurrWnd = GetForegroundWindow();
    DWORD dwThisTID = GetCurrentThreadId(),
          dwCurrTID = GetWindowThreadProcessId(hCurrWnd, 0);

    // we need to bypass some limitations from Microsoft
    if (dwThisTID != dwCurrTID)
    {
      AttachThreadInput(dwThisTID, dwCurrTID, TRUE);
      SystemParametersInfo(SPI_GETFOREGROUNDLOCKTIMEOUT, 0, &lockTimeOut, 0);
      SystemParametersInfo(SPI_SETFOREGROUNDLOCKTIMEOUT, 0, 0, SPIF_SENDWININICHANGE | SPIF_UPDATEINIFILE);
      AllowSetForegroundWindow(ASFW_ANY);
    }

    SetForegroundWindow(hWnd);

    if (dwThisTID != dwCurrTID)
    {
      SystemParametersInfo(SPI_SETFOREGROUNDLOCKTIMEOUT, 0, (PVOID)lockTimeOut, SPIF_SENDWININICHANGE | SPIF_UPDATEINIFILE);
      AttachThreadInput(dwThisTID, dwCurrTID, FALSE);
    }
  }
#endif
}

#pragma pack(pop)
#endif
