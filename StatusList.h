/*
 HA-1 StatusList.h
 
 Copyright: Copyright (C) 2013 Mitsuru Nakada All Rights Reserved.
 License: GNU GPL v2
 */

#ifndef __StatusList_h__
#define __StatusList_h__

#include <stdlib.h>
#include "Error.h"
#include "Types.h"

class StatusList {

public:
  StatusList();
  ~StatusList();
  int Add(const char *name, longword addrL, int port, int func);
  int SearchByName(const char *name, longword *addrL = NULL, int *port = NULL, int *func = NULL);
  int GetEntry(void **savePtr, char *name = NULL, longword *addrL = NULL, int *port = NULL, int *func = NULL);
  int GetEntryNum();

private:
  StatusList *Next;
  StatusList *Prev;
  
  char Name[32];
  longword AddrL;
  int Port;
  int Function;
};

#endif // __StatusList_h__

