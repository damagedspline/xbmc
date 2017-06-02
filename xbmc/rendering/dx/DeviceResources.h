#pragma once

#include <wrl.h>
#include <wrl/client.h>
#include <dxgi1_4.h>
#include <d3d11_3.h>
#include <d2d1_3.h>
#include <DirectXColors.h>
#include <DirectXMath.h>
#include <memory>
#include <agile.h>
#include <concrt.h>
#include <collection.h>
#include <memory>

namespace DX
{
	// Provides an interface for an application that owns DeviceResources to be notified of the device being lost or created.
	interface IDeviceNotify
	{
		virtual void OnDeviceLost() = 0;
		virtual void OnDeviceRestored() = 0;
	};

	// Controls all the DirectX device resources.
	class DeviceResources
	{
	public:
    static std::shared_ptr<DX::DeviceResources> Get();

    DeviceResources();
		void SetWindow(Windows::UI::Core::CoreWindow^ window);
		void SetLogicalSize(Windows::Foundation::Size logicalSize);
		void SetCurrentOrientation(Windows::Graphics::Display::DisplayOrientations currentOrientation);
		void SetDpi(float dpi);
		void ValidateDevice();
		void HandleDeviceLost();
		void RegisterDeviceNotify(IDeviceNotify* deviceNotify);
		void Trim();
		void Present();
    void ShowCursor(bool show);

		// The size of the render target, in pixels.
		Windows::Foundation::Size	GetOutputSize() const { return m_outputSize; }

		// The size of the render target, in dips.
		Windows::Foundation::Size	GetLogicalSize() const { return m_logicalSize; }
		float GetDpi() const { return m_effectiveDpi; }

		// D3D Accessors.
		ID3D11Device3* GetD3DDevice() const { return m_d3dDevice.Get(); }
		ID3D11DeviceContext3* GetD3DDeviceContext() const { return m_d3dContext.Get(); }
		IDXGISwapChain3* GetSwapChain() const { return m_swapChain.Get(); }
    IDXGIFactory2* GetIDXGIFactory2() const { return m_dxgiFactory.Get(); }
    IDXGIAdapter1* GetAdapter() const { return m_adapter.Get(); }

		D3D_FEATURE_LEVEL GetDeviceFeatureLevel() const { return m_d3dFeatureLevel; }
		ID3D11RenderTargetView1* GetBackBufferRenderTargetView() const { return m_d3dRenderTargetView.Get(); }
		ID3D11DepthStencilView* GetDepthStencilView() const { return m_d3dDepthStencilView.Get(); }
		D3D11_VIEWPORT GetScreenViewport() const { return m_screenViewport; }
		DirectX::XMFLOAT4X4 GetOrientationTransform3D() const { return m_orientationTransform3D; }

    Concurrency::critical_section& GetCriticalSection() { return m_criticalSection; }
    bool SetFullScreen(bool fullscreen);
    Windows::UI::Core::CoreDispatcher^ GetDispatcher() { return m_window.Get() ? m_window->Dispatcher : nullptr; }

	private:
		void CreateDeviceIndependentResources();
		void CreateDeviceResources();
		void CreateWindowSizeDependentResources();
		void UpdateRenderTargetSize();
		DXGI_MODE_ROTATION ComputeDisplayRotation();

    Microsoft::WRL::ComPtr<IDXGIFactory2> m_dxgiFactory;
    Microsoft::WRL::ComPtr<IDXGIAdapter1> m_adapter;

    // Direct3D objects.
		Microsoft::WRL::ComPtr<ID3D11Device3> m_d3dDevice;
		Microsoft::WRL::ComPtr<ID3D11DeviceContext3> m_d3dContext;
		Microsoft::WRL::ComPtr<IDXGISwapChain3> m_swapChain;

		// Direct3D rendering objects. Required for 3D.
		Microsoft::WRL::ComPtr<ID3D11RenderTargetView1> m_d3dRenderTargetView;
		Microsoft::WRL::ComPtr<ID3D11DepthStencilView> m_d3dDepthStencilView;
		D3D11_VIEWPORT m_screenViewport;

		// Cached reference to the Window.
		Platform::Agile<Windows::UI::Core::CoreWindow> m_window;

		// Cached device properties.
		D3D_FEATURE_LEVEL m_d3dFeatureLevel;
		Windows::Foundation::Size m_d3dRenderTargetSize;
		Windows::Foundation::Size m_outputSize;
		Windows::Foundation::Size m_logicalSize;
		Windows::Graphics::Display::DisplayOrientations m_nativeOrientation;
		Windows::Graphics::Display::DisplayOrientations m_currentOrientation;
		float m_dpi;

		// This is the DPI that will be reported back to the app. It takes into account whether the app supports high resolution screens or not.
		float m_effectiveDpi;

		// Transforms used for display orientation.
		DirectX::XMFLOAT4X4 m_orientationTransform3D;

		// The IDeviceNotify can be held directly as it owns the DeviceResources.
		IDeviceNotify* m_deviceNotify;

    // scritical section
    Concurrency::critical_section m_criticalSection;
	};
}