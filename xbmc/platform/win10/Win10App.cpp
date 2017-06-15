/*
 *      Copyright (C) 2005-2017 Team Kodi
 *      http://kodi.tv
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

#include "pch.h"
#include "Win10App.h"

#include <ppltasks.h>
#include "Application.h"
#include "AppParamParser.h"
#include "settings/AdvancedSettings.h"
#include "input/MouseStat.h"
#include "platform/MessagePrinter.h"
#include "platform/XbmcContext.h"
#include "platform/win32/CharsetConverter.h"
#include "rendering/dx/DirectXHelper.h"
#include "utils/Environment.h"
#include "utils/log.h"
#include "windowing/WinEvents.h"
#include "windowing/WindowingFactory.h"
#include "platform/xbmc.h"

using namespace KODI::PLATFORM::WINDOWS10;
using namespace concurrency;
using namespace Windows::ApplicationModel;
using namespace Windows::ApplicationModel::Core;
using namespace Windows::ApplicationModel::Activation;
using namespace Windows::Devices::Input;
using namespace Windows::UI::Core;
using namespace Windows::UI::Input;
using namespace Windows::UI::ViewManagement;
using namespace Windows::System;
using namespace Windows::System::Threading;
using namespace Windows::Foundation;
using namespace Windows::Graphics::Display;
using namespace Windows::Storage;
using namespace Windows::Storage::AccessCache;

extern XBMCKey VK_keymap[XBMCK_LAST];
extern void DIB_InitOSKeymap();

IFrameworkView^ ViewProvider::CreateView()
{
  return ref new App();
}

App::App()
{
}

// The first method called when the IFrameworkView is being created.
void App::Initialize(CoreApplicationView^ applicationView)
{
  // Register event handlers for app lifecycle. This example includes Activated, so that we
  // can make the CoreWindow active and start rendering on the window.
  applicationView->Activated += ref new TypedEventHandler<CoreApplicationView^, IActivatedEventArgs^>(this, &App::OnActivated);

  CoreApplication::Suspending += ref new EventHandler<SuspendingEventArgs^>(this, &App::OnSuspending);
  CoreApplication::Resuming += ref new EventHandler<Platform::Object^>(this, &App::OnResuming);
  // TODO 
  // CoreApplication::UnhandledErrorDetected += ref new EventHandler<UnhandledErrorDetectedEventArgs^>(this, &App::OnUnhandledErrorDetected);

  // At this point we have access to the device. 
  // We can create the device-dependent resources.
  m_deviceResources = DX::DeviceResources::Get();

  DIB_InitOSKeymap();

  // this fixes crash if OPENSSL_CONF is set to existed openssl.cfg  
  // need to set it as soon as possible  
  CEnvironment::unsetenv("OPENSSL_CONF");

  // Initialise Winsock
  WSADATA wd;
  WSAStartup(MAKEWORD(2, 2), &wd);

  // set up some xbmc specific relationships
  setlocale(LC_NUMERIC, "C");
}

// Called when the CoreWindow object is created (or re-created).
void App::SetWindow(CoreWindow^ window)
{
  //CoreApplication::GetCurrentView()->TitleBar->ExtendViewIntoTitleBar = true;
  window->SetPointerCapture();
  window->SizeChanged += ref new TypedEventHandler<CoreWindow^, WindowSizeChangedEventArgs^>(this, &App::OnWindowSizeChanged);
  window->VisibilityChanged += ref new TypedEventHandler<CoreWindow^, VisibilityChangedEventArgs^>(this, &App::OnVisibilityChanged);
  window->Activated += ref new TypedEventHandler<CoreWindow^, WindowActivatedEventArgs^>(this, &App::OnWindowActivationChanged);
  window->Closed += ref new TypedEventHandler<CoreWindow^, CoreWindowEventArgs^>(this, &App::OnWindowClosed);
  window->PointerPressed += ref new TypedEventHandler<CoreWindow^, PointerEventArgs^>(this, &App::OnPointerPressed);
  window->PointerMoved += ref new TypedEventHandler<CoreWindow^, PointerEventArgs^>(this, &App::OnPointerMoved);
  window->PointerReleased += ref new TypedEventHandler<CoreWindow^, PointerEventArgs^>(this, &App::OnPointerReleased);
  window->PointerWheelChanged += ref new TypedEventHandler<CoreWindow^, PointerEventArgs^>(this, &App::OnPointerWheelChanged);
  window->KeyDown += ref new TypedEventHandler<CoreWindow^, KeyEventArgs^>(this, &App::OnKeyDown);
  window->KeyUp += ref new TypedEventHandler<CoreWindow^, KeyEventArgs^>(this, &App::OnKeyUp);
  window->CharacterReceived += ref new TypedEventHandler<CoreWindow^, CharacterReceivedEventArgs^>(this, &App::OnCharacterReceived);

  DisplayInformation^ currentDisplayInformation = DisplayInformation::GetForCurrentView();
  currentDisplayInformation->DpiChanged += ref new TypedEventHandler<DisplayInformation^, Object^>(this, &App::OnDpiChanged);
  currentDisplayInformation->OrientationChanged += ref new TypedEventHandler<DisplayInformation^, Object^>(this, &App::OnOrientationChanged);

  DisplayInformation::DisplayContentsInvalidated += ref new TypedEventHandler<DisplayInformation^, Object^>(this, &App::OnDisplayContentsInvalidated);

  m_deviceResources->SetWindow(window);
}

// Initializes scene resources, or loads a previously saved app state.
void App::Load(Platform::String^ entryPoint)
{
}

// This method is called after the window becomes active.
void App::Run()
{
  // Create a task that will be run on a background thread.
  auto dispatcher = CoreWindow::GetForCurrentThread()->Dispatcher;
  // start a thread dedicated to rendering and dedicate the UI thread to input processing.
  auto workItemHandler = ref new WorkItemHandler([this, dispatcher](IAsyncAction^action)
  {
    AppRun(dispatcher);
  });
  // Run task on a dedicated high priority background thread.
  m_renderLoopWorker = ThreadPool::RunAsync(workItemHandler, WorkItemPriority::High, WorkItemOptions::TimeSliced);

  // ProcessUntilQuit will block the thread and process events as they appear until the App terminates.
  dispatcher->ProcessEvents(CoreProcessEventsOption::ProcessUntilQuit);
}

// Required for IFrameworkView.
// Terminate events do not cause Uninitialize to be called. It will be called if your IFrameworkView
// class is torn down while the app is in the foreground.
void App::Uninitialize()
{
}

void App::AppRun(CoreDispatcher^ dispatcher)
{
  {
    XBMC::Context context;
    // Initialize before CAppParamParser so it can set the log level
    g_advancedSettings.Initialize();

    // fix the case then window opened in FS, but current setting is RES_WINDOW
    // the proper way is make window params related to setting, but in this setting isn't loaded yet
    // perhaps we should observe setting changes and change window's Preffered props 
    Concurrency::create_task(dispatcher->RunAsync(CoreDispatcherPriority::High, ref new DispatchedHandler([]()
    {
      auto appView = ApplicationView::GetForCurrentView();
      g_advancedSettings.m_startFullScreen = appView->IsFullScreenMode;
    }))).wait();

    CAppParamParser appParamParser;
    appParamParser.Parse(m_argv.data(), m_argv.size());
    // Create and run the app
    XBMC_Run(true, appParamParser.m_playlist);
  }

  dispatcher->RunAsync(CoreDispatcherPriority::Normal, ref new DispatchedHandler([]()
  {
    Windows::ApplicationModel::Core::CoreApplication::Exit();
  }));
}

void push_back(std::vector<char*> &vec, std::string &str)
{
  if (!str.empty())
  {
    char *val = new char[str.length() + 1];
    std::strcpy(val, str.c_str());
    vec.push_back(val);
  }
}

// Application lifecycle event handlers.
void App::OnActivated(CoreApplicationView^ applicationView, IActivatedEventArgs^ args)
{
  m_argv.clear();
  push_back(m_argv, std::string("dummy"));

  // Check for protocol activation
  if (args->Kind == ActivationKind::Protocol)
  {
    auto protocolArgs = static_cast< ProtocolActivatedEventArgs^>(args);
    Platform::String^ argval = protocolArgs->Uri->ToString();
    // Manipulate arguments …
  }
  // Check for file activation
  else if (args->Kind == ActivationKind::File)
  {
    auto fileArgs = static_cast<FileActivatedEventArgs^>(args);
    if (fileArgs && fileArgs->Files && fileArgs->Files->Size > 0)
    {
      using KODI::PLATFORM::WINDOWS::FromW;
      for(auto file : fileArgs->Files)
      {
        // add file to FAL to get access to it later
        StorageApplicationPermissions::FutureAccessList->Clear();
        StorageApplicationPermissions::FutureAccessList->Add(file, file->Path);

        std::string filePath = FromW(file->Path->Data(), file->Path->Length());
        push_back(m_argv, filePath);
      }
    }
  }
  // Run() won't start until the CoreWindow is activated.
  CoreWindow::GetForCurrentThread()->Activate();
}

void App::OnSuspending(Platform::Object^ sender, SuspendingEventArgs^ args)
{
	// Save app state asynchronously after requesting a deferral. Holding a deferral
	// indicates that the application is busy performing suspending operations. Be
	// aware that a deferral may not be held indefinitely. After about five seconds,
	// the app will be forced to exit.
	SuspendingDeferral^ deferral = args->SuspendingOperation->GetDeferral();

	create_task([this, deferral]()
	{
    m_deviceResources->Trim();
		// Insert your code here.
		deferral->Complete();
	});
}

void App::OnResuming(Platform::Object^ sender, Platform::Object^ args)
{
	// Restore any data or state that was unloaded on suspend. By default, data
	// and state are persisted when resuming from suspend. Note that this event
	// does not occur if the app was previously terminated.

	// Insert your code here.
}

void App::UpdateWindowSize(float width, float height)
{
  auto size = m_deviceResources->GetOutputSize();

  CLog::Log(LOGDEBUG, __FUNCTION__": window resize event %f x %f (as:%s)", size.Width, size.Height, g_advancedSettings.m_fullScreen ? "true" : "false");

  auto appView = ApplicationView::GetForCurrentView(); 
  appView->SetDesiredBoundsMode(ApplicationViewBoundsMode::UseCoreWindow);

  // seems app has lost FS mode
  if (g_advancedSettings.m_fullScreen && !appView->IsFullScreenMode)
    g_advancedSettings.m_fullScreen = false;

  XBMC_Event newEvent;
  memset(&newEvent, 0, sizeof(newEvent));
  newEvent.type = XBMC_VIDEORESIZE;
  newEvent.resize.w = size.Width;
  newEvent.resize.h = size.Height;
  CWinEvents::MessagePush(&newEvent);
}

// Window event handlers.

void App::OnWindowSizeChanged(CoreWindow^ sender, WindowSizeChangedEventArgs^ args)
{
  {
    critical_section::scoped_lock lock(m_deviceResources->GetCriticalSection());
    m_deviceResources->SetLogicalSize(Size(sender->Bounds.Width, sender->Bounds.Height));
  }
  auto size = m_deviceResources->GetOutputSize();
  UpdateWindowSize(size.Width, size.Height);
}

void App::OnVisibilityChanged(CoreWindow^ sender, VisibilityChangedEventArgs^ args)
{
  bool active = g_application.GetRenderGUI();
  g_application.SetRenderGUI(args->Visible);
  if (g_application.GetRenderGUI() != active)
    g_Windowing.NotifyAppActiveChange(g_application.GetRenderGUI());
  CLog::Log(LOGDEBUG, __FUNCTION__": window is %s", g_application.GetRenderGUI() ? "shown" : "hidden");
}

void App::OnWindowActivationChanged(CoreWindow ^ sender, WindowActivatedEventArgs ^ args)
{
  bool active = g_application.GetRenderGUI();
  if (args->WindowActivationState == CoreWindowActivationState::Deactivated)
  {
    g_application.SetRenderGUI(g_Windowing.WindowedMode());
  }
  else if (args->WindowActivationState == CoreWindowActivationState::PointerActivated
    || args->WindowActivationState == CoreWindowActivationState::CodeActivated)
  {
    g_application.SetRenderGUI(true);
  }
  if (g_application.GetRenderGUI() != active)
    g_Windowing.NotifyAppActiveChange(g_application.GetRenderGUI());
  CLog::Log(LOGDEBUG, __FUNCTION__": window is %s", g_application.GetRenderGUI() ? "active" : "inactive");
}

void App::OnWindowClosed(CoreWindow^ sender, CoreWindowEventArgs^ args)
{
  // send quit command to the application if it's still running
  if (!g_application.m_bStop)
  {
    XBMC_Event newEvent;
    memset(&newEvent, 0, sizeof(newEvent));
    newEvent.type = XBMC_QUIT;
    CWinEvents::MessagePush(&newEvent);
  }
}

int mouseButton = 0;

void App::OnPointerPressed(CoreWindow ^ sender, PointerEventArgs ^ args)
{
  XBMC_Event newEvent;
  memset(&newEvent, 0, sizeof(newEvent));

  PointerPoint^ point = args->CurrentPoint;
  auto dpi = m_deviceResources->GetDpi();

  /*if (point->PointerDevice->PointerDeviceType == PointerDeviceType::Touch)
  {
    newEvent.type = XBMC_TOUCH;
    newEvent.touch.type = XBMC_TOUCH;
  }
  else*/
  {
    newEvent.type = XBMC_MOUSEBUTTONDOWN;
    newEvent.button.x = DX::ConvertDipsToPixels(point->Position.X, dpi);
    newEvent.button.y = DX::ConvertDipsToPixels(point->Position.Y, dpi);
    if (point->PointerDevice->PointerDeviceType == PointerDeviceType::Mouse)
    {
      if (point->Properties->IsLeftButtonPressed)
        newEvent.button.button = XBMC_BUTTON_LEFT;
      else if (point->Properties->IsMiddleButtonPressed)
        newEvent.button.button = XBMC_BUTTON_MIDDLE;
      else if (point->Properties->IsRightButtonPressed)
        newEvent.button.button = XBMC_BUTTON_RIGHT;
    }
    else if(point->PointerDevice->PointerDeviceType == PointerDeviceType::Touch)
    {
      newEvent.button.button = XBMC_BUTTON_LEFT;
    }
    else if(point->PointerDevice->PointerDeviceType == PointerDeviceType::Pen)
    {
      // pen
      // TODO
    }
    mouseButton = newEvent.button.button;
  }
  CWinEvents::MessagePush(&newEvent);
}

