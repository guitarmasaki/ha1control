/*
 HA-1 HA1Control_Command.cc
 
 Copyright: Copyright (C) 2013 Mitsuru Nakada All Rights Reserved.
 License: GNU GPL v2
 */

#include "HA1Control.h"
#include "Command.h"

// Name, ExecFlag, Func
// ExecFlag [0] DeviceCommandFlag / [1] JSONIFFlag
const HA1Control::CommandFuncTableSt HA1Control::CommandFuncTable[] = {
  { "swctrl",         0b0011,  &HA1Control::SWControl    }, // swctrl <port 0-2>
  { "hactrl",         0b0011,  &HA1Control::HAControl    }, // hactrl <port 0-1> [open|off|close|on]
  { "gpoctrl",        0b0011,  &HA1Control::GPOCtrl      }, // gpoctrl <port 0-6>
  { "irsend",         0b0011,  &HA1Control::IRSend       }, // irsend <code> [<repeat>]
  { "led",            0b0011,  &HA1Control::SetLEDTape   }, // led [random|<24bit-hex>]
  { "keyled",         0b0011,  &HA1Control::KeypadLED    }, // keyled [on|off|auto]
  { "buzz",           0b0011,  &HA1Control::KeypadBuzz   }, // buzz <length x10ms>
  { "motor",          0b0011,  &HA1Control::MotorControl }, // motor <ch0> <speed0> <count0> ...<chN> <speedN> <countN> N<8
  { "setpw",          0b0011,  &HA1Control::SetPW        }, // setpw <gate> <customer> <passwd> <start(yyyy/mm/dd HH:MM)> <end(yyyy/mm/dd HH:MM)>
  { "virtualsw",      0b0010,  &HA1Control::VirtualSW    }, // virtualsw <port 0-31> [open|off|close|on]
  { "setrain",        0b0010,  &HA1Control::SetRain      }, // setrain
  // for human interface
  { "format",         0b0010,  &HA1Control::FormatChange }, // format [text|json]
  { "irrec",          0b0010,  &HA1Control::IRRecord     }, // irrec <name>
  { "devinfo",        0b0000,  &HA1Control::DeviceInfo   }, // devinfo
  { "wait",           0b0010,  &HA1Control::Wait         }, // wait <length x 1ms>
  { "quit",           0b0010,  &HA1Control::Quit         }, // quit
  { "exit",           0b0010,  &HA1Control::Quit         }, // exit
  { "stat",           0b0000,  &HA1Control::StatusCheck  }, // stat
  { "help",           0b0000,  &HA1Control::Help         }, // help
  // for debug
  { "queue",          0b0000,  &HA1Control::QueueList    }, // queue
  { "update",         0b0001,  &HA1Control::Update       }, // update [all|force]
  { "avrboot",        0b0001,  &HA1Control::AVRBoot      }, // avrboot
  { "avrread",        0b0001,  &HA1Control::AVRRead      }, // avrread <addr> [<size>]
  { "avrstat",        0b0001,  &HA1Control::AVRStat      }, // avrstat
  { "validate",       0b0001,  &HA1Control::Validate     }, // validate
  { NULL,             0b0000,  NULL                      }
};

const char *HA1Control::StatusStr[] = {
  "Open", "Close", "Off", "On", "Unknown", "Opening", "Closing", "Error"
};

int HA1Control::CommandFuncTableSearchByName(const char *name, longword execFlagMask) {

  for(int i = 0; CommandFuncTable[i].Name; i++) {
    if(((CommandFuncTable[i].ExecFlag & execFlagMask) || (execFlagMask == 0xffffffff)) &&
       !strcmp(name, CommandFuncTable[i].Name)) {
      return i;
    }
  }
  return Error;
}

// swctrl
int HA1Control::SWControl(int sockfd, int sequenceID, char *execCmd, char *combiCmd, longword addrL) {

#ifdef DEBUG
  LogPrintf("SWControl %d %08x %d %d\n", sockfd, addrL);
#endif

  int port = -1;
  char *p = strtok(NULL, " \t\r\n");
  if(p) port = strtoul(p, NULL, 10);
  if((port < 0) || (port > 2)) {
    ErrorPrintf("SWControl %08x %d : port number error", addrL, port);
    return Error;
  }

  DeviceList *devData = DeviceTable.SearchByAddrL(addrL);
  if(!devData) {
    ErrorPrintf("SWControl %08x : device error", addrL);
    return Error;
  }
  char name[32];
  devData->GetName(name);
  if(!devData->DeviceFuncSW()) {
    ErrorPrintf("SWControl %s : function error", name);
    return Error;
  }

  byte cmd[16];
  cmd[0] = CmdSWCtrl;
  cmd[1] = port;
  return SendAVRCommand(sockfd, sequenceID, execCmd, combiCmd, NULL, 0, addrL, cmd, 2);
}

// hactrl
int HA1Control::HAControl(int sockfd, int sequenceID, char *execCmd, char *combiCmd, longword addrL) {
  
#ifdef DEBUG
  LogPrintf("HAControl %d %08x %d %d\n", sockfd, addrL);
#endif
  
  int port = -1;
  char *p = strtok(NULL, " \t\r\n");
  if(p) port = strtoul(p, NULL, 10);
  if((port < 0) || (port > 1)) {
    ErrorPrintf("HAControl %08x %d : port number error", addrL, port);
    return Error;
  }
  
  p = strtok(NULL, " \t\r\n");
  if(!p) {
    ErrorPrintf("HAControl %08x : IllegalParameter", addrL);
    return Error;
  }
  int d = -1;
  if(!strcmp(p, "off")) d = 0;
  if(!strcmp(p, "open")) d = 0;
  if(!strcmp(p, "on")) d = 1;
  if(!strcmp(p, "close")) d = 1;
  if(d < 0) {
    ErrorPrintf("%s : IllegalParameter", p);
    return Error;
  }
  
  DeviceList *devData = DeviceTable.SearchByAddrL(addrL);
  if(!devData) {
    ErrorPrintf("HAControl %08x : device error", addrL);
    return Error;
  }
  char name[32];
  devData->GetName(name);
  if(((port == 0) && !devData->DeviceFuncHA0()) ||
     ((port == 1) && !devData->DeviceFuncHA1())) {
    ErrorPrintf("HAControl %s port%d %s: function error", name, port, p);
    return Error;
  }
  
  byte cmd[16];
  int cmdLen = 3;
  cmd[0] = CmdHACtrl;
  cmd[1] = port;
  cmd[2] = d;
  
  return SendAVRCommand(sockfd, sequenceID, execCmd, combiCmd, NULL, 0, addrL, cmd, cmdLen, NULL, 0, port);
}

