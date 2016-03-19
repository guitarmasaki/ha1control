/*
 HA-1 CommandList.cc
 
 Copyright: Copyright (C) 2013 Mitsuru Nakada All Rights Reserved.
 License: GNU GPL v2
 */

#include <string.h>
#include "CommandList.h"

CommandList::CommandList() {
  Next = NULL;
  Prev = NULL;
}

CommandList::~CommandList() {
  if(Prev) {
    TokenSt *t = Prev->Token;
    delete [] Prev->Buf;
    while(t) {
      TokenSt *tn = t->Next;
      delete t;
      t = tn;
    }
    delete Prev;
  }
}

int CommandList::Add(const char *name, const char *commands) {

  CommandList *p = new CommandList;
  if(!p) return Error;
  
  strncpy(p->Name, name, 31);
  p->Name[31] = 0;
  p->Token = NULL;
  p->Buf = new char [strlen(commands) + 1];
  if(!p->Buf) {
    delete p;
    return Error;
  }
  strcpy(p->Buf, commands);
  char *q = strtok(p->Buf, ";\r\n");
  if(!q) {
    delete p;
    delete [] p->Buf;
    return Error;
  }

  TokenSt **prevToken = &p->Token;
  while(q) {
    while((*q == ' ') || (*q == '\t')) q++;
    if(*q) {
      TokenSt *t = new TokenSt;
      if(!t) {
        t = p->Token;
        while(t) {
          TokenSt *tn = t->Next;
          delete t;
          t = tn;
        }
        delete [] p->Buf;
        delete p;
        return Error;
      }
      *prevToken = t;
      t->Next = NULL;
      t->Command = q;
      prevToken = &t->Next;
    }      
    q = strtok(NULL, ";\r\n");  
  }
  
  p->Prev = Prev;
  if(Prev) Prev->Next = p;
  p->Next = NULL;
  Prev = p;
  if(!Next) Next = p;
  return Success;
}

int CommandList::SearchByNameSplit(const char *name, void **savePtr, char *command) {

  if(name) {
    for(CommandList *p = Next; p; p = p->Next) {
      if(!strcmp(p->Name, name)) {
        *savePtr = (void *)p->Token;
        return Success;
      }
    }
    return Error;
  } else {
    if(!*savePtr) return Finish;
    TokenSt *t = (TokenSt *)*savePtr;
    if(command) {
      strncpy(command, t->Command, 255);
      command[255] = 0;
    }
    *savePtr = t->Next;
    return Success;
  }
}

int CommandList::GetEntry(void **savePtr, char *name) {

  CommandList *p = (CommandList *)*savePtr;
  if(p == NULL) p = this;
  p = p->Next;
  if(p == NULL) return Error;

  if(name) strcpy(name, p->Name);
  *savePtr = (void *)p;
  return Success;
}
