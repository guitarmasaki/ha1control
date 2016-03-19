/*
 HA-1 HA1Control_Queue.cc
 
 Copyright: Copyright (C) 2013 Mitsuru Nakada All Rights Reserved.
 License: GNU GPL v2
 */

#include "HA1Control.h"
#include "Command.h"

int HA1Control::AddReadQueue(byte *buf, int len) {
  
#ifdef DEBUG
  LogPrintf("AddReadQueue\n");
#endif
  ReadQueueSt *queue = new ReadQueueSt;
  if(!queue) {
    LogPrintf("new ReadQueue error %d\n", sizeof(ReadQueueSt));
    return Error;
  }
  if(len) {
    queue->Buf = new byte [len];
    if(!queue->Buf) {
      LogPrintf("new ReadQueue Buf error %d\n", sizeof(len));
      delete queue;
      return Error;
    }
  } else {
    queue->Buf = NULL;
  }
  queue->Prev = LastReadQueue;
  if(queue->Prev) queue->Prev->Next = queue;
  LastReadQueue = queue;
  queue->Next = NULL;
  if(!FirstReadQueue) FirstReadQueue = queue;

  gettimeofday(&queue->ReceiveTime, NULL);
  if(len) memcpy(queue->Buf, buf, len);
  queue->Size = len;

#ifdef DEBUG
  LogPrintf("ReadQueue: First %lx Last %lx queue %lx ", (unsigned long)FirstReadQueue, (unsigned long)LastReadQueue, (unsigned long)queue);
  LogPrintf("queue->Size %d\n", queue->Size);
  for(int i = 0; i < len; i++) {
    LogPrintf("%02x ", queue->Buf[i]);
  }
  LogPrintf("\n");
#endif
  
  return Success;
}

int HA1Control::AddWriteQueue(int sockfd, int sequenceID, int state, const char *execCmd, const char *combiCmd, byte *data, int dataSize, longword addrL, byte *buf, int size, int resOffset, int (HA1Control::*nextFunc)(WriteQueueSt *p, ReadQueueSt *q),  int waitMSec, unsigned long param1, unsigned long param2, unsigned long param3) {

#ifdef DEBUG
  LogPrintf("AddWriteQueue %s %s %08x\n", execCmd, combiCmd, addrL);
#endif
  if(!data) dataSize = 0;
  if(!dataSize) data = NULL;
  WriteQueueSt *queue = new WriteQueueSt;
  if(!queue) {
    LogPrintf("new WriteQueue error %d\n", sizeof(WriteQueueSt));
    return Error;
  }
  queue->ExecCmd = new char [256];
  if(!queue->ExecCmd) {
    delete queue;
    LogPrintf("new WriteQueue->ExecCmd error %d\n", 256);
    return Error;
  }
  queue->CombiCmd = new char [256];
  if(!queue->CombiCmd) {
    delete [] queue->ExecCmd;
    delete queue;
    LogPrintf("new WriteQueue->CombiCmd error %d\n", 256);
    return Error;
  }
  if(size) {
    queue->Buf = new byte [size];
    if(!queue->Buf) {
      delete [] queue->CombiCmd;
      delete [] queue->ExecCmd;
      delete queue;
      LogPrintf("new WriteQueue->Buf error %d\n", size);
      return Error;
    }
  } else {
    queue->Buf = NULL;
  }
  if(dataSize) {
    queue->Data = new byte [dataSize];
    if(!queue->Data) {
      if(queue->Buf) delete [] queue->Buf;
      delete [] queue->CombiCmd;
      delete [] queue->ExecCmd;
      delete queue;
      LogPrintf("new WriteQueue->Data error %d\n", dataSize);
      return Error;
    }
  } else {
    queue->Data = NULL;
  }
  queue->Prev = LastWriteQueue;
  if(queue->Prev) queue->Prev->Next = queue;
  LastWriteQueue = queue;
  queue->Next = NULL;
  if(!FirstWriteQueue) FirstWriteQueue = queue;

  queue->Retry = 0;
  queue->ResOffset = resOffset;
  queue->SockFd = sockfd;
  queue->Size = size;
  queue->DataSize = dataSize;
  strcpy(queue->ExecCmd, execCmd);
  if(combiCmd) {
    strcpy(queue->CombiCmd, combiCmd);
  } else {
    strcpy(queue->ExecCmd, execCmd);
  }
  if(size) memcpy(queue->Buf, buf, size);
  if(dataSize) memcpy(queue->Data, data, dataSize);
  queue->SendTime.tv_sec = 0;
  queue->SendTime.tv_usec = 0;
  queue->NextTime.tv_sec = 0;
  queue->NextTime.tv_usec = 0;

  queue->State = state;
  queue->SequenceID = sequenceID;
  queue->NextFunc = nextFunc;
  queue->WaitMSec = waitMSec;
  queue->AddrL = addrL;
  queue->Param1 = param1;
  queue->Param2 = param2;
  queue->Param3 = param3;

#ifdef DEBUG
  LogPrintf("WriteQueue: SeqID %d First %lx Last %lx queue %lx ", sequenceID, (unsigned long)FirstWriteQueue, (unsigned long)LastWriteQueue, (unsigned long)queue);
  LogPrintf("queue->Size %d\n", queue->Size);
  for(int i = 0; i < size; i++) {
    LogPrintf("%02x ", queue->Buf[i]);
  }
  LogPrintf("\n");
#endif

  return Success;
}

