/*
*      Copyright (C) 2010-2016 Hendrik Leppkes
*      http://www.1f0.de
*
*  This program is free software; you can redistribute it and/or modify
*  it under the terms of the GNU General Public License as published by
*  the Free Software Foundation; either version 2 of the License, or
*  (at your option) any later version.
*
*  This program is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*  GNU General Public License for more details.
*
*  You should have received a copy of the GNU General Public License along
*  with this program; if not, write to the Free Software Foundation, Inc.,
*  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#pragma once
#include "DVDVideoCodec.h"
#include "DVDResource.h"
#include "threads/Event.h"
#include <vector>

#include "libmfx/mfxvideo.h"
#include "libmfx/mfxmvc.h"

#define ASYNC_DEPTH 10

typedef struct _MVCBuffer 
{
  mfxFrameSurface1 surface;
  bool queued = false;
  mfxSyncPoint sync = nullptr;
  _MVCBuffer() 
  { 
    memset(&surface, 0, sizeof(surface)); 
  };
} MVCBuffer;

class CAnnexBConverter
{
public:
  CAnnexBConverter(void) {};
  ~CAnnexBConverter(void) {};

  void SetNALUSize(int nalusize) { m_NaluSize = nalusize; }
  bool Convert(uint8_t **poutbuf, int *poutbuf_size, const uint8_t *buf, int buf_size);

private:
  int m_NaluSize = 0;
};

class CDVDVideoCodecMFX;

class CMVCPicture : 
  public IDVDResourceCounted<CMVCPicture>
{
public:
  CMVCPicture(MVCBuffer *pBaseView, MVCBuffer* pExtraBuffer)
    : baseView(pBaseView), extraView(pExtraBuffer)
  {};
  ~CMVCPicture() { free_buffers(this); }
  MVCBuffer *baseView = nullptr;
  MVCBuffer* extraView = nullptr;
  CDVDVideoCodecMFX* dec = nullptr;
  void (*free_buffers)(CMVCPicture* pic);
};

class CDVDVideoCodecMFX : public CDVDVideoCodec
{
public:
  CDVDVideoCodecMFX(CProcessInfo &processInfo);
  virtual ~CDVDVideoCodecMFX();

  virtual bool Open(CDVDStreamInfo &hints, CDVDCodecOptions &options);
  virtual void Dispose() { DestroyDecoder(true); }
  virtual int Decode(uint8_t* pData, int iSize, double dts, double pts);
  virtual void Reset() { Flush(); };
  virtual bool GetPicture(DVDVideoPicture* pDvdVideoPicture);
  virtual void SetDropState(bool bDrop) {};
  virtual const char* GetName() { return "msdk-mvc"; };

  virtual bool GetCodecStats(double &pts, int &droppedPics, int &skippedPics) override { return true; };
  void ReleasePicture(CMVCPicture* pMVCPicture);

private:
  bool Init();
  bool Flush();
  bool EndOfStream();
  void DestroyDecoder(bool bFull);
  bool AllocateMVCExtBuffers();
  void SetStereoMode(CDVDStreamInfo &hints);

  MVCBuffer * GetBuffer();
  MVCBuffer * FindBuffer(mfxFrameSurface1 * pSurface);
  void ReleaseBuffer(mfxFrameSurface1 * pSurface);

  int HandleOutput(MVCBuffer * pOutputBuffer);
  void SyncOutput(MVCBuffer * pBaseView, MVCBuffer * pExtraView);

private:

  mfxSession m_mfxSession = nullptr;
  mfxVersion m_mfxVersion;

  bool                 m_bDecodeReady = false;
  mfxVideoParam        m_mfxVideoParams;

  mfxExtBuffer        *m_mfxExtParam[1];
  mfxExtMVCSeqDesc     m_mfxExtMVCSeq;

  CCriticalSection     m_BufferCritSec;
  std::vector<MVCBuffer*> m_BufferQueue;

  std::vector<BYTE>    m_buff;
  CAnnexBConverter    *m_pAnnexBConverter = nullptr;

  MVCBuffer           *m_pOutputQueue[ASYNC_DEPTH];
  int                  m_nOutputQueuePosition = 0;
  CMVCPicture*         m_pRenderPicture = nullptr;
  std::string          m_stereoMode;
};
