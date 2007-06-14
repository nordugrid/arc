#include "PayloadTLSMCC.h"

namespace Arc {

PayloadTLSMCC::PayloadTLSMCC(MCCInterface* mcc, SSL_CTX *ctx, Logger& logger):
  sslctx_(ctx), logger(logger)
{ 
   master_=true;
   ssl_ = SSL_new(ctx);
   if (ssl_ == NULL){
	//handle tls error;    
   }
   BIO* bio = BIO_new_MCC(mcc);
   SSL_set_bio(ssl_,bio,bio);
   //SSL_set_connect_state(ssl_);
   SSL_connect(ssl_);
   //handle error
   /*
	//SSL_set_accept_state(ssl_);
	SSL_accept(ssl_);
	//handle error
   */
   // if(SSL_in_init(ssl_)){
   //handle error
   // }
}

PayloadTLSMCC::PayloadTLSMCC(PayloadTLSMCC& stream, Logger& logger):
  PayloadTLSStream(stream), logger(logger)
{
   master_=false;
   sslctx_=stream.sslctx_; 
}


PayloadTLSMCC::~PayloadTLSMCC(void) {
   if(!master_) return;
   unsigned long err = 0;
   int counter=0;
   if(ssl_) { 
      if(SSL_shutdown(ssl_) == 0) {
	logger.msg(WARNING, "Failed to shut down SSL.");
      };
/*
    while((err==0)&&(counter<60)){
	err=SSL_shutdown(ssl_);
	if (err==0){
	       	sleep(1);
		counter++;}
    }
    if(err<0)
      logger.msg(ERROR, "Failed to shut down SSL.");
  }
  if(ssl_){
*/
    SSL_free(ssl_);
    ssl_=NULL;
  }
}

} // namespace Arc
