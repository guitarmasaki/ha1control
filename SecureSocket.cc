/*

 SecureSocket.cc
 
 Copyright: Copyright (C) 2015 Mitsuru Nakada All Rights Reserved.
 License: GNU GPL v2
 */

#include <arpa/inet.h>
#include <openssl/err.h>

#include "SecureSocket.h"

SecureSocket::SecureSocket() {

  CTX = NULL;
  SSLp = NULL;
  CACertFile[0] = 0;
  CertFile[0] = 0;
  PrivateFile[0] = 0;
  Issure[0] = 0;
  Subject[0] = 0;
  AccountFile[0] = 0;
  OrganizationUnit[0] = 0;
  CommonName[0] = 0;
  OrganizationUnit[0] = 0;
  CommonName[0] = 0;
  
  SSL_load_error_strings();
  SSL_library_init();
}

SecureSocket::~SecureSocket() {

  Close();
}

int SecureSocket::Setup(const char *caCertFile, const char *certFile, const char *privateFile, const char *issure, const char *subject, const char *accountFile) {

  if(!caCertFile || !strlen(caCertFile)) return Error;
  strncpy(CACertFile, caCertFile, 255);
  CACertFile[255] = 0;
  if(!certFile || !strlen(certFile)) return Error;
  strncpy(CertFile, certFile, 255);
  CertFile[255] = 0;
  if(!privateFile || !strlen(privateFile)) return Error;
  strncpy(PrivateFile, privateFile, 255);
  PrivateFile[255] = 0;
  if(!issure || !strlen(issure)) return Error;
  strncpy(Issure, issure, 255);
  Issure[255] = 0;
  if(!subject || !strlen(subject)) return Error;
  strncpy(Subject, subject, 255);
  Subject[255] = 0;
  if(accountFile && strlen(accountFile)) {
    strncpy(AccountFile, accountFile, 255);
    AccountFile[255] = 0;
  }
  return Success;
}

int SecureSocket::Accept(int sockfd, const char *allowIP, const char *allowMask) {

  int newSocket = Socket::Accept(sockfd, allowIP, allowMask);
  if(newSocket < 0) return newSocket;

  CTX = CreateCTX(Mode_Server);
  if(!CTX) {
    LogPrintf("SecureSocket::Accept Error : CTX = NULL\n");
    Socket::Close();
    return Error;
  }
  
  SSLp = SSL_new(CTX);
  if(!SSLp) {
    LogPrintf("SecureSocket::Accept SSL_new error\n");
    SSL_CTX_free(CTX);
    CTX = NULL;
    Socket::Close();
    return Error;
  }
  
  SSL_set_fd(SSLp, newSocket);
  int ret = SSL_accept(SSLp);
  if(ret <= 0) {
    LogPrintf("SecureSocket::Accept SSL_accept error %d %d\n", ret, SSL_get_error(SSLp, ret));
    LogPrintf("Error: %s\n", ERR_reason_error_string(ERR_get_error()));
    SSL_free(SSLp);
    SSLp = NULL;
    SSL_CTX_free(CTX);
    CTX = NULL;
    Socket::Close();
    return Error;
  }
  // read verify result
  if(SSL_get_verify_result(SSLp) != X509_V_OK) {
    LogPrintf("SecureSocket::Accept SSL_get_verify_result error\n");
    SSL_free(SSLp);
    SSLp = NULL;
    SSL_CTX_free(CTX);
    CTX = NULL;
    Socket::Close();
    return Error;
  }

  if(VerifyPeer()) {
    LogPrintf("SecureSocket::Accept VerifyPeer Error\n");
    SSL_free(SSLp);
    SSLp = NULL;
    SSL_CTX_free(CTX);
    CTX = NULL;
    Socket::Close();
    return Error;
  }
  const char *cipher = SSL_get_cipher(SSLp);
  char *version = SSL_get_cipher_version(SSLp);
  struct sockaddr_in sockAddr;
  GetPeerAddr(&sockAddr);
  LogPrintf("SecureSocket::Accept %d %s/%s %s %s:%d\n", newSocket, OrganizationUnit, CommonName, cipher, inet_ntoa(sockAddr.sin_addr), ntohs(sockAddr.sin_port));
  return Success;
}

