#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <fstream>

#include "cert_util.h"
#include "Credential.h"

using namespace Arc;

namespace ArcLib {
  CredentialError::CredentialError(const std::string& what) : std::runtime_error(what) { }

  //Logger Credential::credentialLogger(Logger::getRootLogger(), "Credential");
    Logger Credential::credentialLogger(Logger::rootLogger, "Credential");

#define PASS_MIN_LENGTH 4
  static int passwordcb(char* pwd, int len, int rwflag, void* u) {
    int i, j, r;
    char prompt[128];
    for(;;) {
      snprintf(prompt, sizeof(prompt), "Enter password for Username Token: ");
      r = EVP_read_pw_string(pwd, len, prompt, 0);
      if(r != 0) {
        std::cerr<<"Failed to read read input password"<<std::endl;
        memset(pwd,0,(unsigned int)len);
        return(-1);
      }
      j = strlen(pwd);
      if(j < PASS_MIN_LENGTH) {
        std::cerr<<"Input phrase is too short (at least "<<PASS_MIN_LENGTH<<" chars)"<<std::endl;
      }
      else { return (0); }
    }
  }

  static int ssl_err_cb(const char *str, size_t len, void *u) {
    Logger& logger = *((Logger*)u);
    logger.msg(ERROR, "OpenSSL Error: %s", str);
    return 1;
  }

  void Credential::LogError(void) {
    ERR_print_errors_cb(&ssl_err_cb, &credentialLogger);
  }

  //Get the life time of the credential
  static void getLifetime(STACK_OF(X509)* &certchain, X509* &cert, Time& start, Period &lifetime) {
    X509* tmp_cert = NULL;
    Time start_time(-1), end_time(-1);
    int n;
    ASN1_UTCTIME* atime = NULL;

    std::cout<<"Cert Chain number: "<<  sk_X509_num(certchain) <<std::endl;

    for (n = 0; n < sk_X509_num(certchain); n++) {
      tmp_cert = X509_dup(sk_X509_value(certchain, n));

      atime = X509_get_notAfter(tmp_cert);
      std::string tmp_notafter;
      tmp_notafter.append("20");
      tmp_notafter.append((char*)(atime->data));
      Time end(tmp_notafter);  //Need debug! probably need some modification on Time class
      if (end_time == Time(-1) || end < end_time) { end_time = end; }

      atime = X509_get_notBefore(tmp_cert);
      std::string tmp_notbefore;
      tmp_notbefore.append("20");
      tmp_notbefore.append((char*)(atime->data));
      Time start(tmp_notbefore);  //Need debug! probably need some modification on Time class
      if (start_time == Time(-1) || start > start_time) { start_time = start; }
    }

    atime = X509_get_notAfter(cert);
    std::string tmp_notafter;
    tmp_notafter.append("20");
    tmp_notafter.append((char*)(atime->data));
    Time e(tmp_notafter);
    if (end_time == Time(-1) || e < end_time) { end_time = e; }

    atime = X509_get_notBefore(cert);
    std::string tmp_notbefore;
    tmp_notbefore.append("20");
    tmp_notbefore.append((char*)(atime->data));
    Time s(tmp_notbefore);
    if (start_time == Time(-1) || s > start_time) { start_time = s; }

    start = start_time;
    lifetime = end_time - start_time; 
    
    std::cout<<"StartTime: "<<start_time.str()<<" EndTime: " <<end_time.str()<<std::endl; 
  }

  //Parse the BIO for certificate and get the format of it
  Credformat Credential::getFormat(BIO* bio) {
    PKCS12* pkcs12 = NULL;
    Credformat format;
    char buf[1];
    char firstbyte;
    int position;
    if((position = BIO_tell(bio))<0 || BIO_read(bio, buf, 1)<=0 || BIO_seek(bio, position)<0) {
      LogError(); 
      throw CredentialError("Can't get the first byte of input BIO to get its format");
    }
    firstbyte = buf[0];
    // DER-encoded structure (including PKCS12) will start with ASCII 048.
    // Otherwise, it's PEM.
    if(firstbyte==48) {
        // DER-encoded, PKCS12 or DER? parse it as PKCS12 ASN.1, if can not parse it, then it is DER format
      if((pkcs12 = d2i_PKCS12_bio(bio,NULL)) == NULL){ format=DER; PKCS12_free(pkcs12); } 
      else {format=PKCS;}

      if( BIO_seek(bio, position) < 0 ) {
        LogError();
        throw CredentialError("Can't reset the BIO");
      }
    }
    else {format = PEM;}
    return format;
  }


  void Credential::loadCertificate(BIO* &certbio, X509* &cert, STACK_OF(X509) **certchain) {
    //Parse the certificate
    Credformat format;
    int n=0;
    
    X509* x509 = NULL;
    PKCS12* pkcs12 = NULL;
    format = getFormat(certbio);
    credentialLogger.msg(INFO,"Certificate format for BIO is: %d", format);

    if(*certchain) { sk_X509_pop_free(*certchain, X509_free); }
 
    switch(format) {
      case PEM:
        //Get the certificte, By default, certificate is without passphrase
        //Read certificate
        if(!(x509 = PEM_read_bio_X509(certbio, &cert, NULL, NULL))) {
          credentialLogger.msg(ERROR,"Can not read cert information from BIO"); LogError();
          throw CredentialError("Can not read cert information from BIO");
        }
 
        //Get the issuer chain
        *certchain = sk_X509_new_null();
        while(!BIO_eof(certbio)){
          X509 * tmp = NULL;
          if(!(PEM_read_bio_X509(certbio, &tmp, NULL, NULL))){
            ERR_clear_error();
            break;
          }
          
          if(!sk_X509_insert(*certchain, tmp, n)) {
            X509_free(tmp);
            std::string str(X509_NAME_oneline(X509_get_subject_name(tmp),0,0));
            credentialLogger.msg(ERROR, "Can not insert cert %s into certificate's issuer chain", str); LogError();
            throw CredentialError("Can not insert cert into certificate's issuer chain");
          }
          ++n;
        }
        break;

      case DER:
        cert=d2i_X509_bio(certbio,NULL);
        if(!cert){
          credentialLogger.msg(ERROR,"Unable to read DER credential from BIO"); LogError();
          throw CredentialError("Unable to read DER credential from BIO");
        }
        break;

      case PKCS:
        STACK_OF(X509)* pkcs12_certs = NULL;
        pkcs12 = d2i_PKCS12_bio(certbio, NULL);
        if(pkcs12){
          char password[100];
          EVP_read_pw_string(password, 100, "Enter Password for PKCS12 certificate:", 0);
          if(!PKCS12_parse(pkcs12, password, NULL, &cert, &pkcs12_certs)) {
            credentialLogger.msg(ERROR,"Can not parse PKCS12 file"); LogError();
            throw CredentialError("Can not parse PKCS12 file");
          }
        }
        else{
          credentialLogger.msg(ERROR,"Can not read PKCS12 credential from BIO"); LogError();
          throw CredentialError("Can not read PKCS12 credential file");
        }

        if (pkcs12_certs && sk_num(pkcs12_certs)){
          X509* tmp;
          for (n = 0; n < sk_X509_num(pkcs12_certs); n++) {
            tmp = X509_dup(sk_X509_value(pkcs12_certs, n));
            sk_X509_insert(*certchain, tmp, n);
          }
        }

        if(pkcs12) { PKCS12_free(pkcs12); }
        if(pkcs12_certs) { sk_X509_pop_free(pkcs12_certs, X509_free); }

        break;
     } // end switch

  }

