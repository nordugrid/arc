#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "PayloadTLSMCC.h"

namespace Arc {

PayloadTLSMCC::PayloadTLSMCC(MCCInterface* mcc, SSL_CTX *ctx, Logger& logger):sslctx_(ctx),logger(logger) { 
   // Client mode
   int err;
   master_=true;
   ssl_ = SSL_new(ctx);
   if (ssl_ == NULL){
	//handle tls error;    
   }
   BIO* bio = BIO_new_MCC(mcc);
   SSL_set_bio(ssl_,bio,bio);
   //SSL_set_connect_state(ssl_);
   if((err=SSL_connect(ssl_)) != 1) {
     logger.msg(ERROR, "Failed to establish SSL connection");
     handle_ssl_error(err);
     //TODO: handle error
   };
   // if(SSL_in_init(ssl_)){
   //handle error
   // }
}

PayloadTLSMCC::PayloadTLSMCC(PayloadStreamInterface* stream, SSL_CTX* ctx, Logger& logger):sslctx_(ctx),logger(logger) {
   // Server mode
   int err;
   master_=true;
   ssl_ = SSL_new(ctx);
   if (ssl_ == NULL){
	//handle tls error;    
   }
   BIO* bio = BIO_new_MCC(stream);
   SSL_set_bio(ssl_,bio,bio);
   //SSL_set_accept_state(ssl_);
   if((err=SSL_accept(ssl_)) != 1) {
     logger.msg(ERROR, "Failed to establish SSL connection");
     handle_ssl_error(err);
     //TODO: handle error
   };
   //handle error
   // if(SSL_in_init(ssl_)){
   //handle error
   // }
}

PayloadTLSMCC::PayloadTLSMCC(PayloadTLSMCC& stream, Logger& logger):
  PayloadTLSStream(stream), logger(logger)
{
   master_=false;
   sslctx_=stream.sslctx_; 
   ssl_=stream.ssl_; 
}


PayloadTLSMCC::~PayloadTLSMCC(void) {
   if(!master_) return;
   if(ssl_) { 
     if(SSL_shutdown(ssl_) == 0) {
       //logger.msg(WARNING, "Failed to shut down SSL");
       logger.msg(VERBOSE, "Failed to shut down SSL");
     };
/*
     while((err==0)&&(counter<60)){
       err=SSL_shutdown(ssl_);
       if (err==0) {
       	 sleep(1);
         counter++;
       }
    }
    if(err<0) logger.msg(ERROR, "Failed to shut down SSL");
*/
    SSL_free(ssl_);
    ssl_=NULL;
  }
}

} // namespace Arc
