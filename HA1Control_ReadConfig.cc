/*
 HA-1 HA1Control_ReadConfig.cc
 
 Copyright: Copyright (C) 2013 Mitsuru Nakada All Rights Reserved.
 License: GNU GPL v2
 */

#include <dirent.h>
#include <sys/stat.h>

#include "HA1Control.h"

int HA1Control::ReadConfFile(const char *file) {

  FILE *fp = fopen(file, "r");
  if(fp == NULL) return Error;
  int mode = -1;
  int error = 0;
  char buf[256];

  int line = 0;
  while(!feof(fp)) {
    char *q = fgets(buf, 255, fp);
    if(q == NULL) break;
    line++;
    char *p = strchr(buf, '#');
    if(p) *p = 0;
    p = strtok(buf, " \t\r\n");
    if(!p) continue;

    if(!strcmp(p, "commandport:")) {
      p = strtok(NULL, " \t\r\n");
      if(!p) {
        error = -1;
        break;
      }
      CommandIFPort = strtoul(p, NULL, 10);
      continue;
    }
    
    if(!strcmp(p, "commandallow:")) {
      p = strtok(NULL, " \t\r\n");
      if(!p) {
        error = -1;
        break;
      }
      strncpy(CommandIFIP, p, 31);
      CommandIFIP[31] = 0;
      p = strtok(NULL, " \t\r\n");
      if(!p) {
        error = -1;
        break;
      }
      strncpy(CommandIFMask, p, 31);
      CommandIFMask[31] = 0;
      continue;
    }
    
    if(!strcmp(p, "xbeedevice:")) {
      p = strtok(NULL, " \t\r\n");
      if(!p) {
        error = -1;
        break;
      }
      strncpy(XBeeDevice, p, 255);
      XBeeDevice[255] = 0;
      
      p = strtok(NULL, " \t\r\n");
      if(!p) {
        error = -1;
        break;
      }
      XBeeDeviceBaudrate = 0;
      if(strtoul(p, NULL, 10) == 9600) XBeeDeviceBaudrate = B9600;
      if(strtoul(p, NULL, 10) == 19200) XBeeDeviceBaudrate = B19200;
      if(strtoul(p, NULL, 10) == 38400) XBeeDeviceBaudrate = B38400;
      if(strtoul(p, NULL, 10) == 57600) XBeeDeviceBaudrate = B57600;
      if(strtoul(p, NULL, 10) == 115200) XBeeDeviceBaudrate = B115200;
      if(!XBeeDeviceBaudrate) {
        error = -1;
        break;
      }
      
      XBeeDeviceFlowCtrl = 0;
      p = strtok(NULL, " \t\r\n");
      if(p && !strcmp(p, "hwflow")) XBeeDeviceFlowCtrl = 1;
      continue;
    }
    
    if(!strcmp(p, "basepath:")) {
      p = strtok(NULL, " \t\r\n");
      if(!p) {
        error = -1;
        break;
      }
      strncpy(BasePath, p, 255);
      BasePath[254] = 0;
      if(strlen(BasePath) && BasePath[strlen(BasePath) - 1] != '/') strcat(BasePath, "/");
      continue;
    }
    
    if(!strcmp(p, "wssserverurl:")) {
      p = strtok(NULL, " \t\r\n");
      if(!p) {
        error = -1;
        break;
      }
      strncpy(WSSServerURL, p, 255);
      WSSServerURL[255] = 0;
      continue;
    }

    if(!strcmp(p, "ca_certificate:")) {
      p = strtok(NULL, " \t\r\n");
      if(!p) {
        error = -1;
        break;
      }
      strncpy(CACertFile, p, 255);
      CACertFile[255] = 0;
      continue;
    }

    if(!strcmp(p, "client_certificate:")) {
      p = strtok(NULL, " \t\r\n");
      if(!p) {
        error = -1;
        break;
      }
      char *q = strtok(NULL, " \t\r\n");
      if(!q) {
        error = -1;
        break;
      }
      strncpy(ClientCertFile, p, 255);
      ClientCertFile[255] = 0;
      strncpy(ClientKeyFile, q, 255);
      ClientKeyFile[255] = 0;
      continue;
    }

    if(!strcmp(p, "server_issuer:")) {
      p = strtok(NULL, "\r\n");
      if(!p) {
        error = -1;
        break;
      }
      while(*p && (*p == ' ')) p++;
      strncpy(ServerIssuer, p, 256);
      continue;
    }
    
    if(!strcmp(p, "server_subject:")) {
      p = strtok(NULL, "\r\n");
      if(!p) {
        error = -1;
        break;
      }
      while(*p && (*p == ' ')) p++;
      strncpy(ServerSubject, p, 256);
      continue;
    }

    if(!strcmp(p, "allow_client:")) {
      AllowClientList = json_array();
      while(1) {
        p = strtok(NULL, " \t\r\n");
        if(!p) break;
        json_t *data = json_object();
        char *qq;
        char *q = strtok_r(p, "/", &qq);
        while(q) {
          if(!strncmp(q, "OU=", 3)) json_object_set_new(data, "OU", json_string(q + 3));
          if(!strncmp(q, "CN=", 3)) json_object_set_new(data, "CN", json_string(q + 3));
          q = strtok_r(NULL, "/", &qq);
        }
        json_array_append(AllowClientList, data);
        json_decref(data);
      }
      continue;
    }

    if(!strcmp(p, "device:")) {
      DeviceTable.SetRegistryFile(BasePath, "devicetable");
      mode = 1;
      continue;
    }

    if(!strcmp(p, "command:")) {
      mode = 2;
      continue;
    }

    if(!strcmp(p, "status:")) {
      mode = 3;
      continue;
    }

    if(!strcmp(p, "gate:")) {
      mode = 10;
      continue;
    }

    if(mode < 0) {
      error = -1;
      break;
    }
    
    if(mode == 1) { // device:
      char *name = p;
      
      p = strtok(NULL, " \t\r\n");
      if(!p) {
        error = -1;
        break;
      }
      longword addrL = strtoul(p, NULL, 16);

      if(DeviceTable.Add(name, addrL)) {
        error = -1;
        break;
      }
    }
    
    if(mode == 2) { // command:
      char *name = p;
      if(DeviceTable.SearchByName(p)) {
        error = -1;
        break;
      }

      if(CommandFuncTableSearchByName(p) >= 0) {
        error = -1;
        break;
      }

      p = strtok(NULL, "\r\n");
      while(*p && ((*p == ' ') || (*p == '\t')))  p++;
      if(!p) {
        error = -1;
        break;
      }

      CommandTable.Add(name, p);
    }
    
    if(mode == 3) { // status:
      char *name = p;
      if(DeviceTable.SearchByName(p)) {
        error = -1;
        break;
      }

      p = strtok(NULL, " \t\r\n");
      if(!p) {
        error = -1;
        break;
      }
      DeviceList *devData = DeviceTable.SearchByName(p);
      if(!devData) {
        error = -1;
        break;
      }

      p = strtok(NULL, " \t\r\n");
      if(!p) {
        error = -1;
        break;
      }
      int func = StatusFuncTableSearchByName(p);
      if(func < 0) {
        error = -1;
        break;
      }

      p = strtok(NULL, " \t\r\n");
      int port = -1;
      if(p) port = strtoul(p, NULL, 10);
      
      if(StatusTable.Add(name, devData->GetAddrL(), port, func)) {
        error = -1;
        break;
      }
    }

    if(mode == 10) { // gate:
      char *name = p;
      p = strtok(NULL, " \t\r\n");
      if(!p) {
        error = -1;
        break;
      }
      DeviceList *devData = DeviceTable.SearchByName(p);
      if(!devData) {
        error = -1;
        break;
      }
      p = strtok(NULL, "\r\n");
      if(!p) {
        error = -1;
        break;
      }
      if(strlen(p) > 31) {
        error = -1;
        break;
      }
      GateTable.Add(name, devData->GetAddrL(), p);
    }

  }
  fclose(fp);

  PasswordTable.SetRegistryFile(BasePath, "passwordtable");

  strcpy(LogFile, BasePath);
  strncat(LogFile, "ha1control.log", 255);
  LogFile[255] = 0;
  LogOpen(LogFile);
  WebSocketIF.LogOpen(LogFile);

  strcpy(IRRemotePath, BasePath);
  strncat(IRRemotePath, "irremote/", 255);
  IRRemotePath[255] = 0;
  // read remocon table
  DIR *dirp = opendir(IRRemotePath);
  if(!dirp) {
    mkdir(IRRemotePath, 0755);
  } else {
    while(1) {
      struct dirent *dent = readdir(dirp);
      if(!dent) break;
      char file[256];
      strcpy(file, dent->d_name);
      if((strlen(file) > 4) && !strcmp(file + strlen(file) - 4, ".rmc")) {
        byte buf[256];
        char path[256];
        strncpy(path, IRRemotePath, 255);
        strncat(path, file, 255);
        path[255] = 0;
        FILE *fp = fopen(path, "r");
        if(fp != NULL) {
          int size = fread(buf, 1, 256, fp);
          file[strlen(file) - 4] = 0;
          RemoconTable.Add(file, buf, size);
          fclose(fp);
        }
      }
    }
    closedir(dirp);
  }

  char file2[256];
  strcpy(file2, BasePath);
  strncat(file2, "client.ui", 255);
  file2[255] = 0;
  json_error_t jerror;
  ClientUi = json_load_file(file2, 0, &jerror);
  if(!ClientUi) {
    fprintf(stderr, "JSON parse error : line %d col %d %s", jerror.line, jerror.column, jerror.text);
    return Error;
  }

  strcpy(FWPath, BasePath);
  strncat(FWPath, "ha2module.ha", 255);
  FWPath[255] = 0;
  if(FWReadFile()) {
    fprintf(stderr, "FWReadFile error\n");
    return Error;
  }

  if(error) {
    fprintf(stderr, "Error : Config format error\n ---> line%d,mode%d: %s\n", line, mode, buf);
    LogPrintf("Error : Config format error\n ---> line%d,mode%d: %s\n", line, mode, buf);
    return Error;
  }
  fprintf(stderr, "Read Config Done.\n");
  return Success;
}

