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
  Credformat Credential::getFormat(BIO* bio) {
    PKCS12* pkcs12 = NULL;
    Credformat format;
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


  void Credential::loadCertificate(BIO* &certbio, X509* &cert, STACK_OF(X509)* &certs) {
    //Parse the certificate
    Credformat format;
    int n=0;
    try{
      X509* x509=NULL;
      PKCS12* p12=NULL;
      format = getFormat(certbio);
      credentialLogger.msg(INFO,"Cerficate format for BIO is: %s", certformat);
      
      switch(format) {
        case PEM:
          //Get the certificte, By default, certificate is without passphrase
          //Read certificate
          if(!(x509 = PEM_read_bio_X509(certbio, &cert, NULL, NULL)) {
            credentialLogger.msg(ERROR,"Can not read cert information from BIO");
            throw CredentialError("Can not read cert information from BIO");
          }
 
          //Get the issuer chain
          certs = sk_X509_new_null();
          while(!BIO_eof(certbio)){
            X509 * tmp = NULL;
            if(!(x509 = PEM_read_bio_X509(certbio, &tmp, NULL, NULL))){
              ERR_clear_error();
              break;
            }
            certs.push_back(x509);
            if(!sk_X509_insert(certs, tmp, n))
            {
              X509_free(tmp);
              std::string str(X509_NAME_oneline(X509_get_subject_name(tmp),0,0));
              credentialLogger.msg(ERROR, "Can not insert cert %s into certificate's issuer chain", str.c_str());
              throw CredentialError("Can not insert cert into certificate's issuer chain");
            }
            ++n;
          }
          break;

        case DER:
          cert=d2i_X509_bio(certbio,NULL);
          if(!cert){
            credentialLogger.msg(ERROR,"Unable to read DER credential from BIO");
            throw CredentialError("Unable to read DER credential from BIO");
          }
          break;

        case PKCS12:
          STACK_OF(X509)* pkcs12_certs = NULL;
          pkcs12 = d2i_PKCS12_bio(certbio, NULL);
          if(pkcs12){
            char password[100];
            EVP_read_pw_string(password, 100, "Enter Password for PKCS12 certificate:", 0));
            if(!PKCS12_parse(pkcs12, password, NULL, &cert, &pkcs12_certs)) {
              credentialLogger.msg(ERROR,"Can not parse PKCS12 file");
              throw CredentialError("Can not parse PKCS12 file");
            }
          }
          else{
            credentialLogger.msg(ERROR,"Can not read PKCS12 credential file: %s from BIO", cert.c_str());
            throw CredentialError("Can not read PKCS12 credential file");
          }

          if (pkcs12_certs && sk_num(pkcs12_certs){
            X509* tmp;
            for (n = 0; n < sk_X509_num(pkcs12_certs); n++) {
              tmp = X509_dup(sk_X509_value(pkcs12_certs, n));
              sk_X509_insert(certs, tmp, n);
	    }
          }

          if(pkcs12) { PKCS12_free(pkcs12); }
          if(pkcs12_certs) { sk_X509_pop_free(pkcs12_certs, X509_free); }

          break;
       } // end switch

  }

  void Credential::loadCA(BIO* &certbio, X509* &cert, STACK_OF(X509)* &certs) {

  }  

  void Credential::loadKey(BIO* &keybio, EVP_PKEY* &pkey) {
    //Read key
    Credformat format;
    format = getFormat(keybio);
    switch(format){
      case PEM:
        if(!(PEM_read_bio_PrivateKey(keybio, &pkey, passwordcallback, NULL))) {  
          credentialLogger.msg(ERROR,"Can not read credential key from PEM key BIO: %s ");
          throw CredentialError("Can not read credential key");
        }
        break;

      case DER:
        pkey=d2i_PrivateKey_bio(keybio, NULL);
        break;

      case PKCS12: 
        PKCS12* pkcs12 = d2i_PKCS12_bio(keybio, NULL);
        if(pkcs12) {
          char password[100];
          EVP_read_pw_string(password, 100, "Enter Password for PKCS12 certificate:", 0));
          if(!PKCS12_parse(pkcs12, password, &pkey, NULL, NULL)) {
            credentialLogger.msg(ERROR,"Can not parse PKCS12 file");
            throw CredentialError("Can not parse PKCS12 file");
          }
          PKCS12_free(pkcs12);
        }
        break;

  }

  int Credential::InitProxyCertInfo(void) {
    #define PROXYCERTINFO_V3      "1.3.6.1.4.1.3536.1.222"
    #define PROXYCERTINFO_V4      "1.3.6.1.5.5.7.1.14"
    #define OBJC(c,n) OBJ_create(c,n,#c)
    X509V3_EXT_METHOD *pci_x509v3_ext_meth;

    /* Proxy Certificate Extension's related objects */
    OBJC(PROXYCERTINFO_V3, "PROXYCERTINFO_V3");
    OBJC(PROXYCERTINFO_V4, "PROXYCERTINFO_V4");

    pci_x509v3_ext_meth = PROXYCERTINFO_v3_x509v3_ext_meth();
    if (pci_x509v3_ext_meth) {
      pci_x509v3_ext_meth->ext_nid = OBJ_txt2nid("PROXYCERTINFO_V3");
      X509V3_EXT_add(pci_x509v3_ext_meth);
    }

    pci_x509v3_ext_meth = PROXYCERTINFO_v4_x509v3_ext_meth();
    if (pci_x509v3_ext_meth) {
      pci_x509v3_ext_meth->ext_nid = OBJ_txt2nid("PROXYCERTINFO_V4");
      X509V3_EXT_add(pci_x509v3_ext_meth);
    }
  }
  
  Credential::Verify_cert_chain(void) {
    

int                                 i;
    int                                 j;
    int                                 retval = 0;
    X509_STORE *                        cert_store = NULL;
    X509_LOOKUP *                       lookup = NULL;
    X509_STORE_CTX                      csc;
    X509 *                              xcert = NULL;
    X509 *                              scert = NULL;
    scert = ucert;
 
    cert_store = X509_STORE_new();
    X509_STORE_set_verify_cb_func(cert_store, proxy_verify_callback);
    if (cert_chain_ != NULL) {
      for (i=0;i<sk_X509_num(cert_chain_);i++) {
            xcert = sk_X509_value(cert_chain_,i);
            if (!scert)
            {
                scert = xcert;
            }
            else
            {
                j = X509_STORE_add_cert(cert_store, xcert);
                if (!j)
                {
                    if ((ERR_GET_REASON(ERR_peek_error()) ==
                         X509_R_CERT_ALREADY_IN_HASH_TABLE))
                    {
                        ERR_clear_error();
                        break;
                    }
                    else
                    {
                        /*DEE need errprhere */
                        goto err;
                    }
                }
            }
        }
    }
    if ((lookup = X509_STORE_add_lookup(cert_store,
                                        X509_LOOKUP_hash_dir())))
    {
        X509_LOOKUP_add_dir(lookup,pvd->pvxd->certdir,X509_FILETYPE_PEM);
        X509_STORE_CTX_init(&csc,cert_store,scert,NULL);

#if SSLEAY_VERSION_NUMBER >=  0x0090600fL
        /* override the check_issued with our version */
        csc.check_issued = proxy_check_issued;
#endif
        X509_STORE_CTX_set_ex_data(&csc,
                                   PVD_STORE_EX_DATA_IDX, (void *)pvd);
                 
        if(!X509_verify_cert(&csc))
        {
            goto err;
        }
    } 
    retval = 1;

err:
    return retval;}
 


  Credential::Credential(const std::string& certfile, const std::string& keyfile, const std::string& cafile) {
   BIO* certbio, BIO* keybio; 
   Credformat format;
   certbio = BIO_new_file(certfile.c_str(), "r"); 
   if(!certbio){
     credentialLogger.msg(ERROR,"Can not read certificate file: %s ", certfile.c_str());
     throw CredentialError("Can not read certificate file");
   }
   keybio = BIO_new_file(keyfile.c_str(), "r");
   if(!keybio){
     credentialLogger.msg(ERROR,"Can not read key file: %s ", keyfile.c_str());
     throw CredentialError("Can not read key file");
   }

   loadCertificate(certbio, cert_, cert_chain_);

   loadKey(keybio, key_);

   //load CA
    
    //Get the lifetime of the credential
    getLifetime(certs_, lifetime_);

    //Initiate the proxy certificate method
    InitProxyCertInfo();

    //Verify whether the certificate is signed by trusted CAs
    Verify();

  }

  #ifdef HAVE_OPENSSL_OLDRSA
  static void keygen_cb(int p, int, void*) {
    char c='*';
    if (p == 0) c='.';
    if (p == 1) c='+';
    if (p == 2) c='*';
    if (p == 3) c='\n';
    std::cerr<<c;
  }
  #else
  static int keygen_cb(int p, int, BN_GENCB*) {
    char c='*';
    if (p == 0) c='.';
    if (p == 1) c='+';
    if (p == 2) c='*';
    if (p == 3) c='\n';
    std::cerr<<c;
    return 1;
  }
  #endif


  bool Credential::GenerateRequest(BIO* &reqbio){
    bool res = false;
    X509_NAME *         req_name = NULL;
    X509_NAME_ENTRY *   req_name_entry = NULL;
    RSA *               rsa_key = NULL;
    int keybits = 1024;
    const EVP_MD *digest = EVP_md5();
    EVP_PKEY* pkey; 
  #ifdef HAVE_OPENSSL_OLDRSA
    unsigned long prime = RSA_F4; 
    rsa_key = RSA_generate_key(keybits, prime, keygen_cb, NULL);
    if(!rsa_key) { credentialLogger.msg("RSA_generate_key failed"); }
  #else
    BN_GENCB cb;
    BIGNUM *prime = BN_new();
    rsa_key = RSA_new();

    BN_GENCB_set(&cb,&keygen_cb,NULL);
    if(prime && rsa_key) {
      if(BN_set_word(bn,RSA_F4)) {
        if(!RSA_generate_key_ex(rsa_key, keybits, prime, &cb)) { credentialLogger.msg("RSA_generate_key_ex failed"); }
      }      
      else{ credentialLogger.msg("BN_set_word failed");}
    } 
    else { credentialLogger.msg("BN_new || RSA_new failed"); }

    if(prime) BN_free(prime);
  #endif
    
    pkey = EVP_PKEY_new();
    if(pkey){
      if(rsa_key) {
        if(EVP_PKEY_set1_RSA(pkey, rsa_key)) {
          X509_REQ *req = X509_REQ_new();
          if(req) {
            if(X509_REQ_set_version(req,2L)) {
              if(X509_REQ_set_pubkey(req,pkey)) {
                if(X509_REQ_sign(req,pkey,digest)) {
                  if(!(PEM_write_bio_X509_REQ(reqbio,req))){ credentialLogger.msg("PEM_write_bio_X509_REQ failed");}
                  else { res = true; rsa_key_ = rsa_key; }
                }
              }
            }
          }
          X509_REQ_free(req);
        }
      }
    }
    EVP_PKEY_free(pkey);

    return res;
  }

  bool Credential::SignRequest(BIO* &reqbio){

    //Fill the extentions      

  }

}