void App::OnPointerMoved(CoreWindow^ sender, PointerEventArgs^ args)
{
  PointerPoint^ point = args->CurrentPoint;
  auto dpi = m_deviceResources->GetDpi();

  XBMC_Event newEvent;
  memset(&newEvent, 0, sizeof(newEvent));
  newEvent.type = XBMC_MOUSEMOTION;
  newEvent.button.x = DX::ConvertDipsToPixels(point->Position.X, dpi);
  newEvent.button.y = DX::ConvertDipsToPixels(point->Position.Y, dpi);
  newEvent.button.button = 0;
  CWinEvents::MessagePush(&newEvent);
}

void App::OnPointerReleased(CoreWindow^ sender, PointerEventArgs^ args)
{
  PointerPoint^ point = args->CurrentPoint;
  auto dpi = m_deviceResources->GetDpi();

  XBMC_Event newEvent;
  memset(&newEvent, 0, sizeof(newEvent));
  newEvent.type = XBMC_MOUSEBUTTONUP;
  newEvent.button.x = DX::ConvertDipsToPixels(point->Position.X, dpi);
  newEvent.button.y = DX::ConvertDipsToPixels(point->Position.Y, dpi);

  // use cached value from OnPressed event
  newEvent.button.button = mouseButton;
  CWinEvents::MessagePush(&newEvent);
}

