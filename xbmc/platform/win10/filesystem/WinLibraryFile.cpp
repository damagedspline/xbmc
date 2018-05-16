/*
 *      Copyright (C) 2011-2013 Team XBMC
 *      http://kodi.tv
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

#include "WinLibraryFile.h"
#include "WinLibraryDirectory.h"
#include "platform/win10/AsyncHelpers.h"
#include "platform/win32/CharsetConverter.h"
#include "platform/win32/WIN32Util.h"
#include "utils/log.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "URL.h"

#include <string>
#include <robuffer.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Security.Cryptography.h>
#include <winrt/Windows.Storage.AccessCache.h>
#include <winrt/Windows.Storage.FileProperties.h>
#include <winrt/Windows.Storage.Search.h>
#include <winrt/Windows.Storage.Streams.h>

using namespace XFILE;
using namespace KODI::PLATFORM::WINDOWS;
namespace winrt 
{
  using namespace Windows::Foundation;
}
using namespace winrt::Windows::Foundation::Collections;
using namespace winrt::Windows::Security::Cryptography;
using namespace winrt::Windows::Storage;
using namespace winrt::Windows::Storage::AccessCache;
using namespace winrt::Windows::Storage::Search;
using namespace winrt::Windows::Storage::Streams;

#define WINRT_STA_ASYNC_GUARD { if (winrt::impl::is_sta()) co_await winrt::resume_background(); }

struct __declspec(uuid("905a0fef-bc53-11df-8c49-001e4fc686da")) IBufferByteAccess : ::IUnknown
{
  virtual HRESULT __stdcall Buffer(void** value) = 0;
};

struct CustomBuffer : winrt::implements<CustomBuffer, IBuffer, IBufferByteAccess>
{
  void *m_address;
  uint32_t m_capacity;
  uint32_t m_length;

  CustomBuffer(void *address, uint32_t capacity) : m_address(address), m_capacity(capacity), m_length(0) { }
  uint32_t Capacity() const { return m_capacity; }
  uint32_t Length() const { return m_length; }
  void Length(uint32_t length) { m_length = length; }

  HRESULT __stdcall Buffer(void** value) final
  {
    *value = m_address;
    return S_OK;
  }
};

CWinLibraryFile::CWinLibraryFile() = default;
CWinLibraryFile::~CWinLibraryFile(void) = default;

bool CWinLibraryFile::IsValid(const CURL & url)
{
  return CWinLibraryDirectory::IsValid(url)
    && !url.GetFileName().empty()
    && !URIUtils::HasSlashAtEnd(url.GetFileName(), false);
}

bool CWinLibraryFile::Open(const CURL& url)
{
  return OpenAsync(url, FileAccessMode::Read).get();
}

bool CWinLibraryFile::OpenForWrite(const CURL& url, bool bOverWrite)
{
  return OpenAsync(url, FileAccessMode::ReadWrite).get();
}

void CWinLibraryFile::Close()
{
  if (m_fileStream)
  {
    m_fileStream.Close();
    m_fileStream = nullptr;
  }
  if (m_sFile)
    m_sFile = nullptr;
}

ssize_t CWinLibraryFile::Read(void* lpBuf, size_t uiBufSize)
{
  if (!m_fileStream)
    return -1;

  IBuffer buf = winrt::make<CustomBuffer>(lpBuf, static_cast<uint32_t>(uiBufSize));
  Wait(m_fileStream.ReadAsync(buf, buf.Capacity(), InputStreamOptions::None));

  return static_cast<intptr_t>(buf.Length());
}

ssize_t CWinLibraryFile::Write(const void* lpBuf, size_t uiBufSize)
{
  if (!m_fileStream || !m_allowWrite)
    return -1;

  const uint8_t* buff = static_cast<const uint8_t*>(lpBuf);
  auto winrt_buffer = CryptographicBuffer::CreateFromByteArray({ buff, buff + uiBufSize });

  uint32_t result = Wait(m_fileStream.WriteAsync(winrt_buffer));
  return static_cast<intptr_t>(result);
}

int64_t CWinLibraryFile::Seek(int64_t iFilePosition, int iWhence)
{
  if (m_fileStream != nullptr)
  {
    int64_t pos = iFilePosition;
    if (iWhence == SEEK_CUR)
      pos += m_fileStream.Position();
    else if (iWhence == SEEK_END)
      pos += m_fileStream.Size();

    uint64_t seekTo;
    if (pos < 0)
      seekTo = 0;
    else if (static_cast<uint64_t>(pos) > m_fileStream.Size())
      seekTo = m_fileStream.Size();
    else
      seekTo = static_cast<uint64_t>(pos);

    m_fileStream.Seek(seekTo);
    return GetPosition();
  }
  return -1;
}

int CWinLibraryFile::Truncate(int64_t toSize)
{
  // not allowed
  return -1;
}

int64_t CWinLibraryFile::GetPosition()
{
  if (m_fileStream != nullptr)
    return static_cast<int64_t>(m_fileStream.Position());

  return -1;
}

int64_t CWinLibraryFile::GetLength()
{
  if (m_fileStream != nullptr)
    return m_fileStream.Size();

  return 0;
}

void CWinLibraryFile::Flush()
{
  if (m_fileStream != nullptr)
    m_fileStream.FlushAsync();
}

bool CWinLibraryFile::Delete(const CURL & url)
{
  return DeleteAsync(url).get();
}

bool CWinLibraryFile::Rename(const CURL & urlCurrentName, const CURL & urlNewName)
{
  if (!IsValid(urlNewName))
    return false;

  return RenameAsync(urlCurrentName, urlNewName).get();
}

bool CWinLibraryFile::SetHidden(const CURL& url, bool hidden)
{
  return false;
}

bool CWinLibraryFile::Exists(const CURL& url)
{
  return Wait(GetFile(url)) != nullptr;
}

int CWinLibraryFile::Stat(const CURL& url, struct __stat64* statData)
{
  return StatAsync(url, statData).get();
}

int CWinLibraryFile::Stat(struct __stat64* statData)
{
  return StatAsync(m_sFile, statData).get();
}

bool CWinLibraryFile::IsInAccessList(const CURL& url)
{
  // skip local folder and installation folder
  using KODI::PLATFORM::WINDOWS::FromW;

  auto localFolder = winrt::Windows::Storage::ApplicationData::Current().LocalFolder();
  std::string path = FromW(localFolder.Path().c_str());
  if (StringUtils::StartsWithNoCase(url.Get(), path))
    return false;

  auto appFolder = winrt::Windows::ApplicationModel::Package::Current().InstalledLocation();
  path = FromW(appFolder.Path().c_str());
  if (StringUtils::StartsWithNoCase(url.Get(), path))
    return false;

  return IsInList(url, StorageApplicationPermissions::FutureAccessList())
      || IsInList(url, StorageApplicationPermissions::MostRecentlyUsedList());
}


bool CWinLibraryFile::IsInList(const CURL& url, const IStorageItemAccessList& list)
{
  auto token = GetTokenFromList(url, list);
  return token != nullptr && !token.empty();
}

winrt::hstring CWinLibraryFile::GetTokenFromList(const CURL& url, const IStorageItemAccessList& list)
{
  AccessListEntryView listview = list.Entries();
  if (listview.Size() == 0)
    return nullptr;

  using KODI::PLATFORM::WINDOWS::ToW;
  std::string filePath = url.Get();
  std::wstring filePathW = ToW(filePath);

  for (uint32_t i = 0; i < listview.Size(); i++)
  {
    auto listEntry = listview.GetAt(i);
    if (listEntry.Metadata == filePathW)
    {
      return listEntry.Token;
    }
  }

  return nullptr;
}

std::future<bool> CWinLibraryFile::OpenAsync(const CURL &url, FileAccessMode mode)
{
  WINRT_STA_ASYNC_GUARD;

  try
  {
    if (mode == FileAccessMode::Read)
    {
      m_sFile = co_await GetFile(url);
    }
    else if (mode == FileAccessMode::ReadWrite)
    { 
      auto destFolder = CURL(URIUtils::GetParentPath(url.Get()));
      auto folder = CWinLibraryDirectory::GetFolder(destFolder);
      if (folder)
      {
        std::wstring fileNameW = ToW(url.GetFileNameWithoutPath());
        m_sFile = co_await folder.CreateFileAsync(fileNameW, CreationCollisionOption::ReplaceExisting);
        if (m_sFile)
          m_allowWrite = true;
      }
    }

    if (m_sFile)
      m_fileStream = co_await m_sFile.OpenAsync(mode);
  }
  catch (const winrt::hresult_error& ex)
  {
    std::string error = FromW(ex.message().c_str());
    CLog::LogF(LOGERROR, "an exception occurs while openning a file '%s' (mode: %s) : %s"
                       , url.GetRedacted().c_str()
                       , mode == FileAccessMode::Read ? "r" : "rw"
                       , error.c_str());
    return false;
  }

  return m_fileStream != nullptr;
}

winrt::IAsyncOperation<StorageFile> CWinLibraryFile::GetFile(const CURL& url)
{
  WINRT_STA_ASYNC_GUARD;

  // check that url is library url
  if (CWinLibraryDirectory::IsValid(url))
  {
    StorageFolder rootFolder = CWinLibraryDirectory::GetRootFolder(url);
    if (rootFolder == nullptr)
      return nullptr;

    std::string filePath = URIUtils::FixSlashesAndDups(url.GetFileName(), '\\');
    if (url.GetHostName() == "removable")
    {
      // here filePath has the form e\path\file.ext where first segment is 
      // drive letter, we should make path form like regular e:\path\file.ext
      auto index = filePath.find('\\');
      if (index == std::string::npos)
      {
        CLog::LogF(LOGDEBUG, "wrong file path '%s'", url.GetRedacted().c_str());
        return nullptr;
      }

      filePath = filePath.insert(index, 1, ':');
    }

    try
    {
      std::wstring wpath = ToW(filePath);
      auto item = co_await rootFolder.TryGetItemAsync(wpath);

      if (item && item.IsOfType(StorageItemTypes::File))
        return item.as<StorageFile>();

      return nullptr;
    }
    catch (const winrt::hresult_error& ex)
    {
      std::string error = FromW(ex.message().c_str());
      CLog::LogF(LOGERROR, "unable to get file '%s' with error %s"
                         , url.GetRedacted().c_str()
                         , error.c_str());
    }
  }
  else if (url.GetProtocol() == "file" || url.GetProtocol().empty())
  {
    // check that a file in feature access list or most rescently used list
    // search in FAL
    IStorageItemAccessList list = StorageApplicationPermissions::FutureAccessList();
    winrt::hstring token = GetTokenFromList(url, list);
    if (token.empty())
    {
      // serach in MRU list
      IStorageItemAccessList list = StorageApplicationPermissions::MostRecentlyUsedList();
      token = GetTokenFromList(url, list);
    }
    if (token.empty())
      return co_await list.GetFileAsync(token);
  }

  return nullptr;
}

std::future<bool> CWinLibraryFile::DeleteAsync(const CURL & url)
{
  WINRT_STA_ASYNC_GUARD;

  bool success = false;
  auto file = co_await GetFile(url);
  if (file)
  {
    try
    {
      co_await file.DeleteAsync();
    }
    catch (const winrt::hresult_error&)
    {
      return false;
    }
    return true;
  }
  return false;
}

std::future<bool> CWinLibraryFile::RenameAsync(const CURL & urlCurrentName, const CURL & urlNewName)
{
  WINRT_STA_ASYNC_GUARD;

  auto currFile = co_await GetFile(urlCurrentName);
  if (currFile)
  {
    auto destFile = co_await GetFile(urlNewName);
    if (destFile)
    {
      // replace exiting
      try
      {
        co_await currFile.MoveAndReplaceAsync(destFile);
      }
      catch (const winrt::hresult_error&)
      {
        return false;
      }
      return true;
    }

    // move
    CURL defFolder = CURL(urlNewName.GetWithoutFilename());
    StorageFolder destFolder = CWinLibraryDirectory::GetFolder(defFolder);
    if (destFolder != nullptr)
    {
      try
      {
        co_await currFile.MoveAsync(destFolder);
      }
      catch (const winrt::hresult_error&)
      {
        return false;
      }
      return true;
    }
  }
  return false;
}

std::future<int> CWinLibraryFile::StatAsync(const CURL& url, struct __stat64* statData)
{
  WINRT_STA_ASYNC_GUARD;

  auto file = co_await GetFile(url);
  if (!file)
    return -1;

  return co_await StatAsync(file, statData);
}

std::future<int> CWinLibraryFile::StatAsync(const StorageFile& file, struct __stat64* statData)
{
  WINRT_STA_ASYNC_GUARD;

  if (!statData)
    return -1;

  /* set st_gid */
  statData->st_gid = 0; // UNIX group ID is always zero on Win32
  /* set st_uid */
  statData->st_uid = 0; // UNIX user ID is always zero on Win32
  /* set st_ino */
  statData->st_ino = 0; // inode number is not implemented on Win32

  auto requestedProps = co_await file.Properties().RetrievePropertiesAsync({
    L"System.DateAccessed",
    L"System.DateCreated",
    L"System.DateModified",
    L"System.Size"
  });

  auto dateAccessed = requestedProps.Lookup(L"System.DateAccessed").as<winrt::IPropertyValue>();
  if (dateAccessed)
  {
    statData->st_atime = winrt::clock::to_time_t(dateAccessed.GetDateTime());
  }
  auto dateCreated = requestedProps.Lookup(L"System.DateCreated").as<winrt::IPropertyValue>();
  if (dateCreated)
  {
    statData->st_ctime = winrt::clock::to_time_t(dateCreated.GetDateTime());
  }
  auto dateModified = requestedProps.Lookup(L"System.DateModified").as<winrt::IPropertyValue>();
  if (dateModified)
  {
    statData->st_mtime = winrt::clock::to_time_t(dateModified.GetDateTime());
  }
  auto fileSize = requestedProps.Lookup(L"System.Size").as<winrt::IPropertyValue>();
  if (fileSize)
  {
    /* set st_size */
    statData->st_size = fileSize.GetInt64();
  }

  statData->st_dev = 0;
  statData->st_rdev = statData->st_dev;
  /* set st_nlink */
  statData->st_nlink = 1;
  /* set st_mode */
  statData->st_mode = _S_IREAD; // only read permission for file from library
  // copy user RWX rights to group rights
  statData->st_mode |= (statData->st_mode & (_S_IREAD | _S_IWRITE | _S_IEXEC)) >> 3;
  // copy user RWX rights to other rights
  statData->st_mode |= (statData->st_mode & (_S_IREAD | _S_IWRITE | _S_IEXEC)) >> 6;

  return 0;
}
