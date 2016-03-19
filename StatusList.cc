/*
 HA-1 StatusList.cc
 
 Copyright: Copyright (C) 2013 Mitsuru Nakada All Rights Reserved.
 License: GNU GPL v2
 */

#include <string.h>
#include "StatusList.h"

StatusList::StatusList() {
  Next = NULL;
  Prev = NULL;
}

StatusList::~StatusList() {
  if(Prev) delete Prev;
}

int StatusList::Add(const char *name, longword addrL, int port, int func) {

  StatusList *p = new StatusList;
  if(!p) return Error;
  
  strncpy(p->Name, name, 31);
  p->Name[31] = 0;
  p->AddrL = addrL;
  p->Port = port;
  p->Function = func;

  p->Prev = Prev;
  if(Prev) Prev->Next = p;
  p->Next = NULL;
  Prev = p;
  if(!Next) Next = p;
  return Success;
}

int StatusList::SearchByName(const char *name, longword *addrL, int *port, int *func) {

  for(StatusList *p = Next; p; p = p->Next) {
    if(!strcmp(p->Name, name)) {
      if(addrL) *addrL = p->AddrL;
      if(port) *port = p->Port;
      if(func) *func = p->Function;
      return Success;
    }
  }
  return Error;
}

int StatusList::GetEntry(void **savePtr, char *name, longword *addrL, int *port, int *func) {

  StatusList *p = (StatusList *)*savePtr;
  if(p == NULL) p = this;
  p = p->Next;
  if(p == NULL) return Error;

  if(name) strcpy(name, p->Name);
  if(addrL) *addrL = p->AddrL;
  if(port) *port = p->Port;
  if(func) *func = p->Function;
  *savePtr = (void *)p;
  return Success;
}

int StatusList::GetEntryNum() {

  int num = 0;
  for(StatusList *p = Next; p; p = p->Next) num++;
  return num;
}

  