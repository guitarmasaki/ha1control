/*
 HA-1 LogFacility.h
 
 Copyright: Copyright (C) 2013 Mitsuru Nakada All Rights Reserved.
 License: GNU GPL v2
 */

#ifndef __LogFacility_h__
#define __LogFacility_h__

#include <stdio.h>

// #define LOG_DEBUG

class LogFacility {

public:
  LogFacility();
  ~LogFacility();
  void LogOpen(const char *file);
  void LogPrintf(const char *fmt, ...);
  void LogClose();
  
private:
  static const int LogBufferSize = 1024;
  char LogFile[256];
  char LogBuffer[LogBufferSize];
  int  LogBufferPtr;
  
};

#endif // __LogFacility_h__

