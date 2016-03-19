/*
 HA-1 HA1Control_Comunication.cc
 
 Copyright: Copyright (C) 2013 Mitsuru Nakada All Rights Reserved.
 License: GNU GPL v2
 */

#include <fcntl.h>
#include <stdarg.h>
#include <netdb.h>

#include "HA1Control.h"

int HA1Control::ReadExternalPort(int sockfd, int externalPort) {

  if(externalPort == ExternalPort_ListeningPort) return AcceptCommandIF(sockfd);
  if(externalPort == ExternalPort_CommandIFPort) return CommandIFReceive(sockfd);
  if(externalPort == ExternalPort_WebSocketIFPort) return CommandIFReceive(sockfd);
  if(externalPort == ExternalPort_UART) return XBeeReceive(sockfd);
  return Error;
}

int HA1Control::AcceptCommandIF(int sockfd) {

#ifdef DEBUG
  LogPrintf("AcceptCommandIF %d\n", sockfd);
#endif

  Socket *cmdIF = new Socket;
  cmdIF->LogOpen(LogFile);
  int newSocket = cmdIF->Accept(sockfd, CommandIFIP, CommandIFMask);
  if(newSocket < 0) return Error;
  if(newSocket >= MaxConnect) {
    LogPrintf("Connection too much\n");
    cmdIF->LogClose();
    cmdIF->Close();
    return Error;
  }
  
  ClientInfo[newSocket].IF = cmdIF;
  ClientInfo[newSocket].Format = Format_TEXT;
  InputBuffer[newSocket].Clean();
  AddExternalPort(newSocket, ExternalPort_CommandIFPort);
  return newSocket;
}

