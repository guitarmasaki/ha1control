/*
 HA-1 RemoconList.h
 
 Copyright: Copyright (C) 2013 Mitsuru Nakada All Rights Reserved.
 License: GNU GPL v2
 */

#ifndef __RemoconList_h__
#define __RemoconList_h__

#include <stdlib.h>
#include "Error.h"
#include "Types.h"

class RemoconList {

public:
  RemoconList();
  ~RemoconList();
  int Add(const char *name, byte *buf, int size);

  int SearchByName(const char *name, byte *buf = NULL, int size = 0);
  int SearchByCode(const byte *buf, int size, char *name);
  int GetEntry(void **savePtr, char *name = NULL, byte *buf = NULL, int size = 0);

private:
  RemoconList *Next;
  RemoconList *Prev;
  
  char Name[256];
  byte Buf[256];
  int Size;
};

#endif // __RemoconList_h__

