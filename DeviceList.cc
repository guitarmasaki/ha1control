/*
 HA-1 DeviceList.cc
 
 Copyright: Copyright (C) 2013 Mitsuru Nakada All Rights Reserved.
 License: GNU GPL v2
 */

#include <string.h>
#include <stdio.h>
#include <sys/time.h>

#include "DeviceList.h"

DeviceList::DeviceList() {
  Next = NULL;
  Prev = NULL;
  Path = NULL;
  Name[0] = 0;
  AddrL = 0xfffffffe;
  Type[0] = 0;
  Func = 0;
  AliveInterval = 30;
}

DeviceList::~DeviceList() {
  if(Prev) {
    if(Path) delete [] Path;
    delete Prev;
  }
}

int DeviceList::SetRegistryFile(const char *path, const char *registry) {

  if(AddrL != 0xfffffffe) return Error;

  Path = new char [strlen(path) + 1 + strlen(registry) + 5];
  if(!Path) return Error;
  strcpy(Path, path);
  if(Path[strlen(Path) - 1] != '/') strcat(Path, "/");
  strcat(Path, registry);
  strcat(Path, ".reg");

  FILE *fp = fopen(Path, "r");
  if(fp == NULL) return Error;
  
  longword magic;
  if(fread(&magic, sizeof(longword), 1, fp) != 1) {
    fclose(fp);
    return Error;
  }
  if(magic != MagicCode) {
    fclose(fp);
    return Error;
  }
  
  while(!feof(fp)) {
    DeviceList *p = new DeviceList;
    if(!p) return Error;
    if(fread(p, sizeof(DeviceList), 1, fp) != 1) {
      delete p;
      break;
    }
    p->AddrL = 0xffffffff;
    p->SequenceID = 0;
    p->AliveInterval = AliveInterval;
    p->Voltage = 0;
    p->NetworkAddr = 0xfffe;
    p->FWVer = 0;
    p->Func = 0;
    p->FWUpdateFlag = 0;
    p->LastDeviceCheck.tv_sec = 0;
    p->LastDeviceCheck.tv_usec = 0;

    p->Prev = Prev;
    if(Prev) Prev->Next = p;
    p->Next = NULL;
    Prev = p;
    if(!Next) Next = p;
  }
  fclose(fp);
  return Success;
}

int DeviceList::Add(const char *name, longword addrL) {

  if(AddrL != 0xfffffffe) return Error;

  if(!strcmp(name, "server")) addrL = 0;
  for(DeviceList *p = Next; p; p = p->Next) {
    if(!strcmp(p->Name, name)) {
      if(addrL != 0xffffffff) p->AddrL = addrL;
      return Success;
    }
  }

  DeviceList *p = new DeviceList;
  if(!p) return Error;
  memset(p, 0, sizeof(DeviceList));

  strncpy(p->Name, name, 31);
  p->Name[31] = 0;
  if(addrL != 0xffffffff) p->AddrL = addrL;
  p->AliveInterval = AliveInterval;
  p->NetworkAddr = 0xfffe;

  p->Prev = Prev;
  if(Prev) Prev->Next = p;
  p->Next = NULL;
  Prev = p;
  if(!Next) Next = p;
  return Success;
}

int DeviceList::UpdateStatus() {

  if(AddrL != 0xfffffffe) return Error;

  char file[256];
  strcpy(file, Path);
  strcat(file, "~");
  FILE *fp = fopen(file, "w");
  if(fp == NULL) return -1;
  longword magic = MagicCode;
  if(fwrite(&magic, sizeof(longword), 1, fp) != 1) {
    fclose(fp);
    return Error;
  }
  for(DeviceList *p = Next; p; p = p->Next) {
    if(fwrite(p, sizeof(DeviceList), 1, fp) != 1) {
      fclose(fp);
      return Error;
    }
  }
  fclose(fp);
  return rename(file, Path);
}

DeviceList *DeviceList::SearchByName(const char *name) {

  if(AddrL != 0xfffffffe) return NULL;

  for(DeviceList *p = Next; p; p = p->Next) {
    if(!strcmp(p->Name, name) || ((!strcmp(p->Name, "server") && !strcmp(name, "-")))) {
      return p;
    }
  }
  return NULL;
}

