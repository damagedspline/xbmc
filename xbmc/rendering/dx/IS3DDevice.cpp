/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://www.xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#ifdef HAS_DX

#include <windows.h>
#include <comdef.h>
#include "IS3DDevice.h"
#include "CIntelS3DDevice.h"
#include "CNvidiaS3DDevice.h"
#include "CAmdS3DDevice.h"
#include "utils/log.h"

typedef enum _S3D_ADAPTER_VENDOR
{
  ADAPTER_VENDOR_INTEL   = 0x8086,
  ADAPTER_VENDOR_NVIDIA  = 0x10DE,
  ADAPTER_VENDOR_AMD     = 0x1002,
  ADAPTER_VENDOR_UNKNOWN = 0x0000,

} S3D_ADAPTER_VENDOR;

// TODO needs to check ids
static DWORD IntelDeviceID [] = {
 0x0102, // 2nd Generation Core Processor Family Integrated Graphics Controller
 0x0112, // 2nd Generation Core Processor Family Integrated Graphics Controller
 0x0116, // 2nd Generation Core Processor Family Integrated Graphics Controller
 0x0122, // 2nd Generation Core Processor Family Integrated Graphics Controller
 0x0126, // 2nd Generation Core Processor Family Integrated Graphics Controller
 0x0162, // 3rd Gen Core processor Graphics Controller
 0x0166, // 3rd Gen Core processor Graphics Controller
 0x0406, // 4th Gen Core Processor Integrated Graphics Controller
 0x0416, // 4th Gen Core Processor Integrated Graphics Controller
 0x0A06, // Haswell-ULT Integrated Graphics Controller
 0x0A16, // Haswell-ULT Integrated Graphics Controller
 0x0A22, // Haswell-ULT Integrated Graphics Controller
 0x0A26, // Haswell-ULT Integrated Graphics Controller
 0x0A2A, // Haswell-ULT Integrated Graphics Controller
 0x0000
};

static DWORD NvidiaDeviceID [] = {
 0x0000
};

static DWORD AmdDeviceID [] = {
 0x0000
};

static bool CheckDeviceID(DWORD * deviceList, DWORD deviceId)
{
  for (unsigned idx = 0; deviceList[idx] != 0; idx++)
  {
    if (deviceList[idx] == deviceId)
      return true;
  }
  return false;
}


IS3DDevice* IS3DDevice::CreateDevice(IDirect3D9Ex* pD3D, UINT uAdapterID)
{
  CLog::Log(LOGDEBUG, __FUNCTION__" - Trying create S3D device on adapter:%d", uAdapterID);

  IS3DDevice* device = new IS3DDevice(pD3D);

  if (uAdapterID != (UINT)-1)
  {
    D3DADAPTER_IDENTIFIER9 aIdentifier;
    if(pD3D->GetAdapterIdentifier(uAdapterID, 0, &aIdentifier) == D3D_OK)
    {
      switch (aIdentifier.VendorId)
      {
        case ADAPTER_VENDOR_INTEL:
          // while deviceId list is not complete just intel's method will checks support
          //if (CheckDeviceID(IntelDeviceID, aIdentifier.DeviceId))
          {
            CLog::Log(LOGDEBUG, __FUNCTION__" - Create Intel S3D device on adapter:%d", uAdapterID);
            device = new CIntelS3DDevice(pD3D);
          }
          break;
        case ADAPTER_VENDOR_NVIDIA:
          {
            CLog::Log(LOGDEBUG, __FUNCTION__" - Create Nvidia 3D Vision device on adapter:%d", uAdapterID);
            device = new CNvidiaS3DDevice(pD3D);
          }
          break;
        case ADAPTER_VENDOR_AMD:
          {
            CLog::Log(LOGDEBUG, __FUNCTION__" - Create AMD 3DHD device on adapter:%d", uAdapterID);
            device = new CAmdS3DDevice(pD3D);
          }
          break;
        default:
            CLog::Log(LOGDEBUG, __FUNCTION__" - Create generic S3D device on adapter:%d", uAdapterID);
      }
    }
  }

  return device;
}

#endif
