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


#ifdef HAS_DX

#include <DirectXPackedVector.h>
#include "Application.h"
#include "RenderSystemDXWin10.h"
#include "cores/VideoPlayer/VideoRenderers/RenderManager.h"
#include "guilib/D3DResource.h"
#include "guilib/GUIShaderDX.h"
#include "guilib/GUITextureD3D.h"
#include "guilib/GUIWindowManager.h"
#include "messaging/ApplicationMessenger.h"
#include "settings/AdvancedSettings.h"
#include "threads/SingleLock.h"
#include "utils/CharsetConverter.h"
#include "platform/win32/CharsetConverter.h"
#include "utils/MathUtils.h"
#include "utils/log.h"
#include "platform/win32/dxerr.h"
#include "utils/SystemInfo.h"
#include "rendering/dx/DeviceResources.h"

#pragma warning(disable: 4091)
//#include <d3d10umddi.h>
#pragma warning(default: 4091)
#include <algorithm>

#pragma comment(lib, "dxgi.lib")

#define RATIONAL_TO_FLOAT(rational) ((rational.Denominator != 0) ? \
 static_cast<float>(rational.Numerator) / static_cast<float>(rational.Denominator) : 0.0f)

using namespace DirectX::PackedVector;
using namespace concurrency;

CRenderSystemDX::CRenderSystemDX() : CRenderSystemBase()
{
  m_enumRenderingSystem = RENDERING_SYSTEM_DIRECTX;

  m_bVSync = true;
  m_bRenderCreated = false;
  ZeroMemory(&m_cachedMode, sizeof(m_cachedMode));
  ZeroMemory(&m_scissor, sizeof(CRect));
  ZeroMemory(&m_adapterDesc, sizeof(DXGI_ADAPTER_DESC));
}

CRenderSystemDX::~CRenderSystemDX()
{
}

ID3D11Device* CRenderSystemDX::Get3D11Device() const 
{ 
  return DX::DeviceResources::Get()->GetD3DDevice(); 
}

ID3D11DeviceContext* CRenderSystemDX::Get3D11Context() const 
{ 
  return DX::DeviceResources::Get()->GetD3DDeviceContext(); 
}

ID3D11DeviceContext* CRenderSystemDX::GetImmediateContext() const 
{ 
  return DX::DeviceResources::Get()->GetD3DDeviceContext();
}

unsigned CRenderSystemDX::GetFeatureLevel() const 
{ 
  return DX::DeviceResources::Get()->GetDeviceFeatureLevel(); 
}

bool CRenderSystemDX::InitRenderSystem()
{
  m_bVSync = true;
  
  CLog::Log(LOGDEBUG, __FUNCTION__" - Initializing D3D11 Factory...");

  UpdateMonitor();
  return CreateDevice();
}

void CRenderSystemDX::SetRenderParams(unsigned int width, unsigned int height, bool fullScreen, float refreshRate)
{
  CLog::Log(LOGERROR, "%s is not fully implemented", __FUNCTION__);

  m_bFullScreenDevice = fullScreen;
  m_refreshRate       = refreshRate;
}

void CRenderSystemDX::SetMonitor(HMONITOR monitor)
{
#if 0
  if (!m_dxgiFactory)
    return;

  DXGI_OUTPUT_DESC outputDesc;
  if (m_pOutput && SUCCEEDED(m_pOutput->GetDesc(&outputDesc)) && outputDesc.Monitor == monitor)
    return;

  // find the appropriate screen
  IDXGIAdapter1*  pAdapter;
  for (unsigned i = 0; m_dxgiFactory->EnumAdapters1(i, &pAdapter) != DXGI_ERROR_NOT_FOUND; ++i)
  {
    DXGI_ADAPTER_DESC1 adapterDesc;
    pAdapter->GetDesc1(&adapterDesc);

    IDXGIOutput* pOutput;
    for (unsigned j = 0; pAdapter->EnumOutputs(j, &pOutput) != DXGI_ERROR_NOT_FOUND; ++j)
    {
      pOutput->GetDesc(&outputDesc);
      if (outputDesc.Monitor == monitor)
      {
        CLog::Log(LOGDEBUG, __FUNCTION__" - Selected %S output. ", outputDesc.DeviceName);

        // update monitor info
        SAFE_RELEASE(m_pOutput);
        m_pOutput = pOutput;

        // check if adapter is changed
        if ( m_adapterDesc.AdapterLuid.HighPart != adapterDesc.AdapterLuid.HighPart 
          || m_adapterDesc.AdapterLuid.LowPart != adapterDesc.AdapterLuid.LowPart)
        {
          CLog::Log(LOGDEBUG, __FUNCTION__" - Selected %S adapter. ", adapterDesc.Description);

          pAdapter->GetDesc(&m_adapterDesc);
          SAFE_RELEASE(m_adapter);
          m_adapter = pAdapter;
          m_needNewDevice = true;

          return;
        }

        return;
      }
      pOutput->Release();
    }
    pAdapter->Release();
  }
#endif
}

bool CRenderSystemDX::ResetRenderSystem(int width, int height, bool fullScreen, float refreshRate)
{
#if 0
  if (!m_pD3DDev)
    return false;

  if (m_hDeviceWnd != nullptr)
  {
    HMONITOR hMonitor = MonitorFromWindow(m_hDeviceWnd, MONITOR_DEFAULTTONULL);
    if (hMonitor)
      SetMonitor(hMonitor);
  }

  SetRenderParams(width, height, fullScreen, refreshRate);

  CRect rc(0, 0, float(width), float(height));
  SetViewPort(rc);

  if (!m_needNewDevice)
  {
    SetFullScreenInternal();
    CreateWindowSizeDependentResources();
  }
  else 
  {
    OnDeviceLost();
    OnDeviceReset();
  }
#endif

  SetRenderParams(width, height, fullScreen, refreshRate);
  CreateWindowSizeDependentResources();
  return true;
}

void CRenderSystemDX::OnMove()
{
#if 0
  if (!m_bRenderCreated)
    return;

  DXGI_OUTPUT_DESC outputDesc;
  m_pOutput->GetDesc(&outputDesc);

  HMONITOR newMonitor = MonitorFromWindow(m_hDeviceWnd, MONITOR_DEFAULTTONULL);

  if (newMonitor != nullptr && outputDesc.Monitor != newMonitor)
  {
    SetMonitor(newMonitor);
    if (m_needNewDevice)
    {
      CLog::Log(LOGDEBUG, "%s - Adapter changed, resetting render system.", __FUNCTION__);
      ResetRenderSystem(m_nBackBufferWidth, m_nBackBufferHeight, m_bFullScreenDevice, m_refreshRate);
    }
  }
#endif
}

void CRenderSystemDX::OnResize(unsigned int width, unsigned int height)
{
  if (!m_bRenderCreated)
    return;

  CreateWindowSizeDependentResources();
}

CRect CRenderSystemDX::GetBackBufferRect() 
{ 
  auto vp = DX::DeviceResources::Get()->GetScreenViewport();
  return CRect(0.f, 0.f, static_cast<float>(vp.Width), static_cast<float>(vp.Height));
}


