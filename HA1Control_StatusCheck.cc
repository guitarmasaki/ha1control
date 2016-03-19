/*
 HA-1 HA1Control_StatusCheck.cc
 
 Copyright: Copyright (C) 2013 Mitsuru Nakada All Rights Reserved.
 License: GNU GPL v2
 */

#include "HA1Control.h"
#include "Command.h"
#include <sys/wait.h>
#include <unistd.h>

// Name, Param
const HA1Control::StatusFuncTableSt HA1Control::StatusFuncTable[] = {
  { "swstat"             , "swStat"    },
  { "inopenclose"        , "openClose" },
  { "inonoff"            , "onOff"     },
  { "haopenclose"        , "openClose" },
  { "haonoff"            , "onOff"     },
  { "gpiopenclose"       , "openClose" },
  { "gpicloseopen"       , "openClose" },
  { "gpionoff"           , "onOff"     },
  { "gpioffon"           , "onOff"     },
  { "pironoff"           , "pir"       },
  { "piroffon"           , "pir"       },
  { "adtemp"             , "temp"      },
  { "adrain"             , "rain"      },
  { "adhumidity"         , "humidity"  },
  { "adbattery"          , "battery"   },
  { "virtualswopenclose" , "openClose" },
  { "virtualswoffon"     , "onOff"     },
  { "rain_info"          , "rain"      },
  { NULL                 , NULL        }
};

int HA1Control::StatusFuncTableSearchByName(const char *name, char *type) {

  for(int i = 0; StatusFuncTable[i].Name; i++) {
    if(!strcmp(name, StatusFuncTable[i].Name)) {
      if(type) strcpy(type, StatusFuncTable[i].Type);
      return i;
    }
  }
  return Error;
}

int HA1Control::ScanReceive(ReadQueueSt *readQueue) {
  
#ifdef DEBUG
  LogPrintf("ScanReceive\n");
#endif

  longword addrL = (readQueue->Buf[5] << 24) |
    (readQueue->Buf[6] << 16) |
    (readQueue->Buf[7] << 8) |
    readQueue->Buf[8];
  int offset = 13;

  DeviceList *devData = DeviceTable.SearchByAddrL(addrL);
  if(!devData) {
    char name[32];
    sprintf(name, "undef%-d", UndefNum++);
    DeviceTable.Add(name, addrL);
    devData = DeviceTable.SearchByAddrL(addrL);
  }

  DeviceTable.SetAliveInterval(addrL, AliveInterval);
  
  int dm = (readQueue->Buf[offset] << 8) | readQueue->Buf[offset + 1];
  int am = readQueue->Buf[offset + 2];
  offset += 3;
  
  int digital = 0;
  if(dm) {
    digital = (readQueue->Buf[offset] << 8) | readQueue->Buf[offset + 1];
    offset += 2;
  }
  
  int analog[8];
  for(int i = 0; i < 8; i++) {
    analog[i] = -1;
    if(am & (1 << i)) {
      analog[i] = ((readQueue->Buf[offset] << 8) | readQueue->Buf[offset + 1]) * 1200 / 1024;
      offset+=2;
    }
  }
  
  int error = Success;

  devData->SetData(digital, analog, analog[7]);
  if(StatisticsLastMin < 0) StatisticsLastMin = 61;
  StatusPostByDevice(addrL);
  devData->SetLastDigital();
  DeviceTable.UpdateStatus();
  return error;
}

