#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "Credential.h"

namespace Arc {
  static Logger credentialLogger(Logger::getRootLogger(), "Credential");

  static int ssl_err_cb(const char *str, size_t len, void *u) {
    Logger& logger = *((Logger*)u);
    logger.msg(ERROR, "OpenSSL Error: %s", str);
    return 1;
  }

  void Credential::LogError(void) {
    ERR_print_errors_cb(&ssl_err_cb, &credentialLogger);
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

  void Credential::InitVerification(void) {
    verify_ctx_.cert_store = NULL;
    verify_ctx_.cert_depth = 0;
    verify_ctx_.proxy_depth = 0;
    verify_ctx_.max_proxy_depth = -1;
    verify_ctx_.limited_proxy = 0;
    verify_ctx_.cert_type = CERT_TYPE_EEC;  
    verify_ctx_.cert_chain = NULL;
    verify_ctx_.cert_dir = cacertdir;
  }

  bool Credential::Verify(void) {
    if(cert_verify_chain(cert_, cert_chain_, verify_ctx_)) {
      credentialLogger.msg(INFO, "Certificate verify OK");
      return true;
    }
    else {credentialLogger.msg(ERROR, "Certificate verify failed"); LogError(); return false;}
  } 

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

    InitVerification();
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


  X509_EXTENSION* CreateExtension(std::string name, std::string data, bool crit) {
    X509_EXTENSION*   ext = NULL;
    ASN1_OBJECT*      ext_obj = NULL;
    ASN1_OCTET_STRING*  ext_oct = NULL;

    if(!(ext_obj = OBJ_nid2obj(OBJ_txt2nid((char *)name.c_str())))) {
      credentialLogger.msg("Can not convert string into ASN1_OBJECT");
      LogError(); return NULL;
    }
  
    ext_oct = ASN1_OCTET_STRING_new();
  
    ext_oct->data = (unsigned char *)data.c_str();
    ext_oct->length = data.size();
  
    if (!(ext = X509_EXTENSION_create_by_OBJ(NULL, ext_obj, crit, ext_oct))) {
      credentialLogger.msg("Can not create extension for proxy certificate");
      LogError();
    }
	
    if(ext_oct) ASN1_OCTET_STRING_free(ext_oct);
    if(ext_obj) ASN1_OBJECT_free(ext_obj);
    ext_oct = NULL;
	
    return ext;
  }  

  //TODO: self request/sign proxy

  bool Credential::GenerateRequest(BIO* &reqbio){
    bool res = false;
    X509_NAME *         req_name = NULL;
    X509_NAME_ENTRY *   req_name_entry = NULL;
    RSA *               rsa_key = NULL;
    int keybits = 1024;
    const EVP_MD *digest = EVP_md5();
    EVP_PKEY* pkey;
  
    if(pkey_) { credentialLogger.msg("The Credential's private key has already be initialized"); return false; }; 
     
#ifdef HAVE_OPENSSL_OLDRSA
    unsigned long prime = RSA_F4; 
    rsa_key = RSA_generate_key(keybits, prime, keygen_cb, NULL);
    if(!rsa_key) { 
      credentialLogger.msg("RSA_generate_key failed"); 
      LogError(); 
      if(prime) BN_free(prime); 
      return false; 
    }
#else
    BN_GENCB cb;
    BIGNUM *prime = BN_new();
    rsa_key = RSA_new();

    BN_GENCB_set(&cb,&keygen_cb,NULL);
    if(prime && rsa_key) {
      if(BN_set_word(bn,RSA_F4)) {
        if(!RSA_generate_key_ex(rsa_key, keybits, prime, &keygen_cb)) { 
          credentialLogger.msg("RSA_generate_key_ex failed"); 
          LogError();
          if(prime) BN_free(prime);
          return false;
        }
      }      
      else{ 
        credentialLogger.msg("BN_set_word failed"); 
        LogError(); 
        if(prime) BN_free(prime); 
        if(rsa_key) RSA_free(rsa_key); 
        return false; }
    } 
    else { 
      credentialLogger.msg("BN_new || RSA_new failed"); 
      LogError(); 
      if(prime) BN_free(prime); 
      if(rsa_key) RSA_free(rsa_key); 
      return false; 
    }
    if(prime) BN_free(prime);
#endif
    
    pkey = EVP_PKEY_new();
    if(pkey){
      if(rsa_key) {
        if(EVP_PKEY_set1_RSA(pkey, rsa_key)) {
          X509_REQ *req = X509_REQ_new();
          if(req) {
            if(X509_REQ_set_version(req,2L)) {
              //set the DN
              X509_NAME* name = NULL;
              X509_NAME_ENTRY* entry = NULL;
              if(cert_) { //self-sign, copy the X509_NAME
                if ((name = X509_NAME_dup(X509_get_subject_name(cert_))) == NULL) {
                  credentialLogger.msg("Can not duplicate the subject name for the self-signing proxy certificate request");
                  LogError(); res = false; goto err;
                }
              }
              else { name = X509_NAME_new();}
              if((entry = X509_NAME_ENTRY_create_by_NID(NULL, NID_commonName, V_ASN1_APP_CHOOSE,
                          (unsigned char *) "NULL SUBJECT NAME ENTRY", -1)) == NULL) {
                credentialLogger.msg("Can not create a new X509_NAME_ENTRY forthe proxy certificate request");
                LogError(); res = false; X509_NAME_free(name); goto err;
              }
              X509_NAME_add_entry(name, entry, X509_NAME_entry_count(name), 0);
              X509_REQ_set_subject_name(req,name);
              X509_NAME_free(name); name = NULL;
              if(entry) { X509_NAME_ENTRY_free(entry); entry = NULL; }

              // set the default PROXYCERTINFO extension
              std::string certinfo_sn;
              X509_EXTENSION* ext = NULL;
              STACK_OF(X509_EXTENSION)* extensions;
              if(CERT_TYPE_IS_GSI_3_PROXY(cert_type_)) certinfo_sn = "OLD_PROXYCERTINFO";
              else if (CERT_TYPE_IS_RFC_PROXY(cert_type_)) certinfo_sn = "PROXYCERTINFO";
              //if(proxy_cert_info_->version == 3)         

              if(!(certinfo_sn.empty())) {
                X509V3_EXT_METHOD*  ext_method;
                std::string ext_data;
                unsigned char* data = NULL;
                int length;
                ext_method = X509V3_EXT_get_nid(OBJ_sn2nid(certinfo_sn.c_str()));
                length = ext_method->i2d(proxy_cert_info_, NULL);
                if(length < 0) { credentialLogger.msg("Can not convert PROXYCERTINFO struct from internal to DER encoded form"); LogError(); }
                else { data = malloc(length); }
                length = ext_method->i2d(proxy_cert_info_, &data);
                if(length < 0) { 
                  credentialLogger.msg("Can not convert PROXYCERTINFO struct from internal to DER encoded form"); 
                  free(data); data = NULL; LogError(); 
                }
                if(data) {
                  ext_data = data; free(data);
                  ext = CreateExtension(certinfo_sn, ext_data, 1);
                }
              }
              if(ext) {
                extensions = sk_X509_EXTENSION_new_null();
                sk_X509_EXTENSION_push(extensions, ext);
                X509_REQ_add_extensions(req, extensions);
                sk_X509_EXTENSION_pop_free(extensions, X509_EXTENSION_free);
              }

              if(X509_REQ_set_pubkey(req,pkey)) {
                if(X509_REQ_sign(req,pkey,digest)) {
                  if(!(PEM_write_bio_X509_REQ(reqbio,req))){ credentialLogger.msg("PEM_write_bio_X509_REQ failed"); LogError(); res = false;}
                  else { rsa_key_ = rsa_key; rsa_key = NULL; pkey_ = pkey; pkey = NULL; res = true; }
                }
              }
            }
            X509_REQ_free(req);
          }
          else { credentialLogger.msg("Can not generate X509 request"); LogError(); res = false; }
        }
        else { credentialLogger.msg("Can not set private key"); LogError(); res = false;}
      }
    }

err:
    if(pkey) EVP_PKEY_free(pkey);
    if(rsa_key) RSA_free(rsa_key);

    return res;
  }

  //Inquire the input request bio to get PROXYCERTINFO, certType
  bool Credential::InquireRequest(BIO* &reqbio){
    bool res = false;
    if(reqbio == NULL) { credentialLogger.msg("NULL BIO passed to InquireRequest"); return false; }
    if(req_) {X509_REQ_free(req_); req_ = NULL; }
    if(!(PEM_read_bio_X509_REQ(reqbio,req_))) {  credentialLogger.msg("PEM_read_bio_X509_REQ failed"); LogError(); return false; }

    STACK_OF(X509_EXTENSION)* req_extensions = NULL;
    X509_EXTENSION* ext;
    PROXYPOLICY*  policy = NULL;
    ASN1_OBJECT*  policy_lang = NULL;
    ASN1_OBJECT*  extension_oid = NULL;
    int certinfo_NID, certinfo_old_NID, certinfo_v3_NID, certinfo_v4_NID, nid;
    int i;

    req_extensions = X509_REQ_get_extensions(req_);
    certinfo_NID = OBJ_sn2nid("PROXYCERTINFO");
    certinfo_old_NID = OBJ_sn2nid("OLD_PROXYCERTINFO");
    certinfo_v3_NID = OBJ_sn2nid("PROXYCERTINFO_V3");
    certinfo_v4_NID = OBJ_sn2nid("PROXYCERTINFO_V4");
    for(i=0;i<sk_X509_EXTENSION_num(req_extensions);i++) {
      ext = sk_X509_EXTENSION_value(req_extensions,i);
      extension_oid = X509_EXTENSION_get_object(ext);
      nid = OBJ_obj2nid(extension_oid);
      if(nid == certinfo_NID || nid == certinfo_old_NID || nid == certinfo_v3_NID || nid == certinfo_v4_NID) {
        if(proxy_cert_info_) {
          PROXYCERTINFO_free(proxy_cert_info_);
          proxy_cert_info_ = NULL;
        }    
        if((proxy_cert_info_ = X509V3_EXT_d2i(ext)) == NULL) {
           credentialLogger.msg("Can not convert DER encode PROXYCERTINFO extension to internal format"); 
           LogError(); goto err;
        }
        break;
      }
    }
    
    if(proxy_cert_info_) {
      if((policy = PROXYCERTINFO_get_policy(proxy_cert_info_)) == NULL) {
        credentialLogger.msg("Can not get policy from PROXYCERTINFO extension"); 
        LogError(); goto err;
      }    
      if((policy_lang = PROXYPOLICY_get_policy_language(policy)) == NULL) {
        credentialLogger.msg("Can not get policy language from PROXYCERTINFO extension");                                   
        LogError(); goto err;
      }    
      policy_nid = OBJ_obj2nid(policy_lang);
      if(nid == certinfo_old_NID || nid == certinfo_v3_NID) { 
        if(policy_nid == OBJ_sn2nid(IMPERSONATION_PROXY_SN)) { cert_type_= CERT_TYPE_GSI_3_IMPERSONATION_PROXY; }
        else if(policy_nid == OBJ_sn2nid(INDEPENDENT_PROXY_SN)) { cert_type_ = CERT_TYPE_GSI_3_INDEPENDENT_PROXY; }
        else if(policy_nid == OBJ_sn2nid(LIMITED_PROXY_SN)) { cert_type_ = CERT_TYPE_GSI_3_LIMITED_PROXY; }
        else { cert_type_ = CERT_TYPE_GSI_3_RESTRICTED_PROXY; }
      } 
      else {
        if(policy_nid == OBJ_sn2nid(IMPERSONATION_PROXY_SN)) { cert_type_ = CERT_TYPE_RFC_IMPERSONATION_PROXY; }
        else if(policy_nid == OBJ_sn2nid(INDEPENDENT_PROXY_SN)) { cert_type_ = CERT_TYPE_RFC_INDEPENDENT_PROXY; }
        else if(policy_nid == OBJ_sn2nid(LIMITED_PROXY_SN)) { cert_type_ = CERT_TYPE_RFC_LIMITED_PROXY; }
        else { cert_type_ = CERT_TYPE_RFC_RESTRICTED_PROXY; }
      }
    }
    else { cert_type_ = CERT_TYPE_GSI_2_PROXY;}

    res = true;

err:
    if(req_extensions != NULL) { sk_X509_EXTENSION_pop_free(req_extensions, X509_EXTENSION_free); } 

    return res;
  }
  
  bool Credential::SignRequestAssistant(Credential* &proxy, EVP_PKEY* &req_pubkey, X509** tosign_cert){
    

    
  }

  bool Credential::SignRequest(Credential* &proxy, BIO* &outputbio){
    bool res = false;
    if(proxy == NULL) {credentialLogger.msg("The credential to be signed is NULL"); return false; }
    if(outputbio == NULL) { credentialLogger.msg("The BIO for output is NULL"); return false; }
    
    X509*  proxy_cert = NULL;
    EVP_PKEY* req_pubkey = NULL;
    req_pubkey = X509_REQ_get_pubkey(proxy->req_);
    if(!req_pubkey) { credentialLogger.msg("Error when extract public key from request"); LogError(); return false;}

    if((X509_REQ_verify(proxy->req_, req_pubkey)) == 0){
      credentialLogger.msg("Error when extract public key from request"); LogError(); goto err; 
    }
   
    SignRequestAssistant(proxy, req_pubkey, &proxy_cert);

    //Fill the extentions      

err:
   

  }

}