int SecureSocket::Connect(const char *host, int port) {

#ifdef DEBUG
  LogPrintf("SecureSocket::Connect %s:%d\n", host, port);
#endif
  
  int newSocket = Socket::Connect(host, port);
  if(newSocket < 0) return newSocket;

  CTX = CreateCTX(Mode_Client);
  if(!CTX) {
    LogPrintf("SecureSocket::Connect Error : CTX = NULL\n");
    Socket::Close();
    return Error;
  }
  
  SSLp = SSL_new(CTX);
  if(!SSLp) {
    LogPrintf("SecureSocket::Connect SSL_new error\n");
    SSL_CTX_free(CTX);
    CTX = NULL;
    Socket::Close();
    return Error;
  }

  SSL_set_fd(SSLp, newSocket);
  int ret = SSL_connect(SSLp);
  if(ret <= 0) {
    LogPrintf("SecureSocket::Connect SSL_connect error %d %d\n", ret, SSL_get_error(SSLp, ret));
    LogPrintf("Error: %s\n", ERR_reason_error_string(ERR_get_error()));
    SSL_free(SSLp);
    SSLp = NULL;
    SSL_CTX_free(CTX);
    CTX = NULL;
    Socket::Close();
    return Error;
  }

  // read verify result
  if(SSL_get_verify_result(SSLp) != X509_V_OK) {
    LogPrintf("SecureSocket::Connect SSL_get_verify_result error\n");
    SSL_free(SSLp);
    SSLp = NULL;
    SSL_CTX_free(CTX);
    CTX = NULL;
    Socket::Close();
    return Error;
  }
  
  if(VerifyPeer()) {
    LogPrintf("SecureSocket::Connect VerifyPeer Error\n");
    SSL_free(SSLp);
    SSLp = NULL;
    SSL_CTX_free(CTX);
    CTX = NULL;
    Socket::Close();
    return Error;
  }
  char desc[256];
  SSL_CIPHER_description(SSL_get_current_cipher(SSLp), desc, 255);
  const char *cipher = strtok(desc, " \t\r\n");
  char *version = strtok(NULL, " \t\r\n");
  struct sockaddr_in sockAddr;
  GetPeerAddr(&sockAddr);
  LogPrintf("SecureSocket::Connect %d %s %s %s/%s\n", newSocket, cipher, version, OrganizationUnit, CommonName);
  return newSocket;
}

int SecureSocket::Close() {

  if(SSLp) {
    int ret = SSL_shutdown(SSLp);
    if(!ret) SSL_shutdown(SSLp);
    SSL_free(SSLp);
    SSLp = NULL;
  }
  if(CTX) SSL_CTX_free(CTX);
  CTX = NULL;
  Socket::Close();
  return Success;
}

int SecureSocket::Receive(byte *buf, int size) {

  if(!SSLp) return Error;
  int retry = ConnectionRetry;
  while(retry) {
    int ret = SSL_read(SSLp, buf, size);
    if(ret > 0) {
      SetLastAccess();
      return ret;
    }
    retry--;
    if(!ret && (SSL_get_error(SSLp, ret) == SSL_ERROR_WANT_READ)) continue;
  }
  return Error;
}

int SecureSocket::Send(byte *buf, int size) {

  if(!SSLp) return Error;
  int writeSize = 0;
  do {
    int ret = SSL_write(SSLp, buf, size);
    if(ret < 0) return ret;
    if(!ret && (SSL_get_error(SSLp, ret) == SSL_ERROR_WANT_WRITE)) continue;
    writeSize += ret;
  } while(writeSize != size);
  SetLastAccess();
  return Success;
}

SSL_CTX *SecureSocket::CreateCTX(int mode) {
  
  if(!strlen(CACertFile)) return NULL;
  
  int error = Success;
  SSL_CTX *ctx;
  
  ctx = SSL_CTX_new(SSLv23_method());
  SSL_CTX_set_options(ctx, SSL_OP_NO_SSLv2| SSL_OP_NO_SSLv3);
  SSL_CTX_set_cipher_list(ctx, "ALL:!EXP:!ADH:!LOW:!SSLv2:!DES:!3DES:!RC4");
  if(!ctx) {
     LogPrintf("SecureSocket::CreateCTX SSL_CTX_new error\n");
     return NULL;
  }
  
  // certification file
  if(!strlen(CertFile) || 
     (SSL_CTX_use_certificate_file(ctx, CertFile, SSL_FILETYPE_PEM) != 1)) {
    LogPrintf("SecureSocket::CreateCTX SSL_CTX_use_certificate_file error : %s\n", CertFile);
    SSL_CTX_free(ctx);
    return NULL;
  }
  
  // private key file
  if(!strlen(PrivateFile) ||
     (SSL_CTX_use_PrivateKey_file(ctx, PrivateFile, SSL_FILETYPE_PEM) != 1)) {
    LogPrintf("SecureSocket::CreateCTX SSL_CTX_use_PrivateKey_file error : %s\n", PrivateFile);
    SSL_CTX_free(ctx);
    return NULL;
  }
  
  // set verify condition and load CA certification file
  SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT, NULL);
  SSL_CTX_set_verify_depth(ctx, 1);
  if(!SSL_CTX_load_verify_locations(ctx, CACertFile, NULL)) {
    LogPrintf("SecureSocket::CreateCTX SSL_CTX_load_verify_locations error : %s\n", CACertFile);
    SSL_CTX_free(ctx);
    return NULL;
  }
   
  if(mode == Mode_Server) {
    STACK_OF(X509_NAME) *cert_names = SSL_load_client_CA_file(CACertFile);
    if(cert_names == NULL) {
      LogPrintf("SecureSocket::CreateCTX SSL_load_client_CA_file : %s\n", CACertFile);
      SSL_CTX_free(ctx);
      return NULL;
    }
    SSL_CTX_set_client_CA_list(ctx, cert_names);
  }
  return ctx;
}