void CRenderSystemDX::GetClosestDisplayModeToCurrent(IDXGIOutput* output, DXGI_MODE_DESC* outCurrentDisplayMode, bool useCached /*= false*/)
{
#if 0
  DXGI_OUTPUT_DESC outputDesc;
  output->GetDesc(&outputDesc);
  HMONITOR hMonitor = outputDesc.Monitor;
  MONITORINFOEX monitorInfo;
  monitorInfo.cbSize = sizeof(MONITORINFOEX);
  GetMonitorInfo(hMonitor, &monitorInfo);
  DEVMODE devMode;
  devMode.dmSize = sizeof(DEVMODE);
  devMode.dmDriverExtra = 0;
  EnumDisplaySettings(monitorInfo.szDevice, ENUM_CURRENT_SETTINGS, &devMode);

  bool useDefaultRefreshRate = 1 == devMode.dmDisplayFrequency || 0 == devMode.dmDisplayFrequency;
  float refreshRate = RATIONAL_TO_FLOAT(m_cachedMode.RefreshRate);

  // this needed to improve performance for VideoSync bacause FindClosestMatchingMode is very slow
  if (!useCached
    || m_cachedMode.Width  != devMode.dmPelsWidth
    || m_cachedMode.Height != devMode.dmPelsHeight
    || long(refreshRate)   != devMode.dmDisplayFrequency)
  {
    DXGI_MODE_DESC current;
    current.Width = devMode.dmPelsWidth;
    current.Height = devMode.dmPelsHeight;
    current.RefreshRate.Numerator = 0;
    current.RefreshRate.Denominator = 0;
    if (!useDefaultRefreshRate)
      GetRefreshRatio(devMode.dmDisplayFrequency, &current.RefreshRate.Numerator, &current.RefreshRate.Denominator);
    current.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    current.ScanlineOrdering = m_interlaced ? DXGI_MODE_SCANLINE_ORDER_UPPER_FIELD_FIRST : DXGI_MODE_SCANLINE_ORDER_PROGRESSIVE;
    current.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;

    output->FindClosestMatchingMode(&current, &m_cachedMode, m_pD3DDev);
  }

  ZeroMemory(outCurrentDisplayMode, sizeof(DXGI_MODE_DESC));
  outCurrentDisplayMode->Width = m_cachedMode.Width;
  outCurrentDisplayMode->Height = m_cachedMode.Height;
  outCurrentDisplayMode->RefreshRate.Numerator = m_cachedMode.RefreshRate.Numerator;
  outCurrentDisplayMode->RefreshRate.Denominator = m_cachedMode.RefreshRate.Denominator;
  outCurrentDisplayMode->Format = m_cachedMode.Format;
  outCurrentDisplayMode->ScanlineOrdering = m_cachedMode.ScanlineOrdering;
  outCurrentDisplayMode->Scaling = m_cachedMode.Scaling;
#endif
}

void CRenderSystemDX::GetDisplayMode(DXGI_MODE_DESC *mode, bool useCached /*= false*/)
{
  GetClosestDisplayModeToCurrent(m_pOutput, mode, useCached);
}

inline void DXWait(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
  ID3D11Query* wait = nullptr;
  CD3D11_QUERY_DESC qd(D3D11_QUERY_EVENT);
  if (SUCCEEDED(pDevice->CreateQuery(&qd, &wait)))
  {
    pContext->End(wait);
    while (S_FALSE == pContext->GetData(wait, nullptr, 0, 0))
      Sleep(1);
  }
  SAFE_RELEASE(wait);
}

void CRenderSystemDX::SetFullScreenInternal()
{
  if (!m_bRenderCreated)
    return;
#if 0
  HRESULT hr = S_OK;
  BOOL bFullScreen;
  m_pSwapChain->GetFullscreenState(&bFullScreen, nullptr);

  // full-screen to windowed translation. Only change FS state and return
  if (!!bFullScreen && m_useWindowedDX)
  {
    CLog::Log(LOGDEBUG, "%s - Switching swap chain to windowed mode.", __FUNCTION__);

    OnDisplayLost();
    hr = m_pSwapChain->SetFullscreenState(false, nullptr);
    if (SUCCEEDED(hr))
      m_bResizeRequired = true;
    else
      CLog::Log(LOGERROR, "%s - Failed switch full screen state: %s.", __FUNCTION__, GetErrorDescription(hr).c_str());
  }
  // true full-screen
  else if (m_bFullScreenDevice && !m_useWindowedDX)
  {
    IDXGIOutput* pOutput = nullptr;
    m_pSwapChain->GetContainingOutput(&pOutput);

    DXGI_OUTPUT_DESC trgDesc, currDesc;
    m_pOutput->GetDesc(&trgDesc);
    pOutput->GetDesc(&currDesc);

    if (trgDesc.Monitor != currDesc.Monitor || !bFullScreen)
    {
      // swap chain requires to change FS mode after resize or transition from windowed to full-screen.
      CLog::Log(LOGDEBUG, "%s - Switching swap chain to fullscreen state.", __FUNCTION__);

      OnDisplayLost();
      hr = m_pSwapChain->SetFullscreenState(true, m_pOutput);
      if (SUCCEEDED(hr))
        m_bResizeRequired = true;
      else
        CLog::Log(LOGERROR, "%s - Failed switch full screen state: %s.", __FUNCTION__, GetErrorDescription(hr).c_str());
    }
    SAFE_RELEASE(pOutput);

    // do not change modes if hw stereo enabled
    if (m_bHWStereoEnabled)
      goto end;

    DXGI_SWAP_CHAIN_DESC scDesc;
    m_pSwapChain->GetDesc(&scDesc);

    DXGI_MODE_DESC currentMode, // closest to current mode
      toMatchMode,              // required mode
      matchedMode;              // closest to required mode

    // find current mode on target output
    GetClosestDisplayModeToCurrent(m_pOutput, &currentMode);

    float currentRefreshRate = RATIONAL_TO_FLOAT(currentMode.RefreshRate);
    CLog::Log(LOGDEBUG, "%s - Current display mode is: %dx%d@%0.3f", __FUNCTION__, currentMode.Width, currentMode.Height, currentRefreshRate);

    // use backbuffer dimension to find required display mode
    toMatchMode.Width = m_nBackBufferWidth;
    toMatchMode.Height = m_nBackBufferHeight;
    bool useDefaultRefreshRate = 0 == m_refreshRate;
    toMatchMode.RefreshRate.Numerator = 0;
    toMatchMode.RefreshRate.Denominator = 0;
    if (!useDefaultRefreshRate)
      GetRefreshRatio(static_cast<uint32_t>(m_refreshRate), &toMatchMode.RefreshRate.Numerator, &toMatchMode.RefreshRate.Denominator);
    toMatchMode.Format = scDesc.BufferDesc.Format;
    toMatchMode.Scaling = scDesc.BufferDesc.Scaling;
    toMatchMode.ScanlineOrdering = scDesc.BufferDesc.ScanlineOrdering;

    // find closest mode
    m_pOutput->FindClosestMatchingMode(&toMatchMode, &matchedMode, m_pD3DDev);

    float matchedRefreshRate = RATIONAL_TO_FLOAT(matchedMode.RefreshRate);
    CLog::Log(LOGDEBUG, "%s - Found matched mode: %dx%d@%0.3f", __FUNCTION__, matchedMode.Width, matchedMode.Height, matchedRefreshRate);
    // FindClosestMatchingMode doesn't return "fixed" modes, so wee need to check deviation and force switching mode
    float diff = fabs(matchedRefreshRate - m_refreshRate) / matchedRefreshRate;
    // change mode if required (current != required)
    if ( currentMode.Width != matchedMode.Width
      || currentMode.Height != matchedMode.Height
      || currentRefreshRate != matchedRefreshRate
      || diff > 0.0005)
    {
      // change monitor resolution (in fullscreen mode) to required mode
      CLog::Log(LOGDEBUG, "%s - Switching mode to %dx%d@%0.3f.", __FUNCTION__, matchedMode.Width, matchedMode.Height, matchedRefreshRate);

      if (!m_bResizeRequired)
        OnDisplayLost();

      hr = m_pSwapChain->ResizeTarget(&matchedMode);
      if (SUCCEEDED(hr))
        m_bResizeRequired = true;
      else
        CLog::Log(LOGERROR, "%s - Failed to switch output mode: %s", __FUNCTION__, GetErrorDescription(hr).c_str());
    }
  }
end:
  SetMaximumFrameLatency();
#endif
}

