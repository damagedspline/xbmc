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

#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <system.h>
#include <sys/stat.h>

#include "XBTFReader.h"
#include "guilib/XBTF.h"
#include "utils/EndianSwap.h"

#ifdef TARGET_WINDOWS
#include "filesystem/SpecialProtocol.h"
#include "utils/CharsetConverter.h"
#include "platform/win32/PlatformDefs.h"
#endif

static bool ReadString(XFILE::CFile *file, char* str, size_t max_length)
{
  if (file == nullptr || str == nullptr || max_length <= 0)
    return false;

  return (file->Read(str, max_length) == max_length);
}

static bool ReadUInt32(XFILE::CFile *file, uint32_t& value)
{
  if (file == nullptr)
    return false;

  if (file->Read(&value, sizeof(uint32_t)) != sizeof(uint32_t))
    return false;

  value = Endian_SwapLE32(value);
  return true;
}

static bool ReadUInt64(XFILE::CFile *file, uint64_t& value)
{
  if (file == nullptr)
    return false;

  if (file->Read(&value, sizeof(uint64_t)) != sizeof(uint64_t))
    return false;

  value = Endian_SwapLE64(value);
  return true;
}

CXBTFReader::CXBTFReader()
  : CXBTFBase(),
    m_path(),
    m_file(nullptr)
{ }

CXBTFReader::~CXBTFReader()
{
  Close();
}

bool CXBTFReader::Open(const std::string& path)
{
  if (path.empty())
    return false;

  m_path = path;

  std::string strPath = m_path;
#ifdef TARGET_WINDOWS
  strPath = CSpecialProtocol::TranslatePath(m_path);
#endif
  m_file = new XFILE::CFile();
  if (!m_file->Open(strPath))
  {
    Close();
    return false;
  }

  // read the magic word
  char magic[4];
  if (!ReadString(m_file, magic, sizeof(magic)))
    return false;

  if (strncmp(XBTF_MAGIC.c_str(), magic, sizeof(magic)) != 0)
    return false;

  // read the version
  char version[1];
  if (!ReadString(m_file, version, sizeof(version)))
    return false;

  if (strncmp(XBTF_VERSION.c_str(), version, sizeof(version)) != 0)
    return false;

  unsigned int nofFiles;
  if (!ReadUInt32(m_file, nofFiles))
    return false;

  for (uint32_t i = 0; i < nofFiles; i++)
  {
    CXBTFFile xbtfFile;
    uint32_t u32;
    uint64_t u64;

    // one extra char to null terminate the string with the following memset
    char path[CXBTFFile::MaximumPathLength + 1];
    memset(path, 0, sizeof(path));
    if (!ReadString(m_file, path, sizeof(path) - 1))
      return false;
    xbtfFile.SetPath(path);

    if (!ReadUInt32(m_file, u32))
      return false;
    xbtfFile.SetLoop(u32);

    unsigned int nofFrames;
    if (!ReadUInt32(m_file, nofFrames))
      return false;

    for (uint32_t j = 0; j < nofFrames; j++)
    {
      CXBTFFrame frame;

      if (!ReadUInt32(m_file, u32))
        return false;
      frame.SetWidth(u32);

      if (!ReadUInt32(m_file, u32))
        return false;
      frame.SetHeight(u32);

      if (!ReadUInt32(m_file, u32))
        return false;
      frame.SetFormat(u32);

      if (!ReadUInt64(m_file, u64))
        return false;
      frame.SetPackedSize(u64);

      if (!ReadUInt64(m_file, u64))
        return false;
      frame.SetUnpackedSize(u64);

      if (!ReadUInt32(m_file, u32))
        return false;
      frame.SetDuration(u32);

      if (!ReadUInt64(m_file, u64))
        return false;
      frame.SetOffset(u64);

      xbtfFile.GetFrames().push_back(frame);
    }

    AddFile(xbtfFile);
  }

  // Sanity check
  uint64_t pos = static_cast<uint64_t>(m_file->GetPosition());
  if (pos != GetHeaderSize())
    return false;

  return true;
}

bool CXBTFReader::IsOpen() const
{
  return m_file != nullptr;
}

void CXBTFReader::Close()
{
  if (m_file != nullptr)
  {
    m_file->Close();
    SAFE_DELETE(m_file);
  }

  m_path.clear();
  m_files.clear();
}

time_t CXBTFReader::GetLastModificationTimestamp() const
{
  if (m_file == nullptr)
    return 0;

  struct _stat64 fileStat;
  if (m_file->Stat(&fileStat) == -1)
    return 0;

  return fileStat.st_mtime;
}

bool CXBTFReader::Load(const CXBTFFrame& frame, unsigned char* buffer) const
{
  if (m_file == nullptr)
    return false;

  if (m_file->Seek(frame.GetOffset()) == -1)
    return false;

  if (m_file->Read(buffer, static_cast<size_t>(frame.GetPackedSize())) != frame.GetPackedSize())
    return false;

  return true;
}
