#include "RendererSoftware.h"
#include "WinRenderer.h"
#include "rendering/dx/DeviceResources.h"
#include "commons/ilog.h"
#include "utils/log.h"
#include "rendering/dx/RenderContext.h"

using namespace Microsoft::WRL;

CRenderBufferSoftware::CRenderBufferSoftware(AVPixelFormat av_pix_format, unsigned width, unsigned height, DXGI_FORMAT dxgi_format)
  : CRenderBufferBase(av_pix_format, width, height, dxgi_format)
{
}

void CRenderBufferSoftware::AppendPicture(const VideoPicture& picture)
{
  __super::AppendPicture(picture);
  if (videoBuffer->GetFormat() == AV_PIX_FMT_D3D11VA_VLD)
    QueueCopyFromGPU();
}

bool CRenderBufferSoftware::GetDataPlanes(uint8_t*(& planes)[3], int(& strides)[3])
{
  switch (videoBuffer->GetFormat())
  {
  case AV_PIX_FMT_D3D11VA_VLD:
      planes[0] = reinterpret_cast<uint8_t*>(m_msr.pData);
      planes[1] = reinterpret_cast<uint8_t*>(m_msr.pData) + m_msr.RowPitch * m_sDesc.Height;
      strides[0] = strides[1] = m_msr.RowPitch;
    break;
  default:
    videoBuffer->GetPlanes(planes);
    videoBuffer->GetStrides(strides);
  }

  return true;
}

void CRenderBufferSoftware::ReleasePicture()
{
  if (m_staging && m_msr.pData != nullptr)
  {
    DX::DeviceResources::Get()->GetImmediateContext()->Unmap(m_staging.Get(), 0);
    m_msr = {};
  }
  __super::ReleasePicture();
}

bool CRenderBufferSoftware::UploadBuffer()
{
  if (!m_staging)
    return false;

  // map will finish copying data from GPU mem to CPU mem
  return SUCCEEDED(SUCCEEDED(DX::DeviceResources::Get()->GetImmediateContext()->Map(m_staging.Get(), 0, D3D11_MAP_READ, 0, &m_msr)));
}

void CRenderBufferSoftware::QueueCopyFromGPU()
{
  if (!videoBuffer)
    return;

  unsigned index;
  ComPtr<ID3D11Resource> pResource;
  const HRESULT hr = GetHWResource(pResource.GetAddressOf(), &index);

  if (FAILED(hr))
  {
    CLog::LogF(LOGERROR, "unable to open d3d11va resource.");
    return;
  }

  if (!m_staging)
  {
    // create staging texture
    ComPtr<ID3D11Texture2D> surface;
    if (SUCCEEDED(pResource.As(&surface)))
    {
      D3D11_TEXTURE2D_DESC tDesc;
      surface->GetDesc(&tDesc);

      CD3D11_TEXTURE2D_DESC sDesc(tDesc);
      sDesc.ArraySize = 1;
      sDesc.Usage = D3D11_USAGE_STAGING;
      sDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
      sDesc.BindFlags = 0;

      if (SUCCEEDED(DX::DeviceResources::Get()->GetD3DDevice()->CreateTexture2D(
        &sDesc, nullptr, m_staging.GetAddressOf())))
        m_sDesc = sDesc;
    }
  }

  if (m_staging)
  {
    ComPtr<ID3D11DeviceContext> pContext = DX::DeviceResources::Get()->GetImmediateContext();
    // queue copying content from decoder texture to temporary texture.
    // actual data copying will be performed before rendering
    pContext->CopySubresourceRegion(m_staging.Get(), D3D11CalcSubresource(0, 0, 1), 0, 0, 0,
      pResource.Get(), D3D11CalcSubresource(0, index, 1), nullptr);
    m_bPending = true;
  }
}

bool CRendererSoftware::IsFormatSupported(AVPixelFormat av_pixel_format, DXGI_FORMAT dxgi_format)
{
  if (av_pixel_format == AV_PIX_FMT_D3D11VA_VLD)
    return true;

  return av_pixel_format == AV_PIX_FMT_NV12 ||
    av_pixel_format == AV_PIX_FMT_P010 ||
    av_pixel_format == AV_PIX_FMT_P016 ||
    av_pixel_format == AV_PIX_FMT_YUV420P ||
    av_pixel_format == AV_PIX_FMT_YUV420P10 ||
    av_pixel_format == AV_PIX_FMT_YUV420P16;
}

