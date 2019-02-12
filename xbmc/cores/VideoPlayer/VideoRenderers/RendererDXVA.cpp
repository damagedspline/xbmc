#include "RendererDXVA.h"
#include "WinRenderer.h"

bool CRendererDXVA::IsFormatSupported(AVPixelFormat av_pixel_format, DXGI_FORMAT dxgi_format)
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

void CRendererDXVA::GetWeight(std::map<RenderMethod, int>& weights, AVPixelFormat av_pixel_format, DXGI_FORMAT dxgi_format)
{
  if (av_pixel_format == AV_PIX_FMT_D3D11VA_VLD)
    weights[RENDER_DXVA] = 1000;

  else if (av_pixel_format == AV_PIX_FMT_NV12 ||
    av_pixel_format == AV_PIX_FMT_P010 ||
    av_pixel_format == AV_PIX_FMT_P016)
    weights[RENDER_DXVA] = 500;

  else if (av_pixel_format == AV_PIX_FMT_YUV420P ||
    av_pixel_format == AV_PIX_FMT_YUV420P10 ||
    av_pixel_format == AV_PIX_FMT_YUV420P16)
    weights[RENDER_DXVA] = 100;
}

CRenderInfo CRendererDXVA::GetRenderInfo()
{
  auto info = __super::GetRenderInfo();

  //info.optimal_buffer_size = m_processor->Size();
  info.optimal_buffer_size = std::min(info.max_buffer_size, info.optimal_buffer_size);

  if (m_format != AV_PIX_FMT_D3D11VA_VLD)
    info.m_deintMethods.push_back(VS_INTERLACEMETHOD_DXVA_AUTO);

  return  info;
}

