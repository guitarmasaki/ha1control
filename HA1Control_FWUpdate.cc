/*
 HA-1 HA1Control_FWUpdate.cc
 
 Copyright: Copyright (C) 2013 Mitsuru Nakada All Rights Reserved.
 License: GNU GPL v2
 */

#include <sys/stat.h>

#include "HA1Control.h"
#include "Command.h"
#include "CRC.h"

int HA1Control::FWReadFile() {

  struct stat st;
  if(stat(FWPath, &st)) return Error;

  int size = st.st_size - sizeof(FWHeaderSt);
  FWSeqNum = (size + 127) / 128;
  size = FWSeqNum * 128 + sizeof(FWHeaderSt);

  if(FWUpdateBuf) delete [] FWUpdateBuf;
  FWUpdateBuf = new byte [size];
  if(!FWUpdateBuf) {
    LogPrintf("FWReadFile new error\n");
    return Error;
  }
  FILE *fp = fopen(FWPath, "r");
  if(fp == NULL) {
    LogPrintf("FWReadFile fopen error\n");
    delete [] FWUpdateBuf;
    FWUpdateBuf = NULL;
    return Error;
  }
  fread(FWUpdateBuf, 1, st.st_size, fp);
  fclose(fp);
  for(int i = st.st_size; i < size; i++) FWUpdateBuf[i] = 0xff;
  FWHeader = (FWHeaderSt *)FWUpdateBuf;
  if(BeToCpuLongword(FWHeader->Magic) != 0x48415550) { // HAUP
    LogPrintf("FWReadFile Magic error\n");
    delete [] FWUpdateBuf;
    FWUpdateBuf = NULL;
    return Error;
  }
  CRC crc;
  word value = crc.CalcCRC(FWUpdateBuf + sizeof(FWHeaderSt), BeToCpuWord(FWHeader->Size));
  if(BeToCpuWord(FWHeader->CRC) != value) {
    LogPrintf("CRC error\n");
    delete [] FWUpdateBuf;
    FWUpdateBuf = NULL;
    return Error;
  }
  return Success;
}

void HA1Control::FWUpdateCheck() {

#ifdef DEBUG
  LogPrintf("FWUpdateCheck\n");
#endif

  if(!FWHeader || !FWUpdateBuf) return;

  if(!FWUpdateDevicePtr) FWUpdateDevicePtr = &DeviceTable;
  while(1) {
    if((FWUpdateDevicePtr = FWUpdateDevicePtr->NextList()) != NULL) {
      if(FWUpdateDevicePtr->GetFWUpdateFlag() > 1) continue;
      if((FWUpdateDevicePtr->GetFWUpdateFlag() == 1) && !FWUpdateDevicePtr->GetFWVer()) FWUpdateDevicePtr->SetFWUpdateFlag(0);
      if(!FWUpdateDevicePtr->GetFWUpdateFlag() &&
         (BeToCpuWord(FWHeader->Version) <= FWUpdateDevicePtr->GetFWVer()) && !(FWUpdateDevicePtr->GetFWVer() & 0x8000)) continue;

      timeval timeout;
      gettimeofday(&timeout, NULL);
      timeout.tv_sec -= FWUpdateDevicePtr->GetAliveInterval() * 2;
      timeval lastDeviceCheck;
      FWUpdateDevicePtr->GetLastDeviceCheck(&lastDeviceCheck);
      if(!FWUpdateDevicePtr->GetFWUpdateFlag() && timercmp(&lastDeviceCheck, &timeout, >)) continue;
      FWUpdateDevicePtr->SetLastDeviceCheck();

      byte cmd[2];
      cmd[0] = 'N';
      cmd[1] = 'I';
      int error = SendATCommand(0, SequenceID, "FWUpdate", "FWUpdate", NULL, 0, FWUpdateDevicePtr->GetAddrL(), cmd, 2, &HA1Control::FWUpdateNodeIdRes, 0, 0);
      SequenceID++;
      if(SequenceID <= 0) SequenceID = 1;
      if(error < 0) LogPrintf("FWUpdate Error %08x %d\n", FWUpdateDevicePtr->GetAddrL(), error);
    } else {
      FWUpdateDevicePtr = NULL;
      break;
    }
  }
}