// gpoctrl
int HA1Control::GPOCtrl(int sockfd, int sequenceID, char *execCmd, char *combiCmd, longword addrL) {
  
#ifdef DEBUG
  LogPrintf("GPOCtrl %d %08x %d %d\n", sockfd, addrL);
#endif
  
  int port = -1;
  char *p = strtok(NULL, " \t\r\n");
  if(p) port = strtoul(p, NULL, 10);
  if((port < 0) || (port > 6)) {
    ErrorPrintf("GPOCtrl %08x %d : port number error", port);
    return Error;
  }
  
  DeviceList *devData = DeviceTable.SearchByAddrL(addrL);
  if(!devData) {
    ErrorPrintf("GPOCtrl %08x : device error", addrL);
    return Error;
  }
  char name[32];
  devData->GetName(name);
  if(((port == 0) && !devData->DeviceFuncGPO0()) ||
     ((port == 1) && !devData->DeviceFuncGPO1()) ||
     ((port == 2) && !devData->DeviceFuncHA0O()) ||
     ((port == 3) && !devData->DeviceFuncHA1O()) ||
     ((port == 4) && !devData->DeviceFuncSW0O()) ||
     ((port == 5) && !devData->DeviceFuncSW1O()) ||
     ((port == 6) && !devData->DeviceFuncSW2O())) {
    ErrorPrintf("GPOCtrl %s %d : function error", name, port);
    return Error;
  }
   
  p = strtok(NULL, " \t\r\n");
  if(!p) {
    ErrorPrintf("GPOCtrl %s : value error", name);
    return Error;
  }
  int value = strtoul(p, NULL, 10);
  
  byte cmd[16];
  cmd[0] = CmdGPOCtrl;
  cmd[1] = port;
  cmd[2] = value & 1;
  return SendAVRCommand(sockfd, sequenceID, execCmd, combiCmd, NULL, 0, addrL, cmd, 3);
}

// irsend
int HA1Control::IRSend(int sockfd, int sequenceID, char *execCmd, char *combiCmd, longword addrL) {

#ifdef DEBUG
  LogPrintf("IRSend %d %08x %d\n", sockfd, addrL);
#endif

  char *p = strtok(NULL, " \t\r\n");
  if(!p) {
    ErrorPrintf("IRSend %08x : Parameter error", addrL);
    return Error;
  }
  char file[256];
  snprintf(file, 255, "%s%s.rmc", IRRemotePath, p);
  
  p = strtok(NULL, " \t\r\n");
  int repeat = 0;
  if(p) repeat = strtoul(p, NULL, 10);

  if((addrL == 0xffffffff) || !addrL) {
#ifdef ENABLE_IRREMOTE
    if(int error = IR.Send(file, repeat)) {
      ErrorPrintf("IRSend server : Send error");
      return error;
    }
    return Finish;
#else
    ErrorPrintf("IRSend : function error");
    return Error;
#endif
  }

  FILE *fp = fopen(file, "r");
  if(fp == NULL) {
    ErrorPrintf("IRSendAVR : fopen error %s", file);
    return Error;
  }
  byte data[1024];
  data[0] = repeat;
  int size = fread(data + 1, 1, 1024, fp);
  fclose(fp);
  if(size < 3) {
    fclose(fp);
    ErrorPrintf("IRSendAVR : size error");
    return Error;
  }
  size++;
  int (HA1Control::*nextFunc)(WriteQueueSt *p, ReadQueueSt *q) = NULL;
  int cmdSize = size;
  byte seqFlag = SeqFinal;
  if(cmdSize > 128) {
    nextFunc = &HA1Control::IRSend_Next;
    cmdSize = 128;
    seqFlag = 0;
  }
  byte cmd[128 + 2];
  cmd[0] = CmdIRSend;
  cmd[1] = 0 | seqFlag; // sequence No
  memcpy(cmd + 2, data, cmdSize);
  cmdSize += 2;
  return SendAVRCommand(sockfd, sequenceID, execCmd, combiCmd, data, size, addrL, cmd, cmdSize, nextFunc, 0, 0);
}

int HA1Control::IRSend_Next(WriteQueueSt *writeQueue, ReadQueueSt *readQueue) {
  
  // param1  sequence number
  writeQueue->Param1++;

  if(!writeQueue->DataSize || !writeQueue->Data) {
    ErrorPrintf("IRSendAVR %08x : DataSize Error", writeQueue->AddrL);
    return Error;
  }

  int (HA1Control::*nextFunc)(WriteQueueSt *p, ReadQueueSt *q) = NULL;
  int cmdSize = writeQueue->DataSize - 128 * writeQueue->Param1;
  byte seqFlag = SeqFinal;
  if(cmdSize > 128) {
    nextFunc = &HA1Control::IRSend_Next;
    cmdSize = 128;
    seqFlag = 0;
  }
  byte cmd[128 + 2];
  cmd[0] = CmdIRSend;
  cmd[1] = writeQueue->Param1 | seqFlag;
  memcpy(cmd + 2, writeQueue->Data + 128 * writeQueue->Param1, cmdSize);
  cmdSize += 2;

  return SendAVRCommand(writeQueue, cmd, cmdSize, nextFunc, 0);
}

// irrec
int HA1Control::IRRecord(int sockfd, int sequenceID, char *execCmd, char *combiCmd, longword addrL) {

#ifdef DEBUG
  LogPrintf("IRRecord %d %08x %d\n", sockfd, addrL);
#endif
  char *p = strtok(NULL, " \t\r\n");
  if(!p) {
    ErrorPrintf("Parameter error", execCmd);
    return Error;
  }

  char file[256];
  snprintf(file, 255, "%s.rmc", p);
  char path[256];
  strcpy(path, IRRemotePath);
  strcat(path, file);

  if((addrL != 0xffffffff) && addrL) {
    DeviceList *devData = DeviceTable.SearchByAddrL(addrL);
    char name[32];
    devData->GetName(name);
    if(!devData->DeviceFuncIRI()) {
      ErrorPrintf("IRRecord %s : function error", name);
      return Error;
    }
    strncpy(IRRecordFile, file, 255);
    IRRecordFile[255] = 0;
    IRRecordAddrL = addrL;
    IRRecordSockFd = sockfd;
    strcpy(IRRecordExecCmd, execCmd);
    gettimeofday(&IRRecordTimeout, NULL);
    IRRecordTimeout.tv_sec += 10;
    return Success;
  }

#ifdef ENABLE_IRREMOTE
  if(int error = IR.Record(path)) {
    ErrorPrintf("IRRecord : Record error");
    return error;
  }
#else
  ErrorPrintf("IRRecord : function error");
  return Error;
#endif
  byte buf[256];
  FILE *fp = fopen(path, "r");
  if(fp == NULL) {
    ErrorPrintf("IRRecord : open error %s", path);
    return Error;
  }
  int size = fread(buf, 1, 256, fp);
  file[strlen(file) - 4] = 0;
  RemoconTable.Add(file, buf, size);
  fclose(fp);
  return Finish;
}

