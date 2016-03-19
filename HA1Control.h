/*
 HA-1 HA1Control.h
 
 Copyright: Copyright (C) 2013 Mitsuru Nakada All Rights Reserved.
 License: GNU GPL v2
 */

#ifndef __HA1Control_h__
#define __HA1Control_h__

#include <stdlib.h>
#include <errno.h>
#include <sys/time.h>
#include <string.h>
#include <arpa/inet.h>
#include <termios.h>
#include <jansson.h>

#include "Service.h"
#include "Buffer.h"
#include "WebSocket.h"
#include "Socket.h"
#include "IRRemote.h"
#include "CommandList.h"
#include "StatusList.h"
#include "RemoconList.h"
#include "DeviceList.h"
#include "GateList.h"
#include "PasswordList.h"
#include "Error.h"
#include "Types.h"

class HA1Control : public Service {
private:
  struct ReadQueueSt;
  struct WriteQueueSt;

public:
  HA1Control();
  ~HA1Control();
  
  int Connect(int daemonFlag);
  int ReadConfFile(const char *file);

private:
  int Wakeup();
  void KeepAlive();
  int TimeCheck(struct timeval *nextEventTime, int interval);
  void IntervalCheck();
  void SetIntervalTimer();

  int ExecuteCommand(int fd, char *execCmd);
  int ExecuteCommand(WriteQueueSt *writeQueue);
  int ExecuteCommandBody(int sockfd, int sequenceID, char *execCmd, char *combiCmd, int reentrantFlag = 0);

  void QueueCheck();
  WriteQueueSt *QueuePairATCommandProcess(WriteQueueSt *p, ReadQueueSt *q);
  WriteQueueSt *QueuePairAVRCommandDelivery(WriteQueueSt *p, ReadQueueSt *q);
  WriteQueueSt *QueuePairAVRCommandProcess(WriteQueueSt *p, ReadQueueSt *q);
  int ReadQueueCheck();
  void WriteQueueTimerCheck();
  int WriteQueueHighPriorityCheck();
  int WriteQueueLowPriorityCheck();
  void WriteQueueTimeoutCheck();
  void ReadQueueTimeoutCheck();
  int CommandCheck();

  int ReadExternalPort(int fd, int externalPort);
  int AcceptCommandIF(int sock);
  int CommandIFReceive(int sockfd);
  int XBeeReceive(int sockfd);
  void ClientTimeoutCheck();
  void ResPrintf(int fd, const char *fmt, ...);
  int ResWrite(int fd, byte *buf, int size);
  void ErrorPrintf(const char *fmt, ...);
  int IsFormatText(int sockfd);
  void SendResponse(int sockfd, const char *execCmd, int error, json_t *result = NULL);
  int SendNotify(const char *type, json_t *data);
  
  int AddWriteQueue(int sockfd, int sequenceID, int state, const char *execCmd, const char *combiCmd, byte *data, int dataSize, longword addrL, byte *buf, int size, int resOffset, int (HA1Control::*nextFunc)(WriteQueueSt *p, ReadQueueSt *q) = NULL,  int waitMSec = 0, unsigned long param1 = -1, unsigned long param2 = -1, unsigned long param3 = -1);
  void DeleteWriteQueue(WriteQueueSt *writeQueue);
  int AddReadQueue(byte *buf, int size);
  void DeleteReadQueue(ReadQueueSt *readQueue);
  
  int CommandFuncTableSearchByName(const char *name, longword execFlagMask = 0xffffffff);
  int StatusFuncTableSearchByName(const char *name, char *type = NULL);

