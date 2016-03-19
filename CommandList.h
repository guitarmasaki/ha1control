/*
 HA-1 CommandList.h
 
 Copyright: Copyright (C) 2013 Mitsuru Nakada All Rights Reserved.
 License: GNU GPL v2
 */

#ifndef __CommandList_h__
#define __CommandList_h__

#include <stdlib.h>
#include "Error.h"

class CommandList {

public:
  CommandList();
  ~CommandList();
  int Add(const char *name, const char *commands);
  int SearchByNameSplit(const char *name, void **savePtr, char *command = NULL);
  int GetEntry(void **savePtr, char *name = NULL);

private:
  CommandList *Next;
  CommandList *Prev;
  
  char Name[32];
  char *Buf;
  
  struct TokenSt {
    TokenSt *Next;
    char *Command;
  };
  TokenSt *Token;
};

#endif // __CommandList_h__