int HA1Control::GetCondition(const char *command, int *stat, int *lastStat, char *str, char *type) {
  
#ifdef DEBUG
  LogPrintf("GetCondition\n");
#endif

  longword cmdAddrL;
  int port;
  int funcNo;
  if(StatusTable.SearchByName(command, &cmdAddrL, &port, &funcNo)) return Error;
  const char *funcName = StatusFuncTable[funcNo].Name;
  if(type) StatusFuncTableSearchByName(funcName, type);

  DeviceList *devData = DeviceTable.SearchByAddrL(cmdAddrL);
  if(!devData) return Error;

  struct timeval tm;
  gettimeofday(&tm, NULL);
  tm.tv_sec -= AliveCheckTimeout;
  timeval lastAccess;
  devData->GetLastAccess(&lastAccess);

  if(!strcmp(funcName, "inopenclose")) {
    if(timercmp(&lastAccess, &tm, <)) return Error;
    if(stat) *stat = (devData->GetDigital() >> port) & 1;
    if(lastStat) *lastStat = (devData->GetLastDigital() >> port) & 1;
    if(str) strcpy(str, StatusStr[(devData->GetDigital() >> port) & 1]);
    return Success;
  }

  if(!strcmp(funcName, "inonoff")) {
    if(timercmp(&lastAccess, &tm, <)) return Error;
    if(stat) *stat = ((devData->GetDigital() >> port) & 1) + 2;
    if(lastStat) *lastStat = ((devData->GetLastDigital() >> port) & 1) + 2;
    if(str) strcpy(str, StatusStr[((devData->GetDigital() >> port) & 1) + 2]);
    return Success;
  }

  timeval lastAVRAccess;
  devData->GetLastAVRAccess(&lastAVRAccess);
  if(!strcmp(funcName, "adrain")) {
    if((port < 4) && (port > 5)) return Error;
    if(timercmp(&lastAVRAccess, &tm, <)) return Error;
    if(stat) *stat = AD2Rain(devData->GetAnalog(port));
    if(lastStat) *lastStat = AD2Rain(devData->GetAnalog(port));
    if(str) sprintf(str, "%d", AD2Rain(devData->GetAnalog(port)));
    return Success;
  }
  
  if(!strcmp(funcName, "adtemp")) {
    if((port < 0) || (port > 5)) return Error;
    if((port >= 4) && timercmp(&lastAVRAccess, &tm, <)) return Error;
    if((port < 4) && (timercmp(&lastAccess, &tm, <))) return Error;
    if(stat) *stat = AD2Temp(devData->GetAnalog(port));
    if(lastStat) *lastStat = AD2Temp(devData->GetAnalog(port));
    if(str) sprintf(str, "%3.1f", (double)AD2Temp(devData->GetAnalog(port)) / 10.0);
    return Success;
  }

  if(!strcmp(funcName, "adhumidity")) {
    if((port < 0) || (port > 5)) return Error;
    if((port >= 4) && timercmp(&lastAVRAccess, &tm, <)) return Error;
    if((port < 4) && (timercmp(&lastAccess, &tm, <))) return Error;
    if(stat) *stat = AD2Humidity(devData->GetAnalog(port));
    if(lastStat) *lastStat = AD2Humidity(devData->GetAnalog(port));
    if(str) sprintf(str, "%d", AD2Humidity(devData->GetAnalog(port)));
    return Success;
  }

  if(!strcmp(funcName, "adbattery")) {
    if((port < 0) || (port > 5)) return Error;
    if((port >= 4) && timercmp(&lastAVRAccess, &tm, <)) return Error;
    if((port < 4) && (timercmp(&lastAccess, &tm, <))) return Error;
    if(stat) *stat = AD2Battery(devData->GetAnalog(port));
    if(lastStat) *lastStat = AD2Battery(devData->GetAnalog(port));
    if(str) sprintf(str, "%d", AD2Battery(devData->GetAnalog(port)));
    return Success;
  }

  if(!strcmp(funcName, "haopenclose")) {
    if((port < 0) || (port > 1)) return Error;
    if(timercmp(&lastAVRAccess, &tm, <)) return Error;
    if(stat) *stat = (devData->GetStat() >> (port + 2)) & 1;
    if(lastStat) *lastStat = (devData->GetLastStat() >> (port + 2)) & 1;
    if(str) strcpy(str, StatusStr[(devData->GetStat() >> (port + 2)) & 1]);
    return Success;
  }
  
  if(!strcmp(funcName, "haonoff")) {
    if((port < 0) || (port > 1)) return Error;
    if(timercmp(&lastAVRAccess, &tm, <)) return Error;
    if(stat) *stat = ((devData->GetStat() >> (port + 2)) & 1) + 2;
    if(lastStat) *lastStat = ((devData->GetLastStat() >> (port + 2)) & 1) + 2;
    if(str) strcpy(str, StatusStr[((devData->GetStat() >> (port + 2)) & 1) + 2]);
    return Success;
  }

  if(!strcmp(funcName, "gpiopenclose")) {
    if((port < 0) || (port > 6)) return Error;
    if(timercmp(&lastAVRAccess, &tm, <)) return Error;
    if(stat) *stat = ((devData->GetStat() >> port) & 1) ^ 1;
    if(lastStat) *lastStat = ((devData->GetLastStat() >> port) & 1) ^ 1;
    if(str) strcpy(str, StatusStr[((devData->GetStat() >> port) & 1) ^ 1]);
    return Success;
  }

  if(!strcmp(funcName, "gpicloseopen")) {
    if((port < 0) || (port > 6)) return Error;
    if(timercmp(&lastAVRAccess, &tm, <)) return Error;
    if(stat) *stat = ((devData->GetStat() >> port) & 1);
    if(lastStat) *lastStat = ((devData->GetLastStat() >> port) & 1);
    if(str) strcpy(str, StatusStr[((devData->GetStat() >> port) & 1)]);
    return Success;
  }

  if(!strcmp(funcName, "gpionoff") || !strcmp(funcName, "pironoff")) {
    if((port < 0) || (port > 6)) return Error;
    if(timercmp(&lastAVRAccess, &tm, <)) return Error;
    if(stat) *stat = (((devData->GetStat() >> port) & 1) ^ 1) + 2;
    if(lastStat) *lastStat = (((devData->GetLastStat() >> port) & 1) ^ 1) + 2;
    if(str) strcpy(str, StatusStr[(((devData->GetStat() >> port) & 1) ^ 1) + 2]);
    return Success;
  }

  if(!strcmp(funcName, "gpioffon") || !strcmp(funcName, "piroffon")) {
    if((port < 0) || (port > 6)) return Error;
    if(timercmp(&lastAVRAccess, &tm, <)) return Error;
    if(stat) *stat = (((devData->GetStat() >> port) & 1)) + 2;
    if(lastStat) *lastStat = (((devData->GetLastStat() >> port) & 1)) + 2;
    if(str) strcpy(str, StatusStr[(((devData->GetStat() >> port) & 1)) + 2]);
    return Success;
  }
  
  if(!strcmp(funcName, "swstat")) {
    if(timercmp(&lastAccess, &tm, <)) return Error;
    if(stat) *stat = (devData->GetStat() >> 4) & 0b111;
    if(lastStat) *lastStat = (devData->GetLastStat() >> 4) & 0b111;
    if(str) strcpy(str, StatusStr[(devData->GetStat() >> 4) & 0b111]);
    return Success;
  }

  if(!strcmp(funcName, "virtualswoffon")) {
    devData = DeviceTable.SearchByName("server");
    if(stat) *stat = (((devData->GetDigital() >> port) & 1)) + 2;
    if(lastStat) *lastStat = (((devData->GetLastDigital() >> port) & 1)) + 2;
    if(str) strcpy(str, StatusStr[(((devData->GetDigital() >> port) & 1)) + 2]);
    return Success;
  }

  if(!strcmp(funcName, "virtualswopenclose")) {
    devData = DeviceTable.SearchByName("server");
    if(stat) *stat = (((devData->GetDigital() >> port) & 1));
    if(lastStat) *lastStat = (((devData->GetLastDigital() >> port) & 1));
    if(str) strcpy(str, StatusStr[(((devData->GetDigital() >> port) & 1))]);
    return Success;
  }

  if(!strcmp(command, "rain_info")) {
    tm.tv_sec -= AliveCheckTimeout;
    if(timercmp(&RainInfoTime, &tm, <)) return Error;
    if(stat) *stat = RainInfo;
    if(lastStat) *lastStat = RainInfo;
    if(str) sprintf(str, "%d", RainInfo);
    return Success;
  }
  return Error;
}

