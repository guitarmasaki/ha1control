/*
 CRC.h

 Copyright: Copyright (C) 2013,2014 Mitsuru Nakada All Rights Reserved.
 License: GNU GPL v2
*/

#ifndef __CRC_h__
#define __CRC_h__

#include "Types.h"

class CRC {
public:
  word CalcCRC(byte* buf, int size, word init_crc = 0);

  inline word CalcCRC16(byte data, word crc) {
    return (crc << 8) ^ CRCTable[((crc >> 8) ^ data) & 0xff];
  }
  
private:
  static const word CRCTable[];
  
};

#endif /* __CRC_h__ */
