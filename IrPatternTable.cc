/*
 PatternTable.cc
 
 Copyright: Copyright (C) 2013 Mitsuru Nakada All Rights Reserved.
 License: GNU GPL v2
*/

#include "IrPatternTable.h"

void IrPatternTable::InitPatternTable() {

  TableLoMax = 0;
  PatternTableNum = 0;
  DeletePatternNum = 0;
}

int IrPatternTable::SearchPatternTable(unsigned int hi, unsigned int lo) {
  
  if((lo != 0xffff) && (lo > TableLoMax)) TableLoMax = lo;
  unsigned int lox = lo;
  if(lox == 0xffff) lox = TableLoMax;
  int i = 0;
  for(; i < PatternTableNum; i++) {
    if(!IsNearlyEqual(hi, lox, PatternTable[i].Hi, PatternTable[i].Lo)) {
      PatternTable[i].HiTotal += hi;
      PatternTable[i].LoTotal += lox;
      PatternTable[i].Count++;
      return i;
    }
  }
  if(PatternTableNum >= PatternTableSize) return -1;
  PatternTable[i].HiTotal = hi;
  PatternTable[i].LoTotal= lox;
  PatternTable[i].Count = 1;
  PatternTable[i].Hi = hi;
  PatternTable[i].Lo = lox;
  PatternTableNum++;
  return i;
}

void IrPatternTable::CalcAverage(int i) {
  
  if(i < 0) return;
  unsigned int count = PatternTable[i].Count;
  PatternTable[i].Hi = (PatternTable[i].HiTotal + (count >> 1)) / count;
  PatternTable[i].Lo = (PatternTable[i].LoTotal + (count >> 1)) / count;
}

void IrPatternTable::MakePatternConvTable() {
  
  int p1 = -1;
  unsigned int cmax = 0;
  for(int i = 0; i < PatternTableNum; i++) {
    if(PatternTable[i].Count > cmax) {
      p1 = i;
      cmax = PatternTable[i].Count;
    }
  }
  int p2 = -1;
  cmax = 0;
  for(int i = 0; i < PatternTableNum; i++) {
    if(i == p1) continue;
    if(PatternTable[i].Count > cmax) {
      p2 = i;
      cmax = PatternTable[i].Count;
    }
  }
  
  int h1 = PatternTable[p1].Hi;
  int l1 = PatternTable[p1].Lo;
  int h2 = PatternTable[p2].Hi;
  int l2 = PatternTable[p2].Lo;
  if(((h1 + l1) > (h2 + l2)) || (((h1 + l1) == (h2 + l2)) && (h1 > h2))) {
    h1 = p1;
    p1 = p2;
    p2 = h1;
  }
  PatternConvTable[p1] = 0;
  PatternConvTable[p2] = 1;
  int p = 2;
  for(int i = 0; i < PatternTableSize; i++) {
    if((i == p1) || (i == p2)) continue;
    PatternConvTable[i] = p++;
  }
}

int IrPatternTable::IsNearlyEqual(unsigned int cmpHi, unsigned int cmpLo,unsigned int tableHi, unsigned int tableLo) {
  
  if(tableHi < 50) {
    if((tableHi + (tableHi >> 2) + (tableHi >> 3)) < cmpHi) return -1;
    if((tableHi - (tableHi >> 2) - (tableHi >> 3)) > cmpHi) return -1;
  } else {
    if((tableHi + (tableHi >> 2)) < cmpHi) return -1;
    if((tableHi - (tableHi >> 2)) > cmpHi) return -1;
  }
  if(cmpLo != 0xffff) {
    if(tableLo < 50) {
      if((tableLo + (tableLo >> 2) + (tableLo >> 3)) < cmpLo) return -1;
      if((tableLo - (tableLo >> 2) - (tableLo >> 3)) > cmpLo) return -1;
    } else {
      if((tableLo + (tableLo >> 2)) < cmpLo) return -1;
      if((tableLo - (tableLo >> 2)) > cmpLo) return -1;
    }
  } else {
    if((tableLo + (tableLo >> 2)) < TableLoMax) return -1;
    if((tableLo - (tableLo >> 2)) > TableLoMax) return -1;
  }
  return 0;
}

void IrPatternTable::GetPatternTable(unsigned char *buf) {

  for(int i = 0; i < PatternTableNum; i++) {
    if(PatternConvTable[i] >= PatternTableNum) continue;
    int p = PatternConvTable[i] * 4;
    buf[p + 0] = PatternTable[i].Hi >> 8;
    buf[p + 1] = PatternTable[i].Hi;
    buf[p + 2] = PatternTable[i].Lo >> 8;
    buf[p + 3] = PatternTable[i].Lo;
  }
}

void IrPatternTable::CheckPatternTable(unsigned char *buf, int size) {

  unsigned int pattern = 0;
  for(int i = 0; i < size; i++) {
    if(buf[i] & 0x80) {
      pattern |= 1 << (buf[i] & 0x7f);
    } else {
      i += (buf[i] + 8) / 8;
    }
  }
  for(int i = 2; i < PatternTableNum; i++) {
    if(pattern & (1 << i)) continue;
    DeletePattern(i);
    for(int j = 0; j < size; j++) {
      if(buf[j] & 0x80) {
        if((buf[j] & 0x7f) > i) buf[j]--;
      } else {
        j += (buf[j] + 8) / 8;
      }
    }
  }
}

void IrPatternTable::DeletePattern(int n) {
  
  for(int i = 2; i < PatternTableNum; i++) {
    if(PatternConvTable[i] == 0xff) continue;
    if(PatternConvTable[i] == n) {
      PatternConvTable[i] = 0xff;
    } else if(PatternConvTable[i] > n) {
      PatternConvTable[i]--;
    }
  }
  DeletePatternNum++;
}


