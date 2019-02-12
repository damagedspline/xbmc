#include "RendererShaders.h"
#include "WinRenderer.h"
#include "rendering/dx/RenderContext.h"

bool CRendererShaders::IsFormatSupported(AVPixelFormat av_pixel_format, DXGI_FORMAT dxgi_format)
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

void CRendererShaders::GetWeight(std::map<RenderMethod, int>& weights, AVPixelFormat av_pixel_format, DXGI_FORMAT dxgi_format)
{
  if (av_pixel_format == AV_PIX_FMT_D3D11VA_VLD)
  {
    AVPixelFormat decoderFormat = av_pixel_format;
    switch (dxgi_format)
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

    if (win::helpers::contains(DX::Windowing()->m_sharedFormats, decoderFormat))
      weights[RENDER_PS] = 1000;
    else
      weights[RENDER_PS] = 100;
  }

  else if (av_pixel_format == AV_PIX_FMT_YUV420P ||
    av_pixel_format == AV_PIX_FMT_YUV420P10 ||
    av_pixel_format == AV_PIX_FMT_YUV420P16 ||
    av_pixel_format == AV_PIX_FMT_NV12 ||
    av_pixel_format == AV_PIX_FMT_P010 ||
    av_pixel_format == AV_PIX_FMT_P016)
    weights[RENDER_PS] = 1000;
}
