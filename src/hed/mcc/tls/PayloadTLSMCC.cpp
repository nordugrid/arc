#include <iostream>

#include "PayloadTLSMCC.h"

namespace Arc {

PayloadTLSMCC::PayloadTLSMCC(MCCInterface* mcc, SSL_CTX *ctx): sslctx_(ctx) { 
std::cerr<<"PayloadTLSMCC - constructor"<<std::endl;
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

PayloadTLSMCC::PayloadTLSMCC(PayloadTLSMCC& stream):PayloadTLSStream(stream) {
std::cerr<<"PayloadTLSMCC - copy constructor"<<std::endl;
   master_=false;
   sslctx_=stream.sslctx_; 
}


PayloadTLSMCC::~PayloadTLSMCC(void) {
  if(!master_) return;
  unsigned long err = 0;
  int counter=0;
  if(ssl_) { 
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
    SSL_free(ssl_);
    ssl_=NULL;
  }
}

} // namespace Arc
