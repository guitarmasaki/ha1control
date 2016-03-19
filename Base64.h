/*
 Base64.h
 
 Copyright: Copyright (C) 2013 Mitsuru Nakada All Rights Reserved.
 License: GNU GPL v2
 */

#ifndef __Base64_h__
#define __Base64_h__

#include "Types.h"

class Base64 {

public:
  int Encode(char *enc, int bufSize, byte *data, int len);
  int Decode(byte *data, int bufSize, char *enc);
};

#endif // __Base64_h__