int HA1Control::CommandIFReceive(int sockfd) {
  
#ifdef DEBUG
  LogPrintf("CommandIFReceive %d\n", sockfd);
#endif

  int webIF = (ClientInfo[sockfd].IF == &WebSocketIF);
  int error = Success;
  char cmdBuf[ConnectReceiveBufferSize + 1];
  cmdBuf[0] = 0;
  while(1) {
    byte buf[ConnectReceiveBufferSize + 1];
    int flag = 0;
    int size;
    if(webIF) {
      size = WebSocketIF.Receive(&flag, buf, ConnectReceiveBufferSize);
      if(WebSocketIntialState == 0) {
        if((SendNotify("client_ui", ClientUi) >= 0) &&
           (SendNotify("allowClient", AllowClientList) >= 0)) WebSocketIntialState = 1;
        PasswordCheck(1);
      }
    } else {
      size = ClientInfo[sockfd].IF->Receive(buf, ConnectReceiveBufferSize);
    }
    if(size <= 0) {
      error = size;
      break;
    }

    int escFlag = 0;
    for(int i = 0; i < size; i++) {
      if(escFlag == 2) {
        unsigned char buf2[5];
        buf2[0] = 0xff;
        buf2[1] = 0xfc;
        buf2[2] = buf[i];
        buf2[3] = 0x0d;
        buf2[4] = 0x0a;
        if(webIF) {
          WebSocketIF.SendText(1, buf2, 5);
        } else {
          ClientInfo[sockfd].IF->Send(buf2, 5);
        }
        escFlag = 0;
      } else if((escFlag == 1) && (buf[i] == 0xfd)) {
        escFlag++;
      } else if(!escFlag && (buf[i] == 0xff)) {
        escFlag++;
      }
    }
    
    int ret = InputBuffer[sockfd].AddBuffer(buf, size);
    if(ret < 0) LogPrintf("InputBufferFull sockfd %d\n", sockfd);


    while(1) {
      error = Success;
      if(ClientInfo[sockfd].Format == Format_TEXT) {
        if((flag & 0x80) && ((flag & 7) == 1)) InputBuffer[sockfd].PutBuffer(0x0a);
        
        int len = InputBuffer[sockfd].GetLen();
        if(len && (InputBuffer[sockfd].PeekBuffer(0) == 0x04)) {
          error = Disconnect;
          break;
        }
        int i;
        for(i = 0; i < len; i++) {
          if(InputBuffer[sockfd].PeekBuffer(i) == 0x0a) break;
        }
        if(i == len) break;
        
        int p = 0;
        if(len > ConnectReceiveBufferSize) len = ConnectReceiveBufferSize;
        for(int i = 0; i < len; i++) {
          int d = InputBuffer[sockfd].GetBuffer();
          if(d == 0x0a) break;
          if(d == 0x00) break;
          if((d < ' ') || (d > '~')) continue;
          cmdBuf[p++] = d;
        }
        cmdBuf[p] = 0;
      } else {
        if(webIF && (!(flag & 0x80) || !((flag & 7) == 1))) break;
        int len = InputBuffer[sockfd].GetLen();
        if(!webIF) {
          int nest = 0;
          int f = 0;
          for(int i = 0; i < len; i++) {
            int d = InputBuffer[sockfd].PeekBuffer(i);
            if(d == '{') {
              nest++;
              f++;
            }
            if(d == '}') nest--;
            if(d == 0x04) {
              error = Disconnect;
              break;
            }
            if(f && !nest) {
              len = i + 1;
              break;
            }
          }
          if(error == Disconnect) break;
          if(nest < 0) InputBuffer[sockfd].Clean();
          if((nest != 0) || !f) break;
        }

        char buf[ConnectReceiveBufferSize];
        int f = 0;
        for(int i = 0; i < len; i++) {
          int d = InputBuffer[sockfd].GetBuffer();
          if(d  == 0x04) {
            error = Disconnect;
            break;
          }
          if(d > ' ') f = 1;
          buf[i] = d;
        }
        buf[len] = 0;
        if(error == Disconnect) break;
        if(!f) break;
        json_error_t jerror;
        json_t *json = json_loads(buf, 0, &jerror);
        if(!json) {
          ErrorPrintf("JSON parse error : %s", jerror.text);
          error = Error;
          break;
        }

        json_t *type = json_object_get(json, "type");
        if(!json_is_string(type)) {
          ErrorPrintf("JSON define error : type");
          error = Error;
        } else {
          if(!strcmp(json_string_value(type), "command")) {
            json_t *cmd = json_object_get(json, "command");
            if(!cmd || !json_is_string(cmd) || !strlen(json_string_value(cmd))) {
              ErrorPrintf("JSON define error : command");
              error = Error;
            }
            if(!error) {
              strncpy(cmdBuf, json_string_value(cmd), ConnectReceiveBufferSize);
              cmdBuf[ConnectReceiveBufferSize] = 0;
            }
          }
          if(!strcmp(json_string_value(type), "mail")) {
            json_t *jsonMailData = json_object_get(json, "data");
            SendNotify("mail", jsonMailData);
            cmdBuf[0] = 0;
          }
          if(!strcmp(json_string_value(type), "rainInfo")) {
            json_t *jsonData = json_object_get(json, "data");
            if(!jsonData || !json_is_integer(jsonData)) {
              ErrorPrintf("JSON define error : rainInfo");
              error = Error;
            }
            if(!error) {
              RainInfo = json_integer_value(jsonData);
              gettimeofday(&RainInfoTime, NULL);
            }
          }
        }
        json_decref(json);
        if(error) break;
      }

      if(!error && strlen(cmdBuf)) {
        error = ExecuteCommand(sockfd, cmdBuf);
      }
      if(error == Disconnect) break;
      if((error == Error) || (error == Finish)) SendResponse(sockfd, cmdBuf, error);
    }
    if(error == Disconnect) {
      struct sockaddr_in sockAddr;
      ClientInfo[sockfd].IF->GetPeerAddr(&sockAddr);
      LogPrintf("disconnect: %s port %d fd=%d\n",
                inet_ntoa(sockAddr.sin_addr),
                ntohs(sockAddr.sin_port), sockfd);
      ClientInfo[sockfd].IF->LogClose();
      ClientInfo[sockfd].IF->Close();
      DeleteExternalPort(sockfd);
      ClientInfo[sockfd].IF = NULL;
      return Success;
    }
    break;
  }
  return Success;
}

