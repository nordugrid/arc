#include <iostream>
#include <unistd.h>
#include <sys/poll.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <openssl/err.h>
#include <openssl/ssl.h>

#include "PayloadTLSStream.h"

namespace Arc {
PayloadTLSStream::PayloadTLSStream(SSL* ssl):ssl_(ssl) {
  //initialize something
std::cerr<<"PayloadTLSStream - constructor: 0x"<<std::hex<<this<<std::dec<<std::endl;
  
  return;
};

bool PayloadTLSStream::Get(char* buf,int& size) {
std::cerr<<"PayloadTLSStream::Get: "<<size<<std::endl;
  if(ssl_ == NULL) return false;
  //ssl read
  ssize_t l=size;
  size=0;
  l=SSL_read(ssl_,buf,l);
std::cerr<<"PayloadTLSStream::Get: l="<<l<<std::endl;
  if(l <= 0){
     //handle tls error
     return false;
  }
  size=l;
  // std::cerr<<"PayloadTLSStream::Get: *"; for(int n=0;n<l;n++) std::cerr<<buf[n]; std::cerr<<"*"<<std::endl;
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
std::cerr<<"PayloadTLSStream::Put: "<<size<<std::endl;
  if(ssl_ == NULL) return false;
  // std::cerr<<"PayloadTLSStream::Put: *"; for(int n=0;n<size;n++) std::cerr<<buf[n]; std::cerr<<"*"<<std::endl;
  for(;size;){
     l=SSL_write(ssl_,buf,size);
std::cerr<<"PayloadTLSStream::Put: l="<<l<<std::endl;
     if(l <= 0){
        //handle tls error
        return false;
     }
     buf+=l; size-=l;
  }
  return true;
}
} // namespace Arc
