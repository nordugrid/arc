#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <openssl/asn1.h>
#include <openssl/pem.h>
#include <openssl/x509.h>
#include <openssl/x509v3.h>
#include <openssl/pkcs12.h>
#include <openssl/err.h>

#include "credential.h"

namespace Arc {
  static Logger credentialLogger(Logger::getRootLogger(), "Credential");

  //Copy from MCCTLS.cpp, there could be a common library in libs/misc to deal with openssl error? 
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

  //Get the life time of the credential
  static void getLifetime(std::list<X509*> certs, Time& lifetime) {
    X509* tmp_cert = NULL;
    Time time(-1);
    int count;
    std::list<X509*>::iterator it;
    for(it = certs.begin(); it != certs.end(); it++) {
      tmp_cert = *it;
      ASN1_UTCTIME* atime = X509_get_notAfter(tmp_cert);
      Time tmp_time(atime->data);  //Need debug! probably need some modification on Time class
      if (time == Time(-1) || tmp_time < time) { time = tmp_time; }
    }
    lifetime = time;
  }

  //Parse the BIO for certificate and get the format of it
  Credential::getFormat(BIO* bio) {
    PKCS12* pkcs12 = NULL;
    certformat format;
    char buf[1];
    char firstbyte;
    int position;
    try {
      if((position = BIO_tell(bio))<0 || BIO_read(bio, buf, 1)<=0 || BIO_seek(bio, position)<0) {
        tls_process_error(credentialLogger); 
        throw CredentialError("Can't get the first byte of input BIO to get its format");
      }
    }
    firstbyte = buf[0];
    // DER-encoded structure (including PKCS12) will start with ASCII 048.
    // Otherwise, it's PEM.
    if(firstbyte==48) {
        // DER-encoded, PKCS12 or DER? parse it as PKCS12 ASN.1, if can not parse it, then it is DER format
      if((pkcs12=d2i_PKCS12_bio(bio,NULL))==NULL){
        format=DER; } 
      else {format=PKCS12;}
      if(pkcs12){PKCS12_free(p12);}   
      if( BIO_seek(bio, position) < 0 ) {
        tls_process_error(credentialLogger);
        throw CredentialError("Can't reset the BIO");
      }
    }
    else {format = PEM;}
    return format;
  }

  Credential::Credential(const std::string& cert, const std::string& key, const std::string& cainfo) {
    //Parse the certificate
    BIO* certbio;
    int n=0;
    try{
      X509* x509=NULL;
      PKCS12* p12=NULL;
      certbio = BIO_new_file(cert.c_str(), "r");
      if (certbio){
        certformat = getFormat(certbio);
        credentialLogger.msg(INFO,"Cerficate format for certicate: %s is %s", cert.c_str(), certformat);
      }
      else {
        credentialLogger.msg(ERROR,"Can not read certificate file: %s ", cert.c_str());
        throw CredentialError("Can not read certificate file");
      }

      if(cert_) {
        X509_free(cert_);
        cert_ = NULL;
      }      

      if(cert_chain != NULL){
        sk_X509_pop_free(cert_chain_, X509_free);
      }

      switch(certformat) {
        case PEM:
          //Get the certificte, By default, certificate is without passphrase
          x509 = PEM_read_bio_X509(certbio, &cert_, NULL, NULL);
          certs_.push_back(x509);
          //Get the issuer chain
          cert_chain_ = sk_X509_new_null();
          while(!BIO_eof(cert_bio)){
            X509 * tmp = NULL;
            if(!(x509 = PEM_read_bio_X509(cert_bio, &tmp, NULL, NULL))){
              ERR_clear_error();
              break;
            }
            certs_.push_back(x509);
            if(!sk_X509_insert(cert_chain_, tmp, n))
            {
              X509_free(tmp);
              std::string str(X509_NAME_oneline(X509_get_subject_name(tmp_cert),0,0));
              credentialLogger.msg(ERROR, "Can not insert cert %s into certificate's issuer chain", str.c_str());
              BIO_free(cert_bio);
              throw CredentialError("Can not insert cert into certificate's issuer chain");
            }
            ++n;
          }
          //Get the lifetime of the credential
          getLifetime(cert_, lifetime_);
          break;                      

        case DER:
          x509=d2i_X509_bio(cert_bio,NULL);
          if(x509)
            certs_.push_back(x509);
          else {
            tls_process_error(credentialLogger);
            BIO_free(cert_bio);
            throw CredentialError("Unable to read DER credential from BIO");
          }
          break;

        case PKCS12:
         pkcs12 = d2i_PKCS12_bio(cert_bio, NULL);
         if(pkcs12){
           PKCS12_parse(pkcs12, certpass.get(), NULL, &x509, NULL);
           PKCS12_free(p12);
         }
         if(x509) {
           certs_.push_back(x509);
           x509=NULL;
         } 
         else {
           tls_process_error(credentialLogger);
           BIO_free(cert_bio);
           throw CredentialError("Unable to read PKCS12 certificate from BIO);
         }
         break;
       } // end switch

/*
     } 
     else {
       tls_process_error(credentialLogger);
       BIO_free(cert_bio);
       cert_bio=NULL;
       throw CredentialError("Unable to read certificate from BIO);
     }
     if (cert_bio) {
       BIO_free(cert_bio);
       cert_bio=NULL;
     }
    
  }*/

}