  int SWControl(int sockfd, int sequenceID, char *execCmd, char *combiCmd, longword addrL);
  int HAControl(int sockfd, int sequenceID, char *execCmd, char *combiCmd, longword addrL);
  int GPOCtrl(int sockfd, int sequenceID, char *execCmd, char *combiCmd, longword addrL);
  int ATCmdDump(WriteQueueSt *writeQueue, ReadQueueSt *readQueue);
  int IRSend(int sockfd, int sequenceID, char *execCmd, char *combiCmd, longword addrL);
  int IRSend_Next(WriteQueueSt *writeQueue, ReadQueueSt *readQueue);
  int IRRecord(int sockfd, int sequenceID, char *execCmd, char *combiCmd, longword addrL);
  int StatusCheck(int sockfd, int sequenceID, char *execCmd, char *combiCmd, longword addrL);
  int QueueList(int sockfd, int sequenceID, char *execCmd, char *combiCmd, longword addrL);
  int DeviceInfo(int sockfd, int sequenceID, char *execCmd, char *combiCmd, longword addrL);
  int Quit(int sockfd, int sequenceID, char *execCmd, char *combiCmd, longword addrL);
  int FormatChange(int sockfd, int sequenceID, char *execCmd, char *combiCmd, longword addrL);
  int Wait(int sockfd, int sequenceID, char *execCmd, char *combiCmd, longword addrL);
  int KeypadLED(int sockfd, int sequenceID, char *execCmd, char *combiCmd, longword addrL);
  int KeypadBuzz(int sockfd, int sequenceID, char *execCmd, char *combiCmd, longword addrL);
  int SetPW(int sockfd, int sequenceID, char *execCmd, char *combiCmd, longword addrL);
  int MotorControl(int sockfd, int sequenceID, char *execCmd, char *combiCmd, longword addrL);
  int Update(int sockfd, int sequenceID, char *execCmd, char *combiCmd, longword addrL);
  int VirtualSW(int sockfd, int sequenceID, char *execCmd, char *combiCmd, longword addrL);
  int Help(int sockfd, int sequenceID, char *execCmd, char *combiCmd, longword addrL);
  int AVRBoot(int sockfd, int sequenceID, char *execCmd, char *combiCmd, longword addrL);
  int AVRBootRes(WriteQueueSt *writeQueue, ReadQueueSt *readQueue);
  int AVRRead(int sockfd, int sequenceID, char *execCmd, char *combiCmd, longword addrL);
  int AVRReadRes(WriteQueueSt *writeQueue, ReadQueueSt *readQueue);
  int AVRStat(int sockfd, int sequenceID, char *execCmd, char *combiCmd, longword addrL);
  int Validate(int sockfd, int sequenceID, char *execCmd, char *combiCmd, longword addrL);
  int ValidateRes(WriteQueueSt *writeQueue, ReadQueueSt *readQueue);
  int SetLEDTape(int sockfd, int sequenceID, char *execCmd, char *combiCmd, longword addrL);
  int SetRain(int sockfd, int sequenceID, char *execCmd, char *combiCmd, longword addrL);

  int FWReadFile();
  void FWUpdateCheck();
  void FWReboot(longword addrL);
  int FWUpdateNodeIdRes(WriteQueueSt *writeQueue, ReadQueueSt *readQueue);
  int FWUpdateGetVer1Res(WriteQueueSt *writeQueue, ReadQueueSt *readQueue);
  int FWUpdateGetVer2(WriteQueueSt *writeQueue, ReadQueueSt *readQueue);
  int FWUpdateGetVer2Res(WriteQueueSt *writeQueue, ReadQueueSt *readQueue);
  int FWUpdateTransfer(WriteQueueSt *writeQueue, ReadQueueSt *readQueue);
  int FWUpdateTransferRes(WriteQueueSt *writeQueue, ReadQueueSt *readQueue);
  int FWUpdateRebootFW(WriteQueueSt *writeQueue, ReadQueueSt *readQueue);
  int FWUpdateGetVer3(WriteQueueSt *writeQueue, ReadQueueSt *readQueue);
  int FWUpdateGetVer3Res(WriteQueueSt *writeQueue, ReadQueueSt *readQueue);
  int FWUpdateGetVer4(WriteQueueSt *writeQueue, ReadQueueSt *readQueue);
  int FWUpdateGetVer4Res(WriteQueueSt *writeQueue, ReadQueueSt *readQueue);
  int GetVerRes(WriteQueueSt *writeQueue, ReadQueueSt *readQueue);