int HA1Control::XBeeReceive(int sockfd) {
  
#ifdef DEBUG
  LogPrintf("XBeeReceive %d\n", sockfd);
#endif
  
  while(1) {
    byte buf[256];
    int size = read(sockfd, buf, 256);
    if(size <= 0) break;

    for(int i = 0; i < size; i++) {
      int d = buf[i];
      // API=2 escape char
      if(d == 0x7e) {
        ReceiveSeq = 0;
        EscapedFlag = 0;
      }
      if(d == 0x7d) {
        EscapedFlag = 0x20;
        continue;
      }
      d ^= EscapedFlag;
      EscapedFlag = 0;
      int flag7e;
      switch(ReceiveSeq) {
        case 0: // wait delimiter
          if(d == 0x7e) ReceiveSeq++;
          break;
        case 1: // Packet Length High
          ReceiveLength = d << 8;
          ReceiveSeq++;
          break;
        case 2: // Packet Length Low
          ReceiveLength |= d;
          ReceiveSum = 0;
          ReceivePtr = 0;
          flag7e = 0;
          while(InputBuffer[sockfd].GetLen()) {
            if(!flag7e) LogPrintf("XBeeReceive Discarded : ");
            int c = InputBuffer[sockfd].GetBuffer();
            if(c >= 0) LogPrintf("%02x ", d);
            else LogPrintf("--- %d\n", InputBuffer[sockfd].GetLen());
            flag7e++;
          }
          if(flag7e) LogPrintf("\n");
          ReceiveSeq++;
          if(ReceiveLength > 150) ReceiveSeq = 0;
          break;
        case 3: // data
          ReceiveSum += d;
          ReceivePtr++;
          InputBuffer[sockfd].PutBuffer(d);
          if(ReceivePtr >= ReceiveLength) ReceiveSeq++;
          break;
        case 4: // csum
          ReceiveSum += d;
          ReceiveSeq = 0;
          if(ReceiveSum == 0xff) {
            byte buf[150];
            for(int i = 0; i < ReceiveLength; i++) {
              buf[i] = InputBuffer[sockfd].GetBuffer();
            }
#ifdef DEBUG
            LogPrintf("XBeeReceive :");
            for(int i = 0; i < ReceiveLength; i++) {
              LogPrintf("%02x ", buf[i]);
            }
            LogPrintf("\n");
#endif
            int offset = 0;
            if((buf[0] == 0x90) || (buf[0] == 0x92) || (buf[0] == 0x95)) {
              offset = 5;
            } else if(buf[0] == 0x97) {
              offset = 6;
            }
            if(offset) {
              longword addrL = (buf[offset + 0] << 24) | 
                               (buf[offset + 1] << 16) |
                               (buf[offset + 2] <<  8) |
                                buf[offset + 3];
              word netAddr =   (buf[offset + 4] << 8) |
                                buf[offset + 5];
              if(netAddr < 0xfffd) DeviceTable.SetNetworkAddr(addrL, netAddr);
            }
            if(int error = AddReadQueue(buf, ReceiveLength)) return error;
          }
        default:
          ReceiveSeq = 0;
          break;
      }
    }
  }
  return Success;
}

void HA1Control::AddBufferWithEsc(byte *buf, int *p, byte d) {

  if((d == 0x7e) || (d == 0x7d) || (d == 0x11) || (d == 0x13)) {
    d ^= 0x20;
    buf[(*p)++] = 0x7d;
  }
  buf[(*p)++] = d;
}
 
int HA1Control::SendPacket(const byte *buf, int size) {

  if(Uart < 0) return Error;
  if(size > 255) return Error;

#ifdef DEBUG
  LogPrintf("SendPacket:");
  for(int i = 0; i < size; i++) {
    LogPrintf("%02x ", buf[i]);
  }
  LogPrintf("\n");
#endif

  byte sendBuf[520];
  int p = 0;
  sendBuf[p++] = 0x7e;
  AddBufferWithEsc(sendBuf, &p, size >> 8);
  AddBufferWithEsc(sendBuf, &p, size);
  byte sum = 0;
  for(int i = 0; i < size; i++) {
    sum += buf[i];
    AddBufferWithEsc(sendBuf, &p, buf[i]);
  }
  AddBufferWithEsc(sendBuf, &p, 0xff - sum);

  int num = write(Uart, sendBuf, p);
  if(num != p) {
    LogPrintf("write to term failed : %s\n", strerror(errno));
    return Error;
  }
  return Success;
}

