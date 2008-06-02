#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "PayloadTLSMCC.h"
#include <openssl/err.h>

namespace Arc {

// Here certificates are reloaded. There are no locks because certificates 
// are the same unless updated by administrator/user and there are internal
// locks in OpenSSL.
static bool reload_certificates(SSL_CTX* sslctx, const std::string& cert_file, const std::string& key_file, Logger& logger) {
/* @@@@@
  if(!cert_file.empty()) {
    if((SSL_CTX_use_certificate_chain_file(sslctx,cert_file.c_str()) != 1) &&
       (SSL_CTX_use_certificate_file(sslctx,cert_file.c_str(),SSL_FILETYPE_PEM) != 1) &&
       (SSL_CTX_use_certificate_file(sslctx,cert_file.c_str(),SSL_FILETYPE_ASN1) != 1)) {
      logger.msg(ERROR, "Can not load certificate file - %s",cert_file);
      PayloadTLSStream::HandleError(logger);
      return false;
    };
  };
  if(!key_file.empty()) {
    if((SSL_CTX_use_PrivateKey_file(sslctx,key_file.c_str(),SSL_FILETYPE_PEM) != 1) &&
       (SSL_CTX_use_PrivateKey_file(sslctx,key_file.c_str(),SSL_FILETYPE_ASN1) != 1)) {
      logger.msg(ERROR, "Can not load key file - %s",key_file);
      PayloadTLSStream::HandleError(logger);
      return false;
    };
  };
  if(!(SSL_CTX_check_private_key(sslctx))) {
    logger.msg(ERROR, "Private key does not match certificate");
    PayloadTLSStream::HandleError(logger);
    return false;
  };
*/
  return true;
}

PayloadTLSMCC::PayloadTLSMCC(MCCInterface* mcc, SSL_CTX *ctx, const std::string& cert_file, const std::string& key_file, Logger& logger):PayloadTLSStream(logger),sslctx_(ctx) { 
   // Client mode
   int err;
   master_=true;
   BIO* bio = BIO_new_MCC(mcc);
   if(!reload_certificates(sslctx_,cert_file,key_file,logger)) {
      BIO_free(bio); bio=NULL;
      return;
   };
   ssl_ = SSL_new(ctx);
   if (ssl_ == NULL){
      HandleError();
      BIO_free(bio); bio=NULL;
      return;
   };
   SSL_set_bio(ssl_,bio,bio);
   //SSL_set_connect_state(ssl_);
   if((err=SSL_connect(ssl_)) != 1) {
      logger.msg(ERROR, "Failed to establish SSL connection");
      HandleError(SSL_get_error(ssl_,err));
      return;
   };
   // if(SSL_in_init(ssl_)){
   //handle error
   // }
}

PayloadTLSMCC::PayloadTLSMCC(PayloadStreamInterface* stream, SSL_CTX* ctx, const std::string& cert_file, const std::string& key_file, Logger& logger):PayloadTLSStream(logger),sslctx_(ctx) {
   // Server mode
   int err;
   master_=true;
   BIO* bio = BIO_new_MCC(stream);
   if(!reload_certificates(sslctx_,cert_file,key_file,logger)) {
      BIO_free(bio); bio=NULL;
      return;
   };
   ssl_ = SSL_new(ctx);
   if (ssl_ == NULL){
      HandleError();
      BIO_free(bio); bio=NULL;
      return;
   };
   SSL_set_bio(ssl_,bio,bio);
   //SSL_set_accept_state(ssl_);
   if((err=SSL_accept(ssl_)) != 1) {
     logger.msg(ERROR, "Failed to accept SSL connection");
     HandleError(SSL_get_error(ssl_,err));
     return;
   };
   //handle error
   // if(SSL_in_init(ssl_)){
   //handle error
   // }
}

PayloadTLSMCC::PayloadTLSMCC(PayloadTLSMCC& stream):
  PayloadTLSStream(stream)
{
   master_=false;
   sslctx_=stream.sslctx_; 
   ssl_=stream.ssl_; 
}


PayloadTLSMCC::~PayloadTLSMCC(void) {
  if (!master_)
    return;
  if (ssl_) {
    if (SSL_shutdown(ssl_) < 0)
      logger_.msg(ERROR, "Failed to shut down SSL");
    SSL_free(ssl_);
    ssl_ = NULL;
  }
}

} // namespace Arc