  void Credential::loadKey(BIO* &keybio, EVP_PKEY* &pkey) {
    //Read key
    Credformat format;
    format = getFormat(keybio);
    switch(format){
      case PEM:
        if(!(PEM_read_bio_PrivateKey(keybio, &pkey, passwordcb, NULL))) {  
          credentialLogger.msg(ERROR,"Can not read credential key from PEM key BIO"); LogError();
          throw CredentialError("Can not read credential key");
        }
        break;

      case DER:
        pkey=d2i_PrivateKey_bio(keybio, NULL);
        break;

      case PKCS: 
        PKCS12* pkcs12 = d2i_PKCS12_bio(keybio, NULL);
        if(pkcs12) {
          char password[100];
          EVP_read_pw_string(password, 100, "Enter Password for PKCS12 certificate:", 0);
          if(!PKCS12_parse(pkcs12, password, &pkey, NULL, NULL)) {
            credentialLogger.msg(ERROR,"Can not parse PKCS12 file"); LogError();
            throw CredentialError("Can not parse PKCS12 file");
          }
          PKCS12_free(pkcs12);
        }
        break;
    }
  }

  static int proxy_init_ = 0;
  void Credential::InitProxyCertInfo(void) {
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


    OBJ_create(IMPERSONATION_PROXY_OID, IMPERSONATION_PROXY_SN, IMPERSONATION_PROXY_LN);

    OBJ_create(INDEPENDENT_PROXY_OID, INDEPENDENT_PROXY_SN, INDEPENDENT_PROXY_LN);

    OBJ_create(LIMITED_PROXY_OID, LIMITED_PROXY_SN, LIMITED_PROXY_LN);
  }

  void Credential::InitVerification(void) {
    verify_ctx_.cert_store = NULL;
    verify_ctx_.cert_depth = 0;
    verify_ctx_.proxy_depth = 0;
    verify_ctx_.max_proxy_depth = -1;
    verify_ctx_.limited_proxy = 0;
    verify_ctx_.cert_type = CERT_TYPE_EEC;  
    verify_ctx_.cert_chain = sk_X509_new_null();
    verify_ctx_.ca_dir = cacertdir_;
    verify_ctx_.ca_file = cacertfile_;
  }

  bool Credential::Verify(void) {
    if(verify_cert_chain(cert_, cert_chain_, &verify_ctx_)) {
      credentialLogger.msg(INFO, "Certificate verify OK");
      return true;
    }
    else {credentialLogger.msg(ERROR, "Certificate verify failed"); LogError(); return false;}
  } 

  Credential::Credential() : start_(Arc::Time()), lifetime_(Arc::Period(0)),
        req_(NULL), rsa_key_(NULL), signing_alg_((EVP_MD*)EVP_md5()), keybits_(1024),
        cert_(NULL), pkey_(NULL), cert_chain_(NULL), proxy_cert_info_(NULL), extensions_(NULL) {

    OpenSSL_add_all_algorithms();

    extensions_ = sk_X509_EXTENSION_new_null();

    //Initiate the proxy certificate constant and  method which is required by openssl
    if(!proxy_init_) InitProxyCertInfo();

  }