int HA1Control::SendATCommand(int sockfd, int sequenceID, const char *execCmd, const char *combiCmd, byte *data, int dataSize, longword addrL, byte *cmd, int len, int (HA1Control::*nextFunc)(WriteQueueSt *p, ReadQueueSt *q), int waitMSec, longword param1,longword param2, longword param3) {
  
  byte buf[256];
  int cmdOffset;
  int cmpLen;
  int resOffset;
  if(addrL) {
    buf[0] = 0x17;
    buf[1] = FrameID;
    buf[2] = (ADDRH >> 24) & 0xff;
    buf[3] = (ADDRH >> 16) & 0xff;
    buf[4] = (ADDRH >> 8) & 0xff;
    buf[5] = (ADDRH) & 0xff;
    buf[6] = (addrL >> 24) & 0xff;
    buf[7] = (addrL >> 16) & 0xff;
    buf[8] = (addrL >> 8) & 0xff;
    buf[9] = (addrL) & 0xff;
    word netAddr = DeviceTable.GetNetworkAddr(addrL);
    buf[10] = (netAddr >> 8) & 0xff;
    buf[11] = (netAddr) & 0xff;
    buf[12] = 0x02; // apply changes
    cmdOffset = 13;
    resOffset = 12;
  } else {
    buf[0] = 0x08;
    buf[1] = FrameID;
    cmdOffset = 2;
    resOffset = 2;
  }
  for(int i = 0; i < len; i++) buf[cmdOffset + i] = cmd[i];
  FrameID++;
  if(FrameID > 255) FrameID = 1;

  int error = AddWriteQueue(sockfd, sequenceID, 0, execCmd, combiCmd, data, dataSize, addrL, buf, cmdOffset + len, resOffset, nextFunc, waitMSec, param1, param2, param3);
  if(error) return error;

  return Success;
}

int HA1Control::SendAVRCommand(int sockfd, int sequenceID, const char *execCmd, const char *combiCmd, byte *data, int dataSize, longword addrL, byte *cmd, int len, int (HA1Control::*nextFunc)(WriteQueueSt *p, ReadQueueSt *q), int waitMSec, longword param1,longword param2, longword param3) {
  
  byte buf[256];
  int cmdOffset;
  int cmpLen;
  int resOffset;
  buf[0] = 0x10;
  buf[1] = FrameID;
  buf[2] = (ADDRH >> 24) & 0xff;
  buf[3] = (ADDRH >> 16) & 0xff;
  buf[4] = (ADDRH >> 8) & 0xff;
  buf[5] = (ADDRH) & 0xff;
  buf[6] = (addrL >> 24) & 0xff;
  buf[7] = (addrL >> 16) & 0xff;
  buf[8] = (addrL >> 8) & 0xff;
  buf[9] = (addrL) & 0xff;
  word netAddr = DeviceTable.GetNetworkAddr(addrL);
  buf[10] = (netAddr >> 8) & 0xff;
  buf[11] = (netAddr) & 0xff;
  buf[12] = 0x00;
  buf[13] = 0x00;
  buf[14] = FrameID;
  cmdOffset = 15;
  resOffset = 13;
  for(int i = 0; i < len; i++) buf[cmdOffset + i] = cmd[i];
  FrameID++;
  if(FrameID > 255) FrameID = 1;
  
  return AddWriteQueue(sockfd, sequenceID, 0, execCmd, combiCmd, data, dataSize, addrL, buf, cmdOffset + len, resOffset, nextFunc, waitMSec, param1, param2, param3);
}

void HA1Control::ClientTimeoutCheck() {
  
  for(int i = 0; i < MaxConnect; i++) {
    if(!ClientInfo[i].IF) continue;
    
    timeval tm;
    gettimeofday(&tm, NULL);
    timeval last;
    ClientInfo[i].IF->GetLastAccess(&last);
    timersub(&tm, &last, &tm);
    if((tm.tv_sec >= WebSocketAliveInterval) &&
       (ClientInfo[i].IF == &WebSocketIF)) WebSocketIF.SendPing();
    if((tm.tv_sec >= ConnectionTimeout) || ((ClientInfo[i].IF == &WebSocketIF) && (tm.tv_sec >= WebSocketAliveInterval * 2))) {
      struct sockaddr_in sockAddr;
      ClientInfo[i].IF->GetPeerAddr(&sockAddr);
      LogPrintf("timeout disconnect: %s port %d fd=%d\n",
                inet_ntoa(sockAddr.sin_addr),
                ntohs(sockAddr.sin_port), i);
      ClientInfo[i].IF->LogClose();
      ClientInfo[i].IF->Close();
      DeleteExternalPort(i);
      ClientInfo[i].IF = NULL;
    }
  }
  
  if(strlen(WSSServerURL) && WebSocketIF.GetFd() < 0) {
    int sockfd = WebSocketIF.Connect(WSSServerURL);
    if(sockfd >= 0) {
      ClientInfo[sockfd].IF = &WebSocketIF;
      ClientInfo[sockfd].Format = Format_JSON;
      InputBuffer[sockfd].Clean();
      AddExternalPort(sockfd, ExternalPort_WebSocketIFPort);
      WebSocketIntialState = 0;
    }
  }
}

