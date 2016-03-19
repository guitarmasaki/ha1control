/*
 HA-1 RemoconList.cc
 
 Copyright: Copyright (C) 2013 Mitsuru Nakada All Rights Reserved.
 License: GNU GPL v2
 */

#include <string.h>
#include "RemoconList.h"

RemoconList::RemoconList() {
  Next = NULL;
  Prev = NULL;
}

RemoconList::~RemoconList() {
  if(Prev) delete Prev;
}

int RemoconList::Add(const char *name, byte *buf, int size) {

  RemoconList *p = new RemoconList;
  if(!p) return Error;
  
  strncpy(p->Name, name, 255);
  p->Name[255] = 0;
  if(size > 256) size = 256;
  memcpy(p->Buf, buf, size);
  p->Size = size;
  
  p->Prev = Prev;
  if(Prev) Prev->Next = p;
  p->Next = NULL;
  Prev = p;
  if(!Next) Next = p;
  return Success;
}

int RemoconList::SearchByName(const char *name, byte *buf, int size) {

  for(RemoconList *p = Next; p; p = p->Next) {
    if(!strcmp(p->Name, name)) {
      if(size > p->Size) size = p->Size;
      if(buf) memcpy(buf, p->Buf, size);
      return Success;
    }
  }
  return Error;
}

int RemoconList::SearchByCode(const byte *buf, int size, char *name) {
  
  int size1 = 3;
  int offset2 = 4;
  if(buf[2] == 0xff) {
    size1 = 4;
    offset2 = 5;
    size -= (buf[4] & 0x0f) * 4;
  }
  int size2 = size - offset2;
  if(size2 <= 0) return Error;
  for(RemoconList *p = Next; p; p = p->Next) {
    if(!memcmp(p->Buf, buf, size1) && !memcmp(p->Buf + offset2, buf + offset2, size2)) {
      if(name) strcpy(name, p->Name);
      return Success;
    }
  }
  return Error;
}

int RemoconList::GetEntry(void **savePtr, char *name, byte *buf, int size) {

  RemoconList *p = (RemoconList *)*savePtr;
  if(p == NULL) p = this;
  p = p->Next;
  if(p == NULL) return Error;

  if(name) strcpy(name, p->Name);
  if(size > p->Size) size = p->Size;
  if(buf) memcpy(buf, p->Buf, size);
  *savePtr = (void *)p;
  return Success;
}
