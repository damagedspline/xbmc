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
#pragma once

#include "filesystem/IFile.h"
#include <winrt/Windows.Storage.AccessCache.h>
#include <future>

namespace XFILE
{
  class CWinLibraryFile : public IFile
  {
  public:
    CWinLibraryFile();
    virtual ~CWinLibraryFile(void);

    bool Open(const CURL& url) override;
    bool OpenForWrite(const CURL& url, bool bOverWrite = false) override;
    void Close() override;

    ssize_t Read(void* lpBuf, size_t uiBufSize) override;
    ssize_t Write(const void* lpBuf, size_t uiBufSize) override;
    int64_t Seek(int64_t iFilePosition, int iWhence = SEEK_SET) override;
    int Truncate(int64_t toSize) override;
    int64_t GetPosition() override;
    int64_t GetLength() override;
    void Flush() override;

    bool Delete(const CURL& url) override;
    bool Rename(const CURL& urlCurrentName, const CURL& urlNewName) override;
    bool SetHidden(const CURL& url, bool hidden) override;
    bool Exists(const CURL& url) override;
    int Stat(const CURL& url, struct __stat64* statData) override;
    int Stat(struct __stat64* statData) override;

    static IFile* Get(const CURL& url);
    static bool IsValid(const CURL& url);

    static bool IsInAccessList(const CURL& url);

  private:
    template <typename T> using IAsync = winrt::Windows::Foundation::IAsyncOperation<T>;

    IAsync<winrt::Windows::Storage::StorageFile> GetFile(const CURL& url);
    std::future<bool> OpenAsync(const CURL& url, winrt::Windows::Storage::FileAccessMode mode);
    std::future<bool> RenameAsync(const CURL& urlCurrentName, const CURL& urlNewName);
    std::future<bool> DeleteAsync(const CURL& url);
    std::future<int> StatAsync(const CURL& url, struct __stat64* statData);
    std::future<int> StatAsync(const winrt::Windows::Storage::StorageFile& file, struct __stat64* statData);

    static bool IsInList(const CURL& url, const winrt::Windows::Storage::AccessCache::IStorageItemAccessList& list);
    static winrt::hstring GetTokenFromList(const CURL& url, const winrt::Windows::Storage::AccessCache::IStorageItemAccessList& list);

    bool m_allowWrite = false;
    winrt::Windows::Storage::StorageFile m_sFile = nullptr;
    winrt::Windows::Storage::Streams::IRandomAccessStream m_fileStream = nullptr;
  };
}