// led
int HA1Control::SetLEDTape(int sockfd, int sequenceID, char *execCmd, char *combiCmd, longword addrL) {
  
#ifdef DEBUG
  LogPrintf("SetLEDTape %d %08x %d %d\n", sockfd, addrL);
#endif
  
  DeviceList *devData = DeviceTable.SearchByAddrL(addrL);
  if(!devData) {
    ErrorPrintf("SetLEDTape %08x : device not found error", addrL);
    return Error;
  }
  
  if(!devData->DeviceFuncLED()) {
    char name[32];
    devData->GetName(name);
    ErrorPrintf("SetLEDTape %s : function error", name);
    return Error;
  }
  
  char *p = strtok(NULL, "\r\n");
  if(!p) {
    ErrorPrintf("IllegalParameter");
    return Error;
  }
  longword color;
  if(!strcmp(p, "random")) {
    color = rand() & 0xffffff;
  } else {
    color = strtoul(p, NULL, 16);
  }
  byte cmd[16];
  cmd[0] = CmdLEDTape;
  cmd[1] = 30;
  cmd[2] = color >> 16;
  cmd[3] = color >> 8;
  cmd[4] = color;
  return SendAVRCommand(sockfd, sequenceID, execCmd, combiCmd, NULL, 0, addrL, cmd, 5);
}

// keyled
int HA1Control::KeypadLED(int sockfd, int sequenceID, char *execCmd, char *combiCmd, longword addrL) {
  
#ifdef DEBUG
  LogPrintf("KeypadLED %d %08x %d %d\n", sockfd, addrL);
#endif
  
  DeviceList *devData = DeviceTable.SearchByAddrL(addrL);
  if(!devData) {
    ErrorPrintf("KeypadLED : device error");
    return Error;
  }
  
  char name[32];
  devData->GetName(name);
  if(!(devData->DeviceFuncKPD())) {
    ErrorPrintf("KeypadLED %s : function error", name);
    return Error;
  }
  
  char *p = strtok(NULL, "\r\n");
  int d = -1;
  if(!strcmp(p, "off")) d = 0;
  if(!strcmp(p, "on")) d = 1;
  if(!strcmp(p, "auto")) d = 2;
  if(d < 0) {
    ErrorPrintf("KeypadLED %s : IllegalParameter", p);
    return Error;
  }
  byte cmd[16];
  cmd[0] = CmdKeypadCtrl;
  cmd[1] = d;
  cmd[2] = 0;
  return SendAVRCommand(sockfd, sequenceID, execCmd, combiCmd, NULL, 0, addrL, cmd, 3);
}

// buzz
int HA1Control::KeypadBuzz(int sockfd, int sequenceID, char *execCmd, char *combiCmd, longword addrL) {
  
#ifdef DEBUG
  LogPrintf("KeypadBuzz %d %08x %d %d\n", sockfd, addrL);
#endif
  
  DeviceList *devData = DeviceTable.SearchByAddrL(addrL);
  if(!devData) return Error;
  
  char name[32];
  devData->GetName(name);
  if(!(devData->DeviceFuncKPD())) {
    ErrorPrintf("KeypadBuzz %s : function error", name);
    return Error;
  }
  
  char *p = strtok(NULL, "\r\n");
  if(!p) {
    ErrorPrintf("KeypadBuzz %s : IllegalParameter", name);
    return Error;
  }
  int d = strtoul(p, NULL, 10);
  if((d <= 0) || (d > 255)) {
    ErrorPrintf("KeypadBuzz %s %d : IllegalParameter", name, d);
    return Error;
  }
  byte cmd[16];
  cmd[0] = CmdKeypadCtrl;
  cmd[1] = 3;
  cmd[2] = d;
  return SendAVRCommand(sockfd, sequenceID, execCmd, combiCmd, NULL, 0, addrL, cmd, 3);
}

// motor
int HA1Control::MotorControl(int sockfd, int sequenceID, char *execCmd, char *combiCmd, longword addrL) {
  
#ifdef DEBUG
  LogPrintf("MotorControl %d %08x %d %d\n", sockfd, addrL);
#endif
  
  DeviceList *devData = DeviceTable.SearchByAddrL(addrL);
  if(!devData) {
    ErrorPrintf("MotorControl : device error");
    return Error;
  }
  char name[32];
  devData->GetName(name);
  if(!devData->DeviceFuncMTR()) {
    ErrorPrintf("MotorControl %s : function error", name);
    return Error;
  }
  
  byte cmd[1 + 8 * 3];
  cmd[0] = CmdMotorCtrl;
  int i = 0;
  for(; i < 8; i++) {
    char *p = strtok(NULL, " \t\r\n");
    if(!p) break;
    
    int ch = strtoul(p, NULL, 10);
    if((ch < 0) || (ch > 3)) {
      ErrorPrintf("IllegalParameter : ch %d", ch);
      return Error;
    }
    cmd[1 + i * 3 + 0] = ch;
    
    p = strtok(NULL, " \t\r\n");
    if(!p) {
      ErrorPrintf("IllegalParameter : speed");
      return Error;
    }
    int speed = strtoul(p, NULL, 10);
    if((speed < 0) || (speed > 255)) {
      ErrorPrintf("IllegalParameter : speed %d", speed);
      return Error;
    }
    cmd[1 + i * 3 + 1] = speed;
    
    p = strtok(NULL, " \t\r\n");
    if(!p) {
      ErrorPrintf("IllegalParameter : count\n");
      return Error;
    }
    int count = strtoul(p, NULL, 10);
    if((count < 0) || (count > 255)) {
      ErrorPrintf("IllegalParameter : count %d", count);
      return Error;
    }
    cmd[1 + i * 3 + 2] = count;
  }
  if(!i) {
    ErrorPrintf("IllegalParameter : num");
    return Error;
  }
  
  return SendAVRCommand(sockfd, sequenceID, execCmd, combiCmd, NULL, 0, addrL, cmd, 1 + i * 3);
}

// avrboot
int HA1Control::AVRBoot(int sockfd, int sequenceID, char *execCmd, char *combiCmd, longword addrL) {
  
#ifdef DEBUG
  LogPrintf("AVRBoot %d %08x %d %d\n", sockfd, addrL);
#endif
  
  DeviceList *devData = DeviceTable.SearchByAddrL(addrL);
  if(!devData) {
    ErrorPrintf("AVRBoot %08x : device error", addrL);
    return Error;
  }
  if(!devData->DeviceFuncAVR()) {
    char name[32];
    devData->GetName(name);
    ErrorPrintf("AVRBoot %s : device error", name);
    return Error;
  }
  
  byte cmd[3];
  cmd[0] = 'D';
  cmd[1] = '0';
  cmd[2] = 4; // Output Low
  return SendATCommand(sockfd, sequenceID, execCmd, combiCmd, NULL, 0, addrL, cmd, 3, &HA1Control::AVRBootRes);
}