int HA1Control::FWUpdateNodeIdRes(WriteQueueSt *writeQueue, ReadQueueSt *readQueue) {

  char buf[32];
  int size = readQueue->Size - writeQueue->ResOffset - 3;
  if(size > 31) size = 31;
  strncpy(buf, (char *)readQueue->Buf + writeQueue->ResOffset + 3, size);
  buf[size] = 0;
  char *q = strtok(buf, " \t");
  longword func = 0;
  if(q) func = strtoul(q, NULL, 16);
  char *type = strtok(NULL, " \t");
  if(type && (!strcmp(type, "r") || !strcmp(type, "R") || !strcmp(type, "RP-1/RIR") || !strcmp(type, "f") || !strcmp(type, "F") || !strcmp(type, "HA-2/FC"))) {
    byte cmd[16];
    cmd[0] = CmdGetVer;
    return SendAVRCommand(writeQueue, cmd, 1, &HA1Control::FWUpdateGetVer1Res);
  } else if(type) {
    char nodeId[32];
    strcpy(nodeId, type);
    if(!strcmp(nodeId, "c") || !strcmp(nodeId, "C")) strcpy(nodeId, "RP-1/CS");
    if(!strcmp(nodeId, "w") || !strcmp(nodeId, "W")) strcpy(nodeId, "HA-2/SW");
    if(!strcmp(nodeId, "s") || !strcmp(nodeId, "S")) strcpy(nodeId, "HA-2/SE");
    DeviceList *devData = DeviceTable.SearchByAddrL(writeQueue->AddrL);
    if(!devData) return Finish;
    devData->SetFunc(func);
    devData->SetType(nodeId);
  }
  return Finish;
}

int HA1Control::GetVerRes(WriteQueueSt *writeQueue, ReadQueueSt *readQueue) {

  int version = (readQueue->Buf[15] << 8)|readQueue->Buf[16];
  DeviceList *devData = DeviceTable.SearchByAddrL(writeQueue->AddrL);
  if(!devData) return -1;
  devData->SetFWVer(version);
  int bootVer = 0x8000;

  if(version >= 0x8000) {
    bootVer = version;
  } else {
    bootVer = (readQueue->Buf[19] << 8)|readQueue->Buf[20];
    longword func = (readQueue->Buf[21] << 24) |
                    (readQueue->Buf[22] << 16) |
                    (readQueue->Buf[23] << 8) |
                     readQueue->Buf[24];
    char nodeId[16];
    int size = readQueue->Size - 25;
    if(size > 15) size = 15;
    strncpy(nodeId, (char *)readQueue->Buf + 25, size);
    nodeId[size] = 0;
    if(!strcmp(nodeId, "r") || !strcmp(nodeId, "R")) strcpy(nodeId, "RP-1/RIR");
    if(!strcmp(nodeId, "f") || !strcmp(nodeId, "F")) strcpy(nodeId, "HA-2/FC");
    devData->SetFunc(func);
    devData->SetType(nodeId);
  }
  devData->SetBootVer(bootVer);
  return version;
}

int HA1Control::FWUpdateGetVer1Res(WriteQueueSt *writeQueue, ReadQueueSt *readQueue) {
  
  int version = GetVerRes(writeQueue, readQueue);

  if(!XBeeDeviceFlowCtrl && FWUpdateDevice) return Finish;
 
  if(version & 0x8000) { // boot mode
    FWUpdateDevice = writeQueue->AddrL;
    DeviceList *devData = DeviceTable.SearchByAddrL(writeQueue->AddrL);
    devData->SetFWUpdateFlag(2);
    LogPrintf("FWUpdate %08x : ver %04x\n", writeQueue->AddrL, version);
    return FWUpdateTransfer(writeQueue, readQueue);
  }
  // normal FW mode
  DeviceList *devData = DeviceTable.SearchByAddrL(writeQueue->AddrL);
  if(!devData) return Finish;
  int updateFlag = devData->GetFWUpdateFlag();
  if((updateFlag == 1) || (!updateFlag && (BeToCpuWord(FWHeader->Version) > version))) {
    FWUpdateDevice = writeQueue->AddrL;
    int id = (readQueue->Buf[17] << 8)|readQueue->Buf[18];
    int bootVer = devData->GetBootVer();
    LogPrintf("FWUpdate %08x : ver %04x id %04x boot %04x\n", writeQueue->AddrL, version, id, bootVer);
    byte cmd[1];
    cmd[0] = CmdBoot;
    return SendAVRCommand(writeQueue, cmd, 1, &HA1Control::FWUpdateGetVer2);
  } else {
    devData->SetFWUpdateFlag(3);
  }
  return Finish;
}

int HA1Control::FWUpdateGetVer2(WriteQueueSt *writeQueue, ReadQueueSt *readQueue) {

  byte cmd[1];
  cmd[0] = CmdGetVer;
  return SendAVRCommand(writeQueue, cmd, 1, &HA1Control::FWUpdateGetVer2Res, 500);   
}

int HA1Control::FWUpdateGetVer2Res(WriteQueueSt *writeQueue, ReadQueueSt *readQueue) {
  
  int version = GetVerRes(writeQueue, readQueue);
  if(version & 0x8000) { // boot mode
    DeviceList *devData = DeviceTable.SearchByAddrL(writeQueue->AddrL);
    devData->SetFWUpdateFlag(2);
    LogPrintf("FWUpdate %08x : ver %04x\n", writeQueue->AddrL, version);
    return FWUpdateTransfer(writeQueue, readQueue);
  }
  LogPrintf("FWUpdate %08x : ver %04x\n", writeQueue->AddrL, version);
  FWUpdateDevice = 0;
  return Error;
}