bool CRenderSystemDX::IsFormatSupport(DXGI_FORMAT format, unsigned int usage)
{
  UINT supported;
  auto pD3DDev = DX::DeviceResources::Get()->GetD3DDevice();
  pD3DDev->CheckFormatSupport(format, &supported);
  return (supported & usage) != 0;
}

bool CRenderSystemDX::DestroyRenderSystem()
{
  DeleteDevice();

  // restore stereo setting on exit
  if (g_advancedSettings.m_useDisplayControlHWStereo)
    SetDisplayStereoEnabled(m_bDefaultStereoEnabled);

  SAFE_RELEASE(m_pOutput);
  return true;
}

void CRenderSystemDX::DeleteDevice()
{
  CLog::Log(LOGERROR, "%s is not implemented", __FUNCTION__);

  CSingleLock lock(m_resourceSection);

  if (m_pGUIShader)
    m_pGUIShader->End();

  // tell any shared resources
  for (std::vector<ID3DResource *>::iterator i = m_resources.begin(); i != m_resources.end(); ++i)
  {
    // the most of resources like textures and buffers try to 
    // receive and save their status from current device.
    // m_nDeviceStatus contains the last device status and
    // DXGI_ERROR_DEVICE_REMOVED means that we have no possibility
    // to use the device anymore, tell all resouces about this.
    (*i)->OnDestroyDevice(DXGI_ERROR_DEVICE_REMOVED == m_nDeviceStatus);
  }


  SAFE_DELETE(m_pGUIShader);
  SAFE_RELEASE(m_BlendEnableState);
  SAFE_RELEASE(m_BlendDisableState);
  SAFE_RELEASE(m_RSScissorDisable);
  SAFE_RELEASE(m_RSScissorEnable);
  SAFE_RELEASE(m_depthStencilState);

#ifdef _DEBUG
  if (m_d3dDebug)
  {
    m_d3dDebug->ReportLiveDeviceObjects(D3D11_RLDO_SUMMARY | D3D11_RLDO_DETAIL);
    SAFE_RELEASE(m_d3dDebug);
  }
#endif
  m_bResizeRequired = false;
  m_bHWStereoEnabled = false;
  m_bRenderCreated = false;
  m_bStereoEnabled = false;
}

void CRenderSystemDX::OnDeviceLost()
{
  CSingleLock lock(m_resourceSection);
  g_windowManager.SendMessage(GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_RENDERER_LOST);

  OnDisplayLost();

  if (m_needNewDevice)
    DeleteDevice();
  else
  {
    // just resetting the device
    for (std::vector<ID3DResource *>::iterator i = m_resources.begin(); i != m_resources.end(); ++i)
      (*i)->OnLostDevice();
  }
}

void CRenderSystemDX::OnDeviceReset()
{
  CSingleLock lock(m_resourceSection);

  if (m_needNewDevice)
    CreateDevice();
  
  if (m_bRenderCreated)
  {
    // we're back
    for (std::vector<ID3DResource *>::iterator i = m_resources.begin(); i != m_resources.end(); ++i)
      (*i)->OnResetDevice();

    g_windowManager.SendMessage(GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_RENDERER_RESET);
  }

  OnDisplayReset();
}

bool CRenderSystemDX::CreateDevice()
{
  auto featureLevel = DX::DeviceResources::Get()->GetDeviceFeatureLevel();

  if (featureLevel < D3D_FEATURE_LEVEL_9_3)
    m_maxTextureSize = D3D_FL9_1_REQ_TEXTURE2D_U_OR_V_DIMENSION;
  else if (featureLevel < D3D_FEATURE_LEVEL_10_0)
    m_maxTextureSize = D3D_FL9_3_REQ_TEXTURE2D_U_OR_V_DIMENSION;
  else if (featureLevel < D3D_FEATURE_LEVEL_11_0)
    m_maxTextureSize = D3D10_REQ_TEXTURE2D_U_OR_V_DIMENSION;
  else 
    // 11_x and greater feature level. Limit this size to avoid memory overheads
    m_maxTextureSize = D3D11_REQ_TEXTURE2D_U_OR_V_DIMENSION >> 1;

  auto pD3DDev = DX::DeviceResources::Get()->GetD3DDevice();

  SetMaximumFrameLatency();

#ifdef _DEBUG
  if (SUCCEEDED(pD3DDev->QueryInterface(__uuidof(ID3D11Debug), reinterpret_cast<void**>(&m_d3dDebug))))
  {
    ID3D11InfoQueue *d3dInfoQueue = nullptr;
    if (SUCCEEDED(m_d3dDebug->QueryInterface(__uuidof(ID3D11InfoQueue), reinterpret_cast<void**>(&d3dInfoQueue))))
    {
      D3D11_MESSAGE_ID hide[] =
      {
        D3D11_MESSAGE_ID_GETVIDEOPROCESSORFILTERRANGE_UNSUPPORTED,        // avoid GETVIDEOPROCESSORFILTERRANGE_UNSUPPORTED (dx bug)
        D3D11_MESSAGE_ID_DEVICE_RSSETSCISSORRECTS_NEGATIVESCISSOR         // avoid warning for some labels out of screen
        // Add more message IDs here as needed
      };

      D3D11_INFO_QUEUE_FILTER filter;
      ZeroMemory(&filter, sizeof(filter));
      filter.DenyList.NumIDs = _countof(hide);
      filter.DenyList.pIDList = hide;
      d3dInfoQueue->AddStorageFilterEntries(&filter);
      d3dInfoQueue->Release();
    }
  }
#endif

  m_adapterDesc = {};

  DX::DeviceResources::Get()->ValidateDevice();

  auto pAdapter = DX::DeviceResources::Get()->GetAdapter();
  if (SUCCEEDED(pAdapter->GetDesc(&m_adapterDesc)))
  {
    CLog::Log(LOGDEBUG, "%s - on adapter %S (VendorId: %#x DeviceId: %#x) with feature level %#x.", __FUNCTION__, 
                        m_adapterDesc.Description, m_adapterDesc.VendorId, m_adapterDesc.DeviceId, featureLevel);

    m_RenderRenderer = KODI::PLATFORM::WINDOWS::FromW(StringUtils::Format(L"%s", m_adapterDesc.Description));
    IDXGIFactory2* dxgiFactory2 = DX::DeviceResources::Get()->GetIDXGIFactory2();
    m_RenderVersion = StringUtils::Format("DirectX %s (FL %d.%d)", 
                                          dxgiFactory2 != nullptr ? "11.1" : "11.0", 
                                          (featureLevel >> 12) & 0xF, 
                                          (featureLevel >> 8) & 0xF);
    SAFE_RELEASE(dxgiFactory2);
  }

  m_renderCaps = 0;
  unsigned int usage = D3D11_FORMAT_SUPPORT_TEXTURE2D | D3D11_FORMAT_SUPPORT_SHADER_SAMPLE;
  if ( IsFormatSupport(DXGI_FORMAT_BC1_UNORM, usage)
    && IsFormatSupport(DXGI_FORMAT_BC2_UNORM, usage)
    && IsFormatSupport(DXGI_FORMAT_BC3_UNORM, usage))
    m_renderCaps |= RENDER_CAPS_DXT;

  // MSDN: At feature levels 9_1, 9_2 and 9_3, the display device supports the use of 2D textures with dimensions that are not powers of two under two conditions.
  // First, only one MIP-map level for each texture can be created - we are using only 1 mip level)
  // Second, no wrap sampler modes for textures are allowed - we are using clamp everywhere
  // At feature levels 10_0, 10_1 and 11_0, the display device unconditionally supports the use of 2D textures with dimensions that are not powers of two.
  // so, setup caps NPOT
  m_renderCaps |= featureLevel > D3D_FEATURE_LEVEL_9_3 ? RENDER_CAPS_NPOT : 0;
  if ((m_renderCaps & RENDER_CAPS_DXT) != 0)
  {
    if (featureLevel > D3D_FEATURE_LEVEL_9_3 ||
      (!IsFormatSupport(DXGI_FORMAT_BC1_UNORM, D3D11_FORMAT_SUPPORT_MIP_AUTOGEN)
      && !IsFormatSupport(DXGI_FORMAT_BC2_UNORM, D3D11_FORMAT_SUPPORT_MIP_AUTOGEN)
      && !IsFormatSupport(DXGI_FORMAT_BC3_UNORM, D3D11_FORMAT_SUPPORT_MIP_AUTOGEN)))
      m_renderCaps |= RENDER_CAPS_DXT_NPOT;
  }

  // Temporary - allow limiting the caps to debug a texture problem
  if (g_advancedSettings.m_RestrictCapsMask != 0)
    m_renderCaps &= ~g_advancedSettings.m_RestrictCapsMask;

  if (m_renderCaps & RENDER_CAPS_DXT)
    CLog::Log(LOGDEBUG, "%s - RENDER_CAPS_DXT", __FUNCTION__);
  if (m_renderCaps & RENDER_CAPS_NPOT)
    CLog::Log(LOGDEBUG, "%s - RENDER_CAPS_NPOT", __FUNCTION__);
  if (m_renderCaps & RENDER_CAPS_DXT_NPOT)
    CLog::Log(LOGDEBUG, "%s - RENDER_CAPS_DXT_NPOT", __FUNCTION__);

  if (!CreateStates() || !InitGUIShader() || !CreateWindowSizeDependentResources())
    return false;

  m_bRenderCreated = true;
  m_needNewDevice = false;

  // tell any shared objects about our resurrection
  for (std::vector<ID3DResource *>::iterator i = m_resources.begin(); i != m_resources.end(); ++i)
    (*i)->OnCreateDevice();

  RestoreViewPort();

  return true;
#endif
}

