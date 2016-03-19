/*
 HA-1 PasswordList.h
 
 Copyright: Copyright (C) 2014 Mitsuru Nakada All Rights Reserved.
 License: GNU GPL v2
 */

#ifndef __PasswordList_h__
#define __PasswordList_h__

#include <stdlib.h>
#include "Error.h"
#include "Types.h"

class PasswordList {

public:
  PasswordList();
  ~PasswordList();

  int SetRegistryFile(const char *path, const char *registry);
  int Update(const char *gateName, const char *customerName, const char *password, longword startValidity, longword endValidity);
  int SearchByName(const char *gateName, const char *password, char *customerName);
  int GetEntry(void **savePtr, char *gateName, char *customerName, char *password, longword *startValidity, longword *endValidity);
  int UpdatePasswordFile(int updateFlag = 0);
  void ClearDirtyFlag() { DirtyFlag = 0; };
  int UpdateOccured() { return DirtyFlag; };

private:
  PasswordList *DeletePassword(PasswordList *p);

  PasswordList *Next;
  PasswordList *Prev;
  char *Path;
  
  char GateName[32];
  char CustomerName[32];
  char Password[16];
  longword StartValidity;
  longword EndValidity;
  int DirtyFlag;

  static const longword MagicCode = 0x50524547; // PREG
};

#endif // __PasswordList_h__