int HA1Control::FWUpdateTransfer(WriteQueueSt *writeQueue, ReadQueueSt *readQueue) {
  
  byte cmd[128 + 2];
  cmd[0] = CmdUpdate;
  cmd[1] = writeQueue->Param1;
  memcpy(cmd + 2, FWUpdateBuf + sizeof(FWHeaderSt) + 128 * writeQueue->Param1, 128);
  LogPrintf("FWUpdate %08x : Seq %d/%d\n", writeQueue->AddrL, cmd[1], FWSeqNum);
  return SendAVRCommand(writeQueue, cmd, 128 + 2, &HA1Control::FWUpdateTransferRes, 200);
}

int HA1Control::FWUpdateTransferRes(WriteQueueSt *writeQueue, ReadQueueSt *readQueue) {
  
  writeQueue->Param1++;
  if(writeQueue->Param1 < FWSeqNum) {
    return FWUpdateTransfer(writeQueue, readQueue);
  }
  
  byte cmd[128 + 2];
  cmd[0] = CmdUpdate;
  cmd[1] = FWSeqLastPage;
  cmd[2] = 'H';
  cmd[3] = 'A';
  memset(cmd + 4, 0xff, 126);
  LogPrintf("FWUpdate %08x : Seq Last\n", writeQueue->AddrL);
  return SendAVRCommand(writeQueue, cmd, 128 + 2, &HA1Control::FWUpdateRebootFW);
}

int HA1Control::FWUpdateRebootFW(WriteQueueSt *writeQueue, ReadQueueSt *readQueue) {
  
  byte cmd[1];
  cmd[0] = CmdRebootFW;
  LogPrintf("FWUpdate Reboot %08x\n", writeQueue->AddrL);
  DeviceList *devData = DeviceTable.SearchByAddrL(writeQueue->AddrL);
  if(!devData) return Error;
  return SendAVRCommand(writeQueue, cmd, 1, &HA1Control::FWUpdateGetVer3);
}

int HA1Control::FWUpdateGetVer3(WriteQueueSt *writeQueue, ReadQueueSt *readQueue) {
  
  byte cmd[1];
  cmd[0] = CmdGetVer;
  LogPrintf("FWUpdate GetVer %08x\n", writeQueue->AddrL);
  return SendAVRCommand(writeQueue, cmd, 1, &HA1Control::FWUpdateGetVer3Res, 3000);   
}

int HA1Control::FWUpdateGetVer3Res(WriteQueueSt *writeQueue, ReadQueueSt *readQueue) {
  
  int version = GetVerRes(writeQueue, readQueue);
  int id = 0;
  if(version < 0x8000) id = (readQueue->Buf[17] << 8) | readQueue->Buf[18];
  DeviceList *devData = DeviceTable.SearchByAddrL(writeQueue->AddrL);
  if(!devData) return Finish;
  devData->SetFWUpdateFlag(3);
  LogPrintf("FWUpdate Success %08x : ver %04x id %04x\n", writeQueue->AddrL, version, id);
  FWUpdateDevice = 0;
  FWUpdateCheck();
  return Finish;
}

int HA1Control::FWUpdateGetVer4(WriteQueueSt *writeQueue, ReadQueueSt *readQueue) {
  
  byte cmd[1];
  cmd[0] = CmdGetVer;
  return SendAVRCommand(writeQueue, cmd, 1, &HA1Control::FWUpdateGetVer4Res, 300);   
}

void HA1Control::FWReboot(longword addrL) {
  
  DeviceList *devData = DeviceTable.SearchByAddrL(addrL);
  if(!devData || devData->GetFWUpdateFlag() < 3) return;
  LogPrintf("FWReboot %08x\n", addrL);
  byte cmd[1];
  cmd[0] = CmdRebootFW;
  int error = SendAVRCommand(0, SequenceID, "FWUpdate", "FWUpdate", NULL, 0, addrL, cmd, 1, &HA1Control::FWUpdateGetVer4, 0, 0);
  SequenceID++;
  if(SequenceID <= 0) SequenceID = 1;
  if(error < 0) LogPrintf("FWReboot Error %08x %d\n", addrL, error);
}

int HA1Control::FWUpdateGetVer4Res(WriteQueueSt *writeQueue, ReadQueueSt *readQueue) {
  
  int version = GetVerRes(writeQueue, readQueue);
  int id = 0;
  if(version < 0x8000) id = (readQueue->Buf[17] << 8)|readQueue->Buf[18];
  LogPrintf("FWReboot %08x : ver %04x id %04x\n", writeQueue->AddrL, version, id);
  if(version >= 0x8000) {
    DeviceList *devData = DeviceTable.SearchByAddrL(writeQueue->AddrL);
    if(!devData) return Finish;
    if(devData->GetFWUpdateFlag() == 3) devData->SetFWUpdateFlag(1);
    FWUpdateCheck();
  }
  return Finish;
}

