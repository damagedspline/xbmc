#pragma once
#include "RendererHQ.h"
#include <libavutil/pixfmt.h>
#include <d3d11.h>
#include <map>

enum RenderMethod;

class CRendererDXVA : public CRendererHQ
{
public:
  static bool IsFormatSupported(AVPixelFormat av_pixel_format, DXGI_FORMAT dxgi_format);
  static void GetWeight(std::map<RenderMethod, int>& weights, AVPixelFormat av_pixel_format, DXGI_FORMAT dxgi_format);

  CRenderInfo GetRenderInfo() override;
  bool WantsDoublePass() override { return true; }
};
