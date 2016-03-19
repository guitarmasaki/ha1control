/*
 HA-1 HA1Control_Execute.h
 
 Copyright: Copyright (C) 2013 Mitsuru Nakada All Rights Reserved.
 License: GNU GPL v2
 */

#include "HA1Control.h"
#include "Command.h"

int HA1Control::ExecuteCommand(int sockfd, char *execCmd) {

  if(!strlen(execCmd)) return Success;
  LogPrintf("Exec : %s\n", execCmd);
  int error = ExecuteCommandBody(sockfd, SequenceID, execCmd, execCmd);
  SequenceID++;
  if(SequenceID <= 0) SequenceID = 1;
  return error;
}

int HA1Control::ExecuteCommand(WriteQueueSt *writeQueue) {

#ifdef DEBUG
  LogPrintf("ExecuteCommand\n");
#endif

  void *savePtr = (void *)writeQueue->Param1;
  if(CommandTable.SearchByNameSplit(NULL, &savePtr, writeQueue->ExecCmd)) return Finish;
  writeQueue->Param1 = (unsigned long)savePtr;
  LogPrintf("Exec : %s(%s)\n", writeQueue->CombiCmd, writeQueue->ExecCmd);

  int error = ExecuteCommandBody(writeQueue->SockFd, writeQueue->SequenceID, writeQueue->ExecCmd, writeQueue->CombiCmd, 1);
  if(error == Finish) return NoRes;
  return error;
}

int HA1Control::ExecuteCommandBody(int sockfd, int sequenceID, char *execCmd, char *combiCmd, int reentrantFlag) {

#ifdef DEBUG
  LogPrintf("ExecuteCommandBody %s\n", execCmd);
#endif
  char args[256];
  strcpy(args, execCmd);

  // condition check
  char *condition = strtok(args, "@\r\n");
  condition = strtok(NULL, " \t\r\n");
  if(condition) {
    char *conditionValue = strtok(NULL, " \t\r\n");
    if(!conditionValue) {
      ErrorPrintf("Condition error %s", condition);
      return Error;
    }
    int value;
    if(GetCondition(condition, &value)) {
      ErrorPrintf("Condition error %s", condition);
      return Error;
    }
    if((value && strcmp(conditionValue, "close") && strcmp(conditionValue, "on")) ||
       (!value && strcmp(conditionValue, "open") && strcmp(conditionValue, "off"))) {
      LogPrintf("command skip : %s\n", execCmd);
      if(IsFormatText(sockfd)) ResPrintf(sockfd, "Ack\n");
      return NoExec;
    }
  }
  
  char *commandSavePtr;
  char *p = strtok(args, " \t\r\n");
  if(!p) return NoExec;
  
  if(!reentrantFlag) {
    // CommandTable
    void *savePtr = NULL;
    if(!CommandTable.SearchByNameSplit(p, &savePtr)) {
      int error = AddWriteQueue(sockfd, SequenceID, 1000, execCmd, execCmd, NULL, 0, 0xffffffff, NULL, 0, 0, NULL, 0, (unsigned long)savePtr);
      if(error) {
        ErrorPrintf("AddWriteQueue Error");
        return error;
      }
      return NoRes;
    }
  }

  longword addrL = 0xffffffff;
  int function = -1;

  // remocon command
  if((addrL == 0xffffffff) && !RemoconTable.SearchByName(p)) {
    addrL = 0;
    function = CommandFuncTableSearchByName("irsend");
    char buf[256];
    strcpy(buf, "irsend ");
    strcat(buf, p);
    strcpy(args, buf);
    p = strtok(args, " \t\r\n");
  }

  // remote AVR remocon command
  if(addrL == 0xffffffff) {
    DeviceList *devData = DeviceTable.SearchByNameMatch(p);
    if(devData) {
      addrL = devData->GetAddrL();
      char name[32];
      devData->GetName(name);
      int ptr = strlen(name) + 1;
      if(!RemoconTable.SearchByName(p + ptr)) {
        p += ptr;
        function = CommandFuncTableSearchByName("irsend");
        char buf[256];
        strcpy(buf, "irsend ");
        strcat(buf, p);
        strcpy(args, buf);
        p = strtok(args, " \t\r\n");
      }
    }
  }

  // direct function
  if(addrL == 0xffffffff) {
    function = CommandFuncTableSearchByName(p);
    if(function >= 0) addrL = 0;
  }
  
  // device command
  if(addrL == 0xffffffff) {
    DeviceList *devData = DeviceTable.SearchByName(p);
    if(devData) addrL = devData->GetAddrL();
  }
  
  // device function command
  if((addrL != 0xffffffff) && (function < 0)) {
    char *cmd = strtok(NULL, " \t\r\n");
    if(cmd) function = CommandFuncTableSearchByName(cmd, (1 << 0));
    
    if(cmd && (strlen(cmd) == 2) && (function < 0) && !reentrantFlag) {
      // device AT command
      if(!IsFormatText(sockfd)) {
        ErrorPrintf("comunication format error");
        return Error;
      }
      
      byte buf[256];
      buf[0] = cmd[0];
      buf[1] = cmd[1];
      int q = 2;
      if((cmd[0] == 'n') && (cmd[1] == 'i')) {
        p = strtok(NULL, "\r\n");
        if(p) {
          strncpy((char *)buf + 2, p, 20);
          buf[22] = 0;
          q = strlen((char *)buf + 2) + 3;
        }
      } else {
        do {
          p = strtok(NULL, " \t\r\n");
          if(!p) break;
          buf[q++] = strtoul(p, NULL, 16);
        } while(1);
      }
      
      LogPrintf("ATCmd %d %08x : ", sockfd, addrL);
      for(int i = 0; i < q; i++) LogPrintf("%02x ", buf[i]);
      LogPrintf("\n");
      
      ResPrintf(sockfd, "Ack\n");
      if(int error = SendATCommand(sockfd, sequenceID, execCmd, combiCmd, NULL, 0, addrL, buf, q, &HA1Control::ATCmdDump)) {
        ErrorPrintf("ATCmd Error");
        return Error;
      }
      return NoRes;
    }
  }
  if(addrL == 0xffffffff) {
    ErrorPrintf("Device not found");
    return Error;
  }
  if(function < 0) {
    ErrorPrintf("Function not found");
    return Error;
  }
  
  if(!(CommandFuncTable[function].ExecFlag & (1 << 1)) && !IsFormatText(sockfd)) {
    ErrorPrintf("comunication format error");
    return Error;
  }    
  if(!reentrantFlag && IsFormatText(sockfd)) ResPrintf(sockfd, "Ack\n");
  return (this->*CommandFuncTable[function].Func)(sockfd, sequenceID, execCmd, combiCmd, addrL);
}