void HA1Control::DeleteWriteQueue(WriteQueueSt *writeQueue) {
  
#ifdef DEBUG
  LogPrintf("DeleteWriteQueue SeqID %d %lx\n", writeQueue->SequenceID, (unsigned long)writeQueue);
#endif
  
  if(writeQueue) {
    if(writeQueue->Prev) {
      writeQueue->Prev->Next = writeQueue->Next;
    } else {
      FirstWriteQueue = writeQueue->Next;
    }
    if(writeQueue->Next) {
      writeQueue->Next->Prev = writeQueue->Prev;
    } else {
      LastWriteQueue = writeQueue->Prev;
    }
    if(writeQueue->Data) delete [] writeQueue->Data;
    if(writeQueue->Buf) delete [] writeQueue->Buf;
    delete [] writeQueue->CombiCmd;
    delete [] writeQueue->ExecCmd;
    delete writeQueue;
  }
}

void HA1Control::DeleteReadQueue(ReadQueueSt *readQueue) {
  
#ifdef DEBUG
  LogPrintf("DeleteReadQueue %lx\n", (unsigned long)readQueue);
#endif
  
  if(readQueue) {
    if(readQueue->Prev) {
      readQueue->Prev->Next = readQueue->Next;
    } else {
      FirstReadQueue = readQueue->Next;
    }
    if(readQueue->Next) {
      readQueue->Next->Prev = readQueue->Prev;
    } else {
      LastReadQueue = readQueue->Prev;
    }
    if(readQueue->Buf) delete [] readQueue->Buf;
    delete readQueue;
  }
}

void HA1Control::QueueCheck() {

#ifdef DEBUG
  LogPrintf("QueueCheck\n");
#endif
  // Queue Pair Check
  WriteQueueSt *p = FirstWriteQueue;
  while(p) {
    ReadQueueSt *q = FirstReadQueue;
    while(p && (p->State == 2) && q) {
      if(((p->Buf[0] == 0x08) && (q->Buf[0] == 0x88) && (p->Buf[1] == q->Buf[1])) ||
         ((p->Buf[0] == 0x17) && (q->Buf[0] == 0x97) && !memcmp(p->Buf + 1, q->Buf + 1, 9))) {
        p = QueuePairATCommandProcess(p, q); 
        DeleteReadQueue(q);
        q = FirstReadQueue;
        continue;
      } else if((p->Buf[0] == 0x10) && (q->Buf[0] == 0x8b) && (p->Buf[1] == q->Buf[1])) {
        p = QueuePairAVRCommandDelivery(p, q);
        DeleteReadQueue(q);
        q = FirstReadQueue;
        continue;
      } else if((p->Buf[0] == 0x10) && (q->Buf[0] == 0x90) &&
                (p->Buf[1] == q->Buf[12]) && !memcmp(p->Buf + 2, q->Buf + 1, 8)) {
        p = QueuePairAVRCommandProcess(p, q);
        DeleteReadQueue(q);
        q = FirstReadQueue;
        continue;
      }
      if(q) q = q->Next;
    }
    if(p) p = p->Next;
  }
  
  int checkFlag;
  do {
    checkFlag = ReadQueueCheck();
    WriteQueueTimerCheck();
    checkFlag |= WriteQueueHighPriorityCheck(); // WriteQueue Check SequenceID > 0
    checkFlag |= WriteQueueLowPriorityCheck(); // WriteQueue Check SequenceID < 0
    WriteQueueTimeoutCheck();
    ReadQueueTimeoutCheck();
    checkFlag |= CommandCheck();
  } while(checkFlag);
}