  int Retry(WriteQueueSt *p, int retry = 0);
  int SendWriteQueue(WriteQueueSt *p);
  int SendATCommand(int sockfd, int sequenceID, const char *exeCmd, const char *combiCmd, byte *data, int dataSize, longword addrL, byte *cmd, int len, int (HA1Control::*nextFunc)(WriteQueueSt *p, ReadQueueSt *q) = NULL, int waitMSec = 0, longword param1 = -1, longword param2 = -1, longword param3 = -1);
  int SendATCommand(WriteQueueSt *p, byte *cmd, int len, int (HA1Control::*nextFunc)(WriteQueueSt *p, ReadQueueSt *q) = NULL, int waitMSec = 0) {
    return SendATCommand(p->SockFd, p->SequenceID, p->ExecCmd, p->CombiCmd, p->Data, p->DataSize, p->AddrL, cmd, len, nextFunc, waitMSec, p->Param1, p->Param2, p->Param3);
  }
  int SendAVRCommand(int sockfd, int sequenceID, const char *exeCmd, const char *combiCmd, byte *data, int dataSize, longword addrL, byte *cmd, int len, int (HA1Control::*nextFunc)(WriteQueueSt *p, ReadQueueSt *q) = NULL, int waitMSec = 0, longword param1 = -1, longword param2 = -1, longword param3 = -1);
  int SendAVRCommand(WriteQueueSt *p, byte *cmd, int len, int (HA1Control::*nextFunc)(WriteQueueSt *p, ReadQueueSt *q) = NULL, int waitMSec = 0) {
    return SendAVRCommand(p->SockFd, p->SequenceID, p->ExecCmd, p->CombiCmd, p->Data, p->DataSize, p->AddrL, cmd, len, nextFunc, waitMSec, p->Param1, p->Param2, p->Param3);
  }
  int SendPacket(const byte *buf, int size);
  void AddBufferWithEsc(byte *buf, int *p, byte d);
  int AnalyzeAVRMessage(ReadQueueSt *readQueue);
  int StatusChangeReceive(DeviceList *devData, ReadQueueSt *q);
  int RemoconReceive(DeviceList *devData, ReadQueueSt *readQueue);
  int KeypadReceive(DeviceList *devData, ReadQueueSt *readQueue);
  int MotorReceive(DeviceList *devData, ReadQueueSt *readQueue);

  int ScanReceive(ReadQueueSt *q);
  int GetCondition(const char *command, int *stat, int *lastStat = NULL, char *str = NULL, char *type = NULL);
  int StatusPostByDevice(longword addrL = 0xffffffff);
  int SendDeviceInfo();
  int PasswordCheck(int force = 0);

  int AD2Rain(int ad) {
    return ad / 41;
  }
  int AD2Temp(int ad) {
    return ad - 500;
  }
  int AD2Humidity(int ad) {
    return (ad - 500) / 22;
  }
  int AD2Battery(int ad) {
    return ad * 35 / 2;
  }
  static const longword ADDRH = 0x0013a200;
  static const int SendCommandTimeout = 30; // 30sec
  static const int ReceiveQueueTimeout = 10; // 10sec
  static const int SendCommandRetry = 3;
  static const int AnalogSensorCheckInterval = 300;
  static const int AliveCheckTimeout = 300;
  static const int AliveInterval = 30;
  static const int WebSocketAliveInterval = 5;

  int  CommandIFPort;
  char CommandIFIP[32];
  char CommandIFMask[32];
  char LogFile[256];
  int DaemonFlag;
  int UndefNum;
  
  char XBeeDevice[256];
  speed_t XBeeDeviceBaudrate;
  int XBeeDeviceFlowCtrl;
  char BasePath[256];

  static const int ConnectReceiveBufferSize = 1024;

