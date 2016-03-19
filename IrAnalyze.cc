/*
 IrAnalyze.cc
 
 Copyright: Copyright (C) 2013 Mitsuru Nakada All Rights Reserved.
 License: GNU GPL v2
*/

#include <string.h>
#include "IrAnalyze.h"

/*
 code[0],code[1]    = length
 code[2]            = FormatAEHA 0x01
 code[3]            = 1TCLK
 code[4] -          = CustomerCode/Parity/Data0, Data1, Data2, ... DataN
 */

int IrAnalyze::CheckWidth(unsigned int t, unsigned int n, unsigned int d) {

  if(d > (t * n) + (t >> 1)) return -1;
  if(d < (t * n) - (t >> 1)) return -1;
  return 0;
}

int IrAnalyze::AnalyzeAEHA(unsigned char *buf) {

  if(buf[2] != 0xff) return -1;
  int dataSize = buf[3];
  if((buf[4] & 0x0f) != 4) return -1;
  
  unsigned int patternTable[8];
  for(int i = 0; i < 8; i++) {
    patternTable[i] = (buf[5 + dataSize + i * 2] << 8) | buf[6 + dataSize + i * 2];
  }
  unsigned int twidth = (patternTable[4] + patternTable[5] + 6) / 12;
  
  if(CheckWidth(twidth, 1, patternTable[0])) return -1;
  if(CheckWidth(twidth, 1, patternTable[1])) return -1;
  if(CheckWidth(twidth, 1, patternTable[2])) return -1;
  if(CheckWidth(twidth, 3, patternTable[3])) return -1;
  if(CheckWidth(twidth, 8, patternTable[4])) return -1;
  if(CheckWidth(twidth, 4, patternTable[5])) return -1;
  if(CheckWidth(twidth, 1, patternTable[6])) return -1;

  if(buf[5] != 0x82) return -1;
  if(buf[5 + dataSize - 1] != 0x83) return -1;
  for(int i = 6; i < 5 + dataSize - 1; i++) {
    if(buf[i] & 0x80) return -1;
    if((buf[i] + 1) % 8) return -1;
    i += (buf[i] + 8) / 8;
  }

  buf[2] = 0x01;
  buf[3] = twidth;
  int p = 4;
  for(int i = 6; i < 5 + dataSize - 1; i++) {
    int l = (buf[i] + 8) / 8;
    i++;
    memcpy(buf + p, buf + i, l);
    p += l;
    i += l;
  }
  buf[1] = p;
  return p;
}

/*
 code[0],code[1]    = length
 code[2]            = FormatNEC 0x02
 code[3]            = 1TCLK
 code[4] - code[7]  = CustomerCode, Data, ~Data
 */
int IrAnalyze::AnalyzeNEC(unsigned char *buf) {
  
  if(buf[2] != 0xff) return -1;
  int dataSize = buf[3];
  if((buf[4] & 0x0f) != 4) return -1;
  
  unsigned int patternTable[8];
  for(int i = 0; i < 8; i++) {
    patternTable[i] = (buf[5 + dataSize + i * 2] << 8) | buf[6 + dataSize + i * 2];
  }
  unsigned int twidth = (patternTable[4] + patternTable[5] + 6) / 24;
  
  if(CheckWidth(twidth, 1, patternTable[0])) return -1;
  if(CheckWidth(twidth, 1, patternTable[1])) return -1;
  if(CheckWidth(twidth, 1, patternTable[2])) return -1;
  if(CheckWidth(twidth, 3, patternTable[3])) return -1;
  if(CheckWidth(twidth, 16, patternTable[4])) return -1;
  if(CheckWidth(twidth, 8, patternTable[5])) return -1;
  if(CheckWidth(twidth, 1, patternTable[6])) return -1;
  
  if(buf[3] != 7) return -1;
  if(buf[5] != 0x82) return -1;
  if(buf[5 + dataSize - 1] != 0x83) return -1;
  for(int i = 6; i < 5 + dataSize - 1; i++) {
    if(buf[i] & 0x80) return -1;
    if((buf[i] + 1) % 8) return -1;
    i += (buf[i] + 8) / 8;
  }
  
  buf[2] = 0x02;
  buf[3] = twidth;
  int p = 4;
  for(int i = 6; i < 5 + dataSize - 1; i++) {
    int l = (buf[i] + 8) / 8;
    i++;
    memcpy(buf + p, buf + i, l);
    p += l;
    i += l;
  }
  buf[1] = p;
  return p;
}

/*
 code[0],code[1]    = length
 code[2]            = FormatSONY 0x03
 code[3]            = 1TCLK
 code[4]            = AddrLen (5/8/13)
 code[5]            = Data
 code[6],code[7]    = Address
 */
int IrAnalyze::AnalyzeSONY(unsigned char *buf) {

  if(buf[2] != 0xff) return -1;
  int dataSize = buf[3];
  if((buf[4] & 0x0f) != 4) return -1;
  
  unsigned int patternTable[8];
  for(int i = 0; i < 8; i++) {
    patternTable[i] = (buf[5 + dataSize + i * 2] << 8) | buf[6 + dataSize + i * 2];
  }
  unsigned int twidth = (patternTable[4] + patternTable[5] + 6) / 5;
  
  if(CheckWidth(twidth, 1, patternTable[0])) return -1;
  if(CheckWidth(twidth, 1, patternTable[1])) return -1;
  if(CheckWidth(twidth, 2, patternTable[2])) return -1;
  if(CheckWidth(twidth, 1, patternTable[3])) return -1;
  if(CheckWidth(twidth, 4, patternTable[4])) return -1;
  if(CheckWidth(twidth, 1, patternTable[5])) return -1;
  int msb = -1;
  if(!CheckWidth(twidth, 1, patternTable[6])) msb = 0;
  if(!CheckWidth(twidth, 2, patternTable[6])) msb = 1;
  if(msb < 0) return -1;

  if((buf[3] != 5) && (buf[3] != 6)) return -1;
  if(buf[5] != 0x82) return -1;
  if(buf[5 + dataSize - 1] != 0x83) return -1;
  for(int i = 6; i < 5 + dataSize - 1; i++) {
    if(buf[i] & 0x80) return -1;
    if((buf[i] != 10) && (buf[i] != 13) && (buf[i] != 18)) return -1;
    i += (buf[i] + 8) / 8;
  }
  
  buf[2] = 0x03;
  buf[3] = twidth;
  buf[4] = buf[6] - 5;
  buf[5] = buf[7] & 0x7f;
  buf[6] = (buf[7] >> 7) | (buf[8] << 1);
  buf[7] = 0;
  if(buf[4] == 13) buf[7] = (buf[8] >> 7) | (buf[9] << 1);
  if(msb && (buf[4] == 5)) buf[6] |= (1 << 4);
  if(msb && (buf[4] == 8)) buf[6] |= (1 << 7);
  if(msb && (buf[4] == 13)) buf[7] |= (1 << 4);
  buf[1] = 8;
  return 8;
}

