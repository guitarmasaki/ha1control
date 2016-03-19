/*

 Socket.h
 
 Copyright: Copyright (C) 2015 Mitsuru Nakada All Rights Reserved.
 License: GNU GPL v2
 */


#ifndef __Socket_h__
#define __Socket_h__

#include <arpa/inet.h>

#include "LogFacility.h"
#include "Types.h"
#include "Error.h"

class Socket : public LogFacility {

public:
  Socket();
  ~Socket();

  virtual int Accept(int sockfd, const char *allowIP = NULL, const char *allowMask = NULL);
  virtual int Connect(const char *host, int port);
  virtual int Close();
  virtual int Receive(byte *buf, int size);
  virtual int Send(byte *buf, int size);
  int SendText(const char *fmt, ...);
  void GetPeerAddr(struct sockaddr_in *sockAddr);
  int GetFd();
  void SetLastAccess();
  void GetLastAccess(timeval *tm);

private:
  static const int  AliveInterval = 5;

  int SocketFd;
  struct sockaddr_in SockAddr;
  timeval LastAccess;
};

#endif // __Socket_h__

