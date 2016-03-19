/*
 IrAnalyze.h
 
 Copyright: Copyright (C) 2013 Mitsuru Nakada All Rights Reserved.
 License: GNU GPL v2
*/

#ifndef __IrAnalyze_h__
#define __IrAnalyze_h__

class IrAnalyze {

public:
  int AnalyzeAEHA(unsigned char *buf);
  int AnalyzeNEC(unsigned char *buf);
  int AnalyzeSONY(unsigned char *buf);

private:
  int CheckWidth(unsigned int t, unsigned int n, unsigned int d);
};

#endif // __IrAnalyze_h__