  Credential::Credential(Time start, Period lifetime, int keybits, std::string proxyversion, std::string policylang, 
      std::string policyfile, int pathlength) : 
         start_(start), lifetime_(lifetime), 
         req_(NULL), rsa_key_(NULL), signing_alg_((EVP_MD*)EVP_md5()), keybits_(keybits), 
         cert_(NULL), pkey_(NULL), cert_chain_(NULL), proxy_cert_info_(NULL), extensions_(NULL), 
         proxyversion_(proxyversion), policylang_(policylang), policyfile_(policyfile), pathlength_(pathlength) {

    OpenSSL_add_all_algorithms();

    extensions_ = sk_X509_EXTENSION_new_null();

    //Initiate the proxy certificate constant and  method which is required by openssl
    if(!proxy_init_) InitProxyCertInfo();


    //Get certType
    if(proxyversion_.compare("GSI2") == 0 || proxyversion_.compare("gsi2") == 0) { 
      if(policylang_.compare("LIMITED") == 0 || policylang_.compare("limited") == 0) {
        cert_type_ = CERT_TYPE_GSI_2_LIMITED_PROXY;
      }
      else if (policylang_.compare("RESTRICTED") == 0 || policylang_.compare("restricted") == 0) {
        std::cerr<<"Globus legacy proxies can not carry policy data or path length contraints" <<std::endl;
      }
      else {
        cert_type_ = CERT_TYPE_GSI_2_PROXY;
      }
    }
    else if (proxyversion_.compare("RFC") == 0 || proxyversion_.compare("rfc") == 0) {
      if(policylang_.compare("LIMITED") == 0 || policylang_.compare("limited") == 0) {
        cert_type_ = CERT_TYPE_RFC_LIMITED_PROXY;
      }
      else if(policylang_.compare("RESTRICTED") == 0 || policylang_.compare("restricted") == 0) {
        cert_type_ = CERT_TYPE_RFC_RESTRICTED_PROXY;
      }
      else if(policylang_.compare("INDEPENDENT") == 0 || policylang_.compare("independent") == 0) {
        cert_type_ = CERT_TYPE_RFC_INDEPENDENT_PROXY;
      }
      else {
        cert_type_ = CERT_TYPE_RFC_IMPERSONATION_PROXY;
      }
    }
    else {
      if(policylang_.compare("LIMITED") == 0 || policylang_.compare("limited") == 0) {
        cert_type_ = CERT_TYPE_GSI_3_LIMITED_PROXY;
      }
      else if(policylang_.compare("RESTRICTED") == 0 || policylang_.compare("restricted") == 0) {
        cert_type_ = CERT_TYPE_GSI_3_RESTRICTED_PROXY;
      }
      else if(policylang_.compare("INDEPENDENT") == 0 || policylang_.compare("independent") == 0) {
        cert_type_ = CERT_TYPE_GSI_3_INDEPENDENT_PROXY;
      }
      else {
        cert_type_ = CERT_TYPE_GSI_3_IMPERSONATION_PROXY;
      }
    }

    if(!policyfile_.empty() && policylang_.empty()) {
      std::cerr<<"If you specify a policy file you also need to specify a policy language.\n" <<std::endl;
      exit(1);
    }


    proxy_cert_info_ = PROXYCERTINFO_new();

    PROXYPOLICY *   policy;
    ASN1_OBJECT *   policy_object;

    //set policy language, see definiton in: http://dev.globus.org/wiki/Security/ProxyCertTypes
    switch(cert_type_)
    {
      case CERT_TYPE_GSI_3_IMPERSONATION_PROXY:
      case CERT_TYPE_RFC_IMPERSONATION_PROXY:
        policy = PROXYCERTINFO_get_proxypolicy(proxy_cert_info_);
        if((policy_object = OBJ_nid2obj(OBJ_sn2nid(IMPERSONATION_PROXY_SN))) != NULL) {
          PROXYPOLICY_set_policy_language(policy, policy_object); 
          PROXYPOLICY_set_policy(policy, NULL, 0);
        }
        break;

      case CERT_TYPE_GSI_3_INDEPENDENT_PROXY:
      case CERT_TYPE_RFC_INDEPENDENT_PROXY:
        policy = PROXYCERTINFO_get_proxypolicy(proxy_cert_info_);
        if((policy_object = OBJ_nid2obj(OBJ_sn2nid(INDEPENDENT_PROXY_SN))) != NULL) {
          PROXYPOLICY_set_policy_language(policy, policy_object);
          PROXYPOLICY_set_policy(policy, NULL, 0);
        }
        break;

      case CERT_TYPE_GSI_3_LIMITED_PROXY:
      case CERT_TYPE_RFC_LIMITED_PROXY:
        policy = PROXYCERTINFO_get_proxypolicy(proxy_cert_info_);
        if((policy_object = OBJ_nid2obj(OBJ_sn2nid(LIMITED_PROXY_SN))) != NULL) {
          PROXYPOLICY_set_policy_language(policy, policy_object);
          PROXYPOLICY_set_policy(policy, NULL, 0);
        }
        break;
      default:
        break;
    }

    //set path length constraint
    if(pathlength >= 0)
    PROXYCERTINFO_set_path_length(proxy_cert_info_, pathlength_);

    //set policy
    std::string policystring;
    std::ifstream fp;
    if(!policyfile_.empty()) {
      fp.open(policyfile_.c_str());
      if(!fp) {
        std::cerr << "Error: can't open policy file" << std::endl;
        exit(1);
      }
      fp.unsetf(std::ios::skipws);
      char c;
      while(fp.get(c))
        policystring += c;
    }
   
    if(policylang_ == "LIMITED" || policylang_ == "limited") policylang_ = LIMITED_PROXY_OID;
    else if(policylang_ == "INDEPENDENT" || policylang_ == "independent") policylang_ = INDEPENDENT_PROXY_OID;
    else if(policylang_ == "IMPERSONATION" || policylang_ == "impersonation") policylang_ = IMPERSONATION_PROXY_OID;
    else if(policylang_.empty()) {
      if(policyfile_.empty()) policylang_ = IMPERSONATION_PROXY_OID;
      else policylang_ = GLOBUS_GSI_PROXY_GENERIC_POLICY_OID;
    } 

    /**here the definition about policy languange is redundant with above definition, but it could cover "restricted" language
    *which has no explicit definition according to http://dev.globus.org/wiki/Security/ProxyCertTypes 
    */
    OBJ_create((char *)policylang_.c_str(), (char *)policylang_.c_str(), (char *)policylang_.c_str());

    policy_object = OBJ_nid2obj(OBJ_sn2nid(policylang_.c_str()));

    policy = PROXYCERTINFO_get_proxypolicy(proxy_cert_info_);
    //Here only consider the situation when there is policy file
    if(policystring.size() > 0) { 
      PROXYPOLICY_set_policy_language(policy, policy_object);
      PROXYPOLICY_set_policy(policy, (unsigned char*)policystring.c_str(), policystring.size());
    }

    //set the version of PROXYCERTINFO
    if(proxyversion_ == "RFC" || proxyversion_ == "rfc")
      PROXYCERTINFO_set_version(proxy_cert_info_, 4);
    else 
      PROXYCERTINFO_set_version(proxy_cert_info_, 3);

  }

