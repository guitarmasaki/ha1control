/*
 HA-1 GateList.h
 
 Copyright: Copyright (C) 2014 Mitsuru Nakada All Rights Reserved.
 License: GNU GPL v2
 */

#ifndef __GateList_h__
#define __GateList_h__

#include <stdlib.h>
#include "Error.h"
#include "Types.h"

class GateList {

public:
  GateList();
  ~GateList();
  int Add(const char *name, longword addrL, const char *command);
  int SearchByName(const char *name, longword *addrL = NULL, char *command = NULL);
  int SearchByAddrL(longword addrL, char *name = NULL, char *command = NULL);

private:
  GateList *Next;
  GateList *Prev;
  
  char Name[32];
  longword AddrL;
  char Command[32];
};

#endif // __GateList_h__

