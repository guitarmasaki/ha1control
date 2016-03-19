/*

 Socket.cc
 
 Copyright: Copyright (C) 2015 Mitsuru Nakada All Rights Reserved.
 License: GNU GPL v2
 */

#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <netdb.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdarg.h>

#include "Socket.h"

Socket::Socket() {

  SocketFd = -1;
  bzero(&SockAddr, sizeof(SockAddr));
}

Socket::~Socket() {

  Close();
}

int Socket::Accept(int sockfd, const char *allowIP, const char *allowMask) {

  if(SocketFd >= 0) return Error;

  int len = sizeof(SockAddr);
  int newSocket = accept(sockfd, (struct sockaddr *)&SockAddr, (socklen_t *)&len);
  if(newSocket < 0) {
    LogPrintf("Socket::Accept Error\n");
    return Error;
  }
  
  in_addr_t localIP = inet_addr("127.0.0.1");
  in_addr_t allowNetwork = localIP;
  in_addr_t compareMask = inet_addr("255.255.255.255");
  if(allowMask && strlen(allowMask)) compareMask = inet_addr(allowMask);
  if(allowIP && strlen(allowIP)) allowNetwork = inet_addr(allowIP) & compareMask;
  in_addr_t acceptNetwork = SockAddr.sin_addr.s_addr & compareMask;
  if((SockAddr.sin_addr.s_addr != localIP) && (allowNetwork != acceptNetwork)) {
    LogPrintf("Not Allow Hosts : %s\n", inet_ntoa(SockAddr.sin_addr));
    close(newSocket);
     return Error;
  }

  int flag = fcntl(newSocket, F_GETFL, 0);
  fcntl(newSocket, F_SETFL, O_NONBLOCK|flag);
  timeval tv;
  tv.tv_sec = AliveInterval;
  tv.tv_usec = 0;
  setsockopt(newSocket, SOL_SOCKET, SO_SNDTIMEO, (void *)&tv, sizeof(tv));
  setsockopt(newSocket, SOL_SOCKET, SO_RCVTIMEO, (void *)&tv, sizeof(tv));
  
  LogPrintf("connect: %d %s:%d\n", newSocket,
            inet_ntoa(SockAddr.sin_addr), ntohs(SockAddr.sin_port));

  SocketFd = newSocket;
  gettimeofday(&LastAccess, NULL);
  return newSocket;
}

int Socket::Connect(const char *host, int port) {

  if(SocketFd >= 0) return Error;
  
  if((port < 0) || (host[0] == 0)) return Error;
  struct hostent *servhost = gethostbyname(host);
  if(servhost == NULL) {
    LogPrintf("Socket::Connect gethostbyname error %s:%d\n", host, port);
    return Error;
  }
  bzero(&SockAddr, sizeof(SockAddr));
  SockAddr.sin_family = AF_INET;
  bcopy(servhost->h_addr, &SockAddr.sin_addr, servhost->h_length);
  SockAddr.sin_port = htons(port);
  
  int newSocket = socket(AF_INET, SOCK_STREAM, 0);
  if(newSocket < 0) {
    LogPrintf("Socket::Connect socket error %s:%d\n", host, port);
    return newSocket;
  }
  int opt = 1;
  setsockopt(newSocket, SOL_SOCKET, SO_KEEPALIVE, (void *)&opt, sizeof(opt));
  timeval tv;
  tv.tv_sec = AliveInterval;
  tv.tv_usec = 0;
  setsockopt(newSocket, SOL_SOCKET, SO_SNDTIMEO, (void *)&tv, sizeof(tv));
  setsockopt(newSocket, SOL_SOCKET, SO_RCVTIMEO, (void *)&tv, sizeof(tv));

  timeval timeout;
  gettimeofday(&timeout, NULL);
  timeout.tv_sec += 5;
  int ret;
  do {
    ret = connect(newSocket, (struct sockaddr *)&SockAddr, sizeof(SockAddr));
    if(!ret) break;
    if(errno != EINPROGRESS) {
      timeval now;
      gettimeofday(&now, NULL);
      if((errno != EINTR) || timercmp(&now, &timeout, >)) {
        LogPrintf("Socket::Connect connect error %s:%d\n", host, port);
        close(newSocket);
        return Error;        
      }
    }
    
    timeval tv;
    tv.tv_sec = 1;
    tv.tv_usec = 0;
    fd_set checkFDs;
    FD_ZERO(&checkFDs);
    FD_SET(newSocket , &checkFDs);
    ret = select(newSocket + 1, NULL, &checkFDs, NULL, &tv);
    if(ret < 0) {
      LogPrintf("Socket::Connect select error %s:%d\n", host, port);
      close(newSocket);
      return Error;
    }
    socklen_t len = sizeof(opt);
    getsockopt(newSocket, SOL_SOCKET, SO_ERROR, (void *)&opt, &len);
    if(!opt) break;
  } while(ret);
  SocketFd = newSocket;
  gettimeofday(&LastAccess, NULL);
  return newSocket;
}

int Socket::Close() {

  if(SocketFd) close(SocketFd);
  SocketFd = -1;
  return Success;
}

int Socket::Receive(byte *buf, int size) {

  if(SocketFd < 0) return Error;
  
  int ret = read(SocketFd, buf, size);
  if(ret > 0) SetLastAccess();
  if(!ret) ret = Disconnect;
  return ret;
}

int Socket::SendText(const char *fmt, ...) {
  
  va_list args;
  va_start(args, fmt);
  
  char buf[1024];
  vsnprintf(buf, 1024, fmt, args);
  buf[1023] = 0;
  
  return Send((byte *)buf, strlen(buf));
}

int Socket::Send(byte *buf, int size) {

  if(SocketFd < 0) return Error;

  int ret = write(SocketFd, buf, size);
  if(ret > 0) SetLastAccess();
  return ret;
}

void Socket::GetPeerAddr(struct sockaddr_in *sockAddr) {

  bcopy(&SockAddr, sockAddr, sizeof(SockAddr));
  return;
}

int Socket::GetFd() {

  return SocketFd;
}

void Socket::SetLastAccess() {

  gettimeofday(&LastAccess, NULL);
}

void Socket::GetLastAccess(timeval *tm) {

  tm->tv_sec = LastAccess.tv_sec;
  tm->tv_usec = LastAccess.tv_usec;
}