bool CRenderSystemDX::CreateWindowSizeDependentResources()
{
  auto pContext = DX::DeviceResources::Get()->GetD3DDeviceContext();
  auto pDepthStencilView = DX::DeviceResources::Get()->GetDepthStencilView();
  ID3D11RenderTargetView *const targets[1] = { DX::DeviceResources::Get()->GetBackBufferRenderTargetView() };
  pContext->OMSetRenderTargets(1, targets, pDepthStencilView);

  // Reset the viewport to target the whole screen.
  auto viewport = DX::DeviceResources::Get()->GetScreenViewport();

  CRect rect(0.0f, 0.0f,
    static_cast<float>(viewport.Width),
    static_cast<float>(viewport.Height));
  SetViewPort(rect);
  // set camera to center of screen
  CPoint camPoint = { viewport.Width * 0.5f, viewport.Height * 0.5f };
  SetCameraPosition(camPoint, viewport.Width, viewport.Height);



#if 0
  if (m_resizeInProgress)
    return false;


  if (!bNeedRecreate && !bNeedResize)
  {
    CheckInterlacedStereoView();
    return true;
  }




  if (m_viewPort.Height == 0 || m_viewPort.Width == 0)
  {
    CRect rect(0.0f, 0.0f,
      static_cast<float>(m_nBackBufferWidth),
      static_cast<float>(m_nBackBufferHeight));
    SetViewPort(rect);
  }

  // set camera to center of screen
  CPoint camPoint = { m_nBackBufferWidth * 0.5f, m_nBackBufferHeight * 0.5f };
  SetCameraPosition(camPoint, m_nBackBufferWidth, m_nBackBufferHeight);

  CheckInterlacedStereoView();

  if (bRestoreRTView)
    pContext->OMSetRenderTargets(1, &m_pRenderTargetView, m_depthStencilView);

  // notify about resurrection of display
  if (m_bResizeRequired)
    OnDisplayBack();

  m_resizeInProgress = false;
  m_bResizeRequired = false;
#endif
  return true;
}

void CRenderSystemDX::CheckInterlacedStereoView(void)
{
#if 0
  RENDER_STEREO_MODE stereoMode = g_graphicsContext.GetStereoMode();

  if ( m_pRenderTargetViewRight 
    && RENDER_STEREO_MODE_INTERLACED    != stereoMode
    && RENDER_STEREO_MODE_CHECKERBOARD  != stereoMode
    && RENDER_STEREO_MODE_HARDWAREBASED != stereoMode)
  {
    // release resources
    SAFE_RELEASE(m_pRenderTargetViewRight);
    SAFE_RELEASE(m_pShaderResourceViewRight);
    SAFE_RELEASE(m_pTextureRight);
  }

  if ( !m_pRenderTargetViewRight
    && ( RENDER_STEREO_MODE_INTERLACED   == stereoMode 
      || RENDER_STEREO_MODE_CHECKERBOARD == stereoMode))
  {
    // Create a second Render Target for the right eye buffer
    HRESULT hr;
    CD3D11_TEXTURE2D_DESC texDesc(DXGI_FORMAT_B8G8R8A8_UNORM, m_nBackBufferWidth, m_nBackBufferHeight, 1, 1,
                                  D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE);
    hr = m_pD3DDev->CreateTexture2D(&texDesc, nullptr, &m_pTextureRight);
    if (SUCCEEDED(hr))
    {
      CD3D11_RENDER_TARGET_VIEW_DESC rtDesc(D3D11_RTV_DIMENSION_TEXTURE2D);
      hr = m_pD3DDev->CreateRenderTargetView(m_pTextureRight, &rtDesc, &m_pRenderTargetViewRight);

      if (SUCCEEDED(hr))
      {
        CD3D11_SHADER_RESOURCE_VIEW_DESC srDesc(D3D11_SRV_DIMENSION_TEXTURE2D);
        hr = m_pD3DDev->CreateShaderResourceView(m_pTextureRight, &srDesc, &m_pShaderResourceViewRight);

        if (FAILED(hr))
          CLog::Log(LOGERROR, "%s - Failed to create right view shader resource.", __FUNCTION__);
      }
      else
        CLog::Log(LOGERROR, "%s - Failed to create right view render target.", __FUNCTION__);
    }

    if (FAILED(hr))
    {
      SAFE_RELEASE(m_pShaderResourceViewRight);
      SAFE_RELEASE(m_pRenderTargetViewRight);
      SAFE_RELEASE(m_pTextureRight);

      CLog::Log(LOGERROR, "%s - Failed to create right eye buffer.", __FUNCTION__);
      g_graphicsContext.SetStereoMode(RENDER_STEREO_MODE_OFF); // try fallback to mono
    }
  }
#endif
}

