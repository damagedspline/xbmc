/*
 *      Copyright (C) 2005-2013 Team XBMC
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

#include "NetworkWin10.h"
#include "filesystem/SpecialProtocol.h"
#include "platform/win32/WIN32Util.h"
#include "platform/win32/CharsetConverter.h"
#include "settings/Settings.h"
#include "threads/SingleLock.h"
#include "utils/log.h"
#include "utils/StringUtils.h"

#include <collection.h>
#include <errno.h>
#include <iphlpapi.h>
#include <string.h>
#include <Ws2tcpip.h>
#include <ws2ipdef.h>

using namespace Windows::Networking;
using namespace Windows::Networking::Connectivity;
using namespace KODI::PLATFORM::WINDOWS;

std::string GetAddressIp(struct sockaddr* sa)
{
  std::string strIp = "";

  char buffer[INET6_ADDRSTRLEN] = { 0 };
  switch (sa->sa_family)
  {
  case AF_INET:
    inet_ntop(AF_INET, &(reinterpret_cast<const struct sockaddr_in*>(sa)->sin_addr), buffer, INET_ADDRSTRLEN);
    break;
  case AF_INET6:
    inet_ntop(AF_INET6, &(reinterpret_cast<const struct sockaddr_in6*>(sa)->sin6_addr), buffer, INET6_ADDRSTRLEN);
    break;
  }

  strIp = buffer;

  return strIp;
}

std::string GetMaskByPrefix(uint8_t prefix)
{
  std::string result = "";

  if (prefix > 128) // invalid mask
    return result;

  if (prefix > 32) // IPv6
  {
    struct sockaddr_in6 sa;
    sa.sin6_family = AF_INET6;
    int i, j;

    memset(&sa.sin6_addr, 0x0, sizeof(sa.sin6_addr));
    for (i = prefix, j = 0; i > 0; i -= 8, j++) 
    {
      if (i >= 8)
        sa.sin6_addr.s6_addr[j] = 0xff;
      else
        sa.sin6_addr.s6_addr[j] = (unsigned long)(0xffU << (8 - i));
    }
    result = GetAddressIp(reinterpret_cast<struct sockaddr*>(&sa));
  }
  else // IPv4
  {
    struct sockaddr_in sa;
    sa.sin_family = AF_INET;
    sa.sin_addr.s_addr = htonl(~((1 << (32 - prefix)) - 1));;
    result = GetAddressIp(reinterpret_cast<struct sockaddr*>(&sa));
  }

  return result;
}

CNetworkInterfaceWin10::CNetworkInterfaceWin10(CNetworkWin10* network, Windows::Networking::Connectivity::ConnectionProfile^ profile)
{
  m_network = network;
  m_adapter = profile;
  m_adaptername = FromW(profile->ProfileName->Data());
}

CNetworkInterfaceWin10::CNetworkInterfaceWin10(CNetworkWin10 * network, const PIP_ADAPTER_ADDRESSES address)
{
  m_network = network;
  m_adapterAddr = address;
  m_adaptername = address->AdapterName;
}

CNetworkInterfaceWin10::~CNetworkInterfaceWin10(void)
{
  m_adapter = nullptr;
}

std::string& CNetworkInterfaceWin10::GetName(void)
{
  return m_adaptername;
}

bool CNetworkInterfaceWin10::IsWireless()
{
  //WlanConnectionProfileDetails^ wlanConnectionProfileDetails = m_adapter->WlanConnectionProfileDetails;
  //return wlanConnectionProfileDetails != nullptr;
  return m_adapterAddr->IfType == IF_TYPE_IEEE80211;
}

bool CNetworkInterfaceWin10::IsEnabled()
{
  return true;
}

bool CNetworkInterfaceWin10::IsConnected()
{
  //return m_adapter->GetNetworkConnectivityLevel() != NetworkConnectivityLevel::None;
  return m_adapterAddr->OperStatus == IF_OPER_STATUS::IfOperStatusUp;
}

std::string CNetworkInterfaceWin10::GetMacAddress()
{
  std::string result;
  unsigned char* mAddr = m_adapterAddr->PhysicalAddress;
  result = StringUtils::Format("%02X:%02X:%02X:%02X:%02X:%02X", mAddr[0], mAddr[1], mAddr[2], mAddr[3], mAddr[4], mAddr[5]);
  return result;
}

void CNetworkInterfaceWin10::GetMacAddressRaw(char rawMac[6])
{
  memcpy(rawMac, m_adapterAddr->PhysicalAddress, 6);
}

bool CNetworkInterfaceWin10::GetHostMacAddress(unsigned long host, std::string& mac)
{
  mac = "";
  return false;
}

void CNetworkInterfaceWin10::GetSettings(NetworkAssignment& assignment, std::string& ipAddress, std::string& networkMask, std::string& defaultGateway, std::string& essId, std::string& key, EncMode& encryptionMode)
{
}

void CNetworkInterfaceWin10::SetSettings(NetworkAssignment& assignment, std::string& ipAddress, std::string& networkMask, std::string& defaultGateway, std::string& essId, std::string& key, EncMode& encryptionMode)
{
}

std::vector<NetworkAccessPoint> CNetworkInterfaceWin10::GetAccessPoints(void)
{
  std::vector<NetworkAccessPoint> accessPoints;
  return accessPoints;
}

std::string CNetworkInterfaceWin10::GetCurrentIPAddress(void)
{
  std::string result;

  /*Platform::String^ ipAddress = L"0.0.0.0";
  if (m_adapter->NetworkAdapter != nullptr)
  {
    auto  hostnames = NetworkInformation::GetHostNames();
    for (unsigned int i = 0; i < hostnames->Size; ++i)
    {
      auto hostname = hostnames->GetAt(i);
      if (hostname->Type != HostNameType::Ipv4)
      {
        continue;
      }

      if (hostname->IPInformation != nullptr && hostname->IPInformation->NetworkAdapter != nullptr)
      {
        if (hostname->IPInformation->NetworkAdapter->NetworkAdapterId == m_adapter->NetworkAdapter->NetworkAdapterId)
        {
          ipAddress = hostname->CanonicalName;
          break;
        }
      }
    }
  }

  result = FromW(ipAddress->Data());*/
  PIP_ADAPTER_UNICAST_ADDRESS_LH address = m_adapterAddr->FirstUnicastAddress;
  while (address)
  {
    if (address->Address.lpSockaddr->sa_family == AF_INET)
    {
      result = GetAddressIp(address->Address.lpSockaddr);
      break;
    }
    address = address->Next;
  }

  return result;
}

