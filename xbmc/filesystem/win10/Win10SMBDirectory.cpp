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

#include "Win10SMBDirectory.h"
#include "FileItem.h"
#include "platform/win32/WIN32Util.h"
#include "utils/SystemInfo.h"
#include "utils/CharsetConverter.h"
#include "URL.h"
#include "utils/win32/Win32Log.h"
#include "PasswordManager.h"
#include "utils/auto_buffer.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN 1
#endif // WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <cassert>

using namespace XFILE;

CWinSMBDirectory::CWinSMBDirectory(void)
{}

CWinSMBDirectory::~CWinSMBDirectory(void)
{}

bool CWinSMBDirectory::GetDirectory(const CURL& url, CFileItemList &items)
{
  assert(url.IsProtocol("smb"));
  items.Clear();

  return false;
}

bool CWinSMBDirectory::Create(const CURL& url)
{
  return RealCreate(url, true);
}

bool CWinSMBDirectory::RealCreate(const CURL& url, bool tryToConnect)
{
  assert(url.IsProtocol("smb"));
  if (url.GetHostName().empty() || url.GetShareName().empty() || url.GetFileName() == url.GetShareName())
    return false; // can't create new hosts or shares

  return false;
}

bool CWinSMBDirectory::Exists(const CURL& url)
{
  return RealExists(url, true);
}

// this functions can check for: 
// * presence of directory on remove share (smb://server/share/dir)
// * presence of remote share on server (smb://server/share)
// * presence of smb server in network (smb://server)
bool CWinSMBDirectory::RealExists(const CURL& url, bool tryToConnect)
{
  assert(url.IsProtocol("smb"));

  if (url.GetHostName().empty())
    return true; // 'root' of network is always exist
    
  return false;
}

bool CWinSMBDirectory::Remove(const CURL& url)
{
  assert(url.IsProtocol("smb"));
  return !RealExists(url, false);
}

bool CWinSMBDirectory::ConnectAndAuthenticate(CURL& url, bool allowPromptForCredential /*= false*/)
{
  assert(url.IsProtocol("smb"));
  if (url.GetHostName().empty())
    return false; // can't connect to empty host name
  
  return false;
}