int SecureSocket::VerifyPeer() {
  
  // load peer certification
  X509 *peer_cert = SSL_get_peer_certificate(SSLp);
  if(!peer_cert) {
    LogPrintf("SecureSocket::VerifyPeer SSL_get_peer_certificate error\n");
    return Error;
  }
    
  // get peer organization unit name
  X509_NAME_get_text_by_NID(X509_get_subject_name(peer_cert), NID_organizationalUnitName, OrganizationUnit, 255);
  X509_NAME_get_text_by_NID(X509_get_subject_name(peer_cert), NID_commonName, CommonName, 255);

  // verify peer certification
  if(CertCheck(X509_get_issuer_name(peer_cert), Issure)) {
    LogPrintf("SecureSocket::VerifyPeer Issuer Error %s\n",Issure);
    return Error;
  }
  if(CertCheck(X509_get_subject_name(peer_cert), Subject)) {
    LogPrintf("SecureSocket::VerifyPeer Subject Error %s\n", Subject);
    return Error;
  }
  
  if(strlen(AccountFile)) {
    FILE *fp = fopen(AccountFile, "r");
    if(fp == NULL) {
      LogPrintf("SecureSocket::VerifyPeer AccountFileError : %s\n", AccountFile);
      return Error;
    } 
    
    int subjectCheckFlag = 0;
    char buf[256];
    while(!feof(fp)) {
      if(fgets(buf, 255, fp) == NULL) break;
      char *p = strchr(buf, '#');
      if(p) *p = 0;
      p = strtok(buf, ":\r\n");
      if(!p) continue;
      if(!CertCheck(X509_get_subject_name(peer_cert), p)) {
        subjectCheckFlag = 1;
        LogPrintf("SecureSocket::VerifyPeer PeerCert OK : %s\n", p);
        break;
      }
    }
    fclose(fp);
    if(!subjectCheckFlag) {
      LogPrintf("SecureSocket::VerifyPeer Subject Error OU=%s/CN=%s\n", OrganizationUnit, CommonName);
      return Error;
    }
  }
      
  if(peer_cert) X509_free(peer_cert);
  return Success;
}

int SecureSocket::CertCheck(X509_NAME *pName, const char *cmpCert) {
  
  char cmpCert2[256];
  strncpy(cmpCert2, cmpCert, 255);
  char *pp;
  char *p = strtok_r(cmpCert2, "/", &pp);
  int i = 0;
  do {
    X509_NAME_ENTRY *ne = sk_X509_NAME_ENTRY_value(pName->entries, i);
    int nid = OBJ_obj2nid(ne->object);
    const char *s;
    char tmpBuf1[256];
    if(nid == NID_undef || (s = OBJ_nid2sn(nid)) == NULL) {
      i2t_ASN1_OBJECT(tmpBuf1, 255, ne->object);
      s = tmpBuf1;
    }
    
    char tmpBuf2[256];
    X509_NAME_get_text_by_NID(pName, nid, tmpBuf2, 255);
    
    char tmpBuf3[256];
    snprintf(tmpBuf3, 255, "%s=%s", s, tmpBuf2);
    if(strcmp(p, tmpBuf3)) {
      char tmpBuf4[256];
      strncpy(tmpBuf4, p, 255);
      char *q = strtok(tmpBuf4, "=");
      if(strcmp(s, q)) continue;
      LogPrintf("CertCheck  -- %s:%s\n", p, tmpBuf3);
      return Error;
    }
    p = strtok_r(NULL, "/", &pp);
  } while(p && (i++ < sk_X509_NAME_ENTRY_num(pName->entries)));
  return Success;
}
