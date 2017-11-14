/*
*      Copyright (C) 2014 Team XBMC
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

#ifdef TARGET_WINDOWS
#include "Win10SMBFile.h"
#include "Win10SMBDirectory.h"
#include "URL.h"
#include "platform/win32/WIN32Util.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN 1
#endif // WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <cassert>

using namespace XFILE;


bool CWinSMBFile::ConnectAndAuthenticate(const CURL& url)
{
  CURL authUrl(url);
  CWinSMBDirectory smbDir;
  return smbDir.ConnectAndAuthenticate(authUrl, false);
}

CWinSMBFile::CWinSMBFile() : CWinLibraryFile()
{ }

CWinSMBFile::~CWinSMBFile()
{ /* cleanup by CWin32File destructor */ }

bool CWinSMBFile::Open(const CURL& url)
{
  assert(url.IsProtocol("smb")); // function suitable only for SMB files
  if (CWinLibraryFile::Open(url))
    return true;

  return false;
}

bool CWinSMBFile::OpenForWrite(const CURL& url, bool bOverWrite /*= false*/)
{
  assert(url.IsProtocol("smb")); // function suitable only for SMB files
  if (CWinLibraryFile::OpenForWrite(url, bOverWrite))
    return true;

  return false;
}

bool CWinSMBFile::Delete(const CURL& url)
{
  assert(url.IsProtocol("smb")); // function suitable only for SMB files

  if (CWinLibraryFile::Delete(url))
    return true;

  return false;
}

bool CWinSMBFile::Rename(const CURL& urlCurrentName, const CURL& urlNewName)
{
  assert(urlCurrentName.IsProtocol("smb")); // function suitable only for SMB files
  assert(urlNewName.IsProtocol("smb")); // function suitable only for SMB files

  if (CWinLibraryFile::Rename(urlCurrentName, urlNewName))
    return true;

  return false;
}

bool CWinSMBFile::SetHidden(const CURL& url, bool hidden)
{
  assert(url.IsProtocol("smb")); // function suitable only for SMB files

  return false;
}

bool CWinSMBFile::Exists(const CURL& url)
{
  assert(url.IsProtocol("smb")); // function suitable only for SMB files
  std::wstring pathnameW(CWIN32Util::ConvertPathToWin32Form(url));
  if (pathnameW.empty())
    return false;

    return false;
}

int CWinSMBFile::Stat(const CURL& url, struct __stat64* statData)
{
  assert(url.IsProtocol("smb")); // function suitable only for SMB files

  if (CWinLibraryFile::Stat(url, statData) == 0)
    return 0;

  return false;
}


#endif // TARGET_WINDOWS
