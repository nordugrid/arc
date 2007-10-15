#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <sys/socket.h>
#include <netdb.h>

#include <openssl/err.h>

#include "PayloadTLSSocket.h"

namespace Arc {

PayloadTLSSocket::PayloadTLSSocket(int s, SSL_CTX *ctx,
				   bool client, Logger& logger):
  sslctx_(ctx), client_(client), logger(logger)
{ 
   ssl_ = SSL_new(ctx);
   if (ssl_ == NULL){
	//handle tls error;    
   }
   if (!SSL_set_fd(ssl_, s)){
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

PayloadTLSSocket::PayloadTLSSocket(PayloadStream& s, SSL_CTX *ctx,
				   bool client, Logger& logger):
  sslctx_(ctx), client_(client), logger(logger)
{ 
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
  if(ssl_) {
    int count = 10;
    while(count > 0) {
      int err = SSL_shutdown(ssl_);
      if(err == 1) break;
      count++; sleep(1);
      if(err == 0) continue;
      err=SSL_get_error(ssl_,err);
      if(err == SSL_ERROR_WANT_READ) {
      } else if(err == SSL_ERROR_WANT_WRITE) {
      } else {
	logger.msg(WARNING, "Failed to shut down SSL");
        SSL_set_shutdown(ssl_,SSL_SENT_SHUTDOWN | SSL_RECEIVED_SHUTDOWN);
        break;
      };
    };

/*
   unsigned long err = 0;
   int counter=0;
*
   if(ssl_) { 
      if(SSL_shutdown(ssl_) == 0) {
	logger.msg(WARNING, "Failed to shut down SSL");
      };
*
    while((err==0)&&(counter<60)){
	err=SSL_shutdown(ssl_);
	if (err==0){
	       	sleep(1);
		counter++;}
    }
    if(err<0)
      logger.msg(ERROR, "Failed to shut down SSL");
  }
  if(ssl_){
*/
    SSL_free(ssl_);
    ssl_=NULL;
  }
}

} // namespace Arc