int HA1Control::StatusPostByDevice(longword addrL) {

#ifdef DEBUG
  LogPrintf("StatusPostByDevice %08x\n", addrL);
#endif

  json_t *jsonStatusArray = json_array();
  int num = 0;
  if(addrL == 0xffffffff) {
    json_t *jsonStatus = json_object();
    json_object_set_new(jsonStatus, "status", json_string("server"));
    json_object_set_new(jsonStatus, "value", json_string("alive"));
    json_array_append(jsonStatusArray, jsonStatus);
    json_decref(jsonStatus);
    num++;
  }
  void *savePtr = NULL;
  char cmd[32];
  longword cmdAddrL;
  while(!StatusTable.GetEntry(&savePtr, cmd, &cmdAddrL)) {
    if((addrL != 0xffffffff) && (cmdAddrL != addrL)) continue;

    int stat;
    int lastStat;
    char str[256];
    char type[256];
    if(!GetCondition(cmd, &stat, &lastStat, str, type) && ((addrL == 0xffffffff) || (stat != lastStat))) {
      json_t *jsonStatus = json_object();
      json_object_set_new(jsonStatus, "status", json_string(cmd));
      json_object_set_new(jsonStatus, "value", json_string(str));
      json_object_set_new(jsonStatus, "type", json_string(type));
      json_array_append(jsonStatusArray, jsonStatus);
      json_decref(jsonStatus);
      num++;
    }
  }
 
  if(num) SendNotify((addrL != 0xffffffff) ?"change":"interval", jsonStatusArray);
  json_decref(jsonStatusArray);
  return 0;
}

