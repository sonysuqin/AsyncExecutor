/*  Copyright (c) 2013-2015 Tencent. All rights reserved.  */
/**
 * created on : 2012-11-28
 * author : YeRunGui
 */
#include "xlog/comm/platform_comm.h"

#include <string>

#include "NetUtil.h"
#include "xlog/log/loginfo_extract.h"
#include "xlog/log/xlogger.h"

using namespace std;

bool getProxyInfo(int& port, std::string& strProxy, const std::string& _host) {
  //xverbose_function();

  return getProxyInfoImpl(port, strProxy, _host);
}

bool getCurRadioAccessNetworkInfo(struct RadioAccessNetworkInfo& info) {
  //xverbose_function();
  return false;
}

int getNetInfo() {
  //xverbose_function();

  return isNetworkConnected() ? kWifi : kNoNet;
}

unsigned int getSignal(bool isWifi) {
  //xverbose_function();
  return (unsigned int)0;
}

bool getifaddrs_ipv4_hotspot(std::string& _ifname, std::string& _ip) {
  return false;
}

bool isNetworkConnected() {
  return isNetworkConnectedImpl();
}

static const char* const SIMULATOR_NET_INFO = "SIMULATOR";
static const char* const USE_WIRED = "wired";

bool getCurWifiInfo(WifiInfo& wifiInfo) {
  return false;
}

bool getCurSIMInfo(SIMInfo& bsinfo) {
  return false;
}

bool getAPNInfo(APNInfo& info) {
  return false;
}

void ConsoleLog(const XLoggerInfo* _info, const char* _log) {
  if (NULL == _info || NULL == _log)
    return;
  static const char* levelStrings[] = {
      "V",
      "D",  // debug
      "I",  // info
      "W",  // warn
      "E",  // error
      "F"   // fatal
  };
  char strFuncName[128] = {0};
  ExtractFunctionName(_info->func_name, strFuncName, sizeof(strFuncName));
  const char* file_name = ExtractFileName(_info->filename);
  char log[16 * 1024] = {0};
  snprintf(log, sizeof(log), "[%s][%s][%s:%d %s()] %s\n",
           levelStrings[_info->level], NULL == _info->tag ? "" : _info->tag,
           file_name, _info->line, strFuncName, _log);
  ::OutputDebugStringA(log);
  printf(log);
}
