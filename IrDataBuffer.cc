/*
 IrDataBuffer.cc
 
 Copyright: Copyright (C) 2013 Mitsuru Nakada All Rights Reserved.
 License: GNU GPL v2
*/

#include <string.h>
#include "IrDataBuffer.h"


void IrDataBuffer::InitDataBuffer(unsigned char *buf, int size) {

  Buf = buf;
  BufSize = size;
  BufP = 0;
  HeaderP = -1;
}

int IrDataBuffer::SetData(int idx) {

  if(idx < 2) {
    if((HeaderP < 0) || (Buf[HeaderP] == 0x7f)) {
      HeaderP = BufP;
      Buf[BufP++] = 0xff;
    }
    Buf[HeaderP]++;
    int bits = Buf[HeaderP] % 8;
    int offset = HeaderP + 1 + (Buf[HeaderP] / 8);
    if(offset >= BufSize) return -1;
    if(!bits) {
      Buf[offset] = 0;
      BufP++;
    }
    Buf[offset] |= idx << bits;
  } else {
    HeaderP = -1;
    if(BufP >= BufSize) return -1;
    Buf[BufP++] = (1 << 7) | idx;
  }
  return 0;
}

int IrDataBuffer::DataRepeatCheck() {

  int repeatCount = 0;
  for(int r = 4; r * 4 <= BufP; r++) {
    int c = r;
    if(Buf[r - 1] & 0x80) c--;
    if(!memcmp(Buf, Buf + r, c) && 
       !memcmp(Buf, Buf + r * 2, c) &&
       !memcmp(Buf, Buf + r * 3, c)) {
      BufP = r;
      repeatCount = 1;
      break;
    }
  }
  int l = BufP / 3;
  int c = l;
  if(Buf[l - 1] & 0x80) c--;
  if((l >= 4) && (l * 3 == BufP) && 
     !memcmp(Buf, Buf + l, c) && 
     !memcmp(Buf, Buf + l * 2, c)) {
    BufP = l;
    repeatCount = 3;
  }
  l = BufP / 2;
  c = l;
  if(Buf[l - 1] & 0x80) c--;
  if((l >= 4) && (l * 2 == BufP) && 
     !memcmp(Buf, Buf + l, c)) {
    BufP = l;
    repeatCount = 2;
  }
  return repeatCount;
}

void IrDataBuffer::InitPulseBuffer() {

  PulseWP = 0;
  PulseRP = 0;
}

int IrDataBuffer::GetPulse() {

  int d;
  if(PulseRP & 1) {
    d = PulseBuf[PulseRP / 2] & 0x0f;
  } else {
    d = PulseBuf[PulseRP / 2] >> 4;
  }
  PulseRP++;
  if(PulseRP >= PulseBufSize) PulseRP = 0;
  return d;
}

void IrDataBuffer::SetPulse(unsigned char d) {
  
  if(PulseWP & 1) {
    PulseBuf[PulseWP / 2] &= 0xf0;
    PulseBuf[PulseWP / 2] |= d;
  } else {
    PulseBuf[PulseWP / 2] &= 0x0f;
    PulseBuf[PulseWP / 2] |= d << 4;
  }
  PulseWP++;
  if(PulseWP >= PulseBufSize) PulseWP = 0;
}

