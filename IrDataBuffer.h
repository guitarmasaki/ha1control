/*
 IrDataBuffer.h
 
 Copyright: Copyright (C) 2013 Mitsuru Nakada All Rights Reserved.
 License: GNU GPL v2
*/

#ifndef __IrDataBuffer_h__
#define __IrDataBuffer_h__

class IrDataBuffer {

public:
  void InitDataBuffer(unsigned char *buf, int size);
  int SetData(int idx);
  int DataRepeatCheck();
  int GetDataSize() { return BufP; };
  
  void InitPulseBuffer();
  int GetPulse();
  void SetPulse(unsigned char d);
  int GetPulseBufSize() { return PulseBufSize; };

private:
  unsigned char *Buf;
  int BufSize;
  int HeaderP;
  int BufP;
  
  int PulseRP;
  int PulseWP;
  static const int PulseBufSize = 64;
  unsigned char PulseBuf[PulseBufSize / 2];
};

#endif // __IrDataBuffer_h__