std::string CNetworkInterfaceWin10::GetCurrentNetmask(void)
{
  std::string result = "255.255.255.255";

  /*if (m_adapter->NetworkAdapter != nullptr)
  {
    auto  hostnames = NetworkInformation::GetHostNames();
    for (unsigned int i = 0; i < hostnames->Size; ++i)
    {
      auto hostname = hostnames->GetAt(i);
      if (hostname->Type != HostNameType::Ipv4)
      {
        continue;
      }

      if (hostname->IPInformation != nullptr && hostname->IPInformation->NetworkAdapter != nullptr)
      {
        if (hostname->IPInformation->NetworkAdapter->NetworkAdapterId == m_adapter->NetworkAdapter->NetworkAdapterId)
        {
          byte prefixLength = hostname->IPInformation->PrefixLength->Value;
          uint32_t mask = 0xFFFFFFFF << (32 - prefixLength);
          result = StringUtils::Format("%u.%u.%u.%u"
                                     , ((mask & 0xFF000000) >> 24)
                                     , ((mask & 0x00FF0000) >> 16)
                                     , ((mask & 0x0000FF00) >> 8)
                                     ,  (mask & 0x000000FF));
          break;
        }
      }
    }
  }*/

  PIP_ADAPTER_UNICAST_ADDRESS_LH address = m_adapterAddr->FirstUnicastAddress;
  while (address)
  {
    if (address->Address.lpSockaddr->sa_family == AF_INET)
    {
      result = GetMaskByPrefix(address->OnLinkPrefixLength);
      break;
    }
    address = address->Next;
  }

  return result;
}

std::string CNetworkInterfaceWin10::GetCurrentWirelessEssId(void)
{
  std::string result = "";
  if (!IsWireless())
    return result;

  //result = FromW(m_adapter->WlanConnectionProfileDetails->GetConnectedSsid()->Data());

  return result;
}

std::string CNetworkInterfaceWin10::GetCurrentDefaultGateway(void)
{
  std::string result = "0.0.0.0";

  PIP_ADAPTER_GATEWAY_ADDRESS_LH address = m_adapterAddr->FirstGatewayAddress;
  while (address)
  {
    if (address->Address.lpSockaddr->sa_family == AF_INET)
    {
      result = GetAddressIp(address->Address.lpSockaddr);
      break;
    }
    address = address->Next;
  }

  return result;
}

