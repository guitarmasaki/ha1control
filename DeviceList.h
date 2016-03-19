/*
 HA-1 DeviceList.h
 
 Copyright: Copyright (C) 2013 Mitsuru Nakada All Rights Reserved.
 License: GNU GPL v2
 */

#ifndef __DeviceList_h__
#define __DeviceList_h__

#include <stdlib.h>
#include "Error.h"
#include "Types.h"

class DeviceList {

public:
  DeviceList();
  ~DeviceList();

  int SetRegistryFile(const char *path, const char *registry);
  int Add(const char *name, longword addrL);
  int UpdateStatus();

  DeviceList *SearchByName(const char *name);
  DeviceList *SearchByNameMatch(const char *searchName);
  DeviceList *SearchByAddrL(longword addrL);
  DeviceList *NextList();

  void SetAliveInterval(int aliveInterval) {AliveInterval = aliveInterval;}
  int GetAliveInterval() { return AliveInterval; }
  void GetName(char *name) { strcpy(name, Name); }
  void SetType(char *type) { strncpy(Type, type, 31); Type[31] = 0; }
  void GetType(char *type) { strcpy(type, Type); }
  longword GetAddrL() { return AddrL; }
  void SetFunc(longword func) { Func = func; }
  longword GetFunc() { return Func; }

  void SetData(word digital, int *analog, int voltage);
  void SetData(byte stat, int *analog);
  word GetDigital() { return Digital; }
  void SetLastDigital();
  word GetLastDigital() { return LastDigital; }
  word GetAnalog(int n) { if((n >= 0) && (n <= 5)) { return Analog[n]; } else { return 0; }}
  word GetVoltage() { return Voltage; }
  byte GetStat() { return Stat; }
  void SetLastStat();
  byte GetLastStat() { return LastStat; }
  void GetLastAccess(timeval *tv) { tv->tv_sec = LastAccess.tv_sec; tv->tv_usec = LastAccess.tv_usec; }
  void GetLastAVRAccess(timeval *tv) { tv->tv_sec = LastAVRAccess.tv_sec; tv->tv_usec = LastAVRAccess.tv_usec; }
  void SetLastDeviceCheck();
  void ClearLastDeviceCheck() { LastDeviceCheck.tv_sec = 0; LastDeviceCheck.tv_usec = 0; }
  void GetLastDeviceCheck(timeval *tv) { tv->tv_sec = LastDeviceCheck.tv_sec; tv->tv_usec = LastDeviceCheck.tv_usec; }
  void SetBootVer(int ver) { BootVer = ver; }
  word GetBootVer() { return BootVer; }
  void SetFWVer(word ver) { FWVer = ver; }
  word GetFWVer() { return FWVer; }
  void SetFWUpdateFlag(word f) { FWUpdateFlag = f; }
  word GetFWUpdateFlag() { return FWUpdateFlag; }

  void SetSequenceID(longword addrL, int sequenceID);
  int GetSequenceID(longword addrL);
  void SetAliveInterval(longword addrL, int interval);
  int IncreaseAliveInterval(longword addrL);
  void SetNetworkAddr(longword addrL, word netAddr);
  word GetNetworkAddr() { return NetworkAddr; }
  word GetNetworkAddr(longword addrL);

  bool DeviceFuncGPO0()   { return Func & DeviceFunc_GPO0; }
  bool DeviceFuncGPO1()   { return Func & DeviceFunc_GPO1; }
  bool DeviceFuncHA0O()   { return Func & DeviceFunc_HA0O; }
  bool DeviceFuncHA1O()   { return Func & DeviceFunc_HA1O; }
  bool DeviceFuncSW0O()   { return Func & DeviceFunc_SW0O; }
  bool DeviceFuncSW1O()   { return Func & DeviceFunc_SW1O; }
  bool DeviceFuncSW2O()   { return Func & DeviceFunc_SW2O; }
  //
  bool DeviceFuncGPI0()   { return Func & DeviceFunc_GPI0; }
  bool DeviceFuncGPI1()   { return Func & DeviceFunc_GPI1; }
  bool DeviceFuncHA0I()   { return Func & DeviceFunc_HA0I; }
  bool DeviceFuncHA1I()   { return Func & DeviceFunc_HA1I; }
  bool DeviceFuncSW0I()   { return Func & DeviceFunc_SW0I; }
  bool DeviceFuncSW1I()   { return Func & DeviceFunc_SW1I; }
  bool DeviceFuncSW2I()   { return Func & DeviceFunc_SW2I; }
  //
  bool DeviceFuncAD0()    { return Func & DeviceFunc_AD0;  }
  bool DeviceFuncAD1()    { return Func & DeviceFunc_AD1;  }
  bool DeviceFuncHA0()    { return Func & DeviceFunc_HA0;  }
  bool DeviceFuncHA1()    { return Func & DeviceFunc_HA1;  }
  bool DeviceFuncSW()     { return Func & DeviceFunc_SW;   }
  bool DeviceFuncIRO()    { return Func & DeviceFunc_IRO;  }
  bool DeviceFuncIRI()    { return Func & DeviceFunc_IRI;  }
  bool DeviceFuncI2C()    { return Func & DeviceFunc_I2C;  }
  bool DeviceFuncRAIN()   { return Func & DeviceFunc_RAIN; }
  bool DeviceFuncLED()    { return Func & DeviceFunc_LED;  }
  bool DeviceFuncMTR()    { return Func & DeviceFunc_MTR;  }
  bool DeviceFuncKPD()    { return Func & DeviceFunc_KPD;  }
  //
  //
  bool DeviceFuncHB()     { return Func & DeviceFunc_HB;  }
  bool DeviceFuncAVR()    { return Func & DeviceFunc_AVR; }

private:
  DeviceList *Next;
  DeviceList *Prev;
  char *Path;