void App::OnPointerWheelChanged(CoreWindow^ sender, PointerEventArgs^ args)
{
  XBMC_Event newEvent;
  memset(&newEvent, 0, sizeof(newEvent));
  newEvent.type = XBMC_MOUSEBUTTONDOWN;
  newEvent.button.x = args->CurrentPoint->Position.X;
  newEvent.button.y = args->CurrentPoint->Position.Y;
  newEvent.button.button = args->CurrentPoint->Properties->MouseWheelDelta > 0 ? XBMC_BUTTON_WHEELUP : XBMC_BUTTON_WHEELDOWN;
  CWinEvents::MessagePush(&newEvent);
  newEvent.type = XBMC_MOUSEBUTTONUP;
  CWinEvents::MessagePush(&newEvent);
}

void App::TranslateKey(CoreWindow^ window, XBMC_keysym &keysym, VirtualKey vkey, unsigned scancode, unsigned keycode)
{
  keysym.sym = VK_keymap[static_cast<UINT>(vkey)];
  keysym.unicode = 0;
  keysym.scancode = scancode;

  if (window->GetKeyState(VirtualKey::NumberKeyLock) == CoreVirtualKeyStates::Locked
    && vkey >= VirtualKey::NumberPad0 && vkey <= VirtualKey::NumberPad9)
  {
    keysym.unicode = static_cast<UINT>(vkey - VirtualKey::NumberPad0) + '0';
  }
  else
  {
    keysym.unicode = keycode;
  }

  uint16_t mod = (uint16_t)XBMCKMOD_NONE;

  // If left control and right alt are down this usually means that AltGr is down
  if (window->GetKeyState(VirtualKey::LeftControl) == CoreVirtualKeyStates::Down 
    && window->GetKeyState(VirtualKey::RightMenu) == CoreVirtualKeyStates::Down)
  {
    mod |= XBMCKMOD_MODE;
    mod |= XBMCKMOD_MODE;
  }
  else
  {
    if (window->GetKeyState(VirtualKey::LeftControl) == CoreVirtualKeyStates::Down) 
      mod |= XBMCKMOD_LCTRL;
    if (window->GetKeyState(VirtualKey::RightMenu) == CoreVirtualKeyStates::Down) 
      mod |= XBMCKMOD_RALT;
  }

  // Check the remaining modifiers
  if (window->GetKeyState(VirtualKey::LeftShift) == CoreVirtualKeyStates::Down) 
    mod |= XBMCKMOD_LSHIFT;
  if (window->GetKeyState(VirtualKey::RightShift) == CoreVirtualKeyStates::Down) 
    mod |= XBMCKMOD_RSHIFT;
  if (window->GetKeyState(VirtualKey::RightControl) == CoreVirtualKeyStates::Down) 
    mod |= XBMCKMOD_RCTRL;
  if (window->GetKeyState(VirtualKey::LeftMenu) == CoreVirtualKeyStates::Down) 
    mod |= XBMCKMOD_LALT;
  if (window->GetKeyState(VirtualKey::LeftWindows) == CoreVirtualKeyStates::Down) 
    mod |= XBMCKMOD_LSUPER;
  if (window->GetKeyState(VirtualKey::RightWindows) == CoreVirtualKeyStates::Down) 
    mod |= XBMCKMOD_LSUPER;
  keysym.mod = (XBMCMod)mod;
}

