#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <fstream>

#include "GlobusSigningPolicy.h"
#include "GlobusHack.h"

#include "PayloadTLSMCC.h"
#include <openssl/err.h>
#include <glibmm/miscutils.h>
#include <arc/DateTime.h>
#include <arc/StringConv.h>
#include <arc/crypto/OpenSSL.h>

namespace ArcMCCTLS {

static const char * ex_data_id = "ARC_MCC_Payload_TLS";
int PayloadTLSMCC::ex_data_index_ = -1;

#ifndef HAVE_OPENSSL_X509_VERIFY_PARAM
#define FLAG_CRL_DISABLED (0x1)

static unsigned long get_flag_STORE_CTX(X509_STORE_CTX* container) {
  PayloadTLSMCC* it = PayloadTLSMCC::RetrieveInstance(container);
  if(!it) return 0;
  return it->Flags();
}

static void set_flag_STORE_CTX(X509_STORE_CTX* container,unsigned long flags) {
  PayloadTLSMCC* it = PayloadTLSMCC::RetrieveInstance(container);
  if(!it) return;
  it->Flags(flags);
}
#endif

Time asn1_to_utctime(const ASN1_UTCTIME *s) {
  std::string t_str;
  if(!s) return Time();
  if(s->type == V_ASN1_UTCTIME) {
    t_str.append("20");
    t_str.append((char*)(s->data));
  }
  else {//V_ASN1_GENERALIZEDTIME
    t_str.append((char*)(s->data));
  }
  return Time(t_str);
}

// This callback implements additional verification
// algorithms not present in OpenSSL
static int verify_callback(int ok,X509_STORE_CTX *sctx) {
/*
#ifdef HAVE_OPENSSL_X509_VERIFY_PARAM
  unsigned long flag = get_flag_STORE_CTX(sctx);
  if(!(flag & FLAG_CRL_DISABLED)) {
    // Not sure if this will work
#if OPENSSL_VERSION_NUMBER < 0x00908000
    if(!(sctx->flags & X509_V_FLAG_CRL_CHECK)) {
      sctx->flags |= X509_V_FLAG_CRL_CHECK;
#else
    if(!(sctx->param->flags & X509_V_FLAG_CRL_CHECK)) {
      sctx->param->flags |= X509_V_FLAG_CRL_CHECK;
#endif
      ok=X509_verify_cert(sctx);
      return ok;
    };
  };
#endif
*/
  if (ok != 1) {
    int err = X509_STORE_CTX_get_error(sctx);
    switch(err) {
#ifdef HAVE_OPENSSL_PROXY
      case X509_V_ERR_PROXY_CERTIFICATES_NOT_ALLOWED: {
        // This shouldn't happen here because flags are already set
        // to allow proxy. But one can never know because used flag
        // setting method is undocumented.
        X509_STORE_CTX_set_flags(sctx,X509_V_FLAG_ALLOW_PROXY_CERTS);
        // Calling X509_verify_cert will cause a recursive call to verify_callback.
        // But there should be no loop because PROXY_CERTIFICATES_NOT_ALLOWED error
        // can't happen anymore.
        //ok=X509_verify_cert(sctx);
        ok=1;
        if(ok == 1) X509_STORE_CTX_set_error(sctx,X509_V_OK);
      }; break;
#endif
      case X509_V_ERR_UNABLE_TO_GET_CRL: {
        // Missing CRL is not an error (TODO: make it configurable)
        // Consider that to be a policy of site like Globus does
        // Not sure of there is need for recursive X509_verify_cert() here.
        // It looks like openssl runs tests sequentially for whole chain.
        // But not sure if behavior is same in all versions.
#ifdef HAVE_OPENSSL_X509_VERIFY_PARAM
        if(sctx->param) {
          X509_VERIFY_PARAM_clear_flags(sctx->param,X509_V_FLAG_CRL_CHECK);
          //ok=X509_verify_cert(sctx);
          //X509_VERIFY_PARAM_set_flags(sctx->param,X509_V_FLAG_CRL_CHECK);
          ok=1;
          if(ok == 1) X509_STORE_CTX_set_error(sctx,X509_V_OK);
        };
#else

#if OPENSSL_VERSION_NUMBER < 0x00908000
        sctx->flags &= ~X509_V_FLAG_CRL_CHECK;
#else
        sctx->param->flags &= ~X509_V_FLAG_CRL_CHECK;
#endif

        set_flag_STORE_CTX(sctx,get_flag_STORE_CTX(sctx) | FLAG_CRL_DISABLED);
        //ok=X509_verify_cert(sctx);
        //sctx->flags |= X509_V_FLAG_CRL_CHECK;
        //set_flag_STORE_CTX(sctx,get_flag_STORE_CTX(sctx) & (~FLAG_CRL_DISABLED));
        ok=1;
        if(ok == 1) X509_STORE_CTX_set_error(sctx,X509_V_OK);
#endif
      }; break;
      default: {
        PayloadTLSMCC* it = PayloadTLSMCC::RetrieveInstance(sctx);
        if(it) {
          it->SetFailure((std::string)X509_verify_cert_error_string(err));
        } else {
          Logger::getRootLogger().msg(ERROR,"%s",X509_verify_cert_error_string(err));
        }
      }; break;
    };
  };
  if(ok == 1) {
    // Do additional verification here.
    PayloadTLSMCC* it = PayloadTLSMCC::RetrieveInstance(sctx);
    X509* cert = X509_STORE_CTX_get_current_cert(sctx);
    char* subject_name = X509_NAME_oneline(X509_get_subject_name(cert),NULL,0);
    if(it == NULL) {
      Logger::getRootLogger().msg(WARNING,"Failed to retrieve link to TLS stream. Additional policy matching is skipped.");
    } else {
      // Globus signing policy
      // Do not apply to proxies and self-signed CAs.
      if((it->Config().GlobusPolicy()) && (!(it->Config().CADir().empty()))) {
#ifdef HAVE_OPENSSL_PROXY
        int pos = X509_get_ext_by_NID(cert,NID_proxyCertInfo,-1);
        if(pos < 0)
#endif
        {
          std::istream* in = open_globus_policy(X509_get_issuer_name(cert),it->Config().CADir());
          if(in) {
            if(!match_globus_policy(*in,X509_get_issuer_name(cert),X509_get_subject_name(cert))) {
              it->SetFailure(std::string("Certificate ")+subject_name+" failed Globus signing policy");
              ok=0;
              X509_STORE_CTX_set_error(sctx,X509_V_ERR_SUBJECT_ISSUER_MISMATCH);
            };
            delete in;
          };
        };
      };
    };
    //Check the left validity time of the peer certificate;
    //Give warning if the certificate is going to be expired
    //in a while of time
    Time exptime = asn1_to_utctime(X509_get_notAfter(cert));
    if(exptime <= Time()) {
      Logger::getRootLogger().msg(WARNING,"Certificate %s already expired",subject_name);
    } else {
      Arc::Period timeleft = exptime - Time();
#ifdef HAVE_OPENSSL_PROXY
      int pos = X509_get_ext_by_NID(cert,NID_proxyCertInfo,-1);
#else
      int pos = -1;
#endif
      //for proxy certificate, give warning 1 hour in advance
      //for EEC certificate, give warning 5 days in advance
      if(((pos < 0) && (timeleft <= 5*24*3600)) ||
         (timeleft <= 3600)) {
        Logger::getRootLogger().msg(WARNING,"Certificate %s will expire in %s", subject_name, timeleft.istr());
      }
    }
    OPENSSL_free(subject_name);
  };
  return ok;
}

// This callback is just a placeholder. We do not expect
// encrypted private keys here.
static int no_passphrase_callback(char*, int, int, void*) {
   return -1;
}

bool PayloadTLSMCC::StoreInstance(void) {
   if(ex_data_index_ == -1) {
      // In case of race condition we will have 2 indices assigned - harmless?
      ex_data_index_=OpenSSLAppDataIndex(ex_data_id);
   };
   if(ex_data_index_ == -1) {
      logger_.msg(WARNING,"Failed to store application data");
      return false;
   };
   if(!sslctx_) return false;
   SSL_CTX_set_ex_data(sslctx_,ex_data_index_,this);
   return true;
}

bool PayloadTLSMCC::ClearInstance(void) {
  if((ex_data_index_ != -1) && sslctx_) {
    SSL_CTX_set_ex_data(sslctx_,ex_data_index_,NULL);
    return true;
  };
  return false;
}

PayloadTLSMCC* PayloadTLSMCC::RetrieveInstance(X509_STORE_CTX* container) {
  PayloadTLSMCC* it = NULL;
  if(ex_data_index_ != -1) {
    SSL* ssl = (SSL*)X509_STORE_CTX_get_ex_data(container,SSL_get_ex_data_X509_STORE_CTX_idx());
    if(ssl != NULL) {
      SSL_CTX* ssl_ctx = SSL_get_SSL_CTX(ssl);
      if(ssl_ctx != NULL) {
        it = (PayloadTLSMCC*)SSL_CTX_get_ex_data(ssl_ctx,ex_data_index_);
      }
    };
  };
  if(it == NULL) {
    Logger::getRootLogger().msg(WARNING,"Failed to retrieve application data from OpenSSL");
  };
  return it;
}


PayloadTLSMCC::PayloadTLSMCC(MCCInterface* mcc, const ConfigTLSMCC& cfg, Logger& logger):
    PayloadTLSStream(logger),sslctx_(NULL),bio_(NULL),config_(cfg),flags_(0) {
   // Client mode
   int err = SSL_ERROR_NONE;
   char gsi_cmd[1] = { '0' };
   master_=true;
   // Creating BIO for communication through stream which it will
   // extract from provided MCC
   BIO* bio = (bio_ = config_.GlobusIOGSI()?BIO_new_GSIMCC(mcc):BIO_new_MCC(mcc));
   // Initialize the SSL Context object
   if(cfg.IfTLSHandshake()) {
     sslctx_=SSL_CTX_new(SSLv23_client_method());
   } else {
     sslctx_=SSL_CTX_new(SSLv3_client_method());
   };
   if(sslctx_==NULL){
      logger.msg(ERROR, "Can not create the SSL Context object");
      goto error;
   };
   SSL_CTX_set_mode(sslctx_,SSL_MODE_ENABLE_PARTIAL_WRITE);
   SSL_CTX_set_session_cache_mode(sslctx_,SSL_SESS_CACHE_OFF);
   if(!config_.Set(sslctx_)) goto error;
   SSL_CTX_set_verify(sslctx_, SSL_VERIFY_PEER |  SSL_VERIFY_FAIL_IF_NO_PEER_CERT, &verify_callback);
   GlobusSetVerifyCertCallback(sslctx_);

   // Allow proxies, request CRL check
#ifdef HAVE_OPENSSL_X509_VERIFY_PARAM
   if(sslctx_->param == NULL) {
      logger.msg(ERROR,"Can't set OpenSSL verify flags");
      goto error;
   } else {
#ifdef HAVE_OPENSSL_PROXY
      if(sslctx_->param) X509_VERIFY_PARAM_set_flags(sslctx_->param,X509_V_FLAG_CRL_CHECK | X509_V_FLAG_ALLOW_PROXY_CERTS);
#else
      if(sslctx_->param) X509_VERIFY_PARAM_set_flags(sslctx_->param,X509_V_FLAG_CRL_CHECK);
#endif
   };
#endif
   StoreInstance();
#ifdef SSL_OP_NO_TICKET
   SSL_CTX_set_options(sslctx_, SSL_OP_SINGLE_DH_USE | SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3 | SSL_OP_ALL | SSL_OP_NO_TICKET);
#else
   SSL_CTX_set_options(sslctx_, SSL_OP_SINGLE_DH_USE | SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3 | SSL_OP_ALL);
#endif

   SSL_CTX_set_default_passwd_cb(sslctx_, no_passphrase_callback);
   /* Get DN from certificate, and put it into message's attribute */

   // Creating SSL object for handling connection
   ssl_ = SSL_new(sslctx_);
   if (ssl_ == NULL){
      logger.msg(ERROR, "Can not create the SSL object");
      goto error;
   };
   //for(int n = 0;;++n) {
   //  const char * s = SSL_get_cipher_list(ssl_,n);
   //  if(!s) break;
   //  logger.msg(VERBOSE, "Allowed cipher: %s",s);
   //};
   SSL_set_bio(ssl_,bio,bio); bio=NULL;
   //SSL_set_connect_state(ssl_);
   if((err=SSL_connect(ssl_)) != 1) {
      err = SSL_get_error(ssl_,err);
      /* TODO: Print nice message when server side certificate has
       *       expired. Still to investigate if this case is only when
       *       server side certificate has expired.
      if (ERR_GET_REASON(ERR_peek_last_error()) == SSL_R_SSLV3_ALERT_CERTIFICATE_EXPIRED) {
        logger.msg(INFO, "Server side certificate expired.");
      }
      */
      logger.msg(VERBOSE, "Failed to establish SSL connection");
      goto error;
   };
   logger.msg(VERBOSE, "Using cipher: %s",SSL_get_cipher_name(ssl_));
   // if(SSL_in_init(ssl_)){
   //handle error
   // }
   if(config_.GlobusGSI() || config_.GlobusIOGSI()) {
     Put(gsi_cmd,1);
   }
   return;
error:
   if (failure_) SetFailure(err); // Only set if not already set.
   if(bio) BIO_free(bio); bio_=NULL;
   if(ssl_) SSL_free(ssl_); ssl_=NULL;
   if(sslctx_) SSL_CTX_free(sslctx_); sslctx_=NULL;
   return;
}

PayloadTLSMCC::PayloadTLSMCC(PayloadStreamInterface* stream, const ConfigTLSMCC& cfg, Logger& logger):
    PayloadTLSStream(logger),sslctx_(NULL),config_(cfg),flags_(0) {
   // Server mode
   int err = SSL_ERROR_NONE;
   master_=true;
   // Creating BIO for communication through provided stream
   BIO* bio = (bio_ = config_.GlobusIOGSI()?BIO_new_GSIMCC(stream):BIO_new_MCC(stream));
   // Initialize the SSL Context object
   if(cfg.IfTLSHandshake()) {
     sslctx_=SSL_CTX_new(SSLv23_server_method());
   } else {
     sslctx_=SSL_CTX_new(SSLv3_server_method());
   };
   if(sslctx_==NULL){
      logger.msg(ERROR, "Can not create the SSL Context object");
      goto error;
   };
   SSL_CTX_set_mode(sslctx_,SSL_MODE_ENABLE_PARTIAL_WRITE);
   SSL_CTX_set_session_cache_mode(sslctx_,SSL_SESS_CACHE_OFF);
   if(config_.IfClientAuthn()) {
     SSL_CTX_set_verify(sslctx_, SSL_VERIFY_PEER |  SSL_VERIFY_FAIL_IF_NO_PEER_CERT | SSL_VERIFY_CLIENT_ONCE, &verify_callback);
   }
   else {
     SSL_CTX_set_verify(sslctx_, SSL_VERIFY_NONE, NULL);
   }
   GlobusSetVerifyCertCallback(sslctx_);
   if(!config_.Set(sslctx_)) goto error;

   // Allow proxies, request CRL check
#ifdef HAVE_OPENSSL_X509_VERIFY_PARAM
   if(sslctx_->param == NULL) {
      logger.msg(ERROR,"Can't set OpenSSL verify flags");
      goto error;
   } else {
#ifdef HAVE_OPENSSL_PROXY
      if(sslctx_->param) X509_VERIFY_PARAM_set_flags(sslctx_->param,X509_V_FLAG_CRL_CHECK | X509_V_FLAG_ALLOW_PROXY_CERTS);
#else
      if(sslctx_->param) X509_VERIFY_PARAM_set_flags(sslctx_->param,X509_V_FLAG_CRL_CHECK);
#endif
   };
#endif
   StoreInstance();
   SSL_CTX_set_options(sslctx_, SSL_OP_SINGLE_DH_USE | SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3 | SSL_OP_ALL);
   SSL_CTX_set_default_passwd_cb(sslctx_, no_passphrase_callback);

   // Creating SSL object for handling connection
   ssl_ = SSL_new(sslctx_);
   if (ssl_ == NULL){
      logger.msg(ERROR, "Can not create the SSL object");
      goto error;
   };
   //for(int n = 0;;++n) {
   //  const char * s = SSL_get_cipher_list(ssl_,n);
   //  if(!s) break;
   //  logger.msg(VERBOSE, "Allowed cipher: %s",s);
   //};
   SSL_set_bio(ssl_,bio,bio); bio=NULL;
   //SSL_set_accept_state(ssl_);
   if((err=SSL_accept(ssl_)) != 1) {
      err = SSL_get_error(ssl_,err);
      logger.msg(ERROR, "Failed to accept SSL connection");
      goto error;
   };
   logger.msg(VERBOSE, "Using cipher: %s",SSL_get_cipher_name(ssl_));
   //handle error
   // if(SSL_in_init(ssl_)){
   //handle error
   // }
   return;
error:
   if (failure_) SetFailure(err); // Only set if not already set.
   if(bio) BIO_free(bio); bio_=NULL;
   if(ssl_) SSL_free(ssl_); ssl_=NULL;
   if(sslctx_) SSL_CTX_free(sslctx_); sslctx_=NULL;
   return;
}

PayloadTLSMCC::PayloadTLSMCC(PayloadTLSMCC& stream):
    PayloadTLSStream(stream), config_(stream.config_), flags_(0) {
   master_=false;
   sslctx_=stream.sslctx_;
   ssl_=stream.ssl_;
   bio_=stream.bio_;
}


PayloadTLSMCC::~PayloadTLSMCC(void) {
  if (!master_) return;
  // There are indirect evidences that verify callback
  // was called after this object was destroyed. Although
  // that may be misinterpretation it is probably safer
  // to detach code of this object from OpenSSL now by
  // calling SSL_set_verify/SSL_CTX_set_verify and by
  // removing associated ex_data.
  ClearInstance();
  if (ssl_) {
    SSL_set_verify(ssl_,SSL_VERIFY_NONE,NULL);
    int err = SSL_shutdown(ssl_);
    if(err == 0) err = SSL_shutdown(ssl_);
    if(err < 0) { // -1 expected
      err = SSL_get_error(ssl_,err);
      if((err == SSL_ERROR_WANT_READ) || (err == SSL_ERROR_WANT_WRITE)) {
        // We are not going to wait for connection to
        // close nicely. We are too impatient.
        ConfigTLSMCC::HandleError();
      } else if(err == SSL_ERROR_SYSCALL) {
        // It would be interesting to check errno. But
        // unfortunately it seems to be lost already
        // inside SSL_shutdown().
        ConfigTLSMCC::HandleError();
      } else {
        // This case is unexpected. So it is better to
        // report it.
        logger_.msg(VERBOSE, "Failed to shut down SSL: %s",ConfigTLSMCC::HandleError(err));
      }
      // Trying to get out of error
      SSL_set_quiet_shutdown(ssl_,1);
      SSL_shutdown(ssl_);
    };
    // SSL_clear(ssl_); ???
    SSL_free(ssl_);
    ssl_ = NULL;
  }
  if(sslctx_) {
    SSL_CTX_set_verify(sslctx_,SSL_VERIFY_NONE,NULL);
    SSL_CTX_free(sslctx_);
    sslctx_ = NULL;
  }
  // bio_ was passed to ssl_ and hence does not need to
  // be destroyed explicitely.
}

void PayloadTLSMCC::SetFailure(const std::string& err) {
  MCC_Status bioStatus;
  bool isBioFailure = (config_.GlobusIOGSI() ? BIO_GSIMCC_failure(bio_, bioStatus) : BIO_MCC_failure(bio_, bioStatus));
  if (isBioFailure && bioStatus.getOrigin() != "TLS" && !bioStatus) {
    failure_ = bioStatus;
    return;
  }
  PayloadTLSStream::SetFailure(err);
}

void PayloadTLSMCC::SetFailure(int code) {
  // Sources:
  //  1. Already collected failure in failure_
  //  2. SSL layer
  //  3. Underlying stream through BIO

  MCC_Status bioStatus;
  bool isBioFailure = (config_.GlobusIOGSI() ? BIO_GSIMCC_failure(bio_, bioStatus) : BIO_MCC_failure(bio_, bioStatus));
  if (isBioFailure && bioStatus.getOrigin() != "TLS" && !bioStatus) {
    failure_ = bioStatus;
    return;
  }
  
  std::string err_failure = failure_?"":failure_.getExplanation();
  std::string bio_failure = (!isBioFailure || bioStatus.getOrigin() != "TLS" ? "" : bioStatus.getExplanation());
  std::string tls_failure = ConfigTLSMCC::HandleError(code);
  if(!err_failure.empty() && !bio_failure.empty()) err_failure += "\n";
  err_failure += bio_failure;
  if(!err_failure.empty() && !tls_failure.empty()) err_failure += "\n";
  err_failure += tls_failure;
  if (err_failure.empty()) err_failure = "SSL error, with \"unknown\" alert";
  PayloadTLSStream::SetFailure(err_failure);
}

} // namespace Arc
