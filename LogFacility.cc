/*
 HA-1 LogFacility.cc
 
 Copyright: Copyright (C) 2013 Mitsuru Nakada All Rights Reserved.
 License: GNU GPL v2
 */

#include <time.h>
#include <string.h>
#include <unistd.h>
#include <stdarg.h>
#include "LogFacility.h"

LogFacility::LogFacility() {

  LogFile[0] = 0;
  LogBufferPtr = 0;
}

LogFacility::~LogFacility() {

  LogFile[0] = 0;
}

void LogFacility::LogOpen(const char *file) {

  strcpy(LogFile, file);
}
  
void LogFacility::LogPrintf(const char *fmt, ...) {

  va_list args;
  va_start(args, fmt);

  static int crFlag = 1;
  
  char buf[LogBufferSize];
  int len = strlen(buf);
  vsnprintf(buf, LogBufferSize - 1, fmt, args);
  
  char timeBuf[LogBufferSize];
  timeBuf[0] = 0;
  time_t t = time(NULL);
  strftime(timeBuf, LogBufferSize - 1, "%F %T : ", localtime(&t));
  sprintf(timeBuf + strlen(timeBuf), "%d : ", getpid());

  FILE *fd = NULL;
  
  for(int i = 0; i < strlen(buf); i++) {
    LogBuffer[LogBufferPtr++] = buf[i];
    if((LogBufferPtr >= LogBufferSize) || (buf[i] == 0x0a)) {
      if(strlen(LogFile)) {
        if(!fd) fd = fopen(LogFile, "a");
        if(fd) {
          fwrite(timeBuf, strlen(timeBuf), 1, fd);
          fwrite(LogBuffer, LogBufferPtr, 1, fd);
        }
      }
#ifdef LOG_DEBUG
      fwrite(timeBuf, strlen(timeBuf), 1, stderr);
      fwrite(LogBuffer, LogBufferPtr, 1, stderr);
#endif
      LogBufferPtr = 0;
      continue;
    }
  }
  if(fd) fclose(fd);
}

void LogFacility::LogClose() {
  
  LogFile[0] = 0;
}
