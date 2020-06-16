#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <unistd.h>
#include <sys/poll.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <openssl/err.h>
#include <openssl/ssl.h>

#include "ConfigTLSMCC.h"

#include "PayloadTLSStream.h"

namespace ArcMCCTLS {

PayloadTLSStream::PayloadTLSStream(Logger& logger,SSL* ssl):timeout_(0),ssl_(ssl),logger_(logger) {
  //initialize something
  return;
}

PayloadTLSStream::PayloadTLSStream(PayloadTLSStream& stream):timeout_(stream.timeout_),ssl_(stream.ssl_),logger_(stream.logger_) {
}

PayloadTLSStream::~PayloadTLSStream(void) {
  ConfigTLSMCC::ClearError();
}

bool PayloadTLSStream::Get(char* buf,int& size) {
  if(ssl_ == NULL) return false;
  //ssl read
  ssize_t l=size;
  size=0;
  l=SSL_read(ssl_,buf,l);
  if(l <= 0){
     SetFailure(SSL_get_error(ssl_,l));
     return false;
  }
  size=l;
  return true;
}

bool PayloadTLSStream::Get(std::string& buf) {
  char tbuf[1024];
  int l = sizeof(tbuf);
  bool result = Get(tbuf,l);
  buf.assign(tbuf,l);
  return result;
}

bool PayloadTLSStream::Put(const char* buf,Size_t size) {
  //ssl write
  ssize_t l;
  if(ssl_ == NULL) return false;
  for(;size;){
     l=SSL_write(ssl_,buf,size);
     if(l <= 0){
        SetFailure(SSL_get_error(ssl_,l));
        return false;
     }
     buf+=l; size-=l;
  }
  return true;
}

X509* PayloadTLSStream::GetPeerCert(void){
  X509* peercert;
  int err;
  if(ssl_ == NULL) return NULL;
  if((err=SSL_get_verify_result(ssl_)) == X509_V_OK){
    peercert=SSL_get_peer_certificate (ssl_);
    if(peercert!=NULL) return peercert;
    SetFailure("Peer certificate cannot be extracted\n"+ConfigTLSMCC::HandleError());
  }
  else{
    SetFailure(std::string("Peer cert verification failed: ")+X509_verify_cert_error_string(err)+"\n"+ConfigTLSMCC::HandleError(err));
  }
  return NULL;
}

X509* PayloadTLSStream::GetCert(void){
  X509* cert;
  if(ssl_ == NULL) return NULL;
  cert=SSL_get_certificate (ssl_);
  if(cert!=NULL) return cert;
  SetFailure("Peer certificate cannot be extracted\n"+ConfigTLSMCC::HandleError());
  return NULL;
}

STACK_OF(X509)* PayloadTLSStream::GetPeerChain(void){
  STACK_OF(X509)* peerchain;
  int err;
  if(ssl_ == NULL) return NULL;
  if((err=SSL_get_verify_result(ssl_)) == X509_V_OK){
    peerchain=SSL_get_peer_cert_chain (ssl_);
    if(peerchain!=NULL) return peerchain;
    SetFailure("Peer certificate chain cannot be extracted\n"+ConfigTLSMCC::HandleError());
  } else {
    SetFailure(std::string("Peer cert verification failed: ")+X509_verify_cert_error_string(err)+"\n"+ConfigTLSMCC::HandleError(err));
  }
  return NULL;
}

void PayloadTLSStream::SetFailure(const std::string& err) {
  failure_ = MCC_Status(GENERIC_ERROR,"TLS",err);
}

void PayloadTLSStream::SetFailure(int code) {
  failure_ = MCC_Status(GENERIC_ERROR,"TLS", ConfigTLSMCC::HandleError(code));
}

} // namespace ArcMCCTLS
