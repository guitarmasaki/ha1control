/*

 SecureSocket.h
 
 Copyright: Copyright (C) 2015 Mitsuru Nakada All Rights Reserved.
 License: GNU GPL v2
 */


#ifndef __SecureSocket_h__
#define __SecureSocket_h__

#include <openssl/ssl.h>

#include "Socket.h"

class SecureSocket : public Socket {
public:
  SecureSocket();
  ~SecureSocket();

  int Setup(const char *caCertFile, const char *certFile, const char *privateFile, const char *issure, const char *subject, const char *accoutFile);
  int Accept(int sockfd, const char *allowIP = NULL, const char *allowMask = NULL);
  int Connect(const char *host, int port);
  int Close();
  int Receive(byte *buf, int size);
  int Send(byte *buf, int size);

private:
  int VerifyPeer();
  SSL_CTX *CreateCTX(int mode);
  int CertCheck(X509_NAME *pName, const char *cmpCert);

  static const int ConnectionRetry = 5;
  SSL_CTX *CTX;
  SSL *SSLp;
  char CACertFile[256];
  char CertFile[256];
  char PrivateFile[256];
  char Issure[256];
  char Subject[256];
  char AccountFile[256];
  char OrganizationUnit[256];
  char CommonName[256];
  
  enum Mode {
    Mode_Server,
    Mode_Client
  };
};

#endif // __SecureSocket_h__