HA1Control::WriteQueueSt *HA1Control::QueuePairATCommandProcess(WriteQueueSt *p, ReadQueueSt *q) {

#ifdef DEBUG
  LogPrintf("QueuePairATCommandProcess\n");
#endif
  if(q->Buf[p->ResOffset + 2] != 0) { // error
    LogPrintf("AT Command Status Error %08x -> Retry\n", p->AddrL);
    p->WaitMSec = 5000;
    if(Retry(p)) {
      int error = DeviceTable.IncreaseAliveInterval(p->AddrL);
      if(error > 0) LogPrintf("IncreaseAliveInterval %08x %d\n", p->AddrL, error);
      ErrorPrintf("Retry & Response error");
      SendResponse(p->SockFd, p->CombiCmd, Error);
      WriteQueueSt *np = p->Next;
      DeviceTable.SetSequenceID(p->AddrL, 0);
      DeleteWriteQueue(p);
      p = np;
    }
    LogPrintf("ReadQueue : %lx ", (unsigned long)q);
    for(int i = 0; i < q->Size; i++) {
      LogPrintf("%02x ", q->Buf[i]);
    }
    LogPrintf("\n");
    return p;
  }
  
  DeviceTable.SetAliveInterval(p->AddrL, AliveInterval);
  int error = Success;
  if(p->NextFunc) {
    p->State = 3;
    error = (this->*p->NextFunc)(p, q);
  }
  if(!p->NextFunc || (error == Error) || (error == Finish) || (error == NoRes)) {

    if((error != NoRes) && !strcmp(p->ExecCmd, p->CombiCmd)) SendResponse(p->SockFd, p->ExecCmd, error);
    DeviceTable.SetSequenceID(p->AddrL, 0);
  }
  WriteQueueSt *np = p->Next;
  DeleteWriteQueue(p);
  return np;
}

HA1Control::WriteQueueSt *HA1Control::QueuePairAVRCommandDelivery(WriteQueueSt *p, ReadQueueSt *q) {

#ifdef DEBUG
  LogPrintf("QueuePairAVRCommandDelivery\n");
#endif
  if(q->Buf[5] != 0) { // Delivery error
    LogPrintf("Delivery Status Error %08x %02x %02x-> Retry\n", p->AddrL, q->Buf[5], q->Buf[6]);
    p->WaitMSec = 2000;
    int retry = SendCommandRetry;
    int fwUpdate = !strcmp(p->ExecCmd, "FWUpdate");
    if(fwUpdate) retry = 5;
    if(((q->Buf[5] == 0x24) && !fwUpdate) || Retry(p, retry)) {
      if(fwUpdate) {
        if(p->AddrL == FWUpdateDevice) FWUpdateDevice = 0;
        DeviceList *devData = DeviceTable.SearchByAddrL(p->AddrL);
        devData->SetFWUpdateFlag(1);
      } else {
        int error = DeviceTable.IncreaseAliveInterval(p->AddrL);
        if(error > 0) LogPrintf("IncreaseAliveInterval %08x %d\n", p->AddrL, error);
      }
      ErrorPrintf("Retry & Response error");
      SendResponse(p->SockFd, p->CombiCmd, Error);
      WriteQueueSt *np = p->Next;
      DeviceTable.SetSequenceID(p->AddrL, 0);
      DeleteWriteQueue(p);
      p = np;
    }
    LogPrintf("ReadQueue : %lx ", (unsigned long)q);
    for(int i = 0; i < q->Size; i++) {
      LogPrintf("%02x ", q->Buf[i]);
    }
    LogPrintf("\n");
  }
  return p;
}