  char    Name[32];
  char    Type[32];
  longword AddrL;
  longword Func;
  word    Digital;
  word    LastDigital;
  byte    Stat;
  byte    LastStat;
  word    Analog[6];
  int     SequenceID;      // Initialize
  int     AliveInterval;   // Initialize
  word    Voltage;         // Initialize
  word    NetworkAddr;     // Initialize
  word    FWVer;
  word    BootVer;
  word    FWUpdateFlag;    // Initialize 0:UpdateCheck / 1:ForceUpdate / 2:Updateing / 3:UpdateDone
  timeval LastAccess;
  timeval LastAVRAccess;
  timeval LastDeviceCheck; // Initialize

  static const longword DeviceFunc_GPO0   = (1 <<  0); // AVR GPO0 (PC4) Output
  static const longword DeviceFunc_GPO1   = (1 <<  1); // AVR GPO1 (PC5) Output
  static const longword DeviceFunc_HA0O   = (1 <<  2); // HA0 GPO Output
  static const longword DeviceFunc_HA1O   = (1 <<  3); // HA1 GPO Output
  static const longword DeviceFunc_SW0O   = (1 <<  4); // SW0 GPO Output
  static const longword DeviceFunc_SW1O   = (1 <<  5); // SW1 GPO Output
  static const longword DeviceFunc_SW2O   = (1 <<  6); // SW2 GPO Output
  //
  static const longword DeviceFunc_GPI0   = (1 <<  8); // AVR GPI0 (PC4) Input
  static const longword DeviceFunc_GPI1   = (1 <<  9); // AVR GPI1 (PC5) Input
  static const longword DeviceFunc_HA0I   = (1 << 10); // HA0 GPI Input
  static const longword DeviceFunc_HA1I   = (1 << 11); // HA1 GPI Input
  static const longword DeviceFunc_SW0I   = (1 << 12); // SW0 GPI Input
  static const longword DeviceFunc_SW1I   = (1 << 13); // SW1 GPI Input
  static const longword DeviceFunc_SW2I   = (1 << 14); // SW2 GPI Input
  //
  static const longword DeviceFunc_AD0    = (1 << 16); // AVR A/D ch0 (AD6)
  static const longword DeviceFunc_AD1    = (1 << 17); // AVR A/D ch1 (AD7)
  static const longword DeviceFunc_HA0    = (1 << 18); // HA Control ch0
  static const longword DeviceFunc_HA1    = (1 << 19); // HA Control ch1
  static const longword DeviceFunc_SW     = (1 << 20); // FET-SW Control
  static const longword DeviceFunc_IRO    = (1 << 21); // IR-output
  static const longword DeviceFunc_IRI    = (1 << 22); // IR-input
  static const longword DeviceFunc_I2C    = (1 << 23); // AVR I2C
  static const longword DeviceFunc_RAIN   = (1 << 24); // Rain Sensor Input
  static const longword DeviceFunc_LED    = (1 << 25); // LED Tape
  static const longword DeviceFunc_MTR    = (1 << 26); // MotorControl
  static const longword DeviceFunc_KPD    = (1 << 27); // Keypad
  //
  //
  static const longword DeviceFunc_HB     = (1 << 30); // HeartBeat
  static const longword DeviceFunc_AVR    = (1 << 31); // AVR Device

  static const longword MagicCode = 0x56524534; // VRE4
};

#endif // __DeviceList_h__