CNetworkWin10::CNetworkWin10(CSettings &settings)
  : CNetwork(settings)
  , m_adapterAddresses(nullptr)
{
  queryInterfaceList();
  NetworkInformation::NetworkStatusChanged += ref new NetworkStatusChangedEventHandler([this](Platform::Object^) {
    CSingleLock lock(m_critSection);
    queryInterfaceList();
  });
}

CNetworkWin10::~CNetworkWin10(void)
{
  CleanInterfaceList();
}

void CNetworkWin10::CleanInterfaceList()
{
  std::vector<CNetworkInterface*>::iterator it = m_interfaces.begin();
  while(it != m_interfaces.end())
  {
    CNetworkInterface* nInt = *it;
    delete nInt;
    it = m_interfaces.erase(it);
  }
  free(m_adapterAddresses);
  m_adapterAddresses = nullptr;
}

std::vector<CNetworkInterface*>& CNetworkWin10::GetInterfaceList(void)
{
  CSingleLock lock (m_critSection);
  return m_interfaces;
}

void CNetworkWin10::queryInterfaceList()
{
  CleanInterfaceList();

  /*auto connectionProfiles = NetworkInformation::GetConnectionProfiles();
  std::for_each(begin(connectionProfiles), end(connectionProfiles), [this](ConnectionProfile^ connectionProfile)
  {
    if (connectionProfile != nullptr && connectionProfile->GetNetworkConnectivityLevel() != NetworkConnectivityLevel::None)
    {
      m_interfaces.push_back(new CNetworkInterfaceWin10(this, connectionProfile));
    }
  });*/

  const ULONG flags = GAA_FLAG_INCLUDE_GATEWAYS | GAA_FLAG_INCLUDE_PREFIX;
  ULONG ulOutBufLen;

  if (GetAdaptersAddresses(AF_UNSPEC, flags, nullptr, nullptr, &ulOutBufLen) != ERROR_BUFFER_OVERFLOW)
    return;

  m_adapterAddresses = static_cast<PIP_ADAPTER_ADDRESSES>(malloc(ulOutBufLen));
  if (m_adapterAddresses == nullptr)
    return;

  if (GetAdaptersAddresses(AF_INET, flags, nullptr, m_adapterAddresses, &ulOutBufLen) == NO_ERROR)
  {
    for (PIP_ADAPTER_ADDRESSES adapter = m_adapterAddresses; adapter; adapter = adapter->Next)
    {
      if (adapter->IfType == IF_TYPE_SOFTWARE_LOOPBACK)
        continue;

      m_interfaces.push_back(new CNetworkInterfaceWin10(this, adapter));
    }
  }
}

std::vector<std::string> CNetworkWin10::GetNameServers(void)
{
  std::vector<std::string> result;

  const ULONG flags = GAA_FLAG_SKIP_UNICAST | GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST | GAA_FLAG_SKIP_FRIENDLY_NAME;
  ULONG ulOutBufLen;

  if (GetAdaptersAddresses(AF_UNSPEC, flags, nullptr, nullptr, &ulOutBufLen) != ERROR_BUFFER_OVERFLOW)
    return result;

  PIP_ADAPTER_ADDRESSES adapterAddresses = static_cast<PIP_ADAPTER_ADDRESSES>(malloc(ulOutBufLen));
  if (adapterAddresses == nullptr)
    return result;

  if (GetAdaptersAddresses(AF_INET, flags, nullptr, adapterAddresses, &ulOutBufLen) == NO_ERROR)
  {
    for (PIP_ADAPTER_ADDRESSES adapter = adapterAddresses; adapter; adapter = adapter->Next)
    {
      if (adapter->IfType == IF_TYPE_SOFTWARE_LOOPBACK || adapter->OperStatus != IF_OPER_STATUS::IfOperStatusUp)
        continue;

      for (PIP_ADAPTER_DNS_SERVER_ADDRESS dnsAddress = adapter->FirstDnsServerAddress; dnsAddress; dnsAddress = dnsAddress->Next)
      {
        std::string strIp = GetAddressIp(dnsAddress->Address.lpSockaddr);

        if (!strIp.empty())
          result.push_back(strIp);
      }
    }
  }
  free(adapterAddresses);

  return result;
}

void CNetworkWin10::SetNameServers(const std::vector<std::string>& nameServers)
{
  return;
}

bool CNetworkWin10::PingHost(unsigned long host, unsigned int timeout_ms /* = 2000 */)
{
  return false;
}