bool CRenderSystemDX::CreateStates()
{
  auto pDevice = DX::DeviceResources::Get()->GetD3DDevice();
  auto pContext = DX::DeviceResources::Get()->GetD3DDeviceContext();

  SAFE_RELEASE(m_depthStencilState);
  SAFE_RELEASE(m_BlendEnableState);
  SAFE_RELEASE(m_BlendDisableState);

  // Initialize the description of the stencil state.
  D3D11_DEPTH_STENCIL_DESC depthStencilDesc;
  ZeroMemory(&depthStencilDesc, sizeof(D3D11_DEPTH_STENCIL_DESC));

  // Set up the description of the stencil state.
  depthStencilDesc.DepthEnable = false;
  depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
  depthStencilDesc.DepthFunc = D3D11_COMPARISON_NEVER;
  depthStencilDesc.StencilEnable = false;
  depthStencilDesc.StencilReadMask = 0xFF;
  depthStencilDesc.StencilWriteMask = 0xFF;

  // Stencil operations if pixel is front-facing.
  depthStencilDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
  depthStencilDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_INCR;
  depthStencilDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
  depthStencilDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

  // Stencil operations if pixel is back-facing.
  depthStencilDesc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
  depthStencilDesc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_DECR;
  depthStencilDesc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
  depthStencilDesc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

  // Create the depth stencil state.
  HRESULT hr = pDevice->CreateDepthStencilState(&depthStencilDesc, &m_depthStencilState);
  if(FAILED(hr))
    return false;

  // Set the depth stencil state.
	pContext->OMSetDepthStencilState(m_depthStencilState, 0);

  D3D11_RASTERIZER_DESC rasterizerState;
  rasterizerState.CullMode = D3D11_CULL_NONE; 
  rasterizerState.FillMode = D3D11_FILL_SOLID;// DEBUG - D3D11_FILL_WIREFRAME
  rasterizerState.FrontCounterClockwise = false;
  rasterizerState.DepthBias = 0;
  rasterizerState.DepthBiasClamp = 0.0f;
  rasterizerState.DepthClipEnable = true;
  rasterizerState.SlopeScaledDepthBias = 0.0f;
  rasterizerState.ScissorEnable = false;
  rasterizerState.MultisampleEnable = false;
  rasterizerState.AntialiasedLineEnable = false;

  if (FAILED(pDevice->CreateRasterizerState(&rasterizerState, &m_RSScissorDisable)))
    return false;

  rasterizerState.ScissorEnable = true;
  if (FAILED(pDevice->CreateRasterizerState(&rasterizerState, &m_RSScissorEnable)))
    return false;

  pContext->RSSetState(m_RSScissorDisable); // by default

  D3D11_BLEND_DESC blendState = { 0 };
  ZeroMemory(&blendState, sizeof(D3D11_BLEND_DESC));
  blendState.RenderTarget[0].BlendEnable = true;
  blendState.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA; // D3D11_BLEND_SRC_ALPHA;
  blendState.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA; // D3D11_BLEND_INV_SRC_ALPHA;
  blendState.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
  blendState.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
  blendState.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
  blendState.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
  blendState.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

  pDevice->CreateBlendState(&blendState, &m_BlendEnableState);

  blendState.RenderTarget[0].BlendEnable = false;
  pDevice->CreateBlendState(&blendState, &m_BlendDisableState);

  // by default
  pContext->OMSetBlendState(m_BlendEnableState, nullptr, 0xFFFFFFFF);
  m_BlendEnabled = true;

  return true;
}

void CRenderSystemDX::PresentRenderImpl(bool rendered)
{
  if (!rendered)
    return;
  
  if (!m_bRenderCreated || m_resizeInProgress)
    return;

  if (m_nDeviceStatus != S_OK)
  {
    // if DXGI_STATUS_OCCLUDED occurred we just clear command queue and return
    if (m_nDeviceStatus == DXGI_STATUS_OCCLUDED)
      FinishCommandList(false);
    return;
  }

  auto deviceResources = DX::DeviceResources::Get();
  critical_section::scoped_lock lock(deviceResources->GetCriticalSection());

  auto pContext = DX::DeviceResources::Get()->GetD3DDeviceContext();

#if 0
  if ( m_stereoMode == RENDER_STEREO_MODE_INTERLACED
    || m_stereoMode == RENDER_STEREO_MODE_CHECKERBOARD)
  {
    // all views prepared, let's merge them before present
    pContext->OMSetRenderTargets(1, &m_pRenderTargetView, m_depthStencilView);
    CRect destRect = { 0.0f, 0.0f, float(m_nBackBufferWidth), float(m_nBackBufferHeight) };
    SHADER_METHOD method = RENDER_STEREO_MODE_INTERLACED == m_stereoMode
                           ? SHADER_METHOD_RENDER_STEREO_INTERLACED_RIGHT
                           : SHADER_METHOD_RENDER_STEREO_CHECKERBOARD_RIGHT;
    SetAlphaBlendEnable(true);
    CD3DTexture::DrawQuad(destRect, 0, 1, &m_pShaderResourceViewRight, nullptr, method);
    CD3DHelper::PSClearShaderResources(pContext);
  }
#endif

  // time for decoder that may require the context
  {
    CSingleLock lock(m_decoderSection);
    XbmcThreads::EndTime timer;
    timer.Set(5);
    while (!m_decodingTimer.IsTimePast() && !timer.IsTimePast())
    {
      m_decodingEvent.wait(lock, 1);
    }
  }

  FinishCommandList();
  pContext->Flush();

  try
  {
    DX::DeviceResources::Get()->Present();
    //pSwapChain->Present((m_bVSync ? 1 : 0), 0);
  }
  catch (Platform::Exception^ e)
  {
    CLog::Log(LOGDEBUG, "%s - device removed %s", __FUNCTION__, e->Message);
    return;
  }

  auto pDepthStencilView = DX::DeviceResources::Get()->GetDepthStencilView();
  ID3D11RenderTargetView *const targets[1] = { DX::DeviceResources::Get()->GetBackBufferRenderTargetView() };
  pContext->OMSetRenderTargets(1, targets, pDepthStencilView);
}

void CRenderSystemDX::RequestDecodingTime()
{
  CSingleLock lock(m_decoderSection);
  m_decodingTimer.Set(3);
}

void CRenderSystemDX::ReleaseDecodingTime()
{
  CSingleLock lock(m_decoderSection);
  m_decodingTimer.SetExpired();
  m_decodingEvent.notify();
}

bool CRenderSystemDX::BeginRender()
{
  if (!m_bRenderCreated)
    return false;

  auto pContext = DX::DeviceResources::Get()->GetD3DDeviceContext();

  // Reset the viewport to target the whole screen.
  auto viewport = DX::DeviceResources::Get()->GetScreenViewport();
  pContext->RSSetViewports(1, &viewport);

#if 0
  HRESULT oldStatus = m_nDeviceStatus;
  m_nDeviceStatus = pSwapChain->Present(0, DXGI_PRESENT_TEST);
  m_nDeviceStatus = S_OK;

  // handling of return values. 
  switch (m_nDeviceStatus)
  {
  case DXGI_ERROR_DEVICE_REMOVED: // GPU has been physically removed from the system, or a driver upgrade occurred. 
    CLog::Log(LOGERROR, "DXGI_ERROR_DEVICE_REMOVED");
    m_needNewDevice = true;
    break;
  case DXGI_ERROR_DEVICE_RESET: // This is an run-time issue that should be investigated and fixed.
    CLog::Log(LOGERROR, "DXGI_ERROR_DEVICE_RESET");
    m_nDeviceStatus = DXGI_ERROR_DEVICE_REMOVED;
    m_needNewDevice = true;
    break;
  case DXGI_ERROR_INVALID_CALL: // application provided invalid parameter data. Try to return after resize buffers
    CLog::Log(LOGERROR, "DXGI_ERROR_INVALID_CALL");
    // in most cases when DXGI_ERROR_INVALID_CALL occurs it means what DXGI silently leaves from FSE mode.
    // if so, we should return for FSE mode and resize buffers
    SetFullScreenInternal();
    CreateWindowSizeDependentResources();
    m_nDeviceStatus = S_OK;
    break;
  case DXGI_STATUS_OCCLUDED: // decide what we should do when windows content is not visible
    // do not spam to log file
    if (m_nDeviceStatus != oldStatus)
      CLog::Log(LOGDEBUG, "DXGI_STATUS_OCCLUDED");
    // Status OCCLUDED is not an error and not handled by FAILED macro, 
    // but if it occurs we should not render anything, this status will be accounted on present stage
  }

  if (FAILED(m_nDeviceStatus))
  {
    if (DXGI_ERROR_DEVICE_REMOVED == m_nDeviceStatus)
    {
      OnDeviceLost();
      OnDeviceReset();
      if (m_bRenderCreated)
        KODI::MESSAGING::CApplicationMessenger::GetInstance().PostMsg(TMSG_EXECUTE_BUILT_IN, -1, -1, nullptr, "ReloadSkin");
    }
    return false;
  }
#endif

  auto pDepthStencilView = DX::DeviceResources::Get()->GetDepthStencilView();
  ID3D11RenderTargetView *const targets[1] = { DX::DeviceResources::Get()->GetBackBufferRenderTargetView() };
  pContext->OMSetRenderTargets(1, targets, pDepthStencilView);
  m_inScene = true;

  return true;
}