int HA1Control::AVRBootRes(WriteQueueSt *writeQueue, ReadQueueSt *readQueue) {
  
  byte cmd[3];
  cmd[0] = 'D';
  cmd[1] = '0';
  cmd[2] = 5; // Output High
  return SendATCommand(writeQueue, cmd, 3, NULL, 200);
}

// avrread
int HA1Control::AVRRead(int sockfd, int sequenceID, char *execCmd, char *combiCmd, longword addrL) {
  
#ifdef DEBUG
  LogPrintf("AVRRead %d %08x %d %d\n", sockfd, addrL);
#endif
  
  DeviceList *devData = DeviceTable.SearchByAddrL(addrL);
  if(!devData) {
    ErrorPrintf("AVRRead %08x : device error", addrL);
    return Error;
  }
  char name[32];
  devData->GetName(name);
  if(!devData->DeviceFuncAVR()) {
    ErrorPrintf("AVRRead %s : device error", name);
    return Error;
  }
  
  char *p = strtok(NULL, " \t\r\n");
  if(!p) return Error;
  int addr = strtoul(p, NULL, 16);
  
  int size = 32;
  p = strtok(NULL, " \t\r\n");
  if(p) size = strtoul(p, NULL, 16);
  
  byte cmd[16];
  cmd[0] = CmdReadMemory;
  cmd[1] = addr >> 8;
  cmd[2] = addr;
  int sz = size;
  if(size > 32) sz = 32;
  cmd[3] = sz;
  return SendAVRCommand(sockfd, sequenceID, execCmd, combiCmd, NULL, 0, addrL, cmd, 4, &HA1Control::AVRReadRes, 0, addr, size);
}

int HA1Control::AVRReadRes(WriteQueueSt *writeQueue, ReadQueueSt *readQueue) {
  
  // param1      addr
  // param2      size
  int addr = writeQueue->Param1;
  int size = writeQueue->Param2;
  int sz = size;
  if(sz > 32) sz = 32;
  for(int i = 0; i < sz; i++) {
    if((i % 16) == 0) ResPrintf(writeQueue->SockFd, "%04x: ", addr + i);
    ResPrintf(writeQueue->SockFd, "%02x ", readQueue->Buf[writeQueue->ResOffset + 2 + i]);
    if((i % 16) == 15) ResPrintf(writeQueue->SockFd, "\n");
  }
  if(sz % 16) ResPrintf(writeQueue->SockFd, "\n");
  addr += sz;
  size -= sz;
  sz = size;
  if(sz > 32) sz = 32;
  if(size) {
    byte cmd[16];
    cmd[0] = CmdReadMemory;
    cmd[1] = addr >> 8;
    cmd[2] = addr;
    cmd[3] = sz;
    writeQueue->Param1 = addr;
    writeQueue->Param2 = size;
    return SendAVRCommand(writeQueue, cmd, 4, &HA1Control::AVRReadRes, 0);
  }
  return Finish;
}

// avrstat
int HA1Control::AVRStat(int sockfd, int sequenceID, char *execCmd, char *combiCmd, longword addrL) {
  
#ifdef DEBUG
  LogPrintf("AVRStat %d %08x %d %d\n", sockfd, addrL);
#endif
  
  DeviceList *devData = DeviceTable.SearchByAddrL(addrL);
  if(!devData) {
    ErrorPrintf("AVRStat %08x : device error", addrL);
    return Error;
  }
  char name[32];
  devData->GetName(name);
  if(!devData->DeviceFuncAVR()) {
    ErrorPrintf("AVRStat %s : device error", name);
    return Error;
  }
  
  byte cmd[16];
  cmd[0] = CmdGetStatus;
  return SendAVRCommand(sockfd, sequenceID, execCmd, combiCmd, NULL, 0, addrL, cmd, 1);
}

// validate
int HA1Control::Validate(int sockfd, int sequenceID, char *execCmd, char *combiCmd, longword addrL) {
  
#ifdef DEBUG
  LogPrintf("Validate %d %08x %d %d\n", sockfd, addrL);
#endif
  
  DeviceList *devData = DeviceTable.SearchByAddrL(addrL);
  if(!devData) {
    ErrorPrintf("Validate %08x : device error", addrL);
    return Error;
  }
  char name[32];
  devData->GetName(name);
  if(!devData->DeviceFuncAVR()) {
    ErrorPrintf("Validate %s : device error", name);
    return Error;
  }
  
  byte cmd[16];
  cmd[0] = CmdValidateHA2;
  return SendAVRCommand(sockfd, sequenceID, execCmd, combiCmd, NULL, 0, addrL, cmd, 1, &HA1Control::ValidateRes);
}

