/*
 HA-1 Base64.cc
 
 Copyright: Copyright (C) 2013 Mitsuru Nakada All Rights Reserved.
 License: GNU GPL v2
 */

#include "Base64.h"
#include <string.h>
#include <stdio.h>

int Base64::Encode(char *enc, int bufSize, byte *data, int len) {

  int outLen = (len * 8 + 5) / 6;
  if(bufSize < ((outLen + 3) / 4) * 4 + 1) return -1;
  
  int p = 0;
  for(int i = 0; i < len; i += 3) {
    longword d = (data[i] << 16);
    if(i + 1 < len) d |= (data[i + 1] << 8);
    if(i + 2 < len) d |= data[i + 2];
    for(int j = 3; j >= 0; j--) {
      byte c = (d >> (6 * j)) & 0x3f;
      if(c < 26) {
        c = c + 'A';
      } else if(c < 52) {
        c = c - 26 + 'a';
      } else if(c < 62) {
        c = c - 52 + '0';
      } else if(c == 62) {
        c = '+';
      } else {
        c = '/';
      }
      if(p >= outLen) c = '=';
      enc[p++] = c;
    }
  }
  enc[p] = 0;
  return p;
}

int Base64::Decode(byte *dec, int bufSize, char *data) {

  int len = strlen(data);
  if(len % 4) return -1;
  int outLen = len / 4 * 3;
  if(data[len - 1] == '=') outLen--;
  if(data[len - 2] == '=') outLen--;
  if(bufSize < outLen) return -1;

  int p = 0;
  longword d;
  for(int i = 0; i < len; i++) {
    char c = data[i];
    if((c >= 'A') && (c <= 'Z')) {
      c = c - 'A';
    } else if((c >= 'a') && (c <= 'z')) {
      c = c - 'a' + 26;
    } else if((c >= '0') && (c <= '9')) {
      c = c - '0' + 52;
    } else if(c == '+') {
      c = 62;
    } else if(c == '/') {
      c = 63;
    } else if(c == '=') {
      c = 0;
    } else {
      return -1;
    }
    
    d <<= 6;
    d |= c;
    if(i && ((i % 4) == 3)) {
      dec[p++] = d >> 16;
      if(p < outLen) dec[p++] = d >> 8;
      if(p < outLen) dec[p++] = d;
    }
  }
  return p;
}
