/*
 HA-1 IRRemote.h
 
 Copyright: Copyright (C) 2013 Mitsuru Nakada All Rights Reserved.
 License: GNU GPL v2
 */

#ifndef __IRRemote_h__
#define __IRRemote_h__

#include "BCM2835Peri.h"
#include "Types.h"
#include "IrDataBuffer.h"
#include "IrPatternTable.h"
#include "IrAnalyze.h"

class IRRemote : private IrDataBuffer, private IrPatternTable, private IrAnalyze {

public:
  IRRemote();
  ~IRRemote();  
  int Record(char *file);
  int Send(char *file, int repeat = 1);

private:
  int GPClkOutput(int f);
  int PCMRead(longword *buf, int size);
  int PCMWrite(longword *buf, int size);
  int PulseRecord(longword *buf, int bufSize);

  int CodeExpandAEHA(byte *code, int codeSize, longword *buf, int bufSize);
  int CodeExpandNEC(byte *code, int codeSize, longword *buf, int bufSize);
  int CodeExpandSONY(byte *code, int codeSize, longword *buf, int bufSize);
  int CodeExpandOther(byte *code, int codeSize, longword *buf, int bufSize);

  void GPCLKCtlSet(int n, longword d) { GPIOCLK[GPCLKCTL + n * 8] = d; }
  longword GPCLKCtlGet(int n) { return GPIOCLK[GPCLKCTL + n * 8]; }
  void GPCLKDivSet(int n, longword d) { GPIOCLK[GPCLKDIV + n * 8] = d; }

  void GPFSELSet(int n, int f) { GPIO[GPFSEL+n/10] = (GPIO[GPFSEL+n/10] & ~(7<<((n%10)*3)))|(f<<((n%10)*3)); }
  void GPIOSet(int n) { GPIO[GPSET + n / 32] = 1 << (n % 32); }
  void GPIOClr(int n) { GPIO[GPCLR + n / 32] = 1 << (n % 32); }

  void GPIOInit();
  void PCMOutputEnable();
  void PCMOutputDisable();

  static const int PCMBufferSize = 4096;
  static const int CodeBufferSize = 1024;
  static const int RecordTimeout = 10; // 10sec
  static const int SamplingFreq = 38000 * 2;
  static const int ClockStop = 0;

  longword PCMBuffer[PCMBufferSize];
  byte CodeBuffer[CodeBufferSize];
  
  int MemFd;
  longword Revision;
  volatile longword *GPIO;
  volatile longword *GPIOCLK;
  volatile longword *PCM;

  int TWidth;
};

#endif // __IRRemote_h__