  Credential::Credential(const std::string& certfile, const std::string& keyfile, const std::string& cadir, 
        const std::string& cafile) : cert_(NULL), pkey_(NULL), cert_chain_(NULL), proxy_cert_info_(NULL), extensions_(NULL),
        certfile_(certfile), keyfile_(keyfile), cacertfile_(cafile), cacertdir_(cadir),
        req_(NULL), rsa_key_(NULL), signing_alg_((EVP_MD*)EVP_md5()), keybits_(1024) {

     OpenSSL_add_all_algorithms();

     extensions_ = sk_X509_EXTENSION_new_null();

    BIO* certbio = NULL, *keybio = NULL; 
    Credformat format;

    certbio = BIO_new_file(certfile.c_str(), "r"); 
    if(!certbio){
      credentialLogger.msg(ERROR,"Can not read certificate file: %s", certfile); LogError();
      throw CredentialError("Can not read certificate file");
    }
    keybio = BIO_new_file(keyfile.c_str(), "r");
    if(!keybio){
      credentialLogger.msg(ERROR,"Can not read key file: %s", keyfile);  LogError();
      throw CredentialError("Can not read key file");
    }

    loadCertificate(certbio, cert_, &cert_chain_);

    loadKey(keybio, pkey_);

    //load CA
    
    //Get the lifetime of the credential
    getLifetime(cert_chain_, cert_, start_, lifetime_);

    //Initiate the proxy certificate constant and  method which is required by openssl
    if(!proxy_init_) InitProxyCertInfo();

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


  X509_EXTENSION* Credential::CreateExtension(std::string& name, std::string& data, bool crit) {
    X509_EXTENSION*   ext = NULL;
    ASN1_OBJECT*      ext_obj = NULL;
    ASN1_OCTET_STRING*  ext_oct = NULL;

    if(!(ext_obj = OBJ_nid2obj(OBJ_txt2nid((char *)(name.c_str()))))) {
      credentialLogger.msg(ERROR, "Can not convert string into ASN1_OBJECT");
      LogError(); return NULL;
    }
  
    ext_oct = ASN1_OCTET_STRING_new();
  
    ext_oct->data = (unsigned char*) malloc(data.size());
    memcpy(ext_oct->data, data.c_str(), data.size());
    ext_oct->length = data.size();
  
    if (!(ext = X509_EXTENSION_create_by_OBJ(NULL, ext_obj, crit, ext_oct))) {
      credentialLogger.msg(ERROR, "Can not create extension for proxy certificate");
      LogError();
      if(ext_oct) ASN1_OCTET_STRING_free(ext_oct);
      if(ext_obj) ASN1_OBJECT_free(ext_obj);
      return NULL;
    }
    
    ext_oct = NULL;
    return ext;
  }  

  //TODO: self request/sign proxy

  bool Credential::GenerateRequest(BIO* &reqbio){
    bool res = false;
    X509_NAME *         req_name = NULL;
    X509_NAME_ENTRY *   req_name_entry = NULL;
    RSA *               rsa_key = NULL;
    int keybits = keybits_;
    const EVP_MD *digest = signing_alg_;
    EVP_PKEY* pkey;
  
    if(pkey_) { credentialLogger.msg(ERROR, "The credential's private key has already been initialized"); return false; }; 
     
#ifdef HAVE_OPENSSL_OLDRSA
    unsigned long prime = RSA_F4; 
    rsa_key = RSA_generate_key(keybits, prime, keygen_cb, NULL);
    if(!rsa_key) { 
      credentialLogger.msg(ERROR, "RSA_generate_key failed"); 
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
      if(BN_set_word(prime,RSA_F4)) {
        if(!RSA_generate_key_ex(rsa_key, keybits, prime, &cb)) { 
          credentialLogger.msg(ERROR, "RSA_generate_key_ex failed"); 
          LogError();
          if(prime) BN_free(prime);
          return false;
        }
      }      
      else{ 
        credentialLogger.msg(ERROR, "BN_set_word failed"); 
        LogError(); 
        if(prime) BN_free(prime); 
        if(rsa_key) RSA_free(rsa_key); 
        return false; }
    } 
    else { 
      credentialLogger.msg(ERROR, "BN_new || RSA_new failed"); 
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
                  credentialLogger.msg(ERROR, "Can not duplicate the subject name for the self-signing proxy certificate request");
                  LogError(); res = false; goto err;
                }
              }
              else { name = X509_NAME_new();}
              if((entry = X509_NAME_ENTRY_create_by_NID(NULL, NID_commonName, V_ASN1_APP_CHOOSE,
                          (unsigned char *) "NULL SUBJECT NAME ENTRY", -1)) == NULL) {
                credentialLogger.msg(ERROR, "Can not create a new X509_NAME_ENTRY for the proxy certificate request");
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
              if (CERT_IS_RFC_PROXY(cert_type_)) certinfo_sn = "PROXYCERTINFO_V4";
              else certinfo_sn = "PROXYCERTINFO_V3";
              //if(proxy_cert_info_->version == 3)         

              if(!(certinfo_sn.empty())) {
                X509V3_EXT_METHOD*  ext_method;
                unsigned char* data = NULL;
                int length;
                ext_method = X509V3_EXT_get_nid(OBJ_sn2nid(certinfo_sn.c_str()));
 std::cout<<"certinfo "<<certinfo_sn<<std::endl;
                length = ext_method->i2d(proxy_cert_info_, NULL);
 std::cout<<"length "<<length<<std::endl;
                if(length < 0) { 
                  credentialLogger.msg(ERROR, "Can not convert PROXYCERTINFO struct from internal to DER encoded format"); 
                  LogError(); 
                }
                else { data = (unsigned char*) malloc(length); }

                unsigned char* derdata; 
                derdata = data;
                length = ext_method->i2d(proxy_cert_info_,  &derdata);

                if(length < 0) { 
                  credentialLogger.msg(ERROR, "Can not convert PROXYCERTINFO struct from internal to DER encoded format"); 
                  free(data); data = NULL; LogError(); 
                }
                if(data) {
                  std::string ext_data((char*)data, length); free(data);
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
                  if(!(PEM_write_bio_X509_REQ(reqbio,req))){ 
                    credentialLogger.msg(ERROR, "PEM_write_bio_X509_REQ failed"); 
                    LogError(); res = false;
                  }
                  //if(!(i2d_X509_REQ_bio(reqbio,req))){
                  //  credentialLogger.msg(ERROR, "Can't convert X509 request from internal to DER encoded format");
                  //  LogError(); res = false;
                  //}
                  else { rsa_key_ = rsa_key; rsa_key = NULL; pkey_ = pkey; pkey = NULL; res = true; }
                  //TODO
                }
              }
            }
            X509_REQ_free(req);
          }
          else { credentialLogger.msg(ERROR, "Can not generate X509 request"); LogError(); res = false; }
        }
        else { credentialLogger.msg(ERROR, "Can not set private key"); LogError(); res = false;}
      }
    }

