#pragma once

#include "ColorManager.h"
#include "RenderInfo.h"
#include "VideoRenderers/VideoShaders/WinVideoFilter.h"

#include <vector>
#include "BaseRenderer.h"

struct VideoPicture;

namespace win
{
  namespace helpers
  {
    template<typename T>
    bool contains(std::vector<T> vector, T item)
    {
      return find(vector.begin(), vector.end(), item) != vector.end();
    }
  }
}

enum RenderMethod
{
  RENDER_INVALID = 0x00,
  RENDER_PS = 0x01,
  RENDER_SW = 0x02,
  RENDER_DXVA = 0x03,
};

struct IVideoSettingsHolder
{
  virtual ~IVideoSettingsHolder() = default;
  virtual CVideoSettings* GetVideoSettings() = 0;
};

class CRenderBufferBase
{
public:
  virtual ~CRenderBufferBase() = default;
  virtual void AppendPicture(const VideoPicture& picture);
  virtual bool GetDataPlanes(uint8_t*(&planes)[3], int(&strides)[3]) { return false; }
  virtual void ReleasePicture();
  virtual bool IsLoaded() { return false; }
  virtual bool UploadBuffer() { return false; }
  unsigned GetWidth() const { return m_width; }
  unsigned GetHeight() const { return m_height; }

  CVideoBuffer* videoBuffer = nullptr;
  unsigned int pictureFlags = 0;
  AVColorPrimaries primaries = AVCOL_PRI_BT709;
  AVColorSpace color_space = AVCOL_SPC_BT709;
  AVColorTransferCharacteristic color_transfer = AVCOL_TRC_BT709;
  bool full_range = false;
  int bits = 8;
  uint8_t texBits = 8;

  bool hasDisplayMetadata = false;
  bool hasLightMetadata = false;
  AVMasteringDisplayMetadata displayMetadata = {};
  AVContentLightMetadata lightMetadata = {};
  std::string stereoMode;

protected:
  CRenderBufferBase(AVPixelFormat av_pix_format, unsigned width, unsigned height, DXGI_FORMAT dxgi_format = DXGI_FORMAT_UNKNOWN);
  HRESULT GetHWResource(ID3D11Resource** ppResource, unsigned* index) const;

  // video buffer size
  unsigned int m_width;
  unsigned int m_height;
  AVPixelFormat m_av_format;
  DXGI_FORMAT m_dx_format;

};

class CRendererBase
{
public:
  explicit CRendererBase(IVideoSettingsHolder* videoSettings);
  virtual ~CRendererBase() = default;

  virtual CRenderInfo GetRenderInfo();
  virtual bool Configure(const VideoPicture &picture, float fps, unsigned int orientation);
  virtual bool Supports(ESCALINGMETHOD method) = 0;
  virtual void AddVideoPicture(const VideoPicture &picture, int index) = 0;
  void Render(CD3DTexture& target, CRect& sourceRect, CRect& destRect, CRect& viewRect, uint32_t flags);
  void ManageTextures();
  int NextBuffer() const;
  virtual void ReleaseBuffer(int idx);
  virtual bool NeedBuffer(int idx) { return false; }
  virtual bool Flush(bool saveBuffers);
  virtual bool WantsDoublePass() { return false; };

protected:
  virtual void RendererRender(CD3DTexture& target, CRect& sourceRect, CRect& destRect, CRect& viewRect, uint32_t flags) = 0;
  virtual void DeleteRenderBuffer(int index);
  virtual bool CreateRenderBuffer(int index) = 0;
  bool CreateIntermediateTarget(unsigned int width, unsigned int height, bool dynamic);
  static AVColorPrimaries GetSrcPrimaries(AVColorPrimaries srcPrimaries, unsigned int width, unsigned int height);
  virtual void UpdateVideoFilters();

  int m_iYV12RenderBuffer = 0;
  int m_NumYV12Buffers = 0;
  int m_neededBuffers = 0;
  std::map<int, CRenderBufferBase*> m_renderBuffers;

  AVPixelFormat m_format = AV_PIX_FMT_NONE;
  DXGI_FORMAT m_dxva_format = DXGI_FORMAT_UNKNOWN;
  AVColorPrimaries m_srcPrimaries = AVCOL_PRI_BT709;

  CD3DTexture m_IntermediateTarget;
  std::unique_ptr<COutputShader> m_outputShader;
  std::unique_ptr<CColorManager> m_colorManager;
  Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_pCLUTView;

  unsigned m_sourceWidth = 0;
  unsigned m_sourceHeight = 0;
  unsigned m_destWidth = 0;
  unsigned m_destHeight = 0;

  IVideoSettingsHolder* m_videoSettings = nullptr;
  bool m_toneMapping = false;
  bool m_cmsOn = false;
  bool m_clutLoaded = false;
};