int HA1Control::ValidateRes(WriteQueueSt *writeQueue, ReadQueueSt *readQueue) {
  
#define COLOR_RED "\033[31m"
#define COLOR_YELLOW "\033[33m"
#define COLOR_DEFAULT "\033[0m"

  longword stat = (readQueue->Buf[writeQueue->ResOffset + 2] << 24) |
                  (readQueue->Buf[writeQueue->ResOffset + 3] << 16) |
                  (readQueue->Buf[writeQueue->ResOffset + 4] << 8) |
                   readQueue->Buf[writeQueue->ResOffset + 5];
  if(stat) ResPrintf(writeQueue->SockFd, COLOR_RED "Status = %08x\n" COLOR_DEFAULT, stat);
  if(stat & (1 << 0)) {
    ResPrintf(writeQueue->SockFd, COLOR_RED " 0: GPIO1(PC4)=Hi, GPIO2(PC5)=Low -> AD1(ADC6)=%dmV (%dmV - %dmV, AD2=%dmV)\n" COLOR_DEFAULT, ((readQueue->Buf[writeQueue->ResOffset + 6] << 8) | readQueue->Buf[writeQueue->ResOffset + 7]) * 3300 / 4096, 918 * 3300 / 4096, 992 * 3300 / 4096, ((readQueue->Buf[writeQueue->ResOffset + 8] << 8) | readQueue->Buf[writeQueue->ResOffset + 9]) * 3300 / 4096);
    ResPrintf(writeQueue->SockFd, COLOR_YELLOW "U202:27-CN202:9, U202:19-CN202:3, VS202, VS206, U201:19, U201:17\n" COLOR_DEFAULT);
  }
  if(stat & (1 << 1)) {
    ResPrintf(writeQueue->SockFd, COLOR_RED " 1: GPIO1(PC4)=Hi, GPIO2(PC5)=Low -> AD2(ADC7)=%dmV (%dmV - %dmV, AD1=%dmV)\n" COLOR_DEFAULT, ((readQueue->Buf[writeQueue->ResOffset + 8] << 8) | readQueue->Buf[writeQueue->ResOffset + 9]) * 3300 / 4096, 0, 7 * 3300 / 4096,  ((readQueue->Buf[writeQueue->ResOffset + 6] << 8) | readQueue->Buf[writeQueue->ResOffset + 7]) * 3300 / 4096);
    ResPrintf(writeQueue->SockFd, COLOR_YELLOW "U202:28-CN202:11, U202:22-CN202:5, VS201, VS204, U201:18, U201:11\n" COLOR_DEFAULT);
  }
  if(stat & (1 << 2)) {
    ResPrintf(writeQueue->SockFd, COLOR_RED " 2: GPIO1(PC4)=Low, GPIO2(PC5)=Hi -> AD1(ADC6)=%dmV (%dmV - %dmV, AD2=%dmV)\n" COLOR_DEFAULT, ((readQueue->Buf[writeQueue->ResOffset + 10] << 8) | readQueue->Buf[writeQueue->ResOffset + 11]) * 3300 / 4096, 0, 7 * 3300 / 4096, ((readQueue->Buf[writeQueue->ResOffset + 12] << 8) | readQueue->Buf[writeQueue->ResOffset + 13]) * 3300 / 4096);
    ResPrintf(writeQueue->SockFd, COLOR_YELLOW "U202:27-CN202:9, U202:19-CN202:3, VS202, VS206, U201:19, U201:17\n" COLOR_DEFAULT);
  }
  if(stat & (1 << 3)) {
    ResPrintf(writeQueue->SockFd, COLOR_RED " 3: GPIO1(PC4)=Low, GPIO2(PC5)=Hi -> AD2(ADC7)=%dmV (%dmV - %dmV, AD1=%dmV)\n" COLOR_DEFAULT, ((readQueue->Buf[writeQueue->ResOffset + 12] << 8) | readQueue->Buf[writeQueue->ResOffset + 13]) * 3300 / 4096, 918 * 3300 / 4096, 992 * 3300 / 4096,  ((readQueue->Buf[writeQueue->ResOffset + 10] << 8) | readQueue->Buf[writeQueue->ResOffset + 11]) * 3300 / 4096);
    ResPrintf(writeQueue->SockFd, COLOR_YELLOW "U202:28-CN202:11, U202:22-CN202:5, VS201, VS204, U201:18, U201:11\n" COLOR_DEFAULT);
  }

  if(stat & (1 << 4)) {
    ResPrintf(writeQueue->SockFd, COLOR_RED " 4: SW0(PD2)=Low, SW1(PD3)=Low, SW2(PD4)=Low -> SW0(PB0)=Hi\n" COLOR_DEFAULT);
    ResPrintf(writeQueue->SockFd, COLOR_YELLOW "U202:32-R213-Q203:5,4,3-R204-CN202:2-Q203:1,2,6-U202:12, VS207, C208, U201:6, U201:15\n" COLOR_DEFAULT);
  }
  if(stat & (1 << 5)) {
    ResPrintf(writeQueue->SockFd, COLOR_RED " 5: SW0(PD2)=Low, SW1(PD3)=Low, SW2(PD4)=Low -> SW1(PB1)=Hi\n" COLOR_DEFAULT);
    ResPrintf(writeQueue->SockFd, COLOR_YELLOW "U202:1-R214-Q204:5,4,3-R205-CN202:4-Q204:1,2,6-U202:13, VS205, C209, U201:7, U201:16\n" COLOR_DEFAULT);
  }
  if(stat & (1 << 6)) {
    ResPrintf(writeQueue->SockFd, COLOR_RED " 6: SW0(PD2)=Low, SW1(PD3)=Low, SW2(PD4)=Low -> SW2(PB2)=Hi\n" COLOR_DEFAULT);
    ResPrintf(writeQueue->SockFd, COLOR_YELLOW "U202:2-R215-Q205:5,4,3-R206-CN202:6-Q205:1,2,6-U202:14, VS203, C210, U201:4, U201:12\n" COLOR_DEFAULT);
  }

  if(stat & (1 << 7)) {
    ResPrintf(writeQueue->SockFd, COLOR_RED " 7: SW0(PD2)=Hi, SW1(PD3)=Low, SW2(PD4)=Low -> SW0(PB0)=Low\n" COLOR_DEFAULT);
    ResPrintf(writeQueue->SockFd, COLOR_YELLOW "U202:32-R213-Q203:5,4,3-R204-CN202:2-Q203:1,2,6-U202:12, VS207, C208, U201:6, U201:15\n" COLOR_DEFAULT);
  }
  if(stat & (1 << 8)) {
    ResPrintf(writeQueue->SockFd, COLOR_RED " 8: SW0(PD2)=Hi, SW1(PD3)=Low, SW2(PD4)=Low -> SW1(PB1)=Hi\n" COLOR_DEFAULT);
    ResPrintf(writeQueue->SockFd, COLOR_YELLOW "U202:1-R214-Q204:5,4,3-R205-CN202:4-Q204:1,2,6-U202:13, VS205, C209, U201:7, U201:16\n" COLOR_DEFAULT);
  }
  if(stat & (1 << 9)) {
    ResPrintf(writeQueue->SockFd, COLOR_RED " 9: SW0(PD2)=Hi, SW1(PD3)=Low, SW2(PD4)=Low -> SW2(PB2)=Hi\n" COLOR_DEFAULT);
    ResPrintf(writeQueue->SockFd, COLOR_YELLOW "U202:2-R215-Q205:5,4,3-R206-CN202:6-Q205:1,2,6-U202:14, VS203, C210, U201:4, U201:12\n" COLOR_DEFAULT);
  }

  if(stat & (1 << 10)) {
    ResPrintf(writeQueue->SockFd, COLOR_RED "10: SW0(PD2)=Low, SW1(PD3)=Hi, SW2(PD4)=Low -> SW0(PB0)=Hi\n" COLOR_DEFAULT);
    ResPrintf(writeQueue->SockFd, COLOR_YELLOW "U202:32-R213-Q203:5,4,3-R204-CN202:2-Q203:1,2,6-U202:12, VS207, C208, U201:6, U201:15\n" COLOR_DEFAULT);
  }
  if(stat & (1 << 11)) {
    ResPrintf(writeQueue->SockFd, COLOR_RED "11: SW0(PD2)=Low, SW1(PD3)=Hi, SW2(PD4)=Low -> SW1(PB1)=Low\n" COLOR_DEFAULT);
    ResPrintf(writeQueue->SockFd, COLOR_YELLOW "U202:1-R214-Q204:5,4,3-R205-CN202:4-Q204:1,2,6-U202:13, VS205, C209, U201:7, U201:16\n" COLOR_DEFAULT);
  }
  if(stat & (1 << 12)) {
    ResPrintf(writeQueue->SockFd, COLOR_RED "12: SW0(PD2)=Low, SW1(PD3)=Hi, SW2(PD4)=Low -> SW2(PB2)=Hi\n" COLOR_DEFAULT);
    ResPrintf(writeQueue->SockFd, COLOR_YELLOW "U202:2-R215-Q205:5,4,3-R206-CN202:6-Q205:1,2,6-U202:14, VS203, C210, U201:4, U201:12\n" COLOR_DEFAULT);
  }

  if(stat & (1 << 13)) {
    ResPrintf(writeQueue->SockFd, COLOR_RED "13: SW0(PD2)=Low, SW1(PD3)=Low, SW2(PD4)=Hi -> SW0(PB0)=Hi\n" COLOR_DEFAULT);
    ResPrintf(writeQueue->SockFd, COLOR_YELLOW "U202:32-R213-Q203:5,4,3-R204-CN202:2-Q203:1,2,6-U202:12, VS207, C208, U201:6, U201:15\n" COLOR_DEFAULT);
  }
  if(stat & (1 << 14)) {
    ResPrintf(writeQueue->SockFd, COLOR_RED "14: SW0(PD2)=Low, SW1(PD3)=Low, SW2(PD4)=Hi -> SW1(PB1)=Hi\n" COLOR_DEFAULT);
    ResPrintf(writeQueue->SockFd, COLOR_YELLOW "U202:1-R214-Q204:5,4,3-R205-CN202:4-Q204:1,2,6-U202:13, VS205, C209, U201:7, U201:16\n" COLOR_DEFAULT);
  }
  if(stat & (1 << 15)) {
    ResPrintf(writeQueue->SockFd, COLOR_RED "15: SW0(PD2)=Low, SW1(PD3)=Low, SW2(PD4)=Hi -> SW2(PB2)=Low\n" COLOR_DEFAULT);
    ResPrintf(writeQueue->SockFd, COLOR_YELLOW "U202:2-R215-Q205:5,4,3-R206-CN202:6-Q205:1,2,6-U202:14, VS203, C210, U201:4, U201:12\n" COLOR_DEFAULT);
  }
  if(stat & (1 << 16)) {
    ResPrintf(writeQueue->SockFd, COLOR_RED "16: SW0(PD2)=Low, SW1(PD3)=Low, SW2(PD4)=Hi -> IRI(PD6)=Hi\n" COLOR_DEFAULT);
    ResPrintf(writeQueue->SockFd, COLOR_YELLOW "U202:32-R213-Q203:5,4,3-R204-CN202:2-CN202:12-U202:10 R207, VS207, C208, U201:6\n" COLOR_DEFAULT);
  }
  if(stat & (1 << 17)) {
    ResPrintf(writeQueue->SockFd, COLOR_RED "17: SW0(PD2)=Hi, SW1(PD3)=Low, SW2(PD4)=Low -> IRI(PD6)=Low\n" COLOR_DEFAULT);
    ResPrintf(writeQueue->SockFd, COLOR_YELLOW "U202:32-R213-Q203:5,4,3-R204-CN202:2-CN202:12-U202:10 R207, VS207, C208, U201:6\n" COLOR_DEFAULT);
  }
  if(stat & (1 << 18)) {
    ResPrintf(writeQueue->SockFd, COLOR_RED "18: IRO(PD5)=Low -> SW2(PB2)=Hi\n" COLOR_DEFAULT);
    ResPrintf(writeQueue->SockFd, COLOR_YELLOW "U202:9-Q202:1,2,6-R208-CN202:14, CN202:6-R206-Q205:1,2,6-U202:14, VS203, U201:15\n" COLOR_DEFAULT);
  }
  if(stat & (1 << 19)) {
    ResPrintf(writeQueue->SockFd, COLOR_RED "19: IRO(PD5)=Hi -> SW2(PB2)=Low\n" COLOR_DEFAULT);
    ResPrintf(writeQueue->SockFd, COLOR_YELLOW "U202:9-Q202:1,2,6-R208-CN202:14, CN202:6-R206-Q205:1,2,6-U202:14, VS203, U201:15\n" COLOR_DEFAULT);
  }
  if(stat & (1 << 20)) {
    ResPrintf(writeQueue->SockFd, COLOR_RED "20: HA1C(PC0)=Hi, HA2C(PC2)=Low -> HA1M(PC1) = Low\n" COLOR_DEFAULT);
    ResPrintf(writeQueue->SockFd, COLOR_YELLOW "U202:23-Q201:2,1,6-R202-U204:3,4,13,14-D201-R209-CN202:15,17, CN202:19,21-R210-U204:1,2,15,16-R217-U202:24\n" COLOR_DEFAULT);
  }
  if(stat & (1 << 21)) {
    ResPrintf(writeQueue->SockFd, COLOR_RED "21: HA1C(PC0)=Hi, HA2C(PC2)=Low -> HA2M(PC3) = Hi\n" COLOR_DEFAULT);
    ResPrintf(writeQueue->SockFd, COLOR_YELLOW "U202:25-Q201:5,4,3-R203-U204:5,6,11,12-D202-R211-CN202:16,18, CN202:20,22-R212-U204:7,8,9,10-R216-U202:26\n" COLOR_DEFAULT);
  }
  if(stat & (1 << 22)) {
    ResPrintf(writeQueue->SockFd, COLOR_RED "22: HA1C(PC0)=Low, HA2C(PC2)=Hi -> HA1M(PC1) = Hi\n" COLOR_DEFAULT);
    ResPrintf(writeQueue->SockFd, COLOR_YELLOW "U202:23-Q201:2,1,6-R202-U204:3,4,13,14-D201-R209-CN202:15,17, CN202:19,21-R210-U204:1,2,15,16-R217-U202:24\n" COLOR_DEFAULT);
  }
  if(stat & (1 << 23)) {
    ResPrintf(writeQueue->SockFd, COLOR_RED "23: HA1C(PC0)=Low, HA2C(PC2)=Hi -> HA2M(PC3) = Low\n" COLOR_DEFAULT);
    ResPrintf(writeQueue->SockFd, COLOR_YELLOW "U202:25-Q201:5,4,3-R203-U204:5,6,11,12-D202-R211-CN202:16,18, CN202:20,22-R212-U204:7,8,9,10-R216-U202:26\n" COLOR_DEFAULT);
  }
  if(!stat) ResPrintf(writeQueue->SockFd, "AVR Validate command -> OK\n");
  return Finish;
}

