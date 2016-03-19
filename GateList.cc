/*
 HA-1 GateList.cc
 
 Copyright: Copyright (C) 2014 Mitsuru Nakada All Rights Reserved.
 License: GNU GPL v2
 */

#include <string.h>
#include "GateList.h"

GateList::GateList() {
  Next = NULL;
  Prev = NULL;
}

GateList::~GateList() {
  if(Prev) delete Prev;
}

int GateList::Add(const char *name, longword addrL, const char *command) {

  GateList *p = new GateList;
  if(!p) return Error;
  
  strncpy(p->Name, name, 31);
  p->Name[31] = 0;
  p->AddrL = addrL;
  strncpy(p->Command, command, 31);
  p->Command[31] = 0;

  p->Prev = Prev;
  if(Prev) Prev->Next = p;
  p->Next = NULL;
  Prev = p;
  if(!Next) Next = p;
  return Success;
}

int GateList::SearchByName(const char *name, longword *addrL, char *command) {

  for(GateList *p = Next; p; p = p->Next) {
    if(!strcmp(p->Name, name)) {
      if(addrL) *addrL = p->AddrL;
      if(command) strcpy(command, p->Command);
      return Success;
    }
  }
  return Error;
}

int GateList::SearchByAddrL(longword addrL, char *name, char *command) {

  for(GateList *p = Next; p; p = p->Next) {
    if(p->AddrL == addrL) {
      if(name) strcpy(name, p->Name);
      if(command) strcpy(command, p->Command);
      return Success;
    }
  }
  return Error;
}

