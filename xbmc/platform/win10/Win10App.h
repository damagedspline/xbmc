#pragma once
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

#include "utils/CharsetConverter.h" // Required to initialize converters before usage
#include "rendering/dx/DeviceResources.h"

struct XBMC_keysym;

namespace KODI
{
  namespace PLATFORM
  {
    namespace WINDOWS10
    {
ref class ViewProvider sealed : Windows::ApplicationModel::Core::IFrameworkViewSource
{
public:
  virtual Windows::ApplicationModel::Core::IFrameworkView^ CreateView();
};

// Main entry point for our app. Connects the app with the Windows shell and handles application lifecycle events.
ref class App sealed : public Windows::ApplicationModel::Core::IFrameworkView
{
public:
  App();

  // IFrameworkView Methods.
  virtual void Initialize(Windows::ApplicationModel::Core::CoreApplicationView^ applicationView);
  virtual void SetWindow(Windows::UI::Core::CoreWindow^ window);
  virtual void Load(Platform::String^ entryPoint);
  virtual void Run();
  virtual void Uninitialize();

protected:
  void AppRun(Windows::UI::Core::CoreDispatcher^ dispatcher);

  // Application lifecycle event handlers.
  void OnActivated(Windows::ApplicationModel::Core::CoreApplicationView^ applicationView, Windows::ApplicationModel::Activation::IActivatedEventArgs^ args);
  void OnSuspending(Platform::Object^ sender, Windows::ApplicationModel::SuspendingEventArgs^ args);
  void OnResuming(Platform::Object^ sender, Platform::Object^ args);
  // Window event handlers.
  void OnWindowSizeChanged(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::WindowSizeChangedEventArgs^ args);
  void OnVisibilityChanged(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::VisibilityChangedEventArgs^ args);
  void OnWindowClosed(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::CoreWindowEventArgs^ args);
  void OnWindowActivationChanged(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::WindowActivatedEventArgs^ args);
  // touch mouse and pen
  void OnPointerPressed(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::PointerEventArgs^ args);
  void OnPointerMoved(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::PointerEventArgs^ args);
  void OnPointerReleased(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::PointerEventArgs^ args);
  void OnPointerExited(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::PointerEventArgs^ args);
  void OnPointerWheelChanged(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::PointerEventArgs^ args);
  // keyboard
  void OnKeyDown(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::KeyEventArgs^ args);
  void OnKeyUp(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::KeyEventArgs^ args);
  void OnCharacterReceived(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::CharacterReceivedEventArgs^ args);
  // DisplayInformation event handlers.
  void OnDpiChanged(Windows::Graphics::Display::DisplayInformation^ sender, Platform::Object^ args);
  void OnOrientationChanged(Windows::Graphics::Display::DisplayInformation^ sender, Platform::Object^ args);
  void OnDisplayContentsInvalidated(Windows::Graphics::Display::DisplayInformation^ sender, Platform::Object^ args);

private:
  void TranslateKey(Windows::UI::Core::CoreWindow^ window, XBMC_keysym &keysym, Windows::System::VirtualKey vkey, unsigned scancode, unsigned keycode);
  void UpdateWindowSize(float width, float height);

  Windows::Foundation::IAsyncAction^ m_renderLoopWorker;
  std::shared_ptr<DX::DeviceResources> m_deviceResources;
  std::vector<char*> m_argv;
};
    } // namespace WINDOWS10
  } // namespace PLATFORM
} // namespace KODI