//setpw
int HA1Control::SetPW(int sockfd, int sequenceID, char *execCmd, char *combiCmd, longword addrL) {
  
#ifdef DEBUG
  LogPrintf("SetPW %d\n", sockfd);
#endif

  char *gateName = strtok(NULL, " \t\r\n");
  if(!gateName || GateTable.SearchByName(gateName)) {
    if(gateName) {
      ErrorPrintf("SetPW : gate name is not found %s", gateName);
    } else {
      ErrorPrintf("SetPW : parameter error");
    }
    return Error;
  }
  
  char *customerName = strtok(NULL, " \t\r\n");
  if(!customerName) {
    ErrorPrintf("SetPW : customer name error");
    return Error;
  }
  char *password = strtok(NULL, " \t\r\n");
  if(!password || (strlen(password) < 4)) {
    ErrorPrintf("SetPW : password error");
    return Error;
  }
  if(!strcmp(password, "delete")) {
    int error = PasswordTable.Update(gateName, customerName, "000000", 0, 0);
    if(error) {
      ErrorPrintf("SetPW : PasswordTable Delete Error");
      return error;
    }
    PasswordCheck();
    return Success;
  }
  for(int i = 0; i < strlen(password); i++) {
    if((password[i] < '0') || (password[i] > '9')) {
      ErrorPrintf("SetPW : password error");
      return Error;
    }
  }
  char *sday = strtok(NULL, " \t\r\n");
  char *stime = strtok(NULL, " \t\r\n");
  if(!sday || !stime) {
    ErrorPrintf("SetPW : Start date&time error");
    return Error;
  }
  char buf[256];
  strcpy(buf, sday);
  strcat(buf, " ");
  strcat(buf, stime);
  struct tm tm;
  memset(&tm, 0, sizeof(struct tm));
  char *p = strptime(buf, "%Y/%m/%d %R", &tm);
  if(!p || *p) {
    ErrorPrintf("SetPW : Start date&time format error");
    return Error;
  }
  longword startValidity = mktime(&tm);
  
  char *eday = strtok(NULL, " \t\r\n");
  char *etime = strtok(NULL, " \t\r\n");
  if(!eday || !etime) {
    ErrorPrintf("SetPW : End date&time error");
    return Error;
  }
  strcpy(buf, eday);
  strcat(buf, " ");
  strcat(buf, etime);
  p = strptime(buf, "%Y/%m/%d %R", &tm);
  if(!p || *p) {
    ErrorPrintf("SetPW : End date&time format error");
    return Error;
  }
  longword endValidity = mktime(&tm);
  
  int error = PasswordTable.Update(gateName, customerName, password, startValidity, endValidity);
  if(error) {
    ErrorPrintf("SetPW : PasswordTable Update Error");
    return error;
  }
  PasswordCheck();
  return Success;
}