HA1Control::WriteQueueSt *HA1Control::QueuePairAVRCommandProcess(WriteQueueSt *p, ReadQueueSt *q) {

#ifdef DEBUG
  LogPrintf("QueuePairAVRCommandProcess\n");
#endif
  if(q->Buf[p->ResOffset + 1] & 0x80) { // Status error
    LogPrintf("AVR Command Status Error %08x\n", p->AddrL);
    int fwupdate = 0;
    DeviceList *devData = DeviceTable.SearchByAddrL(p->AddrL);
    if(devData && (devData->GetFWUpdateFlag() < 3)) fwupdate = 1;
    if(!fwupdate || Retry(p)) {
      ErrorPrintf("Retry & Response error");
      SendResponse(p->SockFd, p->CombiCmd, Error);
      WriteQueueSt *np = p->Next;
      DeviceTable.SetSequenceID(p->AddrL, 0);
      DeleteWriteQueue(p);
      p = np;
    }
    LogPrintf("ReadQueue : %lx ", (unsigned long)q);
    for(int i = 0; i < q->Size; i++) {
      LogPrintf("%02x ", q->Buf[i]);
    }
    LogPrintf("\n");
    return p;
  }
  DeviceTable.SetAliveInterval(p->AddrL, AliveInterval);
  int error = Success;
  if(p->NextFunc) {
    p->State = 3;
    error = (this->*p->NextFunc)(p, q);
  }
  if(!p->NextFunc || (error == Error) || (error == Finish) || (error == NoRes)) {
    if((error != NoRes) && !strcmp(p->ExecCmd, p->CombiCmd)) SendResponse(p->SockFd, p->ExecCmd, error);
    DeviceTable.SetSequenceID(p->AddrL, 0);
  }
  WriteQueueSt *np = p->Next;
  DeleteWriteQueue(p);
  return np;
}

int HA1Control::ReadQueueCheck() {

#ifdef DEBUG
  LogPrintf("ReadQueueCheck\n");
#endif
  int checkFlag = 0;
  ReadQueueSt *q = FirstReadQueue;
  while(q) {
    ReadQueueSt *nq = q->Next;
    if(q->Buf[0] == 0x92) {
      checkFlag |= ScanReceive(q);
      DeleteReadQueue(q);
    } else if((q->Buf[0] == 0x90) && (q->Buf[12] == 0x00)) { // FrameID = 0
      checkFlag |= AnalyzeAVRMessage(q);
      DeleteReadQueue(q);
    }
    q = nq;
  }
  return checkFlag;
}

void HA1Control::WriteQueueTimerCheck() {

#ifdef DEBUG
  LogPrintf("WriteQueueTimerCheck\n");
#endif
  WriteQueueSt *p = FirstWriteQueue;
  while(p) {
    if(p->State == 0) {
      struct timeval wait;
      wait.tv_sec = p->WaitMSec / 1000;
      wait.tv_usec = (p->WaitMSec % 1000) * 1000;
      timeradd(&WakeupTime, &wait, &p->NextTime);
      p->State = 1;
    }
    p = p->Next;
  }
  return;
}