bool CRenderSystemDX::EndRender()
{
  m_inScene = false;

  if (!m_bRenderCreated)
    return false;
  
  if(m_nDeviceStatus != S_OK)
    return false;

  return true;
}

bool CRenderSystemDX::ClearBuffers(color_t color)
{
  if (!m_bRenderCreated || m_resizeInProgress)
    return false;
  
  auto pContext = DX::DeviceResources::Get()->GetD3DDeviceContext();

  if (!m_bRenderCreated || m_resizeInProgress)
    return false;

  float fColor[4];
  CD3DHelper::XMStoreColor(fColor, color);
  ID3D11RenderTargetView* pRTView = DX::DeviceResources::Get()->GetBackBufferRenderTargetView();

#if 0
  if ( m_stereoMode != RENDER_STEREO_MODE_OFF
    && m_stereoMode != RENDER_STEREO_MODE_MONO)
  {
    // if stereo anaglyph/tab/sbs, data was cleared when left view was rendered
    if (m_stereoView == RENDER_STEREO_VIEW_RIGHT)
    {
      // execute command's queue
      FinishCommandList();

      // do not clear RT for anaglyph modes
      if ( m_stereoMode == RENDER_STEREO_MODE_ANAGLYPH_GREEN_MAGENTA
        || m_stereoMode == RENDER_STEREO_MODE_ANAGLYPH_RED_CYAN
        || m_stereoMode == RENDER_STEREO_MODE_ANAGLYPH_YELLOW_BLUE)
      {
        pRTView = nullptr;
      }
      // for interlaced/checkerboard/hw clear right view
      else if (m_pRenderTargetViewRight)
        pRTView = m_pRenderTargetViewRight;
    }
  }
#endif

  if (pRTView == nullptr)
    return true;

  auto vp = DX::DeviceResources::Get()->GetScreenViewport();

  CRect clRect(0.0f, 0.0f,
    static_cast<float>(vp.Width),
    static_cast<float>(vp.Height));

  // Unlike Direct3D 9, D3D11 ClearRenderTargetView always clears full extent of the resource view. 
  // Viewport and scissor settings are not applied. So clear RT by drawing full sized rect with clear color
  if (m_ScissorsEnabled && m_scissor != clRect)
  {
    bool alphaEnabled = m_BlendEnabled;
    if (alphaEnabled)
      SetAlphaBlendEnable(false);

    CGUITextureD3D::DrawQuad(clRect, color);

    if (alphaEnabled)
      SetAlphaBlendEnable(true);
  }
  else
    pContext->ClearRenderTargetView(pRTView, fColor);

  auto pDepthStencilView = DX::DeviceResources::Get()->GetDepthStencilView();
  if (pDepthStencilView)
  {
    pContext->ClearDepthStencilView(pDepthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0, 0);
  }
  return true;
}

bool CRenderSystemDX::IsExtSupported(const char* extension)
{
  return false;
}

void CRenderSystemDX::SetVSync(bool enable)
{
  m_bVSync = enable;
}

void CRenderSystemDX::CaptureStateBlock()
{
  if (!m_bRenderCreated)
    return;
}