// devinfo
int HA1Control::DeviceInfo(int sockfd, int sequenceID, char *execCmd, char *combiCmd, longword addrL) {
  
#ifdef DEBUG
  LogPrintf("DeviceInfo %d\n", sockfd);
#endif

  ResPrintf(sockfd, "%s :\n", execCmd);

  char *p = strtok(NULL, "\r\n");
  int debugFlag = 0;
  if(p && !strcmp(p, "debug")) debugFlag = 1;

  DeviceList *devData = &DeviceTable;
  while((devData = devData->NextList()) != NULL) {
    char name[32];
    devData->GetName(name);
    if(!strcmp(name, "server")) continue;
    
    struct timeval tm;
    gettimeofday(&tm, NULL);
    tm.tv_sec -= AliveCheckTimeout;
    int alive = 0;
    timeval lastAccess;
    devData->GetLastAccess(&lastAccess);
    if(timercmp(&tm, &lastAccess, <)) alive |= 1;
    devData->GetLastAVRAccess(&lastAccess);
    if(timercmp(&tm, &lastAccess, <)) alive |= 1;
    char type[32];
    devData->GetType(type);
    if(!strlen(type)) strcpy(type, "--------");
    ResPrintf(sockfd, "%-16.16s %-8.8s %08x %04x f:%08x v:%04x:%04x p:%4dmV : %s\n", name, type, devData->GetAddrL(), devData->GetNetworkAddr(), devData->GetFunc(), devData->GetFWVer(), devData->GetBootVer(), devData->GetVoltage(), alive?"alive":"dead");
    if(debugFlag) {
      ResPrintf(sockfd, "  Stat %02x/%02x Digital %04x/%04x Analog %04x %04x\n",
        devData->GetStat(), devData->GetLastStat(),
        devData->GetDigital(), devData->GetLastDigital(),
        devData->GetAnalog(4), devData->GetAnalog(5));
    }
  }
  return Finish;
}

// stat
int HA1Control::StatusCheck(int sockfd, int sequenceID, char *execCmd, char *combiCmd, longword addrL) {

#ifdef DEBUG
  LogPrintf("StatusCheck %d\n", sockfd);
#endif

  ResPrintf(sockfd, "StatusCheck :\n");
  void *savePtr = NULL;
  char cmd[32];
  longword cmdAddrL;
  int portNo;
  int funcNo;
  while(!StatusTable.GetEntry(&savePtr, cmd, &cmdAddrL, &portNo, &funcNo)) {

    int stat;
    char str[256];
    char type[256];
    if(!GetCondition(cmd, &stat, NULL, str, type)) {
      if(!strcmp(type, "humidity")) strcat(str, "%");
      if(!strcmp(type, "temp")) strcat(str, "℃");
      if(!strcmp(type, "battery")) strcat(str, "mV");
      ResPrintf(sockfd, "%-32s : %-10s\n", cmd, str);
    } else {
      ResPrintf(sockfd, "%-32s : error\n", cmd);
    }
  }
  return Finish;
}

// queue
int HA1Control::QueueList(int sockfd, int sequenceID, char *execCmd, char *combiCmd, longword addrL) {
  
#ifdef DEBUG
  LogPrintf("QueueList %d\n", sockfd);
#endif
  
  int writeQueueCount = 0;
  WriteQueueSt *p = FirstWriteQueue;
  ResPrintf(sockfd, "WriteQueue :\n");
  while(p) {
    writeQueueCount++;
    char sendTimeBuf[256];
    strftime(sendTimeBuf, 255, "%F %T", localtime(&p->SendTime.tv_sec));
    char nextTimeBuf[256];
    strftime(nextTimeBuf, 255, "%F %T", localtime(&p->SendTime.tv_sec));
    ResPrintf(sockfd, "%s : sendTime %s.%03d wait %dms nextTime %s.%03d SeqID %d State %d\n", p->ExecCmd, sendTimeBuf, p->SendTime.tv_usec / 1000, p->WaitMSec, nextTimeBuf, p->NextTime.tv_usec / 1000, p->SequenceID, p->State);
    for(int i = 0; i < p->Size; i++) {
      ResPrintf(sockfd, "%02x ", p->Buf[i]);
    }
    ResPrintf(sockfd, "\n");
    p = p->Next;
  }
  
  int readQueueCount = 0;
  ReadQueueSt *q = FirstReadQueue;
  ResPrintf(sockfd, "ReadQueue :\n");
  while(q) {
    readQueueCount++;
    char rcvTimeBuf[256];
    strftime(rcvTimeBuf, 255, "%F %T", localtime(&q->ReceiveTime.tv_sec));
    ResPrintf(sockfd, "ReceiveTime %s.%3d\n", rcvTimeBuf, q->ReceiveTime.tv_usec / 1000);
    for(int i = 0; i < q->Size; i++) {
      ResPrintf(sockfd, "%02x ", q->Buf[i]);
    }
    ResPrintf(sockfd, "\n");
    q = q->Next;
  }
  ResPrintf(sockfd, "WriteQueue %d ReadQueue %d\n", writeQueueCount, readQueueCount);
  return Finish;
}

