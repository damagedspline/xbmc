#pragma once
#include "RendererHQ.h"
#include <d3d11.h>
#include <libavutil/pixfmt.h>
#include <map>

enum RenderMethod;

class CRendererShaders : public CRendererHQ
{
public:
  static bool IsFormatSupported(AVPixelFormat av_pixel_format, DXGI_FORMAT dxgi_format);
  static void GetWeight(std::map<RenderMethod, int>& weights, AVPixelFormat av_pixel_format, DXGI_FORMAT dxgi_format);

};
