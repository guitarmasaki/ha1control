/*
 HA-1 Service.h
 
 Copyright: Copyright (C) 2013 Mitsuru Nakada All Rights Reserved.
 License: GNU GPL v2
 */

#ifndef __Service_h__
#define __Service_h__

#include <unistd.h>

#include "LogFacility.h"
#include "Types.h"
#include "Error.h"

class Service : public LogFacility {
public:
  Service();

  int Scheduler();
  void AddExternalPort(int fd, int port);
  void DeleteExternalPort(int fd);
  int AddListeningSocket(int port, int servicePort);

  void SetTrapFlag(int no);
  
private:
  void ClrTrapFlag(int no);
  bool IsTrap(int no);

  virtual int Wakeup() { return 0; };
  virtual int ReadExternalPort(int fd, int port) { return 0; };

  static const int MaxConnect = 32;
  int PortTable[MaxConnect];
  static const int TrapMax = 32;
  int TrapFlag[TrapMax];
  int SelfPipe[2];

  int    MaxFD;
  fd_set TargetFDs;
};

#endif // __Service_h__

