/*
 HA-1 IRRemote.cc
 
 Copyright: Copyright (C) 2013 Mitsuru Nakada All Rights Reserved.
 License: GNU GPL v2
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>
#include <fcntl.h>
#include <assert.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>

#include "IRRemote.h"

#define ENABLE_BUZ

void IRRemote::GPIOInit() {

  GPFSELSet(4, FSELALT0); // GPCLK0 -> GPIO4
  // PCM
  if(Revision < 0x10) {
    GPFSELSet(28, FSELALT2); // ALT2 PCM_CLK <- GPIO28
    GPFSELSet(29, FSELALT2); // ALT2 PCM_FS -> GPIO29
    GPFSELSet(30, FSELALT2); // ALT2 PCM_DIN <- GPIO30
  } else {
    GPFSELSet(18, FSELALT0); // ALT0 PCM_CLK <- GPIO18
    GPFSELSet(19, FSELALT0); // ALT0 PCM_FS -> GPIO19
    GPFSELSet(20, FSELALT0); // ALT0 PCM_DIN <- GPIO20
  }
  PCMOutputDisable();
}

void IRRemote::PCMOutputEnable() {

  if(Revision < 0x10) {
    GPFSELSet(31, FSELALT2); // ALT2 PCMDOUT -> GPIO31
  } else {
    GPFSELSet(21, FSELALT0); // ALT0 PCMDOUT -> GPIO21
  }
}

void IRRemote::PCMOutputDisable() {

  if(Revision < 0x10) {
    GPIOClr(31);
    GPFSELSet(31, FSELOUT); // ALT2 clear GPIO31
  } else {
    GPIOClr(21);
    GPFSELSet(21, FSELOUT); // ALT0 clear GPIO21
  }
}

IRRemote::IRRemote() {

  FILE *fp = fopen("/proc/cpuinfo", "r");
  if(fp == NULL) {
    fprintf(stderr, "can't open /proc/cpuinfo\n");
    _exit(-1);
  }
  char buf[256];
  char hardware[256];
  hardware[0] = 0;
  Revision = 0;
  while(!feof(fp)) {
    fgets(buf, 255, fp);
    char *p = strtok(buf, " :\t");
    char *q = strtok(NULL, " :\t");
    if(!strcmp(p, "Hardware")) strcpy(hardware, q);
    if(!strcmp(p, "Revision")) Revision = strtoul(q, NULL, 16);
  }
  fclose(fp);
  if(!Revision || !hardware[0]) {
    fprintf(stderr, "not found infomation /proc/cpuinfo\n");
    _exit(-1);
  }
  if(!strcmp(hardware, "BCM2709") && (Revision < 0x100)) Revision |= 0x0100;

  longword gpio_base = BCM2835_PERI_BASE + GPIO_OFFSET;
  longword gpioclk_base = BCM2835_PERI_BASE + GPIOCLK_OFFSET;
  longword pcm_base = BCM2835_PERI_BASE + PCM_OFFSET;
  if(Revision >= 0x100) {
    gpio_base = BCM2836_PERI_BASE + GPIO_OFFSET;
    gpioclk_base = BCM2836_PERI_BASE + GPIOCLK_OFFSET;
    pcm_base = BCM2836_PERI_BASE + PCM_OFFSET;
  }
  if ((MemFd = open("/dev/mem", O_RDWR|O_SYNC)) < 0) {
    fprintf(stderr, "can't open /dev/mem \n");
    _exit(-1);
  }
  GPIO = (volatile longword *)mmap(NULL, BLOCK_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, MemFd, gpio_base);
  if ((long)GPIO == -1) {
    fprintf(stderr, "mmap error %ld\n", (long)GPIO);
    close(MemFd);
    _exit(-1);
  }
  GPIOCLK = (volatile longword *)mmap(NULL, BLOCK_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, MemFd, gpioclk_base);
  if ((long)GPIOCLK == -1) {
    fprintf(stderr, "mmap error %ld\n", (long)GPIOCLK);
    close(MemFd);
    munmap((void *)GPIO, BLOCK_SIZE);
    _exit(-1);
  }
  PCM = (volatile longword *)mmap(NULL, BLOCK_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, MemFd, pcm_base);
  if ((long)PCM == -1) {
    fprintf(stderr, "mmap error %ld\n", (long)PCM);
    close(MemFd);
    munmap((void *)GPIO, BLOCK_SIZE);
    munmap((void *)GPIOCLK, BLOCK_SIZE);
    _exit(-1);
  }
  GPIOInit();

}

IRRemote::~IRRemote() {

  munmap((void *)GPIO, BLOCK_SIZE);
  munmap((void *)GPIOCLK, BLOCK_SIZE);
  munmap((void *)PCM, BLOCK_SIZE);
  close(MemFd);
}

int IRRemote::Record(char *file) {
  
  // GPCLK0 SamplingFreq = 38kHz * 2 = 76kHz output
  GPClkOutput(SamplingFreq);

  fprintf(stderr, "start\n");
  int ret = PCMRead(PCMBuffer, PCMBufferSize);
  if(ret < 0) {
    fprintf(stderr, "PCMRead error\n");
    return -1;
  }
  GPClkOutput(ClockStop);

  PulseRecord(PCMBuffer, PCMBufferSize);
  ret = AnalyzeAEHA(CodeBuffer);
  if(ret <= 0) ret = AnalyzeNEC(CodeBuffer);
  if(ret <= 0) ret = AnalyzeSONY(CodeBuffer);
  
  ret = CodeBuffer[1];
  if(ret > 4) {
    FILE *fp = fopen(file, "w");
    if(!fp) {
      fprintf(stderr, "file open error %s\n", file);
      return -1;
    }
    fwrite(CodeBuffer, ret, 1, fp);
    fclose(fp);
  }
  return 0;
}

int IRRemote::PulseRecord(longword *buf, int bufSize) {

  // search data section
  int last = -1;
  int c = 0;
  int n = 0;
  int start = 0;
  int end = bufSize * 32;
  for(int i = 0; i < bufSize * 32; i++) {
    int d = (buf[i / 32] >> (31 - (i % 32))) & 1;
    if((last < 0) && (d == 1)) continue; // skip nosignal
    if(!start) start = i;
    if(c > 76 * 100) { // > 100ms
      if(n > 24) {
        end = i;
        break;
      } else {
        start = 0;
        c = 0;
        continue;
      }
    }
    if(c && (d != last)) { // data edge
      c = 1;
      n++;
    } else { // data continue
      c++;
    }
    last = d;
  }

  // make pattern table
  InitPatternTable();
  int lastHi = -1;
  int pattern;
  for(int i = start; i < end; i++) {
    int d = (buf[i / 32] >> (31 - (i % 32))) & 1;
    if(c && (d != last)) { // data edge
      if(d) lastHi = c;
      if(!d && (lastHi > 0)) {
         pattern = SearchPatternTable(lastHi, c);
         CalcAverage(pattern);
      }
      c = 1;
      n++;
    } else { // data continue
      c++;
    }
    last = d;
  }
  pattern = SearchPatternTable(lastHi, 0xffff);
  CalcAverage(pattern);
  MakePatternConvTable();

  // make data
  InitDataBuffer(CodeBuffer + 5, CodeBufferSize - 5 - 4 * GetPatternTableSize());
  lastHi = -1;
  for(int i = start; i < end; i++) {
    int d = (buf[i / 32] >> (31 - (i % 32))) & 1;
    if(c && (d != last)) { // data edge
      if(d) lastHi = c;
      if(!d && (lastHi > 0)) {
        pattern = SearchPatternTable(lastHi, c);
        if(SetData(PatternConv(pattern))) break;
      }
      c = 1;
      n++;
    } else { // data continue
      c++;
    }
    last = d;
  }
  pattern = SearchPatternTable(lastHi, 0xffff);
  SetData(PatternConv(pattern));
  
  int repeatCount = DataRepeatCheck();
  if(repeatCount) CheckPatternTable(CodeBuffer + 5, GetDataSize());
  GetPatternTable(CodeBuffer + 5 + GetDataSize());
  CodeBuffer[0] = 0;
  CodeBuffer[1] = 5 + GetDataSize() + GetPatternTableNum() * 4;
  CodeBuffer[2] = 0xff;
  CodeBuffer[3] = GetDataSize();
  CodeBuffer[4] = (repeatCount << 4) | GetPatternTableNum();

  return 0;
}

int IRRemote::Send(char *file, int repeat) {

  FILE *fp = fopen(file, "r");
  if(fp == NULL) {
    fprintf(stderr, "file error %s\n", file);
    return -1;
  }
  int codeSize = fread(CodeBuffer, 1, CodeBufferSize, fp);
  fclose(fp);
  if(codeSize < 3) {
    fprintf(stderr, "code size error %d\n", codeSize);
    return -1;
  }
  if(!repeat) repeat = 1;
   
  fprintf(stderr, "format type %d\n", CodeBuffer[2]);
  int ret = -1;
  int repeatWait = 0;
  if(CodeBuffer[2] == 1) {
    ret = CodeExpandAEHA(CodeBuffer, codeSize, PCMBuffer, PCMBufferSize);
    repeatWait = 50;
    if((ret + 75) / 76 < 110) repeatWait = 130 - (ret + 75) / 76;
  }
  if(CodeBuffer[2] == 2) {
    ret = CodeExpandNEC(CodeBuffer, codeSize, PCMBuffer, PCMBufferSize);
    repeatWait = 50;
    if((ret + 75) / 76 < 90) repeatWait = 108 - (ret + 75) / 76;
  }
  if(CodeBuffer[2] == 3) {
    ret = CodeExpandSONY(CodeBuffer, codeSize, PCMBuffer, PCMBufferSize);
    repeatWait = 45 - (ret + 75) / 76;
    if(repeat < 3) repeat = 3;
  }
  if(CodeBuffer[2] == 0xff) {
    ret = CodeExpandOther(CodeBuffer, codeSize, PCMBuffer, PCMBufferSize);
    repeatWait = 100;
    int repeatCount = CodeBuffer[4] >> 4;
    if(repeatCount) repeatWait = 0;
    if(repeat < repeatCount) repeat = repeatCount;
  }
  if(ret < 0) {
    fprintf(stderr, "format error\n");
    return -1;
  }
  
  // GPCLK0 SamplingFreq = 38kHz * 2 = 76kHz output
  GPClkOutput(SamplingFreq);
  for(int i = 0; i < repeat; i++) {
    if(i) usleep(repeatWait * 1000);
    PCMWrite(PCMBuffer, (ret + 31) / 32);
  }
  GPClkOutput(ClockStop);
  return 0;
}

int IRRemote::GPClkOutput(int f) { // ret = 0 : success / -1 : error

  if(!f) {
    GPCLKCtlSet(0, (0x5a << 24) | (1 << 9) | 0); // disable clk gen
    return 0;
  }
  int clk = 19200000; // osc clk 19.2MHz
  int divi = clk / f;
  int divf = (clk % f) * 1024 / f;
  if(divi > 0xfff) return -1;
  GPCLKCtlSet(0, (0x5a << 24) | (1 << 9) | 1); // disable clk gen
  int i;
  for(i = 0; i < 10000; i++) if(!(GPCLKCtlGet(0) & (1 << 7))) break;
  if(i >= 10000) return -1;
  GPCLKDivSet(0, (0x5a << 24) | (divi << 12) | divf);
  GPCLKCtlSet(0, (0x5a << 24) | (1 << 9) | (1 << 4) | 1); // enable clk gen
  return 0;
}

int IRRemote::PCMRead(longword *buf, int size) { // ret = maxfifo level, ret < 0 : error
  
#ifdef ENABLE_BUZ
  int buz = 16;
#else
  int buz = 0;
#endif
  PCM[PCMCS] = PCMCSSTBY | PCMCSEN;
  PCM[PCMMODE] = PCMMODECLKDIS | PCMMODECLKM | (31 << SHIFT_PCMMODEFLEN) | (buz << SHIFT_PCMMODEFSLEN); // PCMCLK stop
  PCM[PCMRXC] = PCMRXCCH1WEX | PCMRXCCH1EN | (0 << SHIFT_PCMRXCCH1POS) | (8 << SHIFT_PCMRXCCH1WID);
  PCM[PCMMODE] = PCMMODECLKM | (31 << SHIFT_PCMMODEFLEN) | (buz << SHIFT_PCMMODEFSLEN); // PCMCLK start
  PCM[PCMCS] = PCMCSSTBY | PCMCSRXERR | PCMCSRXCLR| PCMCSEN; // RXERR RXFIFO Clear
  usleep(120); // 120us (> PCMCLK 26us x 4clk)
  PCM[PCMCS] = PCMCSSTBY | PCMCSRXON | PCMCSEN; // RXON

  longword d1 = 0xffffffff;
  longword d2 = 0xffffffff;
  int maxfifo = 0;
  int error = 0;
  int c = 0;
  do {
    d1 = d2;
    while(!(PCM[PCMCS] & PCMCSRXR));
    d2 = PCM[PCMFIFO];
    c++;
  } while(((d2 >> 24) & (d2 >> 16) & (d2 >> 8) & d2) && (c < RecordTimeout * SamplingFreq / 32)); // timout
  if(c >= RecordTimeout * SamplingFreq / 32) {
    PCM[PCMCS] = PCMCSSTBY | PCMCSEN; // RXoff
    return -1;
  }
  
  buf[0] = d1;
  buf[1] = d2;
  for(int i = 2; i < size; i++) {
    while(!(PCM[PCMCS] & PCMCSRXR));
    buf[i] = PCM[PCMFIFO];
    if(PCM[PCMCS] & PCMCSRXERR) error = 1;
    int fifo = (PCM[PCMGRAY] >> 16) & 0x3f;
    if(fifo > maxfifo) maxfifo = fifo;
  }
  PCM[PCMCS] = PCMCSSTBY | PCMCSEN; // RXoff
  if(error) return -1;
  return maxfifo;
}

int IRRemote::PCMWrite(longword *buf, int size) { // ret < 0 : error

  PCMOutputEnable();

#ifdef ENABLE_BUZ
  int buz = 16;
#else
  int buz = 0;
#endif

  PCM[PCMCS] = PCMCSSTBY | PCMCSEN;
  PCM[PCMMODE] = PCMMODECLKDIS | PCMMODECLKM | (31 << SHIFT_PCMMODEFLEN) | (buz << SHIFT_PCMMODEFSLEN); // PCMCLK stop
  PCM[PCMTXC] = PCMTXCCH1WEX | PCMTXCCH1EN | (0 << SHIFT_PCMTXCCH1POS) | (8 << SHIFT_PCMTXCCH1WID);
  PCM[PCMMODE] = PCMMODECLKM | (31 << SHIFT_PCMMODEFLEN) | (buz << SHIFT_PCMMODEFSLEN); // PCMCLK start
  PCM[PCMCS] = PCMCSSTBY | PCMCSTXERR | (3 << SHIFT_PCMCSTXTHR) | PCMCSTXCLR| PCMCSEN; // TXERR TXFIFO Clear
  usleep(120); // 120us (> PCMCLK 26us x 4clk)
  PCM[PCMCS] = PCMCSSTBY | (3 << SHIFT_PCMCSTXTHR) | PCMCSEN;
  // fifo set
  int error = 0;
  PCM[PCMCS] = PCMCSSTBY | (3 << SHIFT_PCMCSTXTHR) | PCMCSTXON | PCMCSEN; // TXON

  for(int i = 0; i < size; i++) {
    while(!(PCM[PCMCS] & PCMCSTXW));
    PCM[PCMFIFO] = buf[i];
    if(PCM[PCMCS] & PCMCSTXERR) {
      error = -1;
      break;
    }
  }
  while(!(PCM[PCMCS] & PCMCSTXE));

  PCM[PCMCS] = PCMCSSTBY | PCMCSEN; // TXoff
  PCMOutputDisable();
  return error;
}

/*
 code[0],code[1]    = length
 code[2]            = FormatAEHA 0x01
 code[3]            = 1TCLK
 code[4],code[5]    = CustomerCode
 code[6] -          = Parity/Data0, Data1, Data2, ... DataN
 */
