/*
 HA-1 Service.cc
 
 Copyright: Copyright (C) 2013 Mitsuru Nakada All Rights Reserved.
 License: GNU GPL v2
 */

#include <signal.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <arpa/inet.h>

#include "Service.h"

void TrapSignal(int no);

Service::Service() {

  for(int i = 0; i < MaxConnect; i++) {
    PortTable[i] = 0;
  }
  for(int i = 0; i < 32; i++) {
    ClrTrapFlag(i);
  }
  FD_ZERO(&TargetFDs);
  MaxFD = 0;
}

int Service::Scheduler() {

  struct sigaction sa;
  memset(&sa, 0, sizeof(struct sigaction));
  sa.sa_handler = TrapSignal;
  sa.sa_flags |= SA_RESTART;
  if(sigaction(SIGINT, &sa, NULL) != 0) {
    LogPrintf("sigaction error\n");
    return Error;
  }
  if(sigaction(SIGHUP, &sa, NULL) != 0) {
    LogPrintf("sigaction error\n");
    return Error;
  }
  if(sigaction(SIGTERM, &sa, NULL) != 0) {
    LogPrintf("sigaction error\n");
    return Error;
  }
  if(sigaction(SIGALRM, &sa, NULL) != 0) {
    LogPrintf("sigaction error\n");
    return Error;
  }
  if(sigaction(SIGPIPE, &sa, NULL) != 0) {
    LogPrintf("sigaction error\n");
   return Error;
  }

  if(pipe(SelfPipe)) {
    LogPrintf("pipe error\n");
    return Error;
  }
  int flag = fcntl(SelfPipe[0], F_GETFL, 0);
  fcntl(SelfPipe[0], F_SETFL, O_NONBLOCK|flag);
  flag = fcntl(SelfPipe[1], F_GETFL, 0);
  fcntl(SelfPipe[1], F_SETFL, O_NONBLOCK|flag);

  FD_SET(SelfPipe[0], &TargetFDs);
  MaxFD = (SelfPipe[0] > MaxFD) ? SelfPipe[0] : MaxFD;

  while (!IsTrap(SIGTERM) && !IsTrap(SIGHUP) && !IsTrap(SIGINT)){
    Wakeup();
    fd_set checkFDs;
    memcpy(&checkFDs, &TargetFDs, sizeof(TargetFDs));

    if(select(MaxFD + 1, &checkFDs, NULL, NULL, NULL) == -1) {
      if((errno == EINTR) && IsTrap(SIGINT)) {
        ClrTrapFlag(SIGINT);
        break;
      }
      if((errno == EINTR) && IsTrap(SIGHUP)) {
        ClrTrapFlag(SIGHUP);
        break;
      }
     if((errno == EINTR) && IsTrap(SIGTERM)) {
       ClrTrapFlag(SIGTERM);
       break;
      }
      if((errno == EINTR) && IsTrap(SIGPIPE)) {
        ClrTrapFlag(SIGPIPE);
      }
      if((errno == EINTR) && IsTrap(SIGALRM)) {
        ClrTrapFlag(SIGALRM);
      }
    } else if(FD_ISSET(SelfPipe[0], &checkFDs)) {
      byte buf[256];
      read(SelfPipe[0], buf, 256);
      ClrTrapFlag(SIGALRM);
    } else {
      for(int i = MaxFD; i >= 0; i--) {
        if(FD_ISSET(i, &checkFDs)) ReadExternalPort(i, PortTable[i]);
      }
    }
  }
  close(SelfPipe[0]);
  close(SelfPipe[1]);
  return Success;
}

void Service::AddExternalPort(int fd, int port) {

  PortTable[fd] = port;
  FD_SET(fd, &TargetFDs);
  MaxFD = (fd > MaxFD) ? fd : MaxFD;
}

void Service::DeleteExternalPort(int fd) {

  PortTable[fd] = 0;
  FD_CLR(fd, &TargetFDs);
}

void Service::SetTrapFlag(int no) {

  TrapFlag[no] = 1;
  byte buf;
  buf = 0;
  write(SelfPipe[1], &buf, 1);
}

void Service::ClrTrapFlag(int no) {

  TrapFlag[no] = 0;
}

bool Service::IsTrap(int no) {
  return TrapFlag[no] != 0;
}

int Service::AddListeningSocket(int port, int servicePort) {
  
#ifdef DEBUG
  LogPrintf("AddListeningSocket %d %d\n", port, servicePort);
#endif
  int newSocket = socket(AF_INET, SOCK_STREAM, 0);
  if(newSocket < 0) return Error;
  
  int sock_optval = 1;
  if(setsockopt(newSocket, SOL_SOCKET, SO_REUSEADDR,
                &sock_optval, sizeof(sock_optval)) == -1) {
    LogPrintf("setsockopt : %s\n", strerror(errno));
    close(newSocket);
    return Error;
  }
  
  struct sockaddr_in saddr;
  saddr.sin_family = AF_INET;
  saddr.sin_port = htons(port);
  saddr.sin_addr.s_addr = htonl(INADDR_ANY);
  if(bind(newSocket, (struct sockaddr *)&saddr, sizeof(saddr)) < 0) {
    LogPrintf("bind : %s\n", strerror(errno));
    close(newSocket);
    return Error;
  }
  
  if(listen(newSocket, MaxConnect) == -1) {
    LogPrintf("listen : %s\n", strerror(errno));
    close(newSocket);
    return Error;
  }
  AddExternalPort(newSocket, servicePort);
  return newSocket;
}
