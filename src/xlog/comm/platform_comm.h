// Tencent is pleased to support the open source community by making Mars
// available. Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights
// reserved.

// Licensed under the MIT License (the "License"); you may not use this file
// except in compliance with the License. You may obtain a copy of the License
// at http://opensource.org/licenses/MIT

// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" basis, WITHOUT
// WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
// License for the specific language governing permissions and limitations under
// the License.

/*
 * platform_comm.h
 *
 *  Created on: 2012-11-2
 *      Author: yerungui
 */

#ifndef COMM_PLATFORM_COMM_H_
#define COMM_PLATFORM_COMM_H_

#include <string>

#ifdef ANDROID
#include "mars/comm/thread/mutex.h"
#endif

#ifndef __cplusplus
#error "C++ only"
#endif

enum NetType { kNoNet = -1, kWifi = 1, kMobile = 2, kOtherNet = 3 };
int getNetInfo();

bool getCurRadioAccessNetworkInfo(struct RadioAccessNetworkInfo& _info);

struct WifiInfo {
  std::string ssid;
  std::string bssid;
};
bool getCurWifiInfo(WifiInfo& _wifi_info);

struct SIMInfo {
  std::string isp_code;
  std::string isp_name;
};
bool getCurSIMInfo(SIMInfo& _sim_info);

struct APNInfo {
  APNInfo() : nettype(kNoNet - 1), sub_nettype(0), extra_info("") {}
  int nettype;
  int sub_nettype;
  std::string extra_info;
};
bool getAPNInfo(APNInfo& info);
#if __cplusplus >= 201103L
#define __CXX11_CONSTEXPR__ constexpr
#else
#define __CXX11_CONSTEXPR__
#endif

__CXX11_CONSTEXPR__ static const char* const GPRS = "GPRS";
__CXX11_CONSTEXPR__ static const char* const Edge = "Edge";
__CXX11_CONSTEXPR__ static const char* const WCDMA = "WCDMA";
__CXX11_CONSTEXPR__ static const char* const HSDPA = "HSDPA";
__CXX11_CONSTEXPR__ static const char* const HSUPA = "HSUPA";
__CXX11_CONSTEXPR__ static const char* const CDMA1x = "CDMA1x";
__CXX11_CONSTEXPR__ static const char* const CDMAEVDORev0 = "CDMAEVDORev0";
__CXX11_CONSTEXPR__ static const char* const CDMAEVDORevA = "CDMAEVDORevA";
__CXX11_CONSTEXPR__ static const char* const CDMAEVDORevB = "CDMAEVDORevB";
__CXX11_CONSTEXPR__ static const char* const eHRPD = "eHRPD";
__CXX11_CONSTEXPR__ static const char* const LTE = "LTE";
__CXX11_CONSTEXPR__ static const char* const UMTS = "UMTS";
__CXX11_CONSTEXPR__ static const char* const CDMA = "CDMA";
__CXX11_CONSTEXPR__ static const char* const HSPA = "HSPA";
__CXX11_CONSTEXPR__ static const char* const IDEN = "IDEN";
__CXX11_CONSTEXPR__ static const char* const HSPAP = "HSPA+";

struct RadioAccessNetworkInfo {
  std::string radio_access_network;

  bool Is2G() const {
    return radio_access_network == GPRS || radio_access_network == CDMA1x ||
           radio_access_network == Edge ||
           radio_access_network == CDMAEVDORev0 ||
           radio_access_network == UMTS || radio_access_network == CDMA;
  }
  bool Is3G() const {
    return radio_access_network == WCDMA ||
           radio_access_network == CDMAEVDORevA ||
           radio_access_network == HSDPA || radio_access_network == HSUPA ||
           radio_access_network == CDMAEVDORevB ||
           radio_access_network == eHRPD || radio_access_network == HSPAP ||
           radio_access_network == HSPA;
  }
  bool Is4G() const { return radio_access_network == LTE; }
  bool IsUnknown() const { return !Is2G() && !Is3G() && !Is4G(); }
};

bool getCurRadioAccessNetworkInfo(RadioAccessNetworkInfo& _raninfo);

unsigned int getSignal(bool isWifi);
bool isNetworkConnected();

bool getifaddrs_ipv4_hotspot(std::string& _ifname, std::string& _ifip);

inline int getCurrNetLabel(std::string& netInfo) {
  netInfo = "defalut";
  int netId = getNetInfo();

  if (netId == kNoNet) {
    netInfo = "";
    return netId;
  }

  switch (netId) {
    case kWifi: {
      WifiInfo wifiInfo;

      if (getCurWifiInfo(wifiInfo)) {
        netInfo = wifiInfo.ssid;
      } else {
        netInfo = "no_ssid_wifi";
      }

      break;
    }

    case kMobile: {
      SIMInfo simInfo;

      if (getCurSIMInfo(simInfo)) {
        netInfo = simInfo.isp_code;
      } else {
        netInfo = "no_ispCode_mobile";
      }

      break;
    }

    case kOtherNet: {
      netInfo = "other";
      break;
    }

    default: { break; }
  }

  return netId;
}

#ifdef __APPLE__
void FlushReachability();
float publiccomponent_GetSystemVersion();
#endif

#ifdef ANDROID
bool startAlarm(int64_t id, int after);
bool stopAlarm(int64_t id);

void* wakeupLock_new();
void wakeupLock_delete(void* _object);
void wakeupLock_Lock(void* _object);
void wakeupLock_Lock_Timeout(void* _object, int64_t _timeout);
void wakeupLock_Unlock(void* _object);
bool wakeupLock_IsLocking(void* _object);
#endif

#ifdef ANDROID
extern int g_NetInfo;
extern WifiInfo g_wifi_info;
extern SIMInfo g_sim_info;
extern APNInfo g_apn_info;
extern Mutex g_net_mutex;
#endif

#endif /* COMM_PLATFORM_COMM_H_ */
