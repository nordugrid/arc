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
PayloadTLSStream::PayloadTLSStream(SSL* ssl):ssl_(ssl) {
  //initialize something
  return;
};

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
