
#ifdef TARGET_WIN10

#include "WinLibraryDirectory.h"
#include "FileItem.h"
#include "platform/win32/CharsetConverter.h"
#include "URL.h"
#include "utils/log.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include <string>
#include <ppltasks.h>

using namespace XFILE;
using namespace KODI::PLATFORM::WINDOWS;
using namespace Windows::Storage;
using namespace Windows::Storage::Search;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;

bool CWinLibraryDirectory::GetStoragePath(std::string library, std::string & path)
{
  CURL url;
  url.SetProtocol("win-lib");
  url.SetHostName(library);

  if (!IsValid(url))
    return false;

  path = url.Get();
  return true;
}

StorageFolder^ CWinLibraryDirectory::GetRootFolder(const CURL& url)
{
  std::string protocol = url.GetProtocol();
  std::string lib = url.GetHostName();

  if (protocol != "win-lib")
    return nullptr;

  if (lib == "music")
    return KnownFolders::MusicLibrary;
  if (lib == "video")
    return KnownFolders::VideosLibrary;
  if (lib == "pictures")
    return KnownFolders::PicturesLibrary;
  if (lib == "photos")
    return KnownFolders::CameraRoll;
  if (lib == "documents")
    return KnownFolders::DocumentsLibrary;

  return nullptr;
}

bool CWinLibraryDirectory::IsValid(const CURL & url)
{
  std::string protocol = url.GetProtocol();
  std::string lib = url.GetHostName();

  if (protocol != "win-lib")
    return false;

  if ( lib == "music"
    || lib == "video"
    || lib == "pictures"
    || lib == "photos"
    || lib == "documents")
    return true;
  else
    return false;
}

CWinLibraryDirectory::CWinLibraryDirectory()
{
}

CWinLibraryDirectory::~CWinLibraryDirectory(void)
{
}

bool CWinLibraryDirectory::GetDirectory(const CURL &url, CFileItemList &items)
{
  items.Clear();

  // We accept win-lib://library/path[/]

  auto libname = url.GetHostName();
  std::string path(url.Get());
  URIUtils::AddSlashAtEnd(path); //be sure the dir ends with a slash

  GetFolder(url)
  .then([](StorageFolder^ folder) -> IAsyncOperation<IVectorView<IStorageItem^>^>^
  {
    if (folder)
      return folder->GetItemsAsync();

    return Concurrency::create_async([]() -> IVectorView<IStorageItem^>^ { return nullptr; });
  })
  .then([&items, &path](IVectorView<IStorageItem^>^ vec)
  {
    if (!vec)
      return;

    for (int i = 0; i < vec->Size; i++)
    {
      IStorageItem^ item = vec->GetAt(i);
      std::string itemName = FromW(std::wstring(item->Name->Data()));

      CFileItemPtr pItem(new CFileItem(itemName));
      pItem->m_bIsFolder = (item->Attributes & FileAttributes::Directory) == FileAttributes::Directory;

      if (pItem->m_bIsFolder)
        pItem->SetPath(path + itemName + "/");
      else
        pItem->SetPath(path + itemName);

      if (itemName.front() == '.')
        pItem->SetProperty("file:hidden", true);

      concurrency::create_task(item->GetBasicPropertiesAsync()).then([&](FileProperties::BasicProperties^ props)
      {
        ULARGE_INTEGER ularge = { props->DateModified.UniversalTime };
        FILETIME localTime = { ularge.LowPart, ularge.HighPart };
        pItem->m_dateTime = localTime;
        if (!pItem->m_bIsFolder)
          pItem->m_dwSize = props->Size;
      }).wait();

      items.Add(pItem);
    }
  })
  .wait();

  return true;
}

bool CWinLibraryDirectory::Create(const CURL& url)
{
  // TODO implement
  std::string folderPath = URIUtils::FixSlashesAndDups(url.GetFileName(), '\\');
  std::wstring wStrPath = ToW(folderPath);

  StorageFolder^ rootFolder = GetRootFolder(url);
  Concurrency::create_task(rootFolder->CreateFolderAsync(ref new Platform::String(wStrPath.c_str()))).wait();

  return true;
}

bool CWinLibraryDirectory::Exists(const CURL& url)
{
  bool exists = false;
  GetFolder(url)
  .then([&exists](StorageFolder^ folder)
  {
    exists = folder != nullptr;
  })
  .wait();
  return exists;
}

bool CWinLibraryDirectory::Remove(const CURL& url)
{
  bool exists = false;
  GetFolder(url)
  .then([&exists, &url](StorageFolder^ folder)
  {
    if (!folder)
    {
      exists = false;
      return;
    }

    try
    {
      Concurrency::create_task(folder->DeleteAsync()).wait();
      exists = true;
    }
    catch(Platform::Exception^ ex)
    {
      std::string error = FromW(std::wstring(ex->Message->Data()));
      CLog::Log(LOGERROR, "CWinLibraryDirectory::Remove: unable remove folder '%s' with error", url.Get(), error.c_str());
      exists = false;
    }
  })
  .wait();
  return exists;
}

Concurrency::task<StorageFolder^> CWinLibraryDirectory::GetFolder(const CURL& url)
{
  std::string folderPath = URIUtils::FixSlashesAndDups(url.GetFileName(), '\\');
  StorageFolder^ rootFolder = GetRootFolder(url);

  // find inner folder
  if (!folderPath.empty())
  {
    std::wstring wStrPath = ToW(folderPath);
    try
    {
      Platform::String^ pPath = ref new Platform::String(wStrPath.c_str());
      return Concurrency::create_task(rootFolder->GetFolderAsync(pPath));
    }
    catch (Platform::Exception^ ex)
    {
      std::string error = FromW(std::wstring(ex->Message->Data()));
      CLog::Log(LOGERROR, "CWinLibraryDirectory::GetFolder: unable to get folder '%s' with error", folderPath, error.c_str());
    }
    return Concurrency::create_task([]() -> StorageFolder^ { return nullptr; });
  }

  return Concurrency::create_task([rootFolder]() { return rootFolder; });
}

#endif