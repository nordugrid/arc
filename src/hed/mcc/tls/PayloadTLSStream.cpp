#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <unistd.h>
#ifndef WIN32
#include <sys/poll.h>
#else
#define NOGDI
#include <objbase.h>
#include <io.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <openssl/err.h>
#include <openssl/ssl.h>

#include <arc/Logger.h>
#include "PayloadTLSStream.h"

namespace Arc {

void PayloadTLSStream::handle_ssl_error(int code)
{
//    switch (code) {
//        case SSL_ERROR_SSL: {
            int e = ERR_get_error();
            while(e != 0) {
                Logger::rootLogger.msg(ERROR, "DEBUG: SSL_write error: %d %s:%s:%s", 
                                      e, 
                                      ERR_lib_error_string(e),
                                      ERR_func_error_string(e),
                                      ERR_reason_error_string(e));
                e = ERR_get_error();
            }
//            break;
//        }
//        default:
            Logger::rootLogger.msg(DEBUG, "SSL error Code: %d", code);
//            break;
//    }
}

PayloadTLSStream::PayloadTLSStream(SSL* ssl):ssl_(ssl) {
  //initialize something
  return;
}

bool PayloadTLSStream::Get(char* buf,int& size) {
  if(ssl_ == NULL) return false;
  //ssl read
  ssize_t l=size;
  size=0;
  l=SSL_read(ssl_,buf,l);
  if(l <= 0){
     //handle tls error
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

bool PayloadTLSStream::Put(const char* buf,int size) {
  //ssl write
  ssize_t l;
  if(ssl_ == NULL) return false;
  for(;size;){
     l=SSL_write(ssl_,buf,size);
     if(l <= 0){
        handle_ssl_error(SSL_get_error(ssl_, l));
        //handle tls error
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
    if(peercert!=NULL){
       return peercert;
    }
    else{      
      Logger::rootLogger.msg(ERROR,"Peer cert cannot be extracted");
    }
  }
  else{
    Logger::rootLogger.msg(ERROR,"Peer cert verification fail");
    Logger::rootLogger.msg(ERROR,"%s",X509_verify_cert_error_string(err));
    handle_ssl_error(err);
  }
  return NULL;
}

STACK_OF(X509)* PayloadTLSStream::GetPeerChain(void){
  STACK_OF(X509)* peerchain;
  int err;
  if(ssl_ == NULL) return NULL;
  if((err=SSL_get_verify_result(ssl_)) == X509_V_OK){
    peerchain=SSL_get_peer_cert_chain (ssl_);
    if(peerchain!=NULL){
       return peerchain;
    }
    else{
      Logger::rootLogger.msg(ERROR,"Peer cert cannot be extracted");
    }
  }
  else{
    Logger::rootLogger.msg(ERROR,"Peer cert verification fail");
    Logger::rootLogger.msg(ERROR,"%s",X509_verify_cert_error_string(err));
    handle_ssl_error(err);
  }
  return NULL;
}

} // namespace Arc
