#include "RendererBase.h"
#include "commons/ilog.h"
#include "utils/log.h"
#include "rendering/dx/RenderContext.h"
#include "DVDCodecs/Video/DVDVideoCodec.h"
#include "DVDCodecs/Video/DXVA.h"

using namespace Microsoft::WRL;

void CRenderBufferBase::AppendPicture(const VideoPicture& picture)
{
  videoBuffer = picture.videoBuffer;
  videoBuffer->Acquire();

  pictureFlags = picture.iFlags;
  primaries = static_cast<AVColorPrimaries>(picture.color_primaries);
  color_space = static_cast<AVColorSpace>(picture.color_space);
  color_transfer = static_cast<AVColorTransferCharacteristic>(picture.color_transfer);
  full_range = picture.color_range == 1;
  bits = picture.colorBits;
  stereoMode = picture.stereoMode;

  hasDisplayMetadata = picture.hasDisplayMetadata;
  displayMetadata = picture.displayMetadata;
  lightMetadata = picture.lightMetadata;
  if (picture.hasLightMetadata && picture.lightMetadata.MaxCLL)
    hasLightMetadata = true;
}

void CRenderBufferBase::ReleasePicture()
{
  if (videoBuffer)
    videoBuffer->Release();
  videoBuffer = nullptr;
}

CRenderBufferBase::CRenderBufferBase(AVPixelFormat av_pix_format, unsigned width, unsigned height, DXGI_FORMAT dxgi_format)
  : m_width(width)
  , m_height(height)
  , m_av_format(av_pix_format)
  , m_dx_format(dxgi_format)
{
}

HRESULT CRenderBufferBase::GetHWResource(ID3D11Resource** ppResource, unsigned* index) const
{
  if (!ppResource)
    return E_POINTER;
  if (!index)
    return E_POINTER;

  HRESULT hr = E_UNEXPECTED;
  ComPtr<ID3D11Resource> pResource;
  unsigned idx = -1;

  auto dxva = dynamic_cast<DXVA::CDXVAOutputBuffer*>(videoBuffer);
  if (dxva)
  {
    if (dxva->shared)
    {
      const HANDLE shared_handle = dxva->GetHandle();
      if (shared_handle == INVALID_HANDLE_VALUE)
        return E_HANDLE;

      ComPtr<ID3D11Device> pD3DDevice = DX::DeviceResources::Get()->GetD3DDevice();
      hr = pD3DDevice->OpenSharedResource(shared_handle, __uuidof(ID3D11Resource),
        reinterpret_cast<void**>(pResource.GetAddressOf()));
    }
    else
    {
      if (dxva->view)
      {
        dxva->view->GetResource(&pResource);
        hr = S_OK;
      }
      else
        hr = E_UNEXPECTED;
    }
    idx = dxva->GetIdx();
  }
  if (SUCCEEDED(hr))
  {
    *ppResource = pResource.Detach();
    *index = idx;
  }

  return hr;
}


CRendererBase::CRendererBase(IVideoSettingsHolder* videoSettings)
  : m_videoSettings(videoSettings) 
{
}

CRenderInfo CRendererBase::GetRenderInfo()
{
  CRenderInfo info;
  info.formats =
  {
    AV_PIX_FMT_D3D11VA_VLD,
    AV_PIX_FMT_NV12,
    AV_PIX_FMT_P010,
    AV_PIX_FMT_P016,
    AV_PIX_FMT_YUV420P,
    AV_PIX_FMT_YUV420P10,
    AV_PIX_FMT_YUV420P16
  };
  info.max_buffer_size = 6;
  info.optimal_buffer_size = 4;

  return info;
}

bool CRendererBase::Configure(const VideoPicture& picture, float fps, unsigned orientation)
{
  // need to recreate textures
  m_NumYV12Buffers = 0;
  m_iYV12RenderBuffer = 0;

  m_sourceWidth = picture.iWidth;
  m_sourceHeight = picture.iHeight;

  m_srcPrimaries = GetSrcPrimaries(static_cast<AVColorPrimaries>(picture.color_primaries), picture.iWidth, picture.iHeight);
  m_format = picture.videoBuffer->GetFormat();

  if (m_format == AV_PIX_FMT_D3D11VA_VLD)
  {
    DXVA::CDXVAOutputBuffer *dxvaBuf = dynamic_cast<DXVA::CDXVAOutputBuffer*>(picture.videoBuffer);
    if (dxvaBuf)
    {
      m_dxva_format = dxvaBuf->format;
    }
  }

  return true;
}

void CRendererBase::Render(CD3DTexture& target, CRect& sourceRect, CRect& destRect, CRect& viewRect, uint32_t flags)
{
  CRenderBufferBase* buf = m_renderBuffers[m_iYV12RenderBuffer];
  if (!buf->IsLoaded())
  {
    if (!buf->UploadBuffer())
      return;
  }

  m_destWidth = static_cast<unsigned>(viewRect.Width());
  m_destHeight = static_cast<unsigned>(viewRect.Height());

  UpdateVideoFilters();
}

