/*

 WebSocket.cc
 
 Copyright: Copyright (C) 2015 Mitsuru Nakada All Rights Reserved.
 License: GNU GPL v2
 */

#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <openssl/rand.h>
#include <openssl/sha.h>
#include <openssl/err.h>

#include "WebSocket.h"
#include "Base64.h"

const char *WebSocket::GUID = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

WebSocket::WebSocket() {

  Sequence = 0;
  ReceiveLastPacketFin = 1;
  SendLastPacketFin = 1;
  if(!RAND_load_file("/dev/urandom", 1024)) {
    LogPrintf("RAND_load_file /dev/urandom : error\n");
  }
  SecureMode = 0;
}

WebSocket::~WebSocket() {

  Close();
}

int WebSocket::SecureSetup(const char *caCertFile, const char *certFile, const char *privateFile, const char *issure, const char *subject, const char *accoutFile) {

#ifdef DEBUG
  LogPrintf("WebSocket::SecureSetup\n");
#endif
  SecureMode = 1;
  return SecureSocket::Setup(caCertFile, certFile, privateFile, issure, subject, accoutFile);
}

int WebSocket::NonSecureSetup() {

#ifdef DEBUG
  LogPrintf("WebSocket::NonSecureSetup\n");
#endif
  SecureMode = 2;
  return Success;
}

int WebSocket::Accept(int sockfd, const char *allowIP, const char *allowMask) {

#ifdef DEBUG
  LogPrintf("WebSocket::Accept %d\n", sockfd);
#endif

  int ret = Error;
  if(SecureMode == 1) ret = SecureSocket::Accept(sockfd, allowIP, allowMask);
  if(SecureMode == 2) ret = Socket::Accept(sockfd, allowIP, allowMask);
  if(ret < 0) return ret;
  Sequence = 1;
  return ret;
}

int WebSocket::Connect(const char *url) {

  if(!url || strncmp(url, "wss://", 6)) return Error;

#ifdef DEBUG
  LogPrintf("WebSocket::Connect %s\n", url);
#endif

  char buf[256];
  strncpy(buf, url + 6, 255);
  buf[255] = 0;
  int port = 0;
  char *host = NULL;
  
  char *p = buf;
  char *c1 = strchr(p, ':');
  if(!c1) c1 = p + strlen(p);
  char *c2 = strchr(p, '/');
  if(!c2) c2 = p + strlen(p);
  if(c1 >= c2) {
    p = strtok(p, "/ \t\r\n");
    if(!p) return Error;
    host = p;
    port = 443;
  } else {
    p = strtok(p, ": \t\r\n");
    if(!p) return Error;
    host = p;
    p = strtok(NULL, "/ \t\r\n");
    if(!p) return Error;
    port = strtoul(p, NULL, 10);
  }
  char path[256];
  path[1] = 0;
  p = strtok(NULL, " \t\r\n");
  if(p) strncpy(path + 1, p, 254);
  path[0] = '/';
  path[255] = 0;

  int newSocket = Error;
  if(SecureMode == 1) newSocket = SecureSocket::Connect(host, port);
  if(SecureMode == 2) newSocket = Socket::Connect(host, port);
  if(newSocket < 0) return newSocket;
  
  byte key[16];
  RAND_bytes(key, 16);
  B64.Encode(WebSocketKey, 255, key, 16);
  
  int ret = SendTextWoPacket(
    "GET %s HTTP/1.1\r\n"
    "Host: %s:%d\r\n"
    "Pragma: no-cache\r\n"
    "Cache-Control: no-cache\r\n"
    "Upgrade: websocket\r\n"
    "Connection: Upgrade\r\n"
    "Origin: https://%s:%d\r\n"
    "Sec-WebSocket-Key: %s\r\n"
    "Sec-WebSocket-Version: 13\r\n\r\n",
    path,
    host, port,
    host, port,
    WebSocketKey);
  SendLastPacketFin = 1;
  Sequence = 2;
  if(ret < 0) return ret;
  return newSocket;
}

int WebSocket::Close() {

  SendPacket(1, Opcode_Close, NULL, 0);
  SecureSocket::Close();
  ReceiveLastPacketFin = 1;
  SendLastPacketFin = 1;
  Sequence = 0;
  return Success;
}

