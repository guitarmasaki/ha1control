/*

 WebSocket.h
 
 Copyright: Copyright (C) 2015 Mitsuru Nakada All Rights Reserved.
 License: GNU GPL v2
 */

#ifndef __WebSocket_h__
#define __WebSocket_h__

#include <sys/time.h>
#include <unistd.h>

#include "SecureSocket.h"
#include "Base64.h"

class WebSocket : public SecureSocket {

public:
  WebSocket();
  ~WebSocket();

  int SecureSetup(const char *caCertFile, const char *certFile, const char *privateFile, const char *issure, const char *subject, const char *accoutFile);
  int NonSecureSetup();
  
  int Accept(int sockfd, const char *allowIP = NULL, const char *allowMask = NULL);
  int Connect(const char *url);
  int Close();

  int Receive(byte *buf, int size) { int d; return Receive(&d, buf, size); }
  int Send(byte *buf, int size) { return SendBinary(1, buf, size); }
  int Receive(int *flag, byte *buf, int size);
  int SendText(int fin, const char *fmt, ...);
  int SendText(int fin, const unsigned char *buf, int size);
  int SendBinary(int fin, const unsigned char *buf, int size);
  int SendPing();

private:
  int SendTextWoPacket(const char *fmt, ...);
  int SendPacket(int fin, int opcode, const byte *buf, int size);

  static const char *GUID;
  static const int ConnectReceiveBufferSize = 1024;

  enum Opcode {
    Opcode_Continuation = 0,
    Opcode_Text,
    Opcode_Binary,
    Opcode_Close = 8,
    Opcode_Ping,
    Opcode_Pong
  };
  
  Base64 B64;
  int Sequence;
  int SecureMode;
  int SendLastPacketFin;
  int ReceiveLastPacketFin;
  int ReceiveLastOpcode;
  char WebSocketKey[256];
};

#endif // __WebSocket_h__