int HA1Control::SendDeviceInfo() {

  /*
  {
    type:"deviceInfo",
    data:[{
      device:<string>,
      type:<string>,
      addrL:<string>,
      networkAddr:<string>,
      func:<string>,
      version:<string>,
      voltage:<string>,
      state:<string>
    },....]
  }
  */
  json_t *jsonDataArray = json_array();
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
    
    json_t *jsonDeviceData = json_object();
    char buf[32];
    json_object_set_new(jsonDeviceData, "device", json_string(name));
    devData->GetType(buf);
    if(!strlen(buf)) strcpy(buf, "--------");
    json_object_set_new(jsonDeviceData, "type", json_string(buf));
    sprintf(buf, "%08x", devData->GetAddrL());
    json_object_set_new(jsonDeviceData, "addrL", json_string(buf));
    sprintf(buf, "%04x", devData->GetNetworkAddr());
    json_object_set_new(jsonDeviceData, "networkAddr", json_string(buf));
    sprintf(buf, "%08x", devData->GetFunc());
    json_object_set_new(jsonDeviceData, "func", json_string(buf));
    sprintf(buf, "%04x:%04x", devData->GetFWVer(), devData->GetBootVer());
    json_object_set_new(jsonDeviceData, "version", json_string(buf));
    sprintf(buf, "%4dmV", devData->GetVoltage());
    json_object_set_new(jsonDeviceData, "voltage", json_string(buf));
    sprintf(buf, "%08x", devData->GetNetworkAddr());
    json_object_set_new(jsonDeviceData, "state", json_string(alive?"alive":"dead"));
    json_array_append(jsonDataArray, jsonDeviceData);
    json_decref(jsonDeviceData);
  }
  SendNotify("deviceInfo", jsonDataArray);
  json_decref(jsonDataArray);
  return Success;
}