err:
    if(pkey) EVP_PKEY_free(pkey);
    if(rsa_key) RSA_free(rsa_key);

    return res;
  }

  bool Credential::GenerateRequest(std::string &content) {
    BIO *out = BIO_new(BIO_s_mem());
    if(!out) { credentialLogger.msg(ERROR, "Can not create BIO for request"); LogError(); return false;}
 
    if(GenerateRequest(out)) {
      for(;;) {
        char s[256];
        int l = BIO_read(out,s,sizeof(s));
        if(l <= 0) break;
        content.append(s,l);
      }
    }
    
    BIO_free_all(out);
    return true;
  }

  bool Credential::GenerateRequest(const char* filename) {
    BIO *out = BIO_new(BIO_s_file());
    if(!out) { credentialLogger.msg(ERROR, "Can not create BIO for request"); LogError(); return false;}
    if (!(BIO_write_filename(out, (char*)filename))) {
      credentialLogger.msg(ERROR, "Can not set writable file for request BIO"); LogError(); BIO_free_all(out); return false;
    }

    if(GenerateRequest(out)) {
      credentialLogger.msg(INFO, "Wrote request into a file");
    }
    else {credentialLogger.msg(ERROR, "Failed to write request into a file"); BIO_free_all(out); return false;}
 
    BIO_free_all(out);
    return true;
  }

  //Inquire the input request bio to get PROXYCERTINFO, certType
  bool Credential::InquireRequest(BIO* &reqbio){
    bool res = false;
    if(reqbio == NULL) { credentialLogger.msg(ERROR, "NULL BIO passed to InquireRequest"); return false; }
    if(req_) {X509_REQ_free(req_); req_ = NULL; }
    if(!(PEM_read_bio_X509_REQ(reqbio, &req_, NULL, NULL))) {  
      credentialLogger.msg(ERROR, "PEM_read_bio_X509_REQ failed"); 
      LogError(); return false; 
    }

    //if(!(d2i_X509_REQ_bio(reqbio, &req_))) {
    //  credentialLogger.msg(ERROR, "Can't convert X509_REQ struct from DER encoded to internal format");
    //  LogError(); return false;
    //}

    STACK_OF(X509_EXTENSION)* req_extensions = NULL;
    X509_EXTENSION* ext;
    PROXYPOLICY*  policy = NULL;
    ASN1_OBJECT*  policy_lang = NULL;
    ASN1_OBJECT*  extension_oid = NULL;
    int certinfo_NID, certinfo_old_NID, certinfo_v3_NID, certinfo_v4_NID, nid;
    int i;

    //Get the PROXYCERTINFO from request' extension
    req_extensions = X509_REQ_get_extensions(req_);
    certinfo_v3_NID = OBJ_sn2nid("PROXYCERTINFO_V3");
    certinfo_v4_NID = OBJ_sn2nid("PROXYCERTINFO_V4");
    for(i=0;i<sk_X509_EXTENSION_num(req_extensions);i++) {
      ext = sk_X509_EXTENSION_value(req_extensions,i);
      extension_oid = X509_EXTENSION_get_object(ext);
      nid = OBJ_obj2nid(extension_oid);
      if(nid == certinfo_v3_NID || nid == certinfo_v4_NID) {
        if(proxy_cert_info_) {
          PROXYCERTINFO_free(proxy_cert_info_);
          proxy_cert_info_ = NULL;
        }   

        if((proxy_cert_info_ = (PROXYCERTINFO*)X509V3_EXT_d2i(ext)) == NULL) {
           credentialLogger.msg(ERROR, "Can not convert DER encoded PROXYCERTINFO extension to internal format"); 
           LogError(); goto err;
        }
        break;
      }
    }
    
    if(proxy_cert_info_) {
      if((policy = PROXYCERTINFO_get_proxypolicy(proxy_cert_info_)) == NULL) {
        credentialLogger.msg(ERROR, "Can not get policy from PROXYCERTINFO extension"); 
        LogError(); goto err;
      }    
      if((policy_lang = PROXYPOLICY_get_policy_language(policy)) == NULL) {
        credentialLogger.msg(ERROR, "Can not get policy language from PROXYCERTINFO extension");                                   
        LogError(); goto err;
      }    
      int policy_nid = OBJ_obj2nid(policy_lang);
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

  bool Credential::InquireRequest(std::string &content) {
    BIO *in;
    if(!(in = BIO_new_mem_buf((void*)(content.c_str()), content.length()))) { 
      credentialLogger.msg(ERROR, "Can not create BIO for parsing request"); 
      LogError(); return false;
    }

    if(InquireRequest(in)) {
      credentialLogger.msg(INFO, "Read request from a string");
    }
    else {credentialLogger.msg(ERROR, "Failed to read request from a string"); BIO_free_all(in); return false;}

    BIO_free_all(in);
    return true;
  }

  bool Credential::InquireRequest(const char* filename) {
    BIO *in = BIO_new(BIO_s_file());
    if(!in) { credentialLogger.msg(ERROR, "Can not create BIO for parsing request"); LogError(); return false;}
    if (!(BIO_read_filename(in, (char*)filename))) {
      credentialLogger.msg(ERROR, "Can not set readable file for request BIO"); 
      LogError(); BIO_free_all(in); return false;
    }

    if(InquireRequest(in)) {
      credentialLogger.msg(INFO, "Read request from a file");
    }
    else {credentialLogger.msg(ERROR, "Failed to read request from a file"); BIO_free_all(in); return false;}

    BIO_free_all(in);
    return true;
  }

  bool Credential::SetProxyPeriod(X509* tosign, X509* issuer, Time& start, Period& lifetime) {
    ASN1_UTCTIME  *notBefore = NULL, *notAfter = NULL;
    time_t t1 = start.GetTime();
    Time tmp = start + lifetime;
    time_t t2 = tmp.GetTime();

    //Set "notBefore"
    if( X509_cmp_time(X509_get_notBefore(issuer), &t1) < 0) {
      notBefore = M_ASN1_UTCTIME_new();
      if(notBefore == NULL) { 
        credentialLogger.msg(ERROR, "Failed to create new ASN1_UTCTIME for expiration time of proxy certificate"); 
        LogError(); return false;
      }
      X509_time_adj(notBefore, 0, &t1);
    }
    else {
      if((notBefore = M_ASN1_UTCTIME_dup(X509_get_notBefore(issuer))) == NULL) {
        credentialLogger.msg(ERROR, "Failed to duplicate ASN1_UTCTIME for expiration time of proxy certificate");
        LogError(); return false;
      }
    }

    if(!X509_set_notBefore(tosign, notBefore)) {
      credentialLogger.msg(ERROR, "Failed to set notBefore for proxy certificate");
      LogError(); ASN1_UTCTIME_free(notBefore); return false;
    }

    //Set "not After"
    if( X509_cmp_time(X509_get_notAfter(issuer), &t2) > 0) {
      notAfter = M_ASN1_UTCTIME_new();
      if(notAfter == NULL) { 
        credentialLogger.msg(ERROR, "Failed to create new ASN1_UTCTIME for expiration time of proxy certificate");
        LogError(); ASN1_UTCTIME_free(notBefore); return false;
      }
      X509_time_adj(notAfter, 0, &t2);
    }
    else {
      if((notAfter = M_ASN1_UTCTIME_dup(X509_get_notAfter(issuer))) == NULL) {
        credentialLogger.msg(ERROR, "Failed to duplicate ASN1_UTCTIME for expiration time of proxy certificate");
        LogError(); ASN1_UTCTIME_free(notBefore); return false;
      }
    }

    if(!X509_set_notAfter(tosign, notAfter)) {
      credentialLogger.msg(ERROR, "Failed to set notBefore for proxy certificate");
      LogError(); ASN1_UTCTIME_free(notBefore);  ASN1_UTCTIME_free(notAfter); return false;
    }

    ASN1_UTCTIME_free(notBefore); 
    ASN1_UTCTIME_free(notAfter);

    return true; 
  } 

  EVP_PKEY* Credential::GetPrivKey(void){
    EVP_PKEY* key = NULL;
    BIO*  bio = NULL;
    int length;
    bio = BIO_new(BIO_s_mem());
    length = i2d_PrivateKey_bio(bio, pkey_);
    if(pkey_ == NULL) {credentialLogger.msg(ERROR, "Private key of the credential object is NULL"); BIO_free(bio); return NULL;}
    if(length <= 0) {
      credentialLogger.msg(ERROR, "Can not convert private key to DER format");
      LogError(); BIO_free(bio); return NULL;
    }
    key = d2i_PrivateKey_bio(bio, &key);
    BIO_free(bio);

    return key;
  }

  X509* Credential::GetCert(void) {
    X509* cert = NULL;
    cert = X509_dup(cert_);
    return cert;
  }

  STACK_OF(X509)* Credential::GetCertChain(void) {
    STACK_OF(X509)* chain = NULL;
    chain = sk_X509_dup(cert_chain_);
    return chain;
  }

  bool Credential::AddExtension(std::string name, std::string data, bool crit) {
    X509_EXTENSION* ext = NULL;
    ext = CreateExtension(name, data, crit);
    if(ext && sk_X509_EXTENSION_push(extensions_, ext)) return true;
    else return false;
  }

  bool Credential::AddExtension(std::string name, char** binary, bool crit) {
    X509_EXTENSION* ext = NULL;
    ext = X509V3_EXT_conf_nid(NULL, NULL, OBJ_txt2nid((char*)(name.c_str())), (char*)binary);
    if(ext && sk_X509_EXTENSION_push(extensions_, ext)) return true;
    else return false;
  }

  bool Credential::SignRequestAssistant(Credential* &proxy, EVP_PKEY* &req_pubkey, X509** tosign){
   
    bool res = false; 
    X509* issuer = NULL;

    X509_NAME* subject_name = NULL;
    X509_NAME_ENTRY* name_entry = NULL;

    *tosign = NULL;
    char* CN_name;
    ASN1_INTEGER* serial_number = NULL;
    unsigned char* certinfo_data = NULL;    
    X509_EXTENSION* certinfo_ext = NULL;
    X509_EXTENSION* ext = NULL;
    int certinfo_NID = NID_undef;

    issuer = X509_dup(cert_);
    if((*tosign = X509_new()) == NULL) {
      credentialLogger.msg(ERROR, "Failed to initialize X509 structure");
      LogError(); goto err;
    }

    //TODO: VOMS 
    if(CERT_IS_GSI_3_PROXY(proxy->cert_type_)) { certinfo_NID = OBJ_sn2nid("PROXYCERTINFO_V3"); }
    else if(CERT_IS_RFC_PROXY(proxy->cert_type_)) { certinfo_NID = OBJ_sn2nid("PROXYCERTINFO_V4"); }
    if(certinfo_NID != NID_undef) {
      unsigned char   md[SHA_DIGEST_LENGTH];
      long  sub_hash;
      unsigned int   len;
      X509V3_EXT_METHOD* ext_method;

      ext_method = X509V3_EXT_get_nid(certinfo_NID);
      ASN1_digest((int (*)(void*, unsigned char**))i2d_PUBKEY, EVP_sha1(), (char*)req_pubkey,md,&len);
      sub_hash = md[0] + (md[1] + (md[2] + (md[3] >> 1) * 256) * 256) * 256; 
        
      CN_name = (char*)malloc(sizeof(long)*4 + 1);
      sprintf(CN_name, "%ld", sub_hash);        
     
      serial_number = ASN1_INTEGER_new();
      ASN1_INTEGER_set(serial_number, sub_hash);
        
      int length = ext_method->i2d(proxy->proxy_cert_info_, NULL);
      if(length < 0) {
        credentialLogger.msg(ERROR, "Can not convert PROXYCERTINFO struct from internal to DER encoded format"); LogError();
      }
      else {certinfo_data = (unsigned char*)malloc(length); }

      unsigned char* derdata; derdata = certinfo_data;
      length = ext_method->i2d(proxy->proxy_cert_info_, &derdata);
      if(length < 0) {
        credentialLogger.msg(ERROR, "Can not convert PROXYCERTINFO struct from internal to DER encoded format"); 
        free(certinfo_data); certinfo_data = NULL; LogError();
      }
      if(certinfo_data) {
        std::string certinfo_string((char*)certinfo_data, length); free(certinfo_data); certinfo_data = NULL;
        std::string NID_sn = OBJ_nid2sn(certinfo_NID);
        certinfo_ext = CreateExtension(NID_sn, certinfo_string, 1);
      }
      if(certinfo_ext != NULL) {
        //if(!X509_add_ext(*tosign, certinfo_ext, 0)) {
        if(!sk_X509_EXTENSION_push(proxy->extensions_, certinfo_ext)) {
          credentialLogger.msg(ERROR, "Can not add X509 extension to proxy cert"); LogError(); goto err; 
        }
      }
    }
    else if(proxy->cert_type_ == CERT_TYPE_GSI_2_LIMITED_PROXY){ 
      CN_name = "limited proxy"; 
      serial_number = X509_get_serialNumber(issuer);
    }
    else {
      CN_name = "proxy";
      serial_number = X509_get_serialNumber(issuer);
    }

    int position;

    /* Add any keyUsage and extendedKeyUsage extensions present in the issuer cert */
    if((position = X509_get_ext_by_NID(issuer, NID_key_usage, -1)) > -1) {
      ASN1_BIT_STRING*   usage;
      unsigned char *    ku_data = NULL;
      int ku_length;
  
      if(!(ext = X509_get_ext(issuer, position))) {
        credentialLogger.msg(ERROR, "Can not get extension from issuer certificate");
        LogError(); goto err;  
      }

      if(!(usage = (ASN1_BIT_STRING*)X509_get_ext_d2i(issuer, NID_key_usage, NULL, NULL))) {
        credentialLogger.msg(ERROR, "Can not convert keyUsage struct from DER encoded format");
        LogError(); goto err;
      }

      /* clear bits specified in draft */
      ASN1_BIT_STRING_set_bit(usage, 1, 0); /* Non Repudiation */
      ASN1_BIT_STRING_set_bit(usage, 5, 0); /* Certificate Sign */
        
      ku_length = i2d_ASN1_BIT_STRING(usage, NULL);
      if(ku_length < 0) {
        credentialLogger.msg(ERROR, "Can not convert keyUsage struct from internal to DER format");
        LogError(); ASN1_BIT_STRING_free(usage); goto err;
      }
      ku_data = (unsigned char*) malloc(ku_length);  
      
      unsigned char* derdata; derdata = ku_data;
      ku_length = i2d_ASN1_BIT_STRING(usage, &derdata);
      if(ku_length < 0) {
        credentialLogger.msg(ERROR, "Can not convert keyUsage struct from internal to DER format");
        LogError(); ASN1_BIT_STRING_free(usage); free(ku_data); goto err;
      }
      ASN1_BIT_STRING_free(usage);        
      if(ku_data) {
        std::string ku_string((char*)ku_data, ku_length); free(ku_data); ku_data = NULL;
        std::string name = "keyUsage";
        ext = CreateExtension(name, ku_string, 1);
      }
      if(ext != NULL) {
        //if(!X509_add_ext(*tosign, ext, 0)) {
        if(!sk_X509_EXTENSION_push(proxy->extensions_, ext)) {
          credentialLogger.msg(ERROR, "Can not add X509 extension to proxy cert"); LogError();
          X509_EXTENSION_free(ext); ext = NULL; goto err;
        }
      }
      X509_EXTENSION_free(ext); ext = NULL;
    }

    if((position = X509_get_ext_by_NID(issuer, NID_ext_key_usage, -1)) > -1) {
      if(!(ext = X509_get_ext(issuer, position))) {
        credentialLogger.msg(ERROR, "Can not get extended KeyUsage extension from issuer certificate"); 
        LogError(); goto err;
      }

      ext = X509_EXTENSION_dup(ext);
      if(ext == NULL) { credentialLogger.msg(ERROR, "Can not copy extended KeyUsage extension"); LogError(); goto err; }

      //if(!X509_add_ext(*tosign, ext, 0)) {
      if(!sk_X509_EXTENSION_push(proxy->extensions_, certinfo_ext)) {
        credentialLogger.msg(ERROR, "Can not add X509 extended KeyUsage extension to new proxy certificate"); LogError();
        X509_EXTENSION_free(ext); ext = NULL; goto err;
      }
      //X509_EXTENSION_free(ext); ext = NULL;
    }
    
    /* Create proxy subject name */
    if((subject_name = X509_NAME_dup(X509_get_subject_name(issuer))) == NULL) {
      credentialLogger.msg(ERROR, "Can not copy the subject name from issuer for proxy certificate"); goto err;
    }
    if((name_entry = X509_NAME_ENTRY_create_by_NID(&name_entry, NID_commonName, V_ASN1_APP_CHOOSE,
                                     (unsigned char *) CN_name, -1)) == NULL) {
      credentialLogger.msg(ERROR, "Can not create name entry CN for proxy certificate"); 
      LogError(); X509_NAME_free(subject_name); goto err;
    }
    if(!X509_NAME_add_entry(subject_name, name_entry, X509_NAME_entry_count(subject_name), 0) ||
       !X509_set_subject_name(*tosign, subject_name)) {
      credentialLogger.msg(ERROR, "Can not set CN in proxy certificate"); 
      LogError(); X509_NAME_ENTRY_free(name_entry); goto err;
    }
    X509_NAME_free(subject_name);
    X509_NAME_ENTRY_free(name_entry);

    if(!X509_set_issuer_name(*tosign, X509_get_subject_name(issuer))) {
      credentialLogger.msg(ERROR, "Can not set issuer's subject for proxy certificate"); LogError(); goto err;
    }

    if(!X509_set_version(*tosign, 2)) {
      credentialLogger.msg(ERROR, "Can not set version number for proxy certificate"); LogError(); goto err;
    }

    if(!X509_set_serialNumber(*tosign, serial_number)) {
      credentialLogger.msg(ERROR, "Can not set serial number for proxy certificate"); LogError(); goto err;
    }

    if(!SetProxyPeriod(*tosign, issuer, start_, lifetime_)) {
      credentialLogger.msg(ERROR, "Can not set the lifetime for proxy certificate"); goto err;  
    }
    
    if(!X509_set_pubkey(*tosign, req_pubkey)) {
      credentialLogger.msg(ERROR, "Can not set pubkey for proxy certificate"); LogError(); goto err;
    }
 
    res = true;
     
err:
  if(issuer) { X509_free(issuer); }
    if(res == false && *tosign) { X509_free(*tosign); *tosign = NULL;}
    if(certinfo_NID != NID_undef) {
      if(serial_number) { ASN1_INTEGER_free(serial_number);}
      if(CN_name) { free(CN_name); }
    }
    
    return res;
  }

  bool Credential::SignRequest(Credential* proxy, BIO* outputbio){
    bool res = false;
    if(proxy == NULL) {credentialLogger.msg(ERROR, "The credential to be signed is NULL"); return false; }
    if(outputbio == NULL) { credentialLogger.msg(ERROR, "The BIO for output is NULL"); return false; }
    
    EVP_PKEY* issuer_priv = NULL;
    X509*  proxy_cert = NULL;
    X509_CINF*  proxy_cert_info = NULL;
    X509_EXTENSION* ext = NULL;
    EVP_PKEY* req_pubkey = NULL;
    req_pubkey = X509_REQ_get_pubkey(proxy->req_);
    if(!req_pubkey) { credentialLogger.msg(ERROR, "Error when extracting public key from request"); LogError(); return false;}

    if(!X509_REQ_verify(proxy->req_, req_pubkey)){
      credentialLogger.msg(ERROR,"Failed to verify the request"); LogError(); goto err;
    }

    if(!SignRequestAssistant(proxy, req_pubkey, &proxy_cert)) {
      credentialLogger.msg(ERROR,"Failed to add issuer's extension into proxy"); LogError(); goto err;
    }

    /*Add the extensions which has just been added by application, into the proxy_cert which will be signed soon
     *Note here we suppose it is the signer who will add the extension to to-signed proxy and then sign it;
     * it also could be the request who add the extension and put it inside X509 request' extension, but here the situation
     * has not been considered for now
     */
    proxy_cert_info = proxy_cert->cert_info;
    if (proxy_cert_info->extensions != NULL) {
      sk_X509_EXTENSION_pop_free(proxy_cert_info->extensions, X509_EXTENSION_free);
    }

    if(sk_X509_EXTENSION_num(proxy->extensions_)) {
      proxy_cert_info->extensions = sk_X509_EXTENSION_new_null();
    }    

    std::cout<<"Number of extension, proxy object: "<<sk_X509_EXTENSION_num(proxy->extensions_)<<std::endl;

    for (int i=0; i<sk_X509_EXTENSION_num(proxy->extensions_); i++) {
      ext = X509_EXTENSION_dup(sk_X509_EXTENSION_value(proxy->extensions_, i));
      if (ext == NULL) {
        credentialLogger.msg(ERROR,"Failed to duplicate extension"); LogError(); goto err;
      }
         
      if (!sk_X509_EXTENSION_push(proxy_cert_info->extensions, ext)) {
        credentialLogger.msg(ERROR,"Failed to add extension into proxy"); LogError(); goto err;
      }
    }

    /* Now sign the new certificate */
    if(!(issuer_priv = GetPrivKey())) {
      credentialLogger.msg(ERROR, "Can not get the issuer's private key"); goto err;
    }

    /* Check whether MD5 isn't requested as the signing algorithm in the request*/
    if(EVP_MD_type(proxy->signing_alg_) != NID_md5) {
      credentialLogger.msg(ERROR, "The signing algorithm %s is not allowed, it should be MD5 to sign certificate requests",
      OBJ_nid2sn(EVP_MD_type(proxy->signing_alg_)));
      goto err;
    }

    if(!X509_sign(proxy_cert, issuer_priv, proxy->signing_alg_)) {
      credentialLogger.msg(ERROR, "Failed to sign the request"); LogError(); goto err;
    }

    std::cout<<"Number of extension, proxy object: "<<sk_X509_EXTENSION_num(proxy->extensions_)<<std::endl;
    std::cout<<"Number of extension: "<<sk_X509_EXTENSION_num(proxy_cert->cert_info->extensions)<<std::endl;

    /*Output the signed certificate into BIO*/
    //if(i2d_X509_bio(outputbio, proxy_cert)) {
    if(PEM_write_bio_X509(outputbio, proxy_cert)) {
      credentialLogger.msg(INFO, "Succeeded to sign and output the proxy certificate"); res = true;
    }
    else { credentialLogger.msg(ERROR, "Can not convert signed proxy cert into DER format"); LogError();}
   
err:
    if(issuer_priv) { EVP_PKEY_free(issuer_priv);}
    if(proxy_cert) { X509_free(proxy_cert);}
    if(req_pubkey) { EVP_PKEY_free(req_pubkey); }

    return res;
  }

  bool Credential::SignRequest(Credential* proxy, std::string &content) {
    BIO *out = BIO_new(BIO_s_mem());
    if(!out) { credentialLogger.msg(ERROR, "Can not create BIO for signed proxy certificate"); LogError(); return false;}
              
    if(SignRequest(proxy, out)) {
      for(;;) { 
        char s[256];
        int l = BIO_read(out,s,sizeof(s));
        if(l <= 0) break;
        content.append(s,l);
      }       
    }           
                  
    BIO_free_all(out);
    return true;    
  }               
                  
  bool Credential::SignRequest(Credential* proxy, const char* filename) {
    BIO *out = BIO_new(BIO_s_file());
    if(!out) { credentialLogger.msg(ERROR, "Can not create BIO for signed proxy certificate"); LogError(); return false;}
    if (!(BIO_write_filename(out, (char*)filename))) {
      credentialLogger.msg(ERROR, "Can not set writable file for signed proxy certificate BIO"); LogError(); BIO_free_all(out); return false;
    }     
          
    if(SignRequest(proxy, out)) {
      credentialLogger.msg(INFO, "Wrote signed proxy certificate into a file");
    }
    else {credentialLogger.msg(ERROR, "Failed to write signed proxy certificate into a file"); BIO_free_all(out); return false;}

    BIO_free_all(out);
    return true;
  }

}