void HA1Control::ResPrintf(int fd, const char *fmt, ...) {

  va_list args;
  va_start(args, fmt);
  
  if(fd <= 0) return;
  
  byte buf[ConnectReceiveBufferSize + 1];
  int len = vsnprintf((char *)buf, ConnectReceiveBufferSize, fmt, args);
  if(ClientInfo[fd].IF != &WebSocketIF) {
    ResWrite(fd, buf, len);
    return;
  }
  WebSocketSendBuffer.AddBuffer(buf, len);
  while(1) {
    int i;
    len = WebSocketSendBuffer.GetLen();
    for(i = 0; i < len; i++) {
      if(WebSocketSendBuffer.PeekBuffer(i) == 0x0a) break;
    }
    if(i == len) break;
    i++;
    int p = 0;
    for(int j = 0; j < i; j++) {
      int d = WebSocketSendBuffer.GetBuffer();
      buf[p++] = d;
    }
    buf[p] = 0;
    WebSocketIF.SendText(1, buf, p);
  }
}

int HA1Control::ResWrite(int fd, byte *buf, int size) {

  if(ClientInfo[fd].IF == &WebSocketIF) {
    return WebSocketIF.SendText(1, buf, size);
  } else {
    return ClientInfo[fd].IF->Send(buf, size);
  }
}

void HA1Control::ErrorPrintf(const char *fmt, ...) {
  
  va_list args;
  va_start(args, fmt);
  
  int len = vsnprintf(ErrorMessage, ErrorMessageSize - 1, fmt, args);
}

int HA1Control::IsFormatText(int sockfd) {

  return ClientInfo[sockfd].Format == Format_TEXT;
}

void HA1Control::SendResponse(int sockfd, const char *execCmd, int error, json_t *result) {
  
  if(error >= 0) error = Success;
  if(ClientInfo[sockfd].Format == Format_JSON) {
    json_t *jsonData = json_object();
    json_object_set_new(jsonData, "command", json_string(execCmd));
    if(result) json_object_set(jsonData, "result", result);
    if(error) {
      json_object_set_new(jsonData, "status", json_string("error"));
      json_object_set_new(jsonData, "message", json_string(ErrorMessage));
    } else {
      json_object_set_new(jsonData, "status", json_string("ok"));
    }
    json_t *jsonDataArray = json_array();
    json_array_append(jsonDataArray, jsonData);
    json_decref(jsonData);
    SendNotify("response", jsonDataArray);
    json_decref(jsonDataArray);
  } else if(ClientInfo[sockfd].Format == Format_TEXT) {
    if(error) {
      ResPrintf(sockfd, "%s : Error : %s\n", execCmd, ErrorMessage);
    } else {
      ResPrintf(sockfd, "%s : OK\n", execCmd);
    }
  }
  if(error) LogPrintf("%s : Error : %s\n", execCmd, ErrorMessage);
}

int HA1Control::SendNotify(const char *type, json_t *data) {
  
#ifdef DEBUG
  LogPrintf("SendNotify %s\n", type);
#endif

  int error = Success;
  for(int i = 0; i < MaxConnect; i++) {
    if(!ClientInfo[i].IF) continue;
    if(IsFormatText(i)) continue;
    json_t *notify = json_object();
    json_object_set_new(notify, "type", json_string(type));
    json_object_set(notify, "data", data);
    char *buf = json_dumps(notify, JSON_INDENT(2) | JSON_PRESERVE_ORDER);
    buf = (char *)realloc(buf, strlen(buf) + 2);
    strcat(buf, "\n");
    error |= ResWrite(i, (byte *)buf, strlen(buf) + 1);
    free(buf);
    json_decref(notify);
  }
  return error;
}

