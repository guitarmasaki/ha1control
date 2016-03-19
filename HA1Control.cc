/*
 HA-1 HA1Control.cc
 
 Copyright: Copyright (C) 2013 Mitsuru Nakada All Rights Reserved.
 License: GNU GPL v2
 */

#include <fcntl.h>
#include <termios.h>
#include <signal.h>
#include "HA1Control.h"
#include "RaspberryPiIOSetup.h"

HA1Control::HA1Control() {

  Uart = -1;
  EscapedFlag = 0;
  CommandIFPort = -1;
  CommandIFIP[0] = 0;
  CommandIFMask[0] = 0;
  XBeeDevice[0] = 0;
  XBeeDeviceBaudrate = B9600;
  XBeeDeviceFlowCtrl = 0;
  IRRemotePath[0] = 0;
  FrameID = 1;
  SequenceID = 1;
  StatisticsLastMin = -1;
  RainInfo = 0;
  WSSServerURL[0] = 0;
  CACertFile[0] = 0;
  ClientCertFile[0] = 0;
  ClientKeyFile[0] = 0;
  ServerIssuer[0] = 0;
  ServerSubject[0] = 0;
  strcpy(BasePath, "/var/ha1/");
  FWUpdateBuf = NULL;
  FWHeader = NULL;
  FWUpdateDevice = 0;
  FWUpdateDevicePtr = NULL;
  UndefNum = 0;

  for(int i = 0; i < MaxConnect; i++) {
    ClientInfo[i].IF = NULL;
  }
}

HA1Control::~HA1Control() {

  for(int i = 0; i < MaxConnect; i++) {
    if(ClientInfo[i].IF) {
      DeleteExternalPort(i);
      ClientInfo[i].IF->Close();
      ClientInfo[i].IF = NULL;
    }
  }
  close(ListeningSocket);
  if(Uart >= 0) {
    DeleteExternalPort(Uart);
    close(Uart);
  }
  LogClose();
  
  while(FirstReadQueue) {
    ReadQueueSt *next = FirstReadQueue->Next;
    DeleteReadQueue(FirstReadQueue);
    FirstReadQueue = next;
  }
  while(FirstWriteQueue) {
    WriteQueueSt *next = FirstWriteQueue->Next;
    DeleteWriteQueue(FirstWriteQueue);
    FirstWriteQueue = next;
  }
  
  if(FWUpdateBuf) {
    delete [] FWUpdateBuf;
    FWUpdateBuf = NULL;
  }
}

int HA1Control::Connect(int daemonFlag) {

  if(CommandIFPort < 0) return Error;
  DaemonFlag = daemonFlag;
  
  LogPrintf("HA-1\n");

  if(strlen(XBeeDevice)) {
    Uart = open(XBeeDevice, O_RDWR | O_NONBLOCK | O_NOCTTY);
    if(Uart < 0) {
      LogPrintf("cannot open %s : %s\n", XBeeDevice, strerror(errno));
      return Error;
    }

    if(XBeeDeviceFlowCtrl) {
      RaspberryPiIOSetup raspi;
      raspi.Init();
    }
    
    struct termios original_termios;
    struct termios current_termios;
    tcgetattr(Uart, &original_termios);
    tcgetattr(Uart, &current_termios);
    cfmakeraw(&current_termios);
    cfsetspeed(&current_termios, XBeeDeviceBaudrate);
    current_termios.c_cflag &= ~(PARENB | PARODD);
    current_termios.c_cflag = (current_termios.c_cflag & ~CSIZE) | CS8;
    current_termios.c_cflag &= ~(CRTSCTS);
    if(XBeeDeviceFlowCtrl) current_termios.c_cflag |= CRTSCTS;
    current_termios.c_iflag &= ~(IXON | IXOFF | IXANY);
    current_termios.c_cflag |= CLOCAL;
    current_termios.c_cflag &= ~(HUPCL);
    if(int error = tcsetattr(Uart, TCSAFLUSH, &current_termios)) {
      LogPrintf("Illegal parameter %s : %s\n", XBeeDevice, strerror(errno));
      close(Uart);
      return Error;
    }
    AddExternalPort(Uart, ExternalPort_UART);
  }
  
  ListeningSocket = AddListeningSocket(CommandIFPort, ExternalPort_ListeningPort);
  if(ListeningSocket < 0) {
    LogPrintf("AddListeningSocket: CommandIF error\n");
    return Error;
  }
  LogPrintf("wait CommandIFport %d\n", CommandIFPort);

  int ret = WebSocketIF.SecureSetup(CACertFile, ClientCertFile, ClientKeyFile, ServerIssuer, ServerSubject, NULL);
  if(ret < 0) {
    if(strlen(WSSServerURL)) LogPrintf("WebSocketIF Setup error\n");
    strcpy(WSSServerURL, "");
  }
  gettimeofday(&KeepAliveTime, NULL);
  gettimeofday(&IntervalEventTime, NULL);
  gettimeofday(&AliveCheckTime, NULL);

  return Scheduler();
}

int HA1Control::Wakeup() {

#ifdef DEBUG
  LogPrintf("Wakeup\n");
#endif
  
  gettimeofday(&WakeupTime, NULL);
  if(timercmp(&WakeupTime, &IntervalEventTime, >)) {
    IntervalEventTime.tv_sec += AliveInterval;
  }
  NextWakeupTime = IntervalEventTime;

  if(DaemonFlag) KeepAlive();
  QueueCheck();
  IntervalCheck();
  ClientTimeoutCheck();
  SetIntervalTimer();
  return Success;
}

void HA1Control::IntervalCheck() {

#ifdef DEBUG
  LogPrintf("IntervalCheck\n");
#endif

  if(TimeCheck(&AliveCheckTime, AliveInterval)) {
    FWUpdateCheck();
    StatusPostByDevice();
    SendDeviceInfo();
    QueueCheck();
    PasswordCheck();
  }
}

void HA1Control::KeepAlive() {

#ifdef DEBUG
  LogPrintf("KeepAlive\n");
#endif
  
  if(TimeCheck(&KeepAliveTime, WatchDogInterval)) {
    pid_t ppid = getppid();
    kill(ppid, SIGUSR1);
  }
}

int HA1Control::TimeCheck(struct timeval *nextEventTime, int interval) {

#ifdef DEBUG
  LogPrintf("TimeCheck\n");
#endif
  
  int execFlag = 0;
  timeval now;
  gettimeofday(&now, NULL);
  if(timercmp(&now, nextEventTime, >)) {
    nextEventTime->tv_sec += interval;
    execFlag = 1;
  }
  if(timercmp(&NextWakeupTime, nextEventTime, >)) NextWakeupTime = *nextEventTime;
  return execFlag;
}

void HA1Control::SetIntervalTimer() {

#ifdef DEBUG
  LogPrintf("SetIntervalTimer\n");
#endif

  struct timeval now;
  gettimeofday(&now, NULL);
  timeval tv;
  tv.tv_sec = 0;
  tv.tv_usec = 1;
  if(!timercmp(&NextWakeupTime, &now, >)) timeradd(&now, &tv, &NextWakeupTime);

  struct itimerval tm;
  timerclear(&tm.it_interval);
  timersub(&NextWakeupTime, &now, &tm.it_value);
  tm.it_interval.tv_sec = AliveInterval;
  tm.it_interval.tv_usec = 0;
  setitimer(ITIMER_REAL, &tm, NULL);
}