int IRRemote::CodeExpandAEHA(byte *code, int codeSize, longword *buf, int bufSize) {

  memset(buf, 0, bufSize * 4);
  int len = ((code[0] << 8) | code[1]) - 4;
  int t = code[3];

  int p = 0;
  // leader
  for(int i = 0; i < 8 * t; i++) {
    if(!(p & 1)) buf[p / 32] |= (1 << (31 - (p % 32)));
    p++;
  }
  p += 4 * t;

  // customer code & data
  for(int i = 0; i < len; i++) {
    for(int j = 0; j < 8; j++) {
      int d = (code[4 + i] >> j) & 1;
      for(int k = 0; k < t; k++) {
        if(!(p & 1)) buf[p / 32] |= (1 << (31 - (p % 32)));
        p++;
      }
      p += (d?t*3:t);
    }
  }

  // trailer
  for(int i = 0; i < t; i++) {
    if(!(p & 1)) buf[p / 32] |= (1 << (31 - (p % 32)));
    p++;
  }
  p += 20 * t;
  return p;
}

/*
 code[0],code[1]    = length
 code[2]            = FormatNEC 0x02
 code[3]            = 1TCLK
 code[4],code[5]    = CustomerCode
 code[6],code[7]    = Data, ~Data
 */
int IRRemote::CodeExpandNEC(byte *code, int codeSize, longword *buf, int bufSize) {

  memset(buf, 0, bufSize * 4);
  int len = ((code[0] << 8) | code[1]) - 4;
  int t = code[3];

  int p = 0;
  // leader
  for(int i = 0; i < 16 * t; i++) {
    if(!(p & 1)) buf[p / 32] |= (1 << (31 - (p % 32)));
    p++;
  }
  p += 8 * t;

  // customer code & data
  for(int i = 0; i < len; i++) {
    for(int j = 0; j < 8; j++) {
      int d = (code[4 + i] >> j) & 1;
      for(int k = 0; k < t; k++) {
        if(!(p & 1)) buf[p / 32] |= (1 << (31 - (p % 32)));
        p++;
      }
      p += (d?t*3:t);
    }
  }

  // trailer
  for(int i = 0; i < t; i++) {
    if(!(p & 1)) buf[p / 32] |= (1 << (31 - (p % 32)));
    p++;
  }
  p += 20 * t;
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
int IRRemote::CodeExpandSONY(byte *code, int codeSize, longword *buf, int bufSize) {

  memset(buf, 0, bufSize * 4);
  int t = code[3];
  int alen = code[4];

  int p = 0;
  // leader
  for(int j = 0; j < 4 * t; j++) {
    if(!(p & 1)) buf[p / 32] |= (1 << (31 - (p % 32)));
    p++;
  }
  
  //Data
  for(int j = 0; j < 7; j++) {
    int d = (code[5] >> j) & 1;
    p += t;
    for(int k = 0; k < (d?t*2:t); k++) {
      if(!(p & 1)) buf[p / 32] |= (1 << (31 - (p % 32)));
      p++;
    }
  }
  
  //Addr
  for(int j = 0; j < alen; j++) {
    int d = (code[6 + (j / 8)] >> (j % 8)) & 1;
    p += t;
    for(int k = 0; k < (d?t*2:t); k++) {
      if(!(p & 1)) buf[p / 32] |= (1 << (31 - (p % 32)));
      p++;
    }
  }
  p += t;
  return p;
}

/*
 code[0],code[1]    = length
 code[2]            = FormatOther 0xff
 code[3]            = DataSize
 code[4]            = PatternTableSize
 code[5]-           = Data .....
 code[5+DataSize]-  = PatternTable
 */
int IRRemote::CodeExpandOther(byte *code, int codeSize, longword *buf, int bufSize) {

  memset(buf, 0, bufSize * 4);
  int len = code[3];
  int pmax = code[4] & 0x0f;
  int patternOffset = 5 + len;
  int seq = 0;
  int p = 0;
  int d = 0;
  int bitCount = 0;
  int bitLength = 0;
  for(int i = 5; i - 5 < len; i++) {
    switch(seq) {
      case 0: // header
        d = code[i];
        if(d & 0x80) { // pattern
          int pattern = d & 0x7f;
          int hi = (code[patternOffset + pattern * 4 + 0] << 8) |
                    code[patternOffset + pattern * 4 + 1];
          int lo = (code[patternOffset + pattern * 4 + 2] << 8) |
                    code[patternOffset + pattern * 4 + 3];
          for(int j = 0; j < hi; j++) {
            if(!(p & 1)) buf[p / 32] |= (1 << (31 - (p % 32)));
            p++;
          }
          p += lo;
        } else { // data length
          bitCount = 0;
          bitLength = d + 1;
          seq = 1;
        }
        break;
      case 1: // data
        d = code[i];
        for(int j = 0; j < 8; j++) {
          int bit = (d >> j) & 1;
          int hi = (code[patternOffset + bit * 4 + 0] << 8) |
                    code[patternOffset + bit * 4 + 1];
          int lo = (code[patternOffset + bit * 4 + 2] << 8) |
                    code[patternOffset + bit * 4 + 3];
          for(int k = 0; k < hi; k++) {
            if(!(p & 1)) buf[p / 32] |= (1 << (31 - (p % 32)));
            p++;
          }
          p += lo;
          bitCount++;
          if(bitCount >= bitLength) {
            seq = 0;
            break;
          }
        }
        break;
      default:
        break;
    }
  }
  return p;
}