int HA1Control::AnalyzeAVRMessage(ReadQueueSt *readQueue) {

#ifdef DEBUG
  LogPrintf("AnalyzeAVRMessage\n");
#endif
  int checkFlag = 0;

  longword addrL = (readQueue->Buf[5] << 24) |
  (readQueue->Buf[6] << 16) |
  (readQueue->Buf[7] << 8) |
  readQueue->Buf[8];
  DeviceList *devData = DeviceTable.SearchByAddrL(addrL);
  if(!devData) {
    char name[32];
    sprintf(name, "undef%-d", UndefNum++);
    DeviceTable.Add(name, addrL);
    devData = DeviceTable.SearchByAddrL(addrL);
  }
  devData->SetAliveInterval(AliveInterval);
  char name[32];
  devData->GetName(name);

  switch(readQueue->Buf[13]) {
    case CmdRebootNotification:
    {
      // RebootNotification  : byte reason, word addr, word version
      int reason = readQueue->Buf[15];
      longword wdtAddr = (readQueue->Buf[16] << 8) | readQueue->Buf[17];
      longword version = (readQueue->Buf[18] << 8) | readQueue->Buf[19];
      devData->SetFWVer(version);
      LogPrintf("Reboot : %s %02x addr:%04x version:%04x\n", name, reason, wdtAddr, version);
      break;
    }
    case CmdBootModeNotification:
      devData->SetFWVer((readQueue->Buf[15] << 8)|readQueue->Buf[16]);
      LogPrintf("BootMode : %s version:%04x\n", name, (readQueue->Buf[15] << 8)|readQueue->Buf[16]);
      FWReboot(addrL);
      break;
    case CmdStatusChangeNotification:
      checkFlag |= StatusChangeReceive(devData, readQueue);
      break;
    case CmdIRReceiveNotification:
      checkFlag |= RemoconReceive(devData, readQueue);
      break;
    case CmdKeypadNotification:
      checkFlag |= KeypadReceive(devData, readQueue);
      break;
    case CmdMotorNotification:
      checkFlag |= MotorReceive(devData, readQueue);
      break;
    default:
      LogPrintf("Notify %s : ", name);
      for(int i = 13; i < readQueue->Size; i++) {
        LogPrintf("%02x ", readQueue->Buf[i]);
      }
      LogPrintf("\n");
      break;
  }
  return checkFlag;
}