VirtualKey keyDown = VirtualKey::None;

void App::OnKeyDown(CoreWindow^ sender, KeyEventArgs^ args)
{
  if (!args->KeyStatus.IsExtendedKey)
  {
    // it will be handled in OnCharacterReceived
    // store VirtualKey for future processing
    keyDown = args->VirtualKey;
    return;
  }
  XBMC_keysym keysym;
  TranslateKey(sender, keysym, args->VirtualKey, args->KeyStatus.ScanCode, 0);

  XBMC_Event newEvent;
  memset(&newEvent, 0, sizeof(newEvent));
  newEvent.type = XBMC_KEYDOWN;
  newEvent.key.keysym = keysym;
  CWinEvents::MessagePush(&newEvent);
}

void App::OnKeyUp(CoreWindow^ sender, KeyEventArgs^ args)
{
  XBMC_keysym keysym;
  TranslateKey(sender, keysym, args->VirtualKey, args->KeyStatus.ScanCode, 0);

  XBMC_Event newEvent;
  memset(&newEvent, 0, sizeof(newEvent));
  newEvent.type = XBMC_KEYUP;
  newEvent.key.keysym = keysym;
  CWinEvents::MessagePush(&newEvent);
}

using namespace Windows::Devices::Input;

void App::OnCharacterReceived(CoreWindow^ sender, CharacterReceivedEventArgs^ args)
{
  XBMC_keysym keysym;
  TranslateKey(sender, keysym, keyDown, args->KeyStatus.ScanCode, args->KeyCode);

  XBMC_Event newEvent;
  memset(&newEvent, 0, sizeof(newEvent));
  newEvent.key.keysym = keysym;
  newEvent.type = XBMC_KEYDOWN;
  CWinEvents::MessagePush(&newEvent);
}

// DisplayInformation event handlers.

void App::OnDpiChanged(DisplayInformation^ sender, Object^ args)
{
  // Note: The value for LogicalDpi retrieved here may not match the effective DPI of the app
  // if it is being scaled for high resolution devices. Once the DPI is set on DeviceResources,
  // you should always retrieve it using the GetDpi method.
  // See DeviceResources.cpp for more details.
  critical_section::scoped_lock lock(m_deviceResources->GetCriticalSection());
  m_deviceResources->SetDpi(sender->LogicalDpi);

  auto size = m_deviceResources->GetOutputSize();
  UpdateWindowSize(size.Width, size.Height);
}

void App::OnOrientationChanged(DisplayInformation^ sender, Object^ args)
{
  critical_section::scoped_lock lock(m_deviceResources->GetCriticalSection());
  m_deviceResources->SetCurrentOrientation(sender->CurrentOrientation);

  auto size = m_deviceResources->GetOutputSize();
  UpdateWindowSize(size.Width, size.Height);
}

void App::OnDisplayContentsInvalidated(DisplayInformation^ sender, Object^ args)
{
  critical_section::scoped_lock lock(m_deviceResources->GetCriticalSection());
  m_deviceResources->ValidateDevice();
}
