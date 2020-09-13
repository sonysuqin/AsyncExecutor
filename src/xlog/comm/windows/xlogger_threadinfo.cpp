//
//  xloggr_threadinfo.m
//  MicroMessenger
//
//  Created by YeRunGui on 13-3-13.
//  Copyright (c) 2013 Tencent. All rights reserved.
//

#include "xlog/log/xloggerbase.h"
#define WIN32_LEAN_AND_MEAN
#include <stdint.h>
#include <windows.h>

extern "C" {
intmax_t xlogger_pid() {
  static intmax_t pid = GetCurrentProcessId();
  return pid;
}

intmax_t xlogger_tid() {
  return GetCurrentThreadId();
}

static intmax_t sg_maintid = GetCurrentThreadId();
intmax_t xlogger_maintid() {
  return sg_maintid;
}
}
