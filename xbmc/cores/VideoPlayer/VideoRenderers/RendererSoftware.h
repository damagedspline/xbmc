#pragma once
#include "RendererBase.h"
#include <d3d11.h>
#include <libavutil/pixfmt.h>
#include <map>
#include "DVDCodecs/Video/DVDVideoCodecFFmpeg.h"

class CRenderBufferSoftware : public CRenderBufferBase
{
public:
  CRenderBufferSoftware(AVPixelFormat av_pix_format, unsigned width, unsigned height, DXGI_FORMAT dxgi_format = DXGI_FORMAT_UNKNOWN);

  void AppendPicture(const VideoPicture& picture) override;
  bool GetDataPlanes(uint8_t*(& planes)[3], int(& strides)[3]) override;

  void ReleasePicture() override;
  bool IsLoaded() override { return m_msr.pData != nullptr;}
  bool UploadBuffer() override;

private:
  void QueueCopyFromGPU();

  Microsoft::WRL::ComPtr<ID3D11Texture2D> m_staging;
  D3D11_TEXTURE2D_DESC m_sDesc{};
  D3D11_MAPPED_SUBRESOURCE m_msr{};
  bool m_bPending = false;
};

class CRendererSoftware : public CRendererBase
{
public:
  static bool IsFormatSupported(AVPixelFormat av_pixel_format, DXGI_FORMAT dxgi_format);
  static void GetWeight(std::map<RenderMethod, int>& weights, AVPixelFormat av_pixel_format, DXGI_FORMAT dxgi_format);

  CRendererSoftware() = default;
  ~CRendererSoftware() = default;

  bool Supports(ESCALINGMETHOD method) override;
  void AddVideoPicture(const VideoPicture& picture, int index) override;

protected:
  bool CreateRenderBuffer(int index) override;
  void RendererRender(CD3DTexture& target, CRect& sourceRect, CRect& destRect, CRect& viewRect, uint32_t flags) override;

private:
  SwsContext* m_sw_scale_ctx = nullptr;
};