void CRendererSoftware::GetWeight(std::map<RenderMethod, int>& weights, AVPixelFormat av_pixel_format, DXGI_FORMAT dxgi_format)
{
  if (av_pixel_format == AV_PIX_FMT_D3D11VA_VLD)
    weights[RENDER_SW] = 100;

  else if (av_pixel_format == AV_PIX_FMT_YUV420P ||
    av_pixel_format == AV_PIX_FMT_YUV420P10 ||
    av_pixel_format == AV_PIX_FMT_YUV420P16 ||
    av_pixel_format == AV_PIX_FMT_NV12 ||
    av_pixel_format == AV_PIX_FMT_P010 ||
    av_pixel_format == AV_PIX_FMT_P016)
    weights[RENDER_SW] = 500;
}

bool CRendererSoftware::Supports(ESCALINGMETHOD method)
{
  return method == VS_SCALINGMETHOD_AUTO
    || method == VS_SCALINGMETHOD_LINEAR;
}

void CRendererSoftware::AddVideoPicture(const VideoPicture& picture, int index)
{
  if (m_renderBuffers[index])
    m_renderBuffers[index]->AppendPicture(picture);
}

bool CRendererSoftware::CreateRenderBuffer(int index)
{
  m_renderBuffers.insert(std::make_pair(index, new CRenderBufferSoftware(AV_PIX_FMT_NV12, 1000, 1000)));
  return true;
}

void CRendererSoftware::RendererRender(CD3DTexture& target, CRect& sourceRect, CRect& destRect, CRect& viewRect, uint32_t flags)
{
  // if creation failed
  if (!m_outputShader)
    return;

  // Don't know where this martian comes from but it can happen in the initial frames of a video
  if (destRect.x1 < 0 && destRect.x2 < 0
    || destRect.y1 < 0 && destRect.y2 < 0)
    return;

  // fit format in case of hw decoder
  AVPixelFormat decoderFormat = m_format;
  if (m_format == AV_PIX_FMT_D3D11VA_VLD)
  {
    switch (m_dxva_format)
    {
    case DXGI_FORMAT_NV12:
      decoderFormat = AV_PIX_FMT_NV12;
      break;
    case DXGI_FORMAT_P010:
      decoderFormat = AV_PIX_FMT_P010;
      break;
    case DXGI_FORMAT_P016:
      decoderFormat = AV_PIX_FMT_P016;
      break;
    default:
      break;
    }
  }

  // 1. convert yuv to rgb
  m_sw_scale_ctx = sws_getCachedContext(m_sw_scale_ctx,
    m_sourceWidth, m_sourceHeight, decoderFormat,
    m_sourceWidth, m_sourceHeight, AV_PIX_FMT_BGRA,
    SWS_FAST_BILINEAR, nullptr, nullptr, nullptr);

  CRenderBufferBase* buf = m_renderBuffers[m_iYV12RenderBuffer];

  uint8_t* src[YuvImage::MAX_PLANES];
  int srcStride[YuvImage::MAX_PLANES];
  buf->GetDataPlanes(src, srcStride);

  D3D11_MAPPED_SUBRESOURCE destlr;
  if (m_IntermediateTarget.LockRect(0, &destlr, D3D11_MAP_WRITE_DISCARD))
  {
    uint8_t *dst[] = { static_cast<uint8_t*>(destlr.pData), nullptr, nullptr };
    int dstStride[] = { static_cast<int>(destlr.RowPitch), 0, 0 };

    sws_scale(m_sw_scale_ctx, src, srcStride, 0, m_sourceHeight, dst, dstStride);

    if (m_IntermediateTarget.UnlockRect(0))
    {
      // 2. output to display
      m_outputShader->SetDisplayMetadata(buf->hasDisplayMetadata, buf->displayMetadata, buf->hasLightMetadata, buf->lightMetadata);
      m_outputShader->SetToneMapParam(m_videoSettings->GetVideoSettings()->m_ToneMapParam);

      m_outputShader->Render(m_IntermediateTarget, m_sourceWidth, m_sourceHeight, sourceRect, m_rotatedDestCoords, target,
        DX::Windowing()->UseLimitedColor(), m_videoSettings->GetVideoSettings()->m_Contrast * 0.01f, m_videoSettings->GetVideoSettings()->m_Brightness * 0.01f);
    }
    else
      CLog::Log(LOGERROR, "%s: failed to unlock swtarget texture.", __FUNCTION__);
  }
  else
    CLog::Log(LOGERROR, "%s: failed to lock swtarget texture into memory.", __FUNCTION__);
}
