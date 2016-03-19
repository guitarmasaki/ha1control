/*
 IrPatternTable.h
 
 Copyright: Copyright (C) 2013 Mitsuru Nakada All Rights Reserved.
 License: GNU GPL v2
*/

#ifndef __IrPatternTable_h__
#define __IrPatternTable_h__

class IrPatternTable {

public:
  void InitPatternTable();
  int SearchPatternTable(unsigned int hi, unsigned int lo);
  void CalcAverage(int i);
  void MakePatternConvTable();
  int GetPatternTableSize() { return PatternTableSize; };
  int GetPatternTableNum() { return PatternTableNum - DeletePatternNum; };
  void GetPatternTable(unsigned char *buf);
  int PatternConv(int d) { return PatternConvTable[d]; };
  void CheckPatternTable(unsigned char *buf, int size);

private:
  int IsNearlyEqual(unsigned int cmpHi, unsigned int cmpLo,unsigned int tableHi, unsigned int tableLo);
  void DeletePattern(int d);

  static const int PatternTableSize = 12;
  struct PatternTableSt {
    unsigned int HiTotal;
    unsigned int LoTotal;
    unsigned int Count;
    unsigned int Hi;
    unsigned int Lo;
  };
  PatternTableSt PatternTable[PatternTableSize];
  unsigned char PatternConvTable[PatternTableSize];
  int PatternTableNum;
  int DeletePatternNum;
  unsigned int TableLoMax;
};

#endif // __IrPatternTable_h__