int HA1Control::WriteQueueHighPriorityCheck() {

#ifdef DEBUG
  LogPrintf("WriteQueueHighPriorityCheck\n");
#endif
  int checkFlag = 0;
  WriteQueueSt *p = FirstWriteQueue;
  while(p) {
    WriteQueueSt *np = p->Next;
    if(p->SequenceID > 0) {
      if(p->State == 1) {
        int seqID = DeviceTable.GetSequenceID(p->AddrL);
        if(!timercmp(&p->NextTime, &WakeupTime, >) && ((seqID <= 0) || (seqID == p->SequenceID))) {
          DeviceTable.SetSequenceID(p->AddrL, p->SequenceID);
          if(p->Buf && p->Size) {
            SendWriteQueue(p);
            checkFlag = 1;
            p->State = 2;
          } else {
            int error = Success;
            if(p->NextFunc) error = (this->*p->NextFunc)(p, NULL);
            if(((error == Error) || (error == Finish))  && !strcmp(p->ExecCmd, p->CombiCmd))
              SendResponse(p->SockFd, p->ExecCmd, error);
            DeviceTable.SetSequenceID(p->AddrL, 0);
            DeleteWriteQueue(p);
          }
        } else if((seqID <= 0) || (seqID == p->SequenceID)) {
          if(timercmp(&NextWakeupTime, &p->NextTime, >)) NextWakeupTime = p->NextTime;
        }
      }
    }
    p = np;
  }
  return checkFlag;
}

int HA1Control::WriteQueueLowPriorityCheck() {

#ifdef DEBUG
  LogPrintf("WriteQueueLowPriorityCheck\n");
#endif
  int checkFlag = 0;
  WriteQueueSt *p = FirstWriteQueue;
  while(p) {
    WriteQueueSt *np = p->Next;
    if(p->SequenceID < 0) {
      if(p->State == 1) {
        int seqID = DeviceTable.GetSequenceID(p->AddrL);
        if(!timercmp(&p->NextTime, &WakeupTime, >) && (!seqID || (seqID == p->SequenceID))) {
          DeviceTable.SetSequenceID(p->AddrL, p->SequenceID);
          if(p->Buf && p->Size) {
            SendWriteQueue(p);
            checkFlag = 1;
            p->State = 2;
          } else {
            int error = Success;
            if(p->NextFunc) error = (this->*p->NextFunc)(p, NULL);
            if(((error == Error) || (error == Finish)) && !strcmp(p->ExecCmd, p->CombiCmd))
              SendResponse(p->SockFd, p->ExecCmd, error);
            DeviceTable.SetSequenceID(p->AddrL, 0);
            DeleteWriteQueue(p);
          }
        } else if(!timercmp(&p->NextTime, &WakeupTime, >) && (seqID < 0)) {
          LogPrintf("Sequcence jamming %d %d\n", seqID, p->SequenceID);
          DeleteWriteQueue(p);
        } else if(!seqID) {
          if(timercmp(&NextWakeupTime, &p->NextTime, >)) NextWakeupTime = p->NextTime;
        }
      }
    }
    p = np;
  }
  return checkFlag;
}

void HA1Control::WriteQueueTimeoutCheck() {

#ifdef DEBUG
  LogPrintf("WriteQueueTimeoutCheck\n");
#endif
  WriteQueueSt *p = FirstWriteQueue;
  while(p) {
    WriteQueueSt *np = p->Next;
    if(p->State == 2) {
      timeval timeout = p->SendTime;
      timeout.tv_sec += SendCommandTimeout;
      if(DeviceTable.GetNetworkAddr(p->AddrL) >= 0xfffd) timeout.tv_sec += SendCommandTimeout * 5;
      if(timercmp(&timeout, &WakeupTime, <)) {
        LogPrintf("WriteQueue Timeout -> Retry\n");
        LogPrintf("WriteQueue : %lx ", (unsigned long)p);
        for(int i = 0; i < p->Size; i++) {
          LogPrintf("%02x ", p->Buf[i]);
        }
        LogPrintf("\n");
        
        p->WaitMSec = 0;
        int retry = SendCommandRetry;
        if(!strcmp(p->ExecCmd, "FWUpdate")) retry = 5;
        if(Retry(p, retry)) {
          if(!strcmp(p->ExecCmd, "FWUpdate")) {
            if(p->AddrL == FWUpdateDevice) FWUpdateDevice = 0;
            DeviceList *devData = DeviceTable.SearchByAddrL(p->AddrL);
            devData->SetFWUpdateFlag(1);
          } else {
            int error = DeviceTable.IncreaseAliveInterval(p->AddrL);
            if(error > 0) LogPrintf("IncreaseAliveInterval %08x %d\n", p->AddrL, error);
          }
          WriteQueueSt *np = p->Next;
          DeviceTable.SetSequenceID(p->AddrL, 0);
          DeleteWriteQueue(p);
        }
      } else {
        if(timercmp(&NextWakeupTime, &timeout, >)) NextWakeupTime = timeout;
      }
    }
    p = np;
  }
}

