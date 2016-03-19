/*
 HA-1 PasswordList.cc
 
 Copyright: Copyright (C) 2014 Mitsuru Nakada All Rights Reserved.
 License: GNU GPL v2
 */

#include <string.h>
#include <stdio.h>
#include <sys/time.h>

#include "PasswordList.h"

PasswordList::PasswordList() {
  Next = NULL;
  Prev = NULL;
  Path = NULL;
  DirtyFlag = 1;
}

PasswordList::~PasswordList() {
  if(Path) delete [] Path;
  if(Prev) delete Prev;
}

int PasswordList::SetRegistryFile(const char *path, const char *registry) {

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
  int error = Success;
  while(!feof(fp)) {
    PasswordList *p = new PasswordList;
    if(!p) {
      error = Error;
      break;
    }
    if(fread(p, sizeof(PasswordList), 1, fp) != 1) {
      p->Path = NULL;
      delete p;
      p = NULL;
    }
   
    if(p) {
      p->Path = NULL;
      p->Prev = Prev;
      if(Prev) Prev->Next = p;
      p->Next = NULL;
      Prev = p;
      if(!Next) Next = p;
    }
  }
  fclose(fp);
  if(!error) UpdatePasswordFile();
  return error;
}

int PasswordList::Update(const char *gateName, const char *customerName, const char *password, longword startValidity, longword endValidity) {

  if(startValidity > endValidity) return Error;
  if(!strlen(gateName) || !strlen(customerName) || !strlen(password)) return Error;

  for(PasswordList *p = Next; p; p = p->Next) {
    if(!strcmp(p->GateName, gateName) && !strcmp(p->CustomerName, customerName)) {
      p = DeletePassword(p);
    }
    if(!p) break;
  }

  if(startValidity < endValidity) {
    PasswordList *p = new PasswordList;
    if(!p) return Error;
    strncpy(p->GateName, gateName, 31);
    p->GateName[31] = 0;
    strncpy(p->CustomerName, customerName, 31);
    p->CustomerName[31] = 0;
    strncpy(p->Password, password, 15);
    p->Password[15] = 0;
    p->StartValidity = startValidity;
    p->EndValidity = endValidity;
    p->Prev = Prev;
    if(Prev) Prev->Next = p;
    p->Next = NULL;
    Prev = p;
    if(!Next) Next = p;
  }
  return UpdatePasswordFile(1);
}

int PasswordList::SearchByName(const char *gateName, const char *password, char *customerName) {

  UpdatePasswordFile();

  struct timeval tm;
  gettimeofday(&tm, NULL);
  longword now = tm.tv_sec;
  int findFlag = 0;
  for(PasswordList *p = Next; p; p = p->Next) {
    if(!strcmp(p->GateName, gateName) && !strcmp(p->Password, password) && (p->StartValidity <= now) && (p->EndValidity >= now)) {
      if(customerName) strcpy(customerName, p->CustomerName);
      findFlag = 1;
      break;
    }
  }
  if(findFlag) return Success;
  return Error;
}

int PasswordList::GetEntry(void **savePtr, char *gateName, char *customerName, char *password, longword *startValidity, longword *endValidity) {

  PasswordList *p = (PasswordList *)*savePtr;
  if(p == NULL) {
    UpdatePasswordFile();
    p = this;
  }
  p = p->Next;
  if(p == NULL) return Error;

  if(gateName) strcpy(gateName, p->GateName);
  if(customerName) strcpy(customerName, p->CustomerName);
  if(password) strcpy(password, p->Password);
  if(startValidity) *startValidity = p->StartValidity;
  if(endValidity) *endValidity = p->EndValidity;
  *savePtr = (void *)p;
  return Success;
}

int PasswordList::UpdatePasswordFile(int updateFlag) {

  struct timeval tm;
  gettimeofday(&tm, NULL);
  longword now = tm.tv_sec;

  for(PasswordList *p = Next; p; p = p->Next) {
    if(p->EndValidity <= now) {
      p = DeletePassword(p);
      updateFlag = 1;
    }
    if(!p) break;
  }
  if(updateFlag) {
    DirtyFlag = 1;
    char file[256];
    strcpy(file, Path);
    strcat(file, "~");
    FILE *fp = fopen(file, "w");
    if(fp == NULL) return Error;
    longword magic = MagicCode;
    if(fwrite(&magic, sizeof(longword), 1, fp) != 1) {
      fclose(fp);
      return Error;
    }
    PasswordList *p = Next;
    while(p) {
      if(fwrite(p, sizeof(PasswordList), 1, fp) != 1) {
        fclose(fp);
        return Error;
      }
      p = p->Next;
    }
    fclose(fp);
    return rename(file, Path);
  }
  return Success;
}

PasswordList *PasswordList::DeletePassword(PasswordList *p) {

  if(!p) return NULL;
  if(p->Prev) {
    p->Prev->Next = p->Next;
  } else {
    Next = p->Next;
  }
  if(p->Next) {
    p->Next->Prev = p->Prev;
  } else {
    Prev = p->Prev;
  }
  PasswordList *q = p->Prev;
  p->Prev = NULL;
  delete p;
  return q;
}