void CRendererBase::ManageTextures()
{
  if (m_NumYV12Buffers < m_neededBuffers)
  {
    for (int i = m_NumYV12Buffers; i < m_neededBuffers; i++)
      CreateRenderBuffer(i);

    m_NumYV12Buffers = m_neededBuffers;
  }
  else if (m_NumYV12Buffers > m_neededBuffers)
  {
    for (int i = m_NumYV12Buffers - 1; i >= m_neededBuffers; i--)
      DeleteRenderBuffer(i);

    m_NumYV12Buffers = m_neededBuffers;
    m_iYV12RenderBuffer = m_iYV12RenderBuffer % m_NumYV12Buffers;
  }
}

int CRendererBase::NextBuffer() const
{
  if (m_NumYV12Buffers)
    return (m_iYV12RenderBuffer + 1) % m_NumYV12Buffers;
  return -1;
}

void CRendererBase::ReleaseBuffer(int idx)
{
  m_renderBuffers[idx]->ReleasePicture();
}

bool CRendererBase::Flush(bool saveBuffers)
{
  if (!saveBuffers)
  {
    for (int i = 0; i < NUM_BUFFERS; i++)
      DeleteRenderBuffer(i);
  }

  m_iYV12RenderBuffer = 0;
  m_NumYV12Buffers = 0;

  return true;
}

void CRendererBase::DeleteRenderBuffer(int index)
{
  const auto it = m_renderBuffers.find(index);
  if (it != m_renderBuffers.end())
  {
    delete m_renderBuffers[index];
    m_renderBuffers.erase(it);
  }
}

bool CRendererBase::CreateIntermediateTarget(unsigned width, unsigned height, bool dynamic)
{
  DXGI_FORMAT format = DX::Windowing()->GetBackBuffer()->GetFormat();

  // don't create new one if it exists with requested size and format
  if (m_IntermediateTarget.Get() && m_IntermediateTarget.GetFormat() == format
    && m_IntermediateTarget.GetWidth() == width && m_IntermediateTarget.GetHeight() == height)
    return true;

  if (m_IntermediateTarget.Get())
    m_IntermediateTarget.Release();

  CLog::LogF(LOGDEBUG, "intermediate target format %i.", format);

  if (!m_IntermediateTarget.Create(width, height, 1, dynamic ? D3D11_USAGE_DYNAMIC : D3D11_USAGE_DEFAULT, format))
  {
    CLog::LogF(LOGERROR, "intermediate target creation failed.");
    return false;
  }
  return true;
}

AVColorPrimaries CRendererBase::GetSrcPrimaries(AVColorPrimaries srcPrimaries, unsigned width, unsigned height)
{
  AVColorPrimaries ret = srcPrimaries;
  if (ret == AVCOL_PRI_UNSPECIFIED)
  {
    if (width > 1024 || height >= 600)
      ret = AVCOL_PRI_BT709;
    else
      ret = AVCOL_PRI_BT470BG;
  }
  return ret;
}

void CRendererBase::UpdateVideoFilters()
{
  CRenderBufferBase* buf = m_renderBuffers[m_iYV12RenderBuffer];
  AVColorPrimaries srcPrim = GetSrcPrimaries(buf->primaries, buf->GetWidth(), buf->GetHeight());

  if (srcPrim != m_srcPrimaries)
  {
    m_srcPrimaries = srcPrim;
    m_bFilterInitialized = false;
  }

  bool toneMap = false;
  if (m_videoSettings->GetVideoSettings()->m_ToneMapMethod != VS_TONEMAPMETHOD_OFF)
  {
    if (buf->hasLightMetadata || buf->hasDisplayMetadata && buf->displayMetadata.has_luminance)
      toneMap = true;
  }
  if (toneMap != m_toneMapping)
  {
    m_outputShader.reset();
    m_bFilterInitialized = false;
  }
  m_toneMapping = toneMap;

  bool cmsChanged = m_cmsOn != m_colorManager->IsEnabled()
    || m_cmsOn && !m_colorManager->CheckConfiguration(m_cmsToken, m_iFlags);
  cmsChanged &= m_clutLoaded;

  if (m_scalingMethodGui == m_videoSettings->GetVideoSettings()->m_ScalingMethod
    && m_bFilterInitialized && !cmsChanged)
    return;

  m_bFilterInitialized = true;
  m_scalingMethodGui = m_videoSettings->GetVideoSettings()->m_ScalingMethod;
  m_scalingMethod = m_scalingMethodGui;

  if (!Supports(m_scalingMethod))
  {
    CLog::Log(LOGWARNING, "%s: chosen scaling method %d is not supported by renderer", __FUNCTION__, static_cast<int>(m_scalingMethod));
    m_scalingMethod = VS_SCALINGMETHOD_AUTO;
  }

  if (cmsChanged)
    ColorManagmentUpdate();

  if (cmsChanged || !m_outputShader)
  {
    m_outputShader = std::make_unique<COutputShader>();
    if (!m_outputShader->Create(m_cmsOn, m_useDithering, m_ditherDepth, m_toneMapping))
    {
      CLog::Log(LOGDEBUG, "%s: Unable to create output shader.", __FUNCTION__);
      m_outputShader.reset();
    }
    else if (m_pCLUTView && m_CLUTSize)
      m_outputShader->SetCLUT(m_CLUTSize, m_pCLUTView.Get());
  }

}