void HA1Control::ReadQueueTimeoutCheck() {

#ifdef DEBUG
  LogPrintf("ReadQueueTimeoutCheck\n");
#endif
  ReadQueueSt *q = FirstReadQueue;
  while(q) {
    timeval timeout = q->ReceiveTime;
    timeout.tv_sec += ReceiveQueueTimeout;
    ReadQueueSt *nq = q->Next;
    if(timercmp(&WakeupTime, &timeout, >)) {
      LogPrintf("ReadQueue Timeout : ");
      for(int i = 0; i < q->Size; i++) {
        LogPrintf("%02x ", q->Buf[i]);
      }
      LogPrintf("\n");
      DeleteReadQueue(q);
    }
    q = nq;
  }
}

int HA1Control::CommandCheck() {

#ifdef DEBUG
  LogPrintf("CommandCheck\n");
#endif
  int checkFlag = 0;
  WriteQueueSt *p = FirstWriteQueue;
  while(p) {
    WriteQueueSt *np = p->Next;
    if(p->State == 1000) {
      WriteQueueSt *r = FirstWriteQueue;
      while(r) {
        if((r->State < 1000) && (r->SequenceID == p->SequenceID)) break;
        r = r->Next;
      }
      if(!r) {
        int error = ExecuteCommand(p);
        if((error < 0) || (error == Finish)) {
          SendResponse(p->SockFd, p->CombiCmd, error);
          DeleteWriteQueue(p);
        }
        checkFlag = 1;
      }
    }
    p = np;
  }
  return checkFlag;
}

int HA1Control::Retry(WriteQueueSt *writeQueue, int retry) {
  
#ifdef DEBUG
  LogPrintf("Retry\n");
#endif
  writeQueue->NextTime.tv_sec = 0;
  writeQueue->NextTime.tv_usec = 0;
  writeQueue->Retry++;
  writeQueue->State = 0;

  if(!retry) retry = SendCommandRetry;
  writeQueue->Buf[1] = FrameID;
  if(writeQueue->Buf[0] == 0x10) writeQueue->Buf[14] = FrameID;
  FrameID++;
  if(FrameID > 255) FrameID = 1;

  LogPrintf("Retry WriteQueue: SeqID %d %lx ", writeQueue->SequenceID, (unsigned long)writeQueue);
  for(int i = 0; i < writeQueue->Size; i++) {
    LogPrintf("%02x ", writeQueue->Buf[i]);
  }
  LogPrintf("\n");

  if(writeQueue->Retry > retry) {
    LogPrintf("Retry WriteQueue Error SeqID %d Socket %d : ", writeQueue->SequenceID, writeQueue->SockFd);
    for(int i = 0; i < writeQueue->Size; i++) {
      LogPrintf("%02x ", writeQueue->Buf[i]);
    }
    LogPrintf("\n");
    ErrorPrintf("Retry Error");
    SendResponse(writeQueue->SockFd, writeQueue->ExecCmd, Error);
    if(!writeQueue->Buf || !writeQueue->Size) return Error;
    return Error;
  }
  return Success;
}

int HA1Control::SendWriteQueue(WriteQueueSt *writeQueue) {

#ifdef DEBUG
  LogPrintf("SendWriteQueue SeqID %d %lx\n", writeQueue->SequenceID, (unsigned long)writeQueue);
#endif

  gettimeofday(&writeQueue->SendTime, NULL);

  int error = SendPacket(writeQueue->Buf, writeQueue->Size);
  if(error < 0) {
    LogPrintf("error SendPacket\n");
    return error;
  }
  return Success;
}