// setrain
int HA1Control::SetRain(int sockfd, int sequenceID, char *execCmd, char *combiCmd, longword addrL) {

  char *p = strtok(NULL, " \t\r\n");
  if(!p) return Error;
  RainInfo = strtoul(p, NULL, 10);
  return Finish;
}

//update
int HA1Control::Update(int sockfd, int sequenceID, char *execCmd, char *combiCmd, longword addrL) {

#ifdef DEBUG
  LogPrintf("Update %d\n", sockfd);
#endif
  char *flag = strtok(NULL, " \t\r\n");

  if(FWReadFile()) {
    ErrorPrintf("FWReadFile Error");
    return Error;
  }
  if((addrL == 0xffffffff) && flag && (!strcmp(flag, "all") || !strcmp(flag, "force"))) {
    DeviceList *devData = &DeviceTable;
    while((devData = devData->NextList()) != NULL) {
      if(devData->DeviceFuncAVR()) {
        if(!strcmp(flag, "force")) {
          devData->SetFWUpdateFlag(1);
        } else {
          devData->SetFWUpdateFlag(0);
        }
        devData->ClearLastDeviceCheck();
      }
    }
  } else {
    DeviceList *devData = DeviceTable.SearchByAddrL(addrL);
    if(!devData) return Error;
    if(!devData->DeviceFuncAVR()) {
      ErrorPrintf("update %s : function error", flag);
      return Error;
    }
    devData->SetFWUpdateFlag(1);
    devData->ClearLastDeviceCheck();
  }
  FWUpdateCheck();
  return Finish;
}

// virtualsw
int HA1Control::VirtualSW(int sockfd, int sequenceID, char *execCmd, char *combiCmd, longword addrL) {
  
  char *p = strtok(NULL, " \t\r\n");
  if(!p) {
    ErrorPrintf("IllegalParameter");
    return Error;
  }
  int sw = strtoul(p, NULL, 10);
  if((sw < 0) || (sw > 15)) {
    ErrorPrintf("IllegalParameter");
    return Error;
  }

  p = strtok(NULL, " \t\r\n");
  if(!p) {
    ErrorPrintf("HAControl %08x : IllegalParameter", addrL);
    return Error;
  }
  int d = -1;
  if(!strcmp(p, "off")) d = 0;
  if(!strcmp(p, "open")) d = 0;
  if(!strcmp(p, "on")) d = 1;
  if(!strcmp(p, "close")) d = 1;
  if(d < 0) {
    ErrorPrintf("%s : IllegalParameter", p);
    return Error;
  }

  SendResponse(sockfd, execCmd, Success);
  DeviceList *devData = DeviceTable.SearchByName("server");
  word v = devData->GetDigital();
  int a[4];
  devData->SetData((v & ~(1 << sw)) | (d << sw), a, 0);
  DeviceTable.UpdateStatus();
  StatusPostByDevice(devData->GetAddrL());
  return NoRes;
}

// format
int HA1Control::FormatChange(int sockfd, int sequenceID, char *execCmd, char *combiCmd, longword addrL) {
  
  char *p = strtok(NULL, "\r\n");
  FormatSt f = Format_TEXT;
  if(!p || (strcmp(p, "text") && strcmp(p, "json"))) {
    ErrorPrintf("IllegalParameter");
    return Error;
  }
  if(!strcmp(p, "json")) f = Format_JSON;

  SendResponse(sockfd, execCmd, Success);
  ClientInfo[sockfd].Format = f;
  if(f == Format_JSON) SendNotify("client_ui", ClientUi);
  return NoRes;
}

// help
int HA1Control::Help(int sockfd, int sequenceID, char *execCmd, char *combiCmd, longword addrL) {

  // CommandTable
  ResPrintf(sockfd, "----- Command -----\n");
  void *savePtr = NULL;
  char cmd[32];
  while(!CommandTable.GetEntry(&savePtr, cmd)) {
    ResPrintf(sockfd, "%s\n", cmd);
  }
  
  // direct function
  for(int i = 0; CommandFuncTable[i].Name; i++) {
    ResPrintf(sockfd, "%s\n", CommandFuncTable[i].Name);
  }

  // remote AVR remocon command
  ResPrintf(sockfd, "----- Remocon Device -----\n");
  DeviceList *devData = &DeviceTable;
  while((devData = devData->NextList()) != NULL) {
    if(devData->DeviceFuncIRO()) {
      char name[32];
      devData->GetName(name);
      ResPrintf(sockfd, "%s_*\n",name);
    }
  }
       
  // remocon command
  ResPrintf(sockfd, "----- Remocon Command -----\n");
  savePtr = NULL;
  char remocon[256];
  while(!RemoconTable.GetEntry(&savePtr, remocon)) {
    ResPrintf(sockfd, "%s\n", remocon);
  }

  return Finish;
}

// quit, exit
int HA1Control::Quit(int sockfd, int sequenceID, char *execCmd, char *combiCmd, longword addrL) {

  SendResponse(sockfd, execCmd, Success);
  return Disconnect;
}

// wait
int HA1Control::Wait(int sockfd, int sequenceID, char *execCmd, char *combiCmd, longword addrL) {
  
  char *p = strtok(NULL, "\r\n");
  if(!p) {
    ErrorPrintf("IllegalParameter");
    return Error;
  }
  int msec = strtoul(p, NULL, 10);

  return AddWriteQueue(sockfd, sequenceID, 0, execCmd, combiCmd, NULL, 0, 0xffffffff, NULL, 0, 0, NULL, msec);
}

// SendATCommand
int HA1Control::ATCmdDump(WriteQueueSt *writeQueue, ReadQueueSt *readQueue) {
  
  int p = writeQueue->ResOffset + 2;
  if(readQueue->Buf[p] != 0) {
    ResPrintf(writeQueue->SockFd, "%s : Error %d\n", writeQueue->ExecCmd, readQueue->Buf[p]);
    return Error;
  }
  p++;
  if(p < readQueue->Size) {
    ResPrintf(writeQueue->SockFd, "%s : ", writeQueue->ExecCmd);
    for(; p < readQueue->Size; p++) {
      ResPrintf(writeQueue->SockFd, "%02x ", readQueue->Buf[p]);
    }
    ResPrintf(writeQueue->SockFd, "\n");
  }
  return Finish;
}