int HA1Control::StatusChangeReceive(DeviceList *devData, ReadQueueSt *readQueue) {

#ifdef DEBUG
  LogPrintf("StatusChangeReceive\n");
#endif
  DeviceTable.SetAliveInterval(devData->GetAddrL(), AliveInterval);
  int analog[2];
  for(int i = 0; i < 2; i++) {
    analog[i] = ((readQueue->Buf[16 + i * 2] << 8) | readQueue->Buf[17 + i * 2]) * 3300 / 4096; // mV
  }
  devData->SetData(readQueue->Buf[15], analog);

  StatusPostByDevice(devData->GetAddrL());
  devData->SetLastStat();
  DeviceTable.UpdateStatus();
  return Success;
}

int HA1Control::RemoconReceive(DeviceList *devData, ReadQueueSt *readQueue) {

#ifdef DEBUG
  LogPrintf("RemoconReceive\n");
#endif

  const char *formatInfo[] = { "unknown", "AEHA", "NEC", "SONY", "Other" };
  byte *code = readQueue->Buf + 15;
  int size = readQueue->Size - 15;
  int format = code[2];
  if(format == 0xff) format = 4;
  if(format > 4) format = 0;

  char name[256];
  struct timeval now;
  gettimeofday(&now, NULL);
  if(timercmp(&now, &IRRecordTimeout, <) && (IRRecordAddrL == devData->GetAddrL())) {
    char path[256];
    strcpy(path, IRRemotePath);
    strcat(path, IRRecordFile);
    FILE *fp = fopen(path, "w");
    if(fp != NULL) {
      fwrite(code, 1, size, fp);
      fclose(fp);
      strncpy(name, IRRecordFile, 255);
      name[strlen(name) - 4] = 0;
      RemoconTable.Add(name, code, size);
      LogPrintf("RemoconReceive %s : %s-format ", name, formatInfo[format]);
      for(int i = 0; i < size; i++) LogPrintf("%02x ", code[i]);
      LogPrintf("\n");
      SendResponse(IRRecordSockFd, IRRecordExecCmd, Success);
      IRRecordAddrL = 0;
    }
    return 0;
  }

  char devname[32];
  devData->GetName(devname);
  int error = RemoconTable.SearchByCode(code, size, name);
  if(error && (size > 3)) {
    /*
    {
      type:"irreceive",
      data:[{
        device:<string>,
        format:<string>,
        code:[<integer> ...]
      }]
    }
    */
    json_t *jsonData = json_object();
    json_object_set_new(jsonData, "device", json_string(devname));
    json_object_set_new(jsonData, "format", json_string(formatInfo[format]));
    json_t *jsonCode = json_array();
    for(int i = 0; i < size; i++) {
      json_array_append_new(jsonCode, json_integer(code[i]));
    }
    json_object_set(jsonData, "code", jsonCode);
    json_decref(jsonCode);
    json_t *jsonDataArray = json_array();
    json_array_append(jsonDataArray, jsonData);
    json_decref(jsonData);
    SendNotify("irreceive", jsonDataArray);
    json_decref(jsonDataArray);

    LogPrintf("RemoconReceive %s : %s-format ", devname, formatInfo[format]);
    for(int i = 0; i < size; i++) {
      LogPrintf("%02x ", code[i]);
    }    
    LogPrintf("\n");
    return 0;
  }
  if(!strlen(name)) return 0;

  /*
  {
    type:"irreceive",
    data:[{
      device:<string>,
      format:<string>,
      name:<string>
    }]
  }
  */
  json_t *jsonData = json_object();
  json_object_set_new(jsonData, "device", json_string(devname));
  json_object_set_new(jsonData, "format", json_string(formatInfo[format]));
  json_object_set_new(jsonData, "name", json_string(name));
  json_t *jsonDataArray = json_array();
  json_array_append(jsonDataArray, jsonData);
  json_decref(jsonData);
  SendNotify("irreceive", jsonDataArray);
  json_decref(jsonDataArray);

  LogPrintf("RemoconReceive : %s\n", name);
  return 0;
}

