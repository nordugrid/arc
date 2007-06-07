#include <iostream>
#include <sys/socket.h>
#include <netdb.h>

#include <openssl/err.h>

#include "PayloadTLSSocket.h"

namespace Arc {

PayloadTLSSocket::PayloadTLSSocket(int s, SSL_CTX *ctx, bool client): sslctx_(ctx), client_(client){ 
std::cerr<<"PayloadTLSSocket - constructor 0x"<<std::hex<<this<<std::dec<<std::endl;
   ssl_ = SSL_new(ctx);
   if (ssl_ == NULL){
	//handle tls error;    
   }
std::cerr<<"PayloadTLSSocket - 1: "<<s<<std::endl;
   if (!SSL_set_fd(ssl_, s)){
	//handle tls error 
   }
std::cerr<<"PayloadTLSSocket - 2"<<std::endl;
   if (client){
std::cerr<<"PayloadTLSSocket - 3"<<std::endl;
       	//SSL_set_connect_state(ssl_);
	SSL_connect(ssl_);
        //handle error
std::cerr<<"PayloadTLSSocket - 4"<<std::endl;
   }
   else{
std::cerr<<"PayloadTLSSocket - 5"<<std::endl;
	//SSL_set_accept_state(ssl_);
	SSL_accept(ssl_);
	//handle error
std::cerr<<"PayloadTLSSocket - 6"<<std::endl;
   }
  // if(SSL_in_init(ssl_)){
   //handle error
  // }
std::cerr<<"PayloadTLSSocket -76"<<std::endl;
}

PayloadTLSSocket::PayloadTLSSocket(PayloadStream& s, SSL_CTX *ctx, bool client): sslctx_(ctx), client_(client){ 
std::cerr<<"PayloadTLSSocket - constructor2 0x"<<std::hex<<this<<std::dec<<std::endl;
  int sfd = s.GetHandle();
  ssl_ = SSL_new(ctx);
   if (ssl_ == NULL){
        //handle tls error;    
   }
   if (!SSL_set_fd(ssl_, sfd)){
        //handle tls error 
   }
   if (client){
        //SSL_set_connect_state(ssl_);
        SSL_connect(ssl_);
        //handle error
   }
   else{
        //SSL_set_accept_state(ssl_);
        SSL_accept(ssl_);
        //handle error
   }
  // if(SSL_in_init(ssl_)){
   //handle error
  // }
}



PayloadTLSSocket::~PayloadTLSSocket(void) {
std::cerr<<"PayloadTLSSocket - destructor"<<std::endl;
   unsigned long err = 0;
   int counter=0;
   if(ssl_) { 
      if(SSL_shutdown(ssl_) == 0) {
         std::cerr << "Warning: Failed to shut down SSL" << std::endl;
      };
/*
    while((err==0)&&(counter<60)){
	err=SSL_shutdown(ssl_);
	if (err==0){
	       	sleep(1);
		counter++;}
    }
    if(err<0)
      std::cerr << "Error: Failed to shut down SSL" << std::endl;
  }
  if(ssl_){
*/
    SSL_free(ssl_);
    ssl_=NULL;
  }
}

} // namespace Arc
