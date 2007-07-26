#include <unistd.h>
#include <sys/poll.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <openssl/err.h>
#include <openssl/ssl.h>

#include "../../../libs/common/Logger.h"
#include "PayloadTLSStream.h"

namespace Arc {
static void handle_ssl_error(int code)
{
    switch (code) {
        case SSL_ERROR_SSL: {
            int e = ERR_get_error();
            while(e != 0) {
                Logger::rootLogger.msg(ERROR, "DEBUG: SSL_write error: %d %s:%s:%s", 
                                      e, 
                                      ERR_lib_error_string(e),
                                      ERR_func_error_string(e),
                                      ERR_reason_error_string(e));
                e = ERR_get_error();
            }
            break;
        }
        default:
            Logger::rootLogger.msg(DEBUG, "SSL error Code: %d", code);
            break;
    }
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

X509* PayloadTLSStream::GetPeercert(void){
  X509* peercert;
  if((SSL_get_verify_result(ssl_)) == X509_V_OK){
    peercert=SSL_get_peer_certificate (ssl_);
    if(peercert!=NULL){
       return peercert;
    }
    else{      
      Logger::rootLogger.msg(ERROR,"Peer cert cannot be extracted");
      return NULL;
    }
  }
  else{
      Logger::rootLogger.msg(ERROR,"Peer cert verification fail");
      return NULL;
  }
}

} // namespace Arc