int HA1Control::KeypadReceive(DeviceList *devData, ReadQueueSt *readQueue) {

#ifdef DEBUG
  LogPrintf("KeypadReceive\n");
#endif

  char *enterCode = (char *)readQueue->Buf + 15;
  char devname[32];
  devData->GetName(devname);
  LogPrintf("KeypadReceive %s : %s\n", devname, enterCode);
  char name[32];
  char command[32];
  if(GateTable.SearchByAddrL(devData->GetAddrL(), name, command)) {
    LogPrintf("GateTable SearchByAddrL Error %s %08x\n", devname, devData->GetAddrL());
    return 0;
  }

  /*
  {
    type:"keypad",
    data:[{
      device:<string>,
      password:<string>
    }]
  }
  */
  json_t *jsonData = json_object();
  json_object_set_new(jsonData, "device", json_string(devname));
  json_object_set_new(jsonData, "password", json_string(enterCode));
  json_t *jsonDataArray = json_array();
  json_array_append(jsonDataArray, jsonData);
  json_decref(jsonData);
  SendNotify("keypad", jsonDataArray);
  json_decref(jsonDataArray);

  if(!strcmp(enterCode, "!")) {
    LogPrintf("%s : motion sensor are sensed.\n", name);
    return 0;
  }
  if(!strcmp(enterCode, "_")) {
    LogPrintf("%s : motion sensor are not sensed.\n", name);
    return 0;
  }

  char customerName[32];
  char password[16];
  char body[256];
  if(PasswordTable.SearchByName(name, enterCode, customerName)) {
    LogPrintf("PasswordTable SearchByName Error %s %s %s\n", devname, name, enterCode);
    sprintf(body, "%s %s : password error %s\n", devname, name, enterCode);
    json_t *jsonMailData = json_object();
    json_object_set_new(jsonMailData, "subject", json_string(body));
    json_object_set_new(jsonMailData, "text", json_string(body));
    SendNotify("mail", jsonMailData);
    json_decref(jsonMailData);
    return 0;
  }
  LogPrintf("KeypadReceive %s : %s %s %s %s\n", devname, name, customerName, enterCode, command);
  sprintf(body, "%s %s : valid password %s\n", devname, name, customerName);
  json_t *jsonMailData = json_object();
  json_object_set_new(jsonMailData, "subject", json_string(body));
  json_object_set_new(jsonMailData, "text", json_string(body));
  SendNotify("mail", jsonMailData);
  json_decref(jsonMailData);
 
  int error = ExecuteCommand(0, command);
  if(!error) LogPrintf("OK\n");
  if(error < 0) LogPrintf("Error %d\n", error);
  return 1;
}

int HA1Control::MotorReceive(DeviceList *devData, ReadQueueSt *readQueue) {

#ifdef DEBUG
  LogPrintf("MotorReceive\n");
#endif

  char seqNo = readQueue->Buf[15];
  char stat = readQueue->Buf[16];
  char count = readQueue->Buf[17];
  char devname[32];
  devData->GetName(devname);
  LogPrintf("Notify MotorNotification %s : seq=%d stat=%d count=%d\n", devname, seqNo, stat, count);

  /*
  {
    type:"motor",
    data:[{
      device:<string>,
      status:<string>, // OK or Error
      sequence:<num>,
      count:<num>
    }]
  }
  */
  json_t *jsonData = json_object();
  json_object_set_new(jsonData, "device", json_string(devname));
  if(stat) {
    json_object_set_new(jsonData, "status", json_string("error"));
    json_object_set_new(jsonData, "sequence", json_integer(seqNo));
    json_object_set_new(jsonData, "count", json_integer(count));
  } else {
    json_object_set_new(jsonData, "status", json_string("ok"));
  }
  json_t *jsonDataArray = json_array();
  json_array_append(jsonDataArray, jsonData);
  json_decref(jsonData);
  SendNotify("motor", jsonDataArray);
  json_decref(jsonDataArray);
  return 0;
}

int HA1Control::PasswordCheck(int force) {

  PasswordTable.UpdatePasswordFile();
  if(PasswordTable.UpdateOccured() || force) {
    /*
    {
      type:"gateInfo",
      data:[{
          gate:<string>,
          customer:<string>,
          password:<string>,
          start:<unixTime>,
          end:<unixTime>
        },....]
    }
    */
    json_t *jsonDataArray = json_array();
    void *savePtr = NULL;
    char gateName[32];
    char customerName[32];
    char password[16];
    longword start;
    longword end;
    while(!PasswordTable.GetEntry(&savePtr, gateName, customerName, password, &start, &end)) {
      json_t *jsonReserveData = json_object();
      json_object_set_new(jsonReserveData, "gate", json_string(gateName));
      json_object_set_new(jsonReserveData, "customer", json_string(customerName));
      json_object_set_new(jsonReserveData, "password", json_string(password));
      json_object_set_new(jsonReserveData, "start", json_integer(start));
      json_object_set_new(jsonReserveData, "end", json_integer(end));
      json_array_append(jsonDataArray, jsonReserveData);
      json_decref(jsonReserveData);
    }
    SendNotify("gateInfo", jsonDataArray);
    json_decref(jsonDataArray);
    PasswordTable.ClearDirtyFlag();
  }
  return Success;
}