DeviceList *DeviceList::SearchByNameMatch(const char *searchName) {
  
  if(AddrL != 0xfffffffe) return NULL;

  for(DeviceList *p = Next; p; p = p->Next) {
    int len = strlen(p->Name);
    if(!strncmp(p->Name, searchName, len) && (searchName[len] == '_')) {
      return p;
    }
  }
  return NULL;
}

DeviceList *DeviceList::SearchByAddrL(longword addrL) {
  
  if(AddrL != 0xfffffffe) return NULL;

  for(DeviceList *p = Next; p; p = p->Next) {
    if((addrL == p->AddrL) || (!addrL && !strcmp(p->Name, "server"))) {
      return p;
    }
  }
  return NULL;
}

DeviceList *DeviceList::NextList() {

  DeviceList *p = this;
  do {
    p = p->Next;
    if(p == NULL) break;
  } while(p->AddrL == 0xffffffff);
  return p;
}

// accessor
void DeviceList::SetLastDeviceCheck() {

  gettimeofday(&LastDeviceCheck, NULL);
}

void DeviceList::SetLastDigital() {

  LastDigital = Digital;
}

void DeviceList::SetLastStat() {

  LastStat = Stat;
}

void DeviceList::SetData(word digital, int *analog, int voltage) {

  LastDigital = Digital;
  Digital = digital;
  for(int i = 0; i < 4; i++) {
    Analog[i] = analog[i];
  }
  Voltage = voltage;
  gettimeofday(&LastAccess, NULL);
}

void DeviceList::SetData(byte stat, int *analog) {

  LastStat = Stat;
  Stat = stat;
  for(int i = 0; i < 2; i++) {
    Analog[4 + i] = analog[i];
  }
  gettimeofday(&LastAVRAccess, NULL);
}

void DeviceList::SetSequenceID(longword addrL, int sequenceID) {
  
  if(AddrL != 0xfffffffe) return;

  for(DeviceList *p = Next; p; p = p->Next) {
    if((addrL == p->AddrL) || (!addrL && !strcmp(p->Name, "server"))) {
      p->SequenceID = sequenceID;
      return;
    }
  }
}

int DeviceList::GetSequenceID(longword addrL) {
  
  if(AddrL != 0xfffffffe) return Error;

  for(DeviceList *p = Next; p; p = p->Next) {
    if((addrL == p->AddrL) || (!addrL && !strcmp(p->Name, "server"))) {
      return p->SequenceID;
    }
  }
  return 0;
}

void DeviceList::SetAliveInterval(longword addrL, int interval) {
  
  if(AddrL != 0xfffffffe) return;

  for(DeviceList *p = Next; p; p = p->Next) {
    if((addrL == p->AddrL) || (!addrL && !strcmp(p->Name, "server"))) {
      p->AliveInterval = interval;
      break;
    }
  }
}

int DeviceList::IncreaseAliveInterval(longword addrL) {
  
  if(AddrL != 0xfffffffe) return Error;

  for(DeviceList *p = Next; p; p = p->Next) {
    if((addrL == p->AddrL) || (!addrL && !strcmp(p->Name, "server"))) {
      if(p->AliveInterval < 60 * 60 * 24)
        p->AliveInterval *= 2;
      return p->AliveInterval;
    }
  }
  return Error;
}

void DeviceList::SetNetworkAddr(longword addrL, word netAddr) {
  
  if(AddrL != 0xfffffffe) return;

  for(DeviceList *p = Next; p; p = p->Next) {
    if((addrL == p->AddrL) || (!addrL && !strcmp(p->Name, "server"))) {
      p->NetworkAddr = netAddr;
      break;
    }
  }
}

word DeviceList::GetNetworkAddr(longword addrL) {
  
  if(AddrL != 0xfffffffe) return 0;

  for(DeviceList *p = Next; p; p = p->Next) {
    if((addrL == p->AddrL) || (!addrL && !strcmp(p->Name, "server"))) {
      return p->NetworkAddr;
    }
  }
  return Error;
}