void CRenderSystemDX::ApplyStateBlock()
{
  if (!m_bRenderCreated)
    return;

  auto pContext = DX::DeviceResources::Get()->GetD3DDeviceContext();

  pContext->RSSetState(m_ScissorsEnabled ? m_RSScissorEnable : m_RSScissorDisable);
  pContext->OMSetDepthStencilState(m_depthStencilState, 0);
  float factors[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
  pContext->OMSetBlendState(m_BlendEnabled ? m_BlendEnableState : m_BlendDisableState, factors, 0xFFFFFFFF);

  m_pGUIShader->ApplyStateBlock();
}

void CRenderSystemDX::SetCameraPosition(const CPoint &camera, int screenWidth, int screenHeight, float stereoFactor)
{
  if (!m_bRenderCreated)
    return;

  auto viewport = DX::DeviceResources::Get()->GetScreenViewport();

  // grab the viewport dimensions and location
  float w = viewport.Width*0.5f;
  float h = viewport.Height*0.5f;

  XMFLOAT2 offset = XMFLOAT2(camera.x - screenWidth*0.5f, camera.y - screenHeight*0.5f);

  // world view.  Until this is moved onto the GPU (via a vertex shader for instance), we set it to the identity here.
  m_pGUIShader->SetWorld(XMMatrixIdentity());

  // Initialize the view matrix
  // camera view.  Multiply the Y coord by -1 then translate so that everything is relative to the camera
  // position.
  XMMATRIX flipY, translate;
  flipY = XMMatrixScaling(1.0, -1.0f, 1.0f);
  translate = XMMatrixTranslation(-(w + offset.x - stereoFactor), -(h + offset.y), 2 * h);
  m_pGUIShader->SetView(XMMatrixMultiply(translate, flipY));

  // projection onto screen space
  m_pGUIShader->SetProjection(XMMatrixPerspectiveOffCenterLH((-w - offset.x)*0.5f, (w - offset.x)*0.5f, (-h + offset.y)*0.5f, (h + offset.y)*0.5f, h, 100 * h));
}

void CRenderSystemDX::Project(float &x, float &y, float &z)
{
  if (!m_bRenderCreated)
    return;

  m_pGUIShader->Project(x, y, z);
}

bool CRenderSystemDX::TestRender()
{
  /*
  static unsigned int lastTime = 0;
  static float delta = 0;

  unsigned int thisTime = XbmcThreads::SystemClockMillis();

  if(thisTime - lastTime > 10)
  {
    lastTime = thisTime;
    delta++;
  }

  CLog::Log(LOGINFO, "Delta =  %d", delta);

  if(delta > m_nBackBufferWidth)
    delta = 0;

  LPDIRECT3DVERTEXBUFFER9 pVB = NULL;

  // A structure for our custom vertex type
  struct CUSTOMVERTEX
  {
    FLOAT x, y, z, rhw; // The transformed position for the vertex
    DWORD color;        // The vertex color
  };

  // Our custom FVF, which describes our custom vertex structure
#define D3DFVF_CUSTOMVERTEX (D3DFVF_XYZRHW|D3DFVF_DIFFUSE)

  // Initialize three vertices for rendering a triangle
  CUSTOMVERTEX vertices[] =
  {
    { delta + 100.0f,  50.0f, 0.5f, 1.0f, 0xffff0000, }, // x, y, z, rhw, color
    { delta+200.0f, 250.0f, 0.5f, 1.0f, 0xff00ff00, },
    {  delta, 250.0f, 0.5f, 1.0f, 0xff00ffff, },
  };

  // Create the vertex buffer. Here we are allocating enough memory
  // (from the default pool) to hold all our 3 custom vertices. We also
  // specify the FVF, so the vertex buffer knows what data it contains.
  if( FAILED( m_pD3DDevice->CreateVertexBuffer( 3 * sizeof( CUSTOMVERTEX ),
    0, D3DFVF_CUSTOMVERTEX,
    D3DPOOL_DEFAULT, &pVB, NULL ) ) )
  {
    return false;
  }

  // Now we fill the vertex buffer. To do this, we need to Lock() the VB to
  // gain access to the vertices. This mechanism is required because vertex
  // buffers may be in device memory.
  VOID* pVertices;
  if( FAILED( pVB->Lock( 0, sizeof( vertices ), ( void** )&pVertices, 0 ) ) )
    return false;
  memcpy( pVertices, vertices, sizeof( vertices ) );
  pVB->Unlock();

  m_pD3DDevice->SetStreamSource( 0, pVB, 0, sizeof( CUSTOMVERTEX ) );
  m_pD3DDevice->SetFVF( D3DFVF_CUSTOMVERTEX );
  m_pD3DDevice->DrawPrimitive( D3DPT_TRIANGLELIST, 0, 1 );

  pVB->Release();
  */
  return true;
}

void CRenderSystemDX::ApplyHardwareTransform(const TransformMatrix &finalMatrix)
{
  if (!m_bRenderCreated)
    return;
}

void CRenderSystemDX::RestoreHardwareTransform()
{
  if (!m_bRenderCreated)
    return;
}

void CRenderSystemDX::GetViewPort(CRect& viewPort)
{
  if (!m_bRenderCreated)
    return;

  auto vp = DX::DeviceResources::Get()->GetScreenViewport();

  viewPort.x1 = vp.TopLeftX;
  viewPort.y1 = vp.TopLeftY;
  viewPort.x2 = vp.TopLeftX + vp.Width;
  viewPort.y2 = vp.TopLeftY + vp.Height;
}

void CRenderSystemDX::SetViewPort(CRect& viewPort)
{
  if (!m_bRenderCreated)
    return;

  D3D11_VIEWPORT vp;
  vp.MinDepth   = 0.0f;
  vp.MaxDepth   = 1.0f;
  vp.TopLeftX   = viewPort.x1;
  vp.TopLeftY   = viewPort.y1;
  vp.Width      = viewPort.x2 - viewPort.x1;
  vp.Height     = viewPort.y2 - viewPort.y1;

  auto pContext = DX::DeviceResources::Get()->GetD3DDeviceContext();
  pContext->RSSetViewports(1, &vp);
  m_pGUIShader->SetViewPort(vp);
}

void CRenderSystemDX::RestoreViewPort()
{
  if (!m_bRenderCreated)
    return;

  auto pContext = DX::DeviceResources::Get()->GetD3DDeviceContext();
  auto vp = DX::DeviceResources::Get()->GetScreenViewport();
  pContext->RSSetViewports(1, &vp);
  m_pGUIShader->SetViewPort(vp);
}

bool CRenderSystemDX::ScissorsCanEffectClipping()
{
  if (!m_bRenderCreated)
    return false;

  return m_pGUIShader != nullptr && m_pGUIShader->HardwareClipIsPossible();
}

CRect CRenderSystemDX::ClipRectToScissorRect(const CRect &rect)
{
  if (!m_bRenderCreated)
    return CRect();

  float xFactor = m_pGUIShader->GetClipXFactor();
  float xOffset = m_pGUIShader->GetClipXOffset();
  float yFactor = m_pGUIShader->GetClipYFactor();
  float yOffset = m_pGUIShader->GetClipYOffset();

  return CRect(rect.x1 * xFactor + xOffset,
               rect.y1 * yFactor + yOffset,
               rect.x2 * xFactor + xOffset,
               rect.y2 * yFactor + yOffset);
}

void CRenderSystemDX::SetScissors(const CRect& rect)
{
  if (!m_bRenderCreated)
    return;

  m_scissor = rect;
  CD3D11_RECT scissor(MathUtils::round_int(rect.x1)
                    , MathUtils::round_int(rect.y1)
                    , MathUtils::round_int(rect.x2)
                    , MathUtils::round_int(rect.y2));

  auto pContext = DX::DeviceResources::Get()->GetD3DDeviceContext();
  pContext->RSSetScissorRects(1, &scissor);
  pContext->RSSetState(m_RSScissorEnable);
  m_ScissorsEnabled = true;
}

void CRenderSystemDX::ResetScissors()
{
  if (!m_bRenderCreated)
    return;

  auto vp = DX::DeviceResources::Get()->GetScreenViewport();

  m_scissor.SetRect(0.0f, 0.0f, 
    static_cast<float>(vp.Width),
    static_cast<float>(vp.Height));

  auto pContext = DX::DeviceResources::Get()->GetD3DDeviceContext();
  pContext->RSSetState(m_RSScissorDisable);
  m_ScissorsEnabled = false;
}

void CRenderSystemDX::Register(ID3DResource *resource)
{
  CSingleLock lock(m_resourceSection);
  m_resources.push_back(resource);
}

void CRenderSystemDX::Unregister(ID3DResource* resource)
{
  CSingleLock lock(m_resourceSection);
  std::vector<ID3DResource*>::iterator i = find(m_resources.begin(), m_resources.end(), resource);
  if (i != m_resources.end())
    m_resources.erase(i);
}

std::string CRenderSystemDX::GetErrorDescription(HRESULT hr)
{
  WCHAR buff[1024];
  DXGetErrorDescription(hr, buff, 1024);
  std::wstring error(DXGetErrorString(hr));
  std::wstring descr(buff);
  std::wstring errMsgW = StringUtils::Format(L"%X - %s (%s)", hr, error.c_str(), descr.c_str());
  std::string errMsg;
  g_charsetConverter.wToUTF8(errMsgW, errMsg);
  return errMsg;
}

void CRenderSystemDX::SetStereoMode(RENDER_STEREO_MODE mode, RENDER_STEREO_VIEW view)
{
  CLog::Log(LOGERROR, "%s is not implemented", __FUNCTION__);
  return;

#if 0
  CRenderSystemBase::SetStereoMode(mode, view);
  auto pContext = DX::DeviceResources::Get()->GetD3DDeviceContext();
  auto pDevice = DX::DeviceResources::Get()->GetD3DDevice();

  if (!m_bRenderCreated)
    return;

  UINT writeMask = D3D11_COLOR_WRITE_ENABLE_ALL;
  if(m_stereoMode == RENDER_STEREO_MODE_ANAGLYPH_RED_CYAN)
  {
    if(m_stereoView == RENDER_STEREO_VIEW_LEFT)
      writeMask = D3D11_COLOR_WRITE_ENABLE_RED;
    else if(m_stereoView == RENDER_STEREO_VIEW_RIGHT)
      writeMask = D3D11_COLOR_WRITE_ENABLE_BLUE | D3D11_COLOR_WRITE_ENABLE_GREEN;
  }
  if(m_stereoMode == RENDER_STEREO_MODE_ANAGLYPH_GREEN_MAGENTA)
  {
    if(m_stereoView == RENDER_STEREO_VIEW_LEFT)
      writeMask = D3D11_COLOR_WRITE_ENABLE_GREEN;
    else if(m_stereoView == RENDER_STEREO_VIEW_RIGHT)
      writeMask = D3D11_COLOR_WRITE_ENABLE_BLUE | D3D11_COLOR_WRITE_ENABLE_RED;
  }
  if (m_stereoMode == RENDER_STEREO_MODE_ANAGLYPH_YELLOW_BLUE)
  {
    if (m_stereoView == RENDER_STEREO_VIEW_LEFT)
      writeMask = D3D11_COLOR_WRITE_ENABLE_RED | D3D11_COLOR_WRITE_ENABLE_GREEN;
    else if (m_stereoView == RENDER_STEREO_VIEW_RIGHT)
      writeMask = D3D11_COLOR_WRITE_ENABLE_BLUE;
  }
  if ( RENDER_STEREO_MODE_INTERLACED    == m_stereoMode
    || RENDER_STEREO_MODE_CHECKERBOARD  == m_stereoMode
    || RENDER_STEREO_MODE_HARDWAREBASED == m_stereoMode)
  {
    if (m_stereoView == RENDER_STEREO_VIEW_RIGHT)
    {
      // render right eye view to right render target
      pContext->OMSetRenderTargets(1, &m_pRenderTargetViewRight, m_depthStencilView);
    }
  }

  D3D11_BLEND_DESC desc;
  m_BlendEnableState->GetDesc(&desc);
  // update blend state
  if (desc.RenderTarget[0].RenderTargetWriteMask != writeMask)
  {
    SAFE_RELEASE(m_BlendDisableState);
    SAFE_RELEASE(m_BlendEnableState);

    desc.RenderTarget[0].RenderTargetWriteMask = writeMask;
    pDevice->CreateBlendState(&desc, &m_BlendEnableState);

    desc.RenderTarget[0].BlendEnable = false;
    pDevice->CreateBlendState(&desc, &m_BlendDisableState);

    float blendFactors[] = { 0.0f, 0.0f, 0.0f, 0.0f };
    pContext->OMSetBlendState(m_BlendEnabled ? m_BlendEnableState : m_BlendDisableState, blendFactors, 0xFFFFFFFF);
  }
#endif
}

bool CRenderSystemDX::GetStereoEnabled() const
{
  IDXGIFactory2* dxgiFactory2 = DX::DeviceResources::Get()->GetIDXGIFactory2();
  return dxgiFactory2->IsWindowedStereoEnabled() == TRUE;
}

bool CRenderSystemDX::GetDisplayStereoEnabled() const
{
  bool result = false;

#if 0
  IDXGIDisplayControl * pDXGIDisplayControl = nullptr;
  if (SUCCEEDED(m_dxgiFactory->QueryInterface(__uuidof(IDXGIDisplayControl), reinterpret_cast<void **>(&pDXGIDisplayControl))))
    result = pDXGIDisplayControl->IsStereoEnabled() == TRUE;
  SAFE_RELEASE(pDXGIDisplayControl);
#endif
  return result;
}

void CRenderSystemDX::SetDisplayStereoEnabled(bool enable) const
{
#if 0
  IDXGIDisplayControl * pDXGIDisplayControl = nullptr;
  if (SUCCEEDED(m_dxgiFactory->QueryInterface(__uuidof(IDXGIDisplayControl), reinterpret_cast<void **>(&pDXGIDisplayControl))))
    pDXGIDisplayControl->SetStereoEnabled(enable);
  SAFE_RELEASE(pDXGIDisplayControl);
#endif
}

void CRenderSystemDX::UpdateDisplayStereoStatus(bool first)
{
  if (first)
    m_bDefaultStereoEnabled = GetDisplayStereoEnabled();

  if (!first || !m_bDefaultStereoEnabled)
    SetDisplayStereoEnabled(true);

  m_bStereoEnabled = GetStereoEnabled();
  SetDisplayStereoEnabled(false);
}

bool CRenderSystemDX::SupportsStereo(RENDER_STEREO_MODE mode) const
{
  switch (mode)
  {
    case RENDER_STEREO_MODE_ANAGLYPH_RED_CYAN:
    case RENDER_STEREO_MODE_ANAGLYPH_GREEN_MAGENTA:
    case RENDER_STEREO_MODE_ANAGLYPH_YELLOW_BLUE:
    case RENDER_STEREO_MODE_INTERLACED:
    case RENDER_STEREO_MODE_CHECKERBOARD:
      return true;
    case RENDER_STEREO_MODE_HARDWAREBASED:
      return m_bStereoEnabled || GetStereoEnabled();
    default:
      return CRenderSystemBase::SupportsStereo(mode);
  }
}

void CRenderSystemDX::FlushGPU() const
{
  if (!m_bRenderCreated)
    return;

  FinishCommandList();

  auto pContext = DX::DeviceResources::Get()->GetD3DDeviceContext();
  pContext->Flush();
}

bool CRenderSystemDX::InitGUIShader()
{
  SAFE_DELETE(m_pGUIShader);
  m_pGUIShader = new CGUIShaderDX();
  if (!m_pGUIShader->Initialize())
  {
    CLog::Log(LOGERROR, __FUNCTION__ " - Failed to initialize GUI shader.");
    return false;
  }

  m_pGUIShader->ApplyStateBlock();

  return true;
}

void CRenderSystemDX::SetAlphaBlendEnable(bool enable)
{
  if (!m_bRenderCreated)
    return;

  float blendFactors[] = { 0.0f, 0.0f, 0.0f, 0.0f };
  auto pContext = DX::DeviceResources::Get()->GetD3DDeviceContext();
  pContext->OMSetBlendState(enable ? m_BlendEnableState : m_BlendDisableState, nullptr, 0xFFFFFFFF);
  m_BlendEnabled = enable;
}

void CRenderSystemDX::FinishCommandList(bool bExecute /*= true*/) const
{
  return;
#if 0
  auto pContext = DX::DeviceResources::Get()->GetD3DDeviceContext();
  ID3D11CommandList* pCommandList = nullptr;
  if (FAILED(pContext->FinishCommandList(true, &pCommandList)))
  {
    CLog::Log(LOGERROR, "%s - Failed to finish command queue.", __FUNCTION__);
    return;
  }

  if (bExecute)
    m_pImdContext->ExecuteCommandList(pCommandList, false);

  SAFE_RELEASE(pCommandList);
#endif
}

void CRenderSystemDX::SetMaximumFrameLatency(uint8_t latency) const
{
  auto pDevice = DX::DeviceResources::Get()->GetD3DDevice();
  IDXGIDevice1* pDXGIDevice = nullptr;
  if (SUCCEEDED(pDevice->QueryInterface(__uuidof(IDXGIDevice1), reinterpret_cast<void**>(&pDXGIDevice))))
  {
    // in windowed mode DWM uses triple buffering in any case. 
    // for FSEM we use same buffering to avoid possible shuttering/tearing
    if (latency == -1)
      latency = m_useWindowedDX ? 1 : 3;
    pDXGIDevice->SetMaximumFrameLatency(latency);
    SAFE_RELEASE(pDXGIDevice);
  }
}



void CRenderSystemDX::FixRefreshRateIfNecessary(const D3D10DDIARG_CREATERESOURCE* pResource)
{
#if 0
  if (pResource && pResource->pPrimaryDesc)
  {
    float refreshRate = RATIONAL_TO_FLOAT(pResource->pPrimaryDesc->ModeDesc.RefreshRate);
    if (refreshRate > 10.0f && refreshRate < 300.0f)
    {
      uint32_t refreshNum, refreshDen;
      GetRefreshRatio(static_cast<uint32_t>(m_refreshRate), &refreshNum, &refreshDen);
      float diff = fabs(refreshRate - ((float)refreshNum / (float)refreshDen)) / refreshRate;
      CLog::Log(LOGDEBUG, __FUNCTION__": refreshRate: %0.4f, desired: %0.4f, deviation: %.5f, fixRequired: %s", 
                refreshRate, m_refreshRate, diff, (diff > 0.0005) ? "true" : "false");
      if (diff > 0.0005)
      {
        pResource->pPrimaryDesc->ModeDesc.RefreshRate.Numerator = refreshNum;
        pResource->pPrimaryDesc->ModeDesc.RefreshRate.Denominator = refreshDen;
        CLog::Log(LOGDEBUG, __FUNCTION__": refreshRate fix applied -> %0.3f", RATIONAL_TO_FLOAT(pResource->pPrimaryDesc->ModeDesc.RefreshRate));
      }
    }
  }
#endif
}

void CRenderSystemDX::GetRefreshRatio(uint32_t refresh, uint32_t * num, uint32_t * den)
{
  int i = (((refresh + 1) % 24) == 0 || ((refresh + 1) % 30) == 0) ? 1 : 0;
  *num = (refresh + i) * 1000;
  *den = 1000 + i;
}