int WebSocket::Receive(int *flag, byte *buf, int bufSize) {

#ifdef DEBUG
  LogPrintf("WebSocket::Receive\n");
#endif

  if(Sequence == 0) return Error;

  byte recvBuf[ConnectReceiveBufferSize + 1];
  int size = SecureSocket::Receive(recvBuf, ConnectReceiveBufferSize);
  if(size <= 0) {
    Sequence = 0;
    return Disconnect;
  }

  if(Sequence == 1) { // Server https
    // T.B.D.
  
  
  
  } else if(Sequence == 2) { // client https
    recvBuf[size] = 0;
#ifdef DEBUG
  LogPrintf("\n%s\n", recvBuf);
#endif
    int flag = 0;
    char *savePtr;
    char *p = strtok_r((char *)recvBuf, "\r\n", &savePtr);
    while(p) {
      char *q = strtok(p, " \r\n");
      char *r = strtok(NULL, " \r\n");
      if(!strncmp(q, "HTTP/", 5)) {
        if(!r) return Error;
        if(strtoul(r, NULL, 10) != 101) return Error;
        flag |= (1 << 0);
      } else if(!strcmp(q, "Upgrade:")) {
        if(!r || strcmp(r, "websocket")) return Error;
        flag |= (1 << 1);
      } else if(!strcmp(q, "Connection:")) {
        if(!r || strcmp(r, "Upgrade")) return Error;
        flag |= (1 << 2);
      } else if(!strcmp(q, "Sec-WebSocket-Accept:")) {
        if(!r) return Error;
        strcat(WebSocketKey, GUID);
        byte md[SHA_DIGEST_LENGTH];
        SHA1((unsigned char *)WebSocketKey, strlen(WebSocketKey), md);
        char expectedValue[256];
        B64.Encode(expectedValue, 255, md, SHA_DIGEST_LENGTH);
        if(strcmp(expectedValue, r)) return Error;
        flag |= (1 << 3);
      } else if(!strcmp(q, "Sec-WebSocket-Extensions:")) {
        return Error;
      } else if(!strcmp(q, "Sec-WebSocket-Protocol:")) {
        return Error;
      }
      p = strtok_r(NULL, "\r\n", &savePtr);
    }
    if(flag != 0b1111) {
      LogPrintf("Error flag = %x\n", flag);
      return Error;
    }
    Sequence = 3;
    return Success;
  } else if(Sequence == 3) {
    if(bufSize < size) return Error;
    if(recvBuf[0] & 0b01110000) return Error;
    int fin = recvBuf[0] >> 7;
    int opcode = recvBuf[0] & 0x0f;
    if(!ReceiveLastPacketFin && (opcode & 7)) return Error;
    if(recvBuf[1] & 0b10000000) return Error;
    int offset = 2;
    unsigned long length = recvBuf[1] & 0b01111111;
    if(length == 126) {
      length = (recvBuf[2] << 8) | recvBuf[3];
      offset += 2;
    } else if(length == 127) {
      if(recvBuf[2] | recvBuf[3] | recvBuf[4] | recvBuf[5]) return Error;
      length = (recvBuf[6] << 24) |
      (recvBuf[7] << 16) |
      (recvBuf[8] << 8) |
      recvBuf[9];
      offset += 8;
    }
    if(length > bufSize) return Error;
    memcpy(buf, recvBuf + offset, length);
    ReceiveLastPacketFin = fin;
    if(opcode > Opcode_Binary) {
      switch(opcode) {
        case Opcode_Close:
          return Disconnect;
        case Opcode_Ping:
          SendPacket(1, Opcode_Pong, buf, length);
        case Opcode_Pong:
          break;
        default:
          return Error;
      }
      return Success;
    }
    if(opcode != Opcode_Continuation) ReceiveLastOpcode = opcode;
    *flag = (ReceiveLastPacketFin << 7) | ReceiveLastOpcode;
    return length;
  }
  return Error;
}

int WebSocket::SendText(int fin, const char *fmt, ...) {

  if(Sequence != 3) return Error;

  va_list args;
  va_start(args, fmt);
  
  char buf[1024];
  vsnprintf(buf, 1024, fmt, args);
  buf[1023] = 0;

  return SendText(fin, (byte *)buf, strlen(buf));
}

int WebSocket::SendText(int fin, const unsigned char *buf, int size) {

  if(Sequence != 3) return Error;

  int opcode = 0;
  if(SendLastPacketFin) opcode = 1;
  int ret = SendPacket(fin, opcode, buf, size);
  SendLastPacketFin = fin;
  return ret;
}

int WebSocket::SendBinary(int fin, const unsigned char *buf, int size) {
  
  if(Sequence != 3) return Error;

  int opcode = 0;
  if(SendLastPacketFin) opcode = 2;
  int ret = SendPacket(fin, opcode, buf, size);
  SendLastPacketFin = fin;
  return ret;
}

int WebSocket::SendPing() {

  if(Sequence != 3) return Error;

  return SendPacket(1, Opcode_Ping, NULL, 0);
}

int WebSocket::SendTextWoPacket(const char *fmt, ...) {
  
  va_list args;
  va_start(args, fmt);
  
  char buf[1024];
  vsnprintf(buf, 1024, fmt, args);
  buf[1023] = 0;

#ifdef DEBUG
  LogPrintf("\n%s\n", buf);
#endif
  return SecureSocket::Send((byte *)buf, strlen(buf));
}

int WebSocket::SendPacket(int fin, int opcode, const byte *buf, int size) {

  if(size > 0x100000) return Error;

  byte *sendBuf = new byte [size + 14];
  sendBuf[0] = ((fin?1:0) << 7) | opcode;
  int headerSize = 1;
  if(size < 126) {
    sendBuf[1] = (1 << 7) | size;
    headerSize += 1;
  } else if(size < 0x10000) {
    sendBuf[1] = (1 << 7) | 126;
    sendBuf[2] = size >> 8;
    sendBuf[3] = size;
    headerSize += 3;
  } else {
    sendBuf[1] = (1 << 7) | 127;
    sendBuf[2] = 0;
    sendBuf[3] = 0;
    sendBuf[4] = 0;
    sendBuf[5] = 0;
    sendBuf[6] = size >> 24;
    sendBuf[7] = size >> 16;
    sendBuf[8] = size >> 8;
    sendBuf[9] = size;
    headerSize += 9;
  }
  byte key[4];
  RAND_bytes(key, 4);
  sendBuf[headerSize++] = key[0];
  sendBuf[headerSize++] = key[1];
  sendBuf[headerSize++] = key[2];
  sendBuf[headerSize++] = key[3];
  for(int i = 0; i < size; i++) {
    sendBuf[headerSize + i] = buf[i] ^ key[i % 4];
  }
  int ret = SecureSocket::Send(sendBuf, headerSize + size);
  delete [] sendBuf;
  return ret;
}