  char IRRemotePath[256];
  char IRRecordFile[256];
  longword IRRecordAddrL;
  timeval IRRecordTimeout;
  int IRRecordSockFd;
  char IRRecordExecCmd[256];

  char WSSServerURL[256];
  char CACertFile[256];
  char ClientCertFile[256];
  char ClientKeyFile[256];
  char ServerIssuer[256];
  char ServerSubject[256];

  struct FWHeaderSt {
    longword Magic;
    word SetId;
    word Version;
    word Addr;
    word Size;
    word Reserve;
    word CRC;
  };
  char FWPath[256];
  byte *FWUpdateBuf;
  FWHeaderSt *FWHeader;
  int FWSeqNum;
  longword FWUpdateDevice;
  DeviceList *FWUpdateDevicePtr;
  static const int FWSeqLastPage = 0xdf;

  DeviceList DeviceTable;
  StatusList StatusTable;
  CommandList CommandTable;
  RemoconList RemoconTable;
  GateList GateTable;
  PasswordList PasswordTable;

  struct CommandFuncTableSt {
    const char *Name;
    longword ExecFlag;
    int (HA1Control::*Func)(int sockfd, int sequenceID, char *execCmd, char *combiCmd, longword addrL);
  };
  static const CommandFuncTableSt CommandFuncTable[];

  struct StatusFuncTableSt {
    const char *Name;
    const char *Type;
  };
  static const StatusFuncTableSt StatusFuncTable[];

  static const int MaxConnect = 32;
  static const int ConnectionTimeout = 300;

  enum ExternalPort {
    ExternalPort_ListeningPort = 0,
    ExternalPort_CommandIFPort,
    ExternalPort_WebSocketIFPort,
    ExternalPort_UART,
    ExternalPort_Num
  };

  int ListeningSocket;
  WebSocket WebSocketIF;
  int WebSocketIntialState;
  enum FormatSt {
    Format_TEXT,
    Format_JSON
  };
  
  struct ClientInfoSt {
    Socket    *IF;
    FormatSt  Format;
  };
  ClientInfoSt ClientInfo[MaxConnect];
  Buffer InputBuffer[MaxConnect];
  Buffer WebSocketSendBuffer;

  int FrameID;
  int SequenceID;
  int Uart;
  unsigned char EscapedFlag;
  int ReceiveSeq;
  int ReceivePtr;
  int ReceiveLength;
  byte ReceiveSum;
  static const int ErrorMessageSize = 256;
  char ErrorMessage[ErrorMessageSize];
  json_t *ClientUi;
  json_t *AllowClientList;

  struct ReadQueueSt {
    ReadQueueSt *Prev;
    ReadQueueSt *Next;
    byte *Buf;
    int Size;
    timeval ReceiveTime;
  };
  ReadQueueSt *FirstReadQueue;
  ReadQueueSt *LastReadQueue;

  struct WriteQueueSt {
    WriteQueueSt *Prev;
    WriteQueueSt *Next;
    int SequenceID;
    int State;
    int WaitMSec;
    int Retry;
    int SockFd;
    int ResOffset;
    byte *Buf;
    int Size;
    char *ExecCmd;
    char *CombiCmd;
    byte *Data;
    int DataSize;
    timeval SendTime;
    timeval NextTime;
    int (HA1Control::*NextFunc)(WriteQueueSt *p, ReadQueueSt *q);
    longword AddrL;
    unsigned long Param1;
    unsigned long Param2;
    unsigned long Param3;
  };
  WriteQueueSt *FirstWriteQueue;
  WriteQueueSt *LastWriteQueue;

  static const char *StatusStr[];

#ifdef ENABLE_IRREMOTE
  IRRemote IR;
#endif
   
  int    RainInfo;
  timeval RainInfoTime;

  timeval WakeupTime;
  timeval NextWakeupTime;
  timeval IntervalEventTime;
  timeval KeepAliveTime;
  timeval AliveCheckTime;

  int StatisticsLastMin;

};

static const int WatchDogInterval = 5;

#endif // __HA1Control_h__

