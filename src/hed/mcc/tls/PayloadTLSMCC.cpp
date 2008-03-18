#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "PayloadTLSMCC.h"
#include <openssl/err.h>

namespace Arc {

static void tls_process_error(Logger& logger){
   unsigned long err;
   err = ERR_get_error();
   if (err != 0)
   {
     logger.msg(ERROR, "OpenSSL Error -- %s", ERR_error_string(err, NULL));
     logger.msg(ERROR, "Library  : %s", ERR_lib_error_string(err));
     logger.msg(ERROR, "Function : %s", ERR_func_error_string(err));
     logger.msg(ERROR, "Reason   : %s", ERR_reason_error_string(err));
   }
   return;
}

// Here certificates are reloaded. There are no locks because certificates 
// are the same unless updated by administrator/user and there are internal
// locks in OpenSSL.
static bool reload_certificates(SSL_CTX* sslctx, const std::string& cert_file, const std::string& key_file, Logger& logger) {
  if(!cert_file.empty()) {
    if((SSL_CTX_use_certificate_chain_file(sslctx,cert_file.c_str()) != 1) &&
       (SSL_CTX_use_certificate_file(sslctx,cert_file.c_str(),SSL_FILETYPE_PEM) != 1) &&
       (SSL_CTX_use_certificate_file(sslctx,cert_file.c_str(),SSL_FILETYPE_ASN1) != 1)) {
      logger.msg(ERROR, "Can not load certificate file - %s",cert_file);
      tls_process_error(logger);
      return false;
    };
  };
  if(!key_file.empty()) {
    if((SSL_CTX_use_PrivateKey_file(sslctx,key_file.c_str(),SSL_FILETYPE_PEM) != 1) &&
       (SSL_CTX_use_PrivateKey_file(sslctx,key_file.c_str(),SSL_FILETYPE_ASN1) != 1)) {
      logger.msg(ERROR, "Can not load key file - %s",key_file);
      tls_process_error(logger);
      return false;
    };
  };
  if(!(SSL_CTX_check_private_key(sslctx))) {
    logger.msg(ERROR, "Private key does not match certificate");
    tls_process_error(logger);
    return false;
  };
  return true;
}

PayloadTLSMCC::PayloadTLSMCC(MCCInterface* mcc, SSL_CTX *ctx, const std::string& cert_file, const std::string& key_file, Logger& logger):sslctx_(ctx),logger(logger) { 
   // Client mode
   int err;
   master_=true;
   BIO* bio = BIO_new_MCC(mcc);
   if(!reload_certificates(sslctx_,cert_file,key_file,logger)) {
      tls_process_error(logger);
      BIO_free(bio);
      return;
   };
   ssl_ = SSL_new(ctx);
   if (ssl_ == NULL){
      tls_process_error(logger);
      BIO_free(bio);
      return;
   };
   SSL_set_bio(ssl_,bio,bio);
   //SSL_set_connect_state(ssl_);
   if((err=SSL_connect(ssl_)) != 1) {
     logger.msg(ERROR, "Failed to establish SSL connection");
     tls_process_error(logger);
     return;
   };
   // if(SSL_in_init(ssl_)){
   //handle error
   // }
}

PayloadTLSMCC::PayloadTLSMCC(PayloadStreamInterface* stream, SSL_CTX* ctx, const std::string& cert_file, const std::string& key_file, Logger& logger):sslctx_(ctx),logger(logger) {
   // Server mode
   int err;
   master_=true;
   BIO* bio = BIO_new_MCC(stream);
   if(!reload_certificates(sslctx_,cert_file,key_file,logger)) {
      tls_process_error(logger);
      BIO_free(bio);
      return;
   };
   ssl_ = SSL_new(ctx);
   if (ssl_ == NULL){
      tls_process_error(logger);
      BIO_free(bio);
      return;
   };
   SSL_set_bio(ssl_,bio,bio);
   //SSL_set_accept_state(ssl_);
   if((err=SSL_accept(ssl_)) != 1) {
     logger.msg(ERROR, "Failed to establish SSL connection");
     tls_process_error(logger);
     return;
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
