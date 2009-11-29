/**Some of the following code is compliant to OpenSSL license*/
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <fstream>
#include <fcntl.h>
#include <openssl/ui.h>

#include <glibmm/fileutils.h>

#include <arc/crypto/OpenSSL.h>
#include <arc/Utils.h>

#include "Credential.h"

using namespace ArcCredential;

namespace Arc {
  CredentialError::CredentialError(const std::string& what) : std::runtime_error(what) { }

  Logger CredentialLogger(Logger::rootLogger, "Credential");

#if 0
#define PASS_MIN_LENGTH 4
  static int passwordcb(char* pwd, int len, int verify, void* passphrase) {
    int j, r;
    //char prompt[128];
    const char* prompt;
    if(passphrase) {
      j=strlen((const char*)passphrase);
      j=(j > len)?len:j;
      memcpy(pwd,passphrase,j);
      return(j);
    }
    prompt=EVP_get_pw_prompt();
    if (prompt == NULL)
      prompt="Enter PEM pass phrase:";

    for(;;) {
      //if(!verify)
      //  snprintf(prompt, sizeof(prompt), "Enter passphrase to decrypte the key file: ");
      //else
      //  snprintf(prompt, sizeof(prompt), "Enter passphrase to encrypte the key file: ");
      r = EVP_read_pw_string(pwd, len, prompt, verify);
      if(r != 0) {
        CredentialLogger.msg(ERROR,"Failed to read input passphrase");
        memset(pwd,0,(unsigned int)len);
        return(-1);
      }
      j = strlen(pwd);
      if(verify && (j < PASS_MIN_LENGTH)) {
        CredentialLogger.msg(ERROR,"Input phrase is too short (at least %d char",PASS_MIN_LENGTH);
      }
      else { return j; }
    }
  }
#endif

  static int ssl_err_cb(const char *str, size_t, void *u) {
    Logger& logger = *((Logger*)u);
    logger.msg(ERROR, "OpenSSL Error: %s", str);
    return 1;
  }

#define PASS_MIN_LENGTH 4
  typedef struct pw_cb_data {
    const void *password;
    const char *prompt_info;
  } PW_CB_DATA;

  static int passwordcb(char *buf, int bufsiz, int verify, PW_CB_DATA *cb_tmp) {
    UI *ui = NULL;
    int res = 0;
    const char *prompt_info = NULL;
    const char *password = NULL;
    PW_CB_DATA *cb_data = (PW_CB_DATA *)cb_tmp;

    if (bufsiz < 1) return res;
    if (cb_data) {
      if (cb_data->password)
      password = (const char*)(cb_data->password);
      if (cb_data->prompt_info)
        prompt_info = cb_data->prompt_info;
    }

    if (password) {
      res = strlen(password);
      if (res > (bufsiz-1))
        res = bufsiz-1;
      if(buf) {
        memcpy(buf, password, res);
        buf[res] = 0;
      }
      return res;
    }

    //ui = UI_new_method(ui_method);
    ui = UI_new();
    if (ui) {
      int ok = 0;
      char *buf1 = NULL;
      char *buf2 = NULL;
      int ui_flags = 0;
      char *prompt = NULL;

      prompt = UI_construct_prompt(ui, "pass phrase", cb_data->prompt_info);

      ui_flags |= UI_INPUT_FLAG_DEFAULT_PWD;
      UI_ctrl(ui, UI_CTRL_PRINT_ERRORS, 1, 0, 0);

      if (ok >= 0) {
        ok = -1;
        if((buf1 = (char *)OPENSSL_malloc(bufsiz)) != NULL) {
          memset(buf1,0,(unsigned int)bufsiz);
          ok = UI_add_input_string(ui,prompt,ui_flags,buf1,PASS_MIN_LENGTH,bufsiz-1);
        }
      }
      if (ok >= 0 && verify) {
        ok = -1;
        if((buf2 = (char *)OPENSSL_malloc(bufsiz)) != NULL) {
          memset(buf2,0,(unsigned int)bufsiz);
          ok = UI_add_verify_string(ui,prompt,ui_flags,buf2,PASS_MIN_LENGTH,bufsiz-1, buf);
        }
      }
      if (ok >= 0)
        do{
          ok = UI_process(ui);
        }while (ok < 0 && UI_ctrl(ui, UI_CTRL_IS_REDOABLE, 0, 0, 0));

      if (buf2){
        memset(buf2,0,(unsigned int)bufsiz);
        OPENSSL_free(buf2);
      }

      if (ok >= 0) {
        if(buf1) {
          buf1[bufsiz-1] = 0;
          res = strlen(buf1);
          if(buf) memcpy(buf,buf1,res+1);
        }
      }

      if (buf1){
        memset(buf1,0,(unsigned int)bufsiz);
        OPENSSL_free(buf1);
      }

      if (ok == -1){
        std::cerr<<"User interface error\n"<<std::endl;
        ERR_print_errors_cb(&ssl_err_cb, &CredentialLogger);
        if(buf) memset(buf,0,(unsigned int)bufsiz);
        res = 0;
      }
      if (ok == -2) {
        std::cerr<<"Aborted!\n"<<std::endl;
        if(buf) memset(buf,0,(unsigned int)bufsiz);
        res = 0;
      }
      UI_free(ui);
      OPENSSL_free(prompt);
    }
    return res;
  }

  void Credential::LogError(void) {
    ERR_print_errors_cb(&ssl_err_cb, &CredentialLogger);
  }

  Time asn1_to_utctime(const ASN1_UTCTIME *s) {
    if(s == NULL) return Time();
    std::string t_str;
    if(s->type == V_ASN1_UTCTIME) {
      t_str.append("20");
      t_str.append((char*)(s->data));
    }
    else {//V_ASN1_GENERALIZEDTIME
      t_str.append((char*)(s->data));
    } 
    return Time(t_str);
  }

  //Get the life time of the credential
  static void getLifetime(STACK_OF(X509)* certchain, X509* cert, Time& start, Period &lifetime) {
    X509* tmp_cert = NULL;
    Time start_time(-1), end_time(-1);
    int n;
    ASN1_UTCTIME* atime = NULL;

    if((cert == NULL) || (certchain == NULL)) {
      start = Time();
      lifetime = Period();
      return;
    }

    for (n = 0; n < sk_X509_num(certchain); n++) {
      tmp_cert = sk_X509_value(certchain, n);

      atime = X509_get_notAfter(tmp_cert);
      Time end = asn1_to_utctime(atime);
      if (end_time == Time(-1) || end < end_time) { end_time = end; }

      atime = X509_get_notBefore(tmp_cert);
      Time start = asn1_to_utctime(atime);
      if (start_time == Time(-1) || start > start_time) { start_time = start; }
    }

    atime = X509_get_notAfter(cert);
    Time e = asn1_to_utctime(atime);
    if (end_time == Time(-1) || e < end_time) { end_time = e; }

    atime = X509_get_notBefore(cert);
    Time s = asn1_to_utctime(atime);
    if (start_time == Time(-1) || s > start_time) { start_time = s; }

    start = start_time;
    lifetime = end_time - start_time;
  }

  //Parse the BIO for certificate and get the format of it
  Credformat Credential::getFormat(BIO* bio, const bool is_file) {
    Credformat format = CRED_UNKNOWN;
    if(bio == NULL) return format;
    if(is_file) {
      char buf[1];
      char firstbyte;
      int position;
      if((position = BIO_tell(bio))<0 || BIO_read(bio, buf, 1)<=0 || BIO_seek(bio, position)<0) {
        LogError();
        CredentialLogger.msg(ERROR,"Can't get the first byte of input BIO to get its format");
        return format;
      }
      firstbyte = buf[0];
      // DER-encoded structure (including PKCS12) will start with ASCII 048.
      // Otherwise, it's PEM.
      if(firstbyte==48) {
        //DER-encoded, PKCS12 or DER? firstly parse it as PKCS12 ASN.1,
        //if can not parse it, then it is DER format
        PKCS12* pkcs12 = NULL;
        if((pkcs12 = d2i_PKCS12_bio(bio,NULL)) == NULL){ format=CRED_DER; PKCS12_free(pkcs12); }
        else { format = CRED_PKCS; PKCS12_free(pkcs12); }
        if( BIO_seek(bio, position) < 0 ) {
          LogError();
          CredentialLogger.msg(ERROR,"Can't reset the BIO");
          return format;
        }
      }
      else { format = CRED_PEM; }
    }
    else {
      unsigned char* bio_str;
      int len;
      len = BIO_get_mem_data(bio, (unsigned char *) &bio_str);
      char firstbyte;
      if(len>0) {  
        firstbyte = bio_str[0];
        if(firstbyte==48)  {}
        else { format = CRED_PEM; }
      }
      else {
        CredentialLogger.msg(ERROR,"Can't get the first byte of input BIO to get its format");
        return format;
      }
    }
    return format;
  }

  std::string Credential::GetDN(void) {
    X509_NAME *subject = NULL;
    if(!cert_) return "";
    subject = X509_get_subject_name(cert_);
    std::string str;
    char buf[256];
    if(subject!=NULL)
      X509_NAME_oneline(subject,buf,sizeof(buf));
    str.append(buf);
    return str;
  }

  std::string Credential::GetIdentityName(void) {
    X509_NAME *subject = NULL;
    X509_NAME_ENTRY *ne = NULL;
    if(!cert_) return "";
    subject = X509_NAME_dup(X509_get_subject_name(cert_));

#if 0
    int proxy_depth = verify_ctx_.proxy_depth;
    //std::cout<<"proxy depth: +++"<<verify_ctx_.proxy_depth<<std::endl;
    for(int i=0; i<proxy_depth; i++) {
      ne = X509_NAME_delete_entry(subject, X509_NAME_entry_count(subject)-1);
      if(ne)
        X509_NAME_ENTRY_free(ne);
    }
#endif

    ASN1_STRING* entry;
    std::string entry_str;
    for(;;) {
      ne = X509_NAME_get_entry(subject, X509_NAME_entry_count(subject)-1);
      if (!OBJ_cmp(ne->object,OBJ_nid2obj(NID_commonName))) {
        entry = X509_NAME_ENTRY_get_data(ne);
        entry_str.assign((const char*)(entry->data), (std::size_t)(entry->length));
        if(entry_str == "proxy" || entry_str == "limited proxy" ||
           entry_str.find_first_not_of("0123456789") == std::string::npos) {
          //Drop the name entry "proxy", "limited proxy", or the random digital(RFC)
          ne = X509_NAME_delete_entry(subject, X509_NAME_entry_count(subject)-1);
          X509_NAME_ENTRY_free(ne);
          ne = NULL;
        }
        else break;
      }
      else break;
    }

    std::string str;
    char buf[256];
    if(subject!=NULL) {
      X509_NAME_oneline(subject,buf,sizeof(buf));
      X509_NAME_free(subject);
    }
    str.append(buf);
    return str;
  }

  certType Credential::GetType(void) {
    return cert_type_;
  }

  std::string Credential::GetProxyPolicy(void) {
    return (verify_ctx_.proxy_policy);
  }

  Period Credential::GetLifeTime(void) {
    return lifetime_;
  }

  Time Credential::GetStartTime() {
    return start_;
  }

  Time Credential::GetEndTime() {
    return start_+lifetime_;
  }

  void Credential::SetLifeTime(const Period& period) {
    lifetime_ = period;
  }

  void Credential::SetStartTime(const Time& start_time) {
    start_ = start_time;
  }

  class AutoBIO {
   private:
    BIO* bio_;
   public:
    AutoBIO(BIO* bio):bio_(bio) { };
    ~AutoBIO(void) { if(bio_) { BIO_set_close(bio_,BIO_CLOSE); BIO_free_all(bio_); } };
    operator bool(void) { return (bio_ != NULL); };
    operator BIO*(void) { return bio_; };
    BIO& operator*(void) const { return *bio_; };
    BIO* operator->(void) const { return bio_; };
  };

  void Credential::loadCertificateString(const std::string& cert, X509* &x509, STACK_OF(X509) **certchain) {
    AutoBIO certbio(BIO_new_mem_buf((void*)(cert.c_str()), cert.length()));
    if(!certbio){
      CredentialLogger.msg(ERROR,"Can not read certificate string");
      LogError();
      throw CredentialError("Can not read certificate string");
    }
    loadCertificate(certbio,x509,certchain, false);
  }

  void Credential::loadCertificateFile(const std::string& certfile, X509* &x509, STACK_OF(X509) **certchain) {
    if(!Glib::file_test(certfile,Glib::FILE_TEST_IS_REGULAR)) {
        CredentialLogger.msg(ERROR,"Can not find certificate file: %s", certfile);
        throw CredentialError("Can not find certificate file");
    }
    AutoBIO certbio(BIO_new_file(certfile.c_str(), "r"));
    if(!certbio){
      CredentialLogger.msg(ERROR,"Can not read certificate file: %s", certfile);
      LogError();
      throw CredentialError("Can not read certificate file");
    }
    loadCertificate(certbio,x509,certchain, true);
  }

  void Credential::loadCertificate(BIO* certbio, X509* &x509, STACK_OF(X509) **certchain, const bool is_file) {
    //Parse the certificate
    Credformat format = CRED_UNKNOWN;
    PKCS12* pkcs12 = NULL;
    STACK_OF(X509)* pkcs12_certs = NULL;

    if(certbio == NULL) return;
    format = getFormat(certbio, is_file);
    int n;
    if(*certchain) {
      sk_X509_pop_free(*certchain, X509_free);
      *certchain = NULL;
    }

    switch(format) {
      case CRED_PEM:
        CredentialLogger.msg(DEBUG,"Certificate format is PEM");
        //Get the certificte, By default, certificate is without passphrase
        //Read certificate
        if(!(PEM_read_bio_X509(certbio, &x509, NULL, NULL))) {
          throw CredentialError("Can not read cert information from BIO");
        }
        //Get the issuer chain
        *certchain = sk_X509_new_null();
        n = 0;
        while(!BIO_eof(certbio)){
          X509 * tmp = NULL;
          if(!(PEM_read_bio_X509(certbio, &tmp, NULL, NULL))){
            ERR_clear_error(); break;
          }
          if(!sk_X509_insert(*certchain, tmp, n)) {
            //std::string str(X509_NAME_oneline(X509_get_subject_name(tmp),0,0));
            X509_free(tmp);
            throw CredentialError("Can not insert cert into certificate's issuer chain");
          }
          ++n;
        }
        break;

      case CRED_DER:
        CredentialLogger.msg(DEBUG,"Certificate format is DER");
        x509=d2i_X509_bio(certbio,NULL);
        if(!x509){
          throw CredentialError("Unable to read DER credential from BIO");
        }
        //Get the issuer chain
        *certchain = sk_X509_new_null();
        n = 0;
        while(!BIO_eof(certbio)){
          X509 * tmp = NULL;
          if(!(d2i_X509_bio(certbio, &tmp))){
            ERR_clear_error(); break;
          }
          if(!sk_X509_insert(*certchain, tmp, n)) {
            //std::string str(X509_NAME_oneline(X509_get_subject_name(tmp),0,0));
            X509_free(tmp);
            throw CredentialError("Can not insert cert into certificate's issuer chain");
          }
          ++n;
        }
        break;

      case CRED_PKCS:
        CredentialLogger.msg(DEBUG,"Certificate format is PKCS");
        pkcs12 = d2i_PKCS12_bio(certbio, NULL);
        if(pkcs12){
          char password[100];
          EVP_read_pw_string(password, 100, "Enter Password for PKCS12 certificate:", 0);
          if(!PKCS12_parse(pkcs12, password, NULL, &x509, &pkcs12_certs)) {
            if(pkcs12) PKCS12_free(pkcs12);
            throw CredentialError("Can not parse PKCS12 file");
          }
        }
        else{
          throw CredentialError("Can not read PKCS12 credential from BIO");
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

      default:
        CredentialLogger.msg(DEBUG,"Certificate format is unknown");
        break;
     } // end switch
  }

  void Credential::loadKeyString(const std::string& key, EVP_PKEY* &pkey, const std::string& passphrase) {
    AutoBIO keybio(BIO_new_mem_buf((void*)(key.c_str()), key.length()));
    if(!keybio){
      CredentialLogger.msg(ERROR,"Can not read key string");
      LogError();
      throw CredentialError("Can not read key string");
    }
    loadKey(keybio,pkey,passphrase, "Load key from a string", false);
  }

  void Credential::loadKeyFile(const std::string& keyfile, EVP_PKEY* &pkey, const std::string& passphrase) {
    if(!Glib::file_test(keyfile,Glib::FILE_TEST_IS_REGULAR)) {
        CredentialLogger.msg(ERROR,"Can not find key file: %s", keyfile);
        throw CredentialError("Can not find key file");
    }
    AutoBIO keybio(BIO_new_file(keyfile.c_str(), "r"));
    if(!keybio){
      CredentialLogger.msg(ERROR,"Can not read key file: %s", keyfile);
      LogError();
      throw CredentialError("Can not read key file");
    }
    loadKey(keybio,pkey,passphrase, keyfile, true);
  }

  void Credential::loadKey(BIO* keybio, EVP_PKEY* &pkey, const std::string& passphrase, const std::string& prompt_info, const bool is_file) {
    //Read key
    Credformat format;
    PKCS12* pkcs12 = NULL;

    if(keybio == NULL) return;
    format = getFormat(keybio, is_file);
    std::string prompt;
    switch(format){
      case CRED_PEM:
#if 0
        prompt = "Enter passphrase to decrypt the private key: ";
        EVP_set_pw_prompt((char*)(prompt.c_str()));
        if(!(pkey = PEM_read_bio_PrivateKey(keybio, NULL, passwordcb,
          (passphrase.empty()) ? NULL : (void*)(passphrase.c_str())))) {
          throw CredentialError("Can not read credential key from PEM key BIO");
        }
#endif
        PW_CB_DATA cb_data;
        cb_data.password = (passphrase.empty()) ? NULL : (void*)(passphrase.c_str());
        cb_data.prompt_info = prompt_info.empty() ? NULL : prompt_info.c_str();
        if(!(pkey = PEM_read_bio_PrivateKey(keybio, NULL, (pem_password_cb *)passwordcb, &cb_data))) {
          throw CredentialError("Can not read credential key from PEM key BIO");
        }
        break;

      case CRED_DER:
        pkey=d2i_PrivateKey_bio(keybio, NULL);
        break;

      case CRED_PKCS:
        pkcs12 = d2i_PKCS12_bio(keybio, NULL);
        if(pkcs12) {
          char password[100];
          EVP_read_pw_string(password, 100, "Enter Password for PKCS12 certificate:", 0);
          if(!PKCS12_parse(pkcs12, password, &pkey, NULL, NULL)) {
            throw CredentialError("Can not parse PKCS12 file");
          }
          PKCS12_free(pkcs12);
        }
        break;

      default:
        break;
    }
  }

  static bool proxy_init_ = false;

  void Credential::InitProxyCertInfo(void) {
    static Glib::Mutex lock_;
    #define OBJC(c,n) OBJ_create(c,n,#c)
    X509V3_EXT_METHOD *pci_x509v3_ext_meth = NULL;

    // At least in some versions of OpenSSL functions manupulating
    // global lists seems to be not thread-safe despite locks
    // installed (tested for 0.9.7). Hence it is safer to protect
    // such calls. 
    // It is also good idea to protect proxy_init_ too.

    Glib::Mutex::Lock lock(lock_);
    if(proxy_init_) return;

    /* Proxy Certificate Extension's related objects */
    if(OBJ_txt2nid(PROXYCERTINFO_V3) == NID_undef) {
      OBJC(PROXYCERTINFO_V3, "PROXYCERTINFO_V3");
      pci_x509v3_ext_meth = PROXYCERTINFO_v3_x509v3_ext_meth();
      if (pci_x509v3_ext_meth) {
        pci_x509v3_ext_meth->ext_nid = OBJ_txt2nid(PROXYCERTINFO_V3);
        X509V3_EXT_add(pci_x509v3_ext_meth);
      }
    }

    if(OBJ_txt2nid(PROXYCERTINFO_V4) == NID_undef) {
      OBJC(PROXYCERTINFO_V4, "PROXYCERTINFO_V4");
      pci_x509v3_ext_meth = PROXYCERTINFO_v4_x509v3_ext_meth();
      if (pci_x509v3_ext_meth) {
        pci_x509v3_ext_meth->ext_nid = OBJ_txt2nid(PROXYCERTINFO_V4);
        X509V3_EXT_add(pci_x509v3_ext_meth);
      }
    }

    if(OBJ_txt2nid(IMPERSONATION_PROXY_OID) == NID_undef) {
      OBJ_create(IMPERSONATION_PROXY_OID, IMPERSONATION_PROXY_SN, IMPERSONATION_PROXY_LN);
    }
    if(OBJ_txt2nid(INDEPENDENT_PROXY_OID) == NID_undef) {
      OBJ_create(INDEPENDENT_PROXY_OID, INDEPENDENT_PROXY_SN, INDEPENDENT_PROXY_LN);
    }
    if(OBJ_txt2nid(ANYLANGUAGE_PROXY_OID) == NID_undef) {
      OBJ_create(ANYLANGUAGE_PROXY_OID, ANYLANGUAGE_PROXY_SN, ANYLANGUAGE_PROXY_LN);
    }
    if(OBJ_txt2nid(LIMITED_PROXY_OID) == NID_undef) {
      OBJ_create(LIMITED_PROXY_OID, LIMITED_PROXY_SN, LIMITED_PROXY_LN);
    }
    // This library provides methods and objects which when registred in 
    // global OpenSSL lists can't be unregistred anymore. Hence it must not 
    // be allowed to unload.
    if(!PersistentLibraryInit("arccredential")) {
      CredentialLogger.msg(WARNING, "Failed to lock arccredential library in memory");
    };
    proxy_init_=true;
  }

  void Credential::AddCertExtObj(std::string& sn, std::string& oid) {
    OBJ_create(oid.c_str(), sn.c_str(), sn.c_str());
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
    if(verify_cert_chain(cert_, &cert_chain_, &verify_ctx_)) {
      CredentialLogger.msg(DEBUG, "Certificate verification succeeded");
      return true;
    }
    else {CredentialLogger.msg(ERROR, "Certificate verification failed"); LogError(); return false;}
  }

  Credential::Credential() : cert_(NULL), pkey_(NULL), cert_chain_(NULL), proxy_cert_info_(NULL),
        start_(Time()), lifetime_(Period("PT12H")),
        req_(NULL), rsa_key_(NULL), signing_alg_((EVP_MD*)EVP_sha1()), keybits_(1024),
        extensions_(NULL) {

    OpenSSLInit();
    //EVP_add_digest(EVP_sha1());

    InitVerification();

    extensions_ = sk_X509_EXTENSION_new_null();

    //Initiate the proxy certificate constant and  method which is required by openssl
    if(!proxy_init_) InitProxyCertInfo();
  }

  Credential::Credential(int keybits) : cert_(NULL), pkey_(NULL), cert_chain_(NULL), proxy_cert_info_(NULL),
	  start_(Time()), lifetime_(Period("PT12H")),
	  req_(NULL), rsa_key_(NULL), signing_alg_((EVP_MD*)EVP_sha1()), keybits_(keybits),
	  extensions_(NULL) {

    OpenSSLInit();
    //EVP_add_digest(EVP_sha1());

    InitVerification();

    extensions_ = sk_X509_EXTENSION_new_null();

    //Initiate the proxy certificate constant and  method which is required by openssl
    if(!proxy_init_) InitProxyCertInfo();
  }

  Credential::Credential(Time start, Period lifetime, int keybits, std::string proxyversion,
        std::string policylang, std::string policy, int pathlength) :
        cert_(NULL), pkey_(NULL), cert_chain_(NULL), proxy_cert_info_(NULL),
        start_(start), lifetime_(lifetime), req_(NULL), rsa_key_(NULL),
        signing_alg_((EVP_MD*)EVP_sha1()), keybits_(keybits), extensions_(NULL) {

    OpenSSLInit();
    //EVP_add_digest(EVP_sha1());

    InitVerification();

    extensions_ = sk_X509_EXTENSION_new_null();

    //Initiate the proxy certificate constant and  method which is required by openssl
    if(!proxy_init_) InitProxyCertInfo();

    SetProxyPolicy(proxyversion, policylang, policy, pathlength);
  }

  void Credential::SetProxyPolicy(const std::string& proxyversion, const std::string& policylang,
        const std::string& policy, int pathlength) {

    proxyversion_ = proxyversion;
    policy_ = policy;
    policylang_ = policylang;
    pathlength_ = pathlength;

    //Get certType
    if(proxyversion_.compare("GSI2") == 0 || proxyversion_.compare("gsi2") == 0) {
      if(policylang_.compare("LIMITED") == 0 || policylang_.compare("limited") == 0) {
        cert_type_ = CERT_TYPE_GSI_2_LIMITED_PROXY;
      }
      else if (policylang_.compare("RESTRICTED") == 0 || policylang_.compare("restricted") == 0) {
        CredentialLogger.msg(ERROR,"Globus legacy proxies can not carry policy data or path length contraints");
      }
      else {
        cert_type_ = CERT_TYPE_GSI_2_PROXY;
      }
    }
    else if (proxyversion_.compare("RFC") == 0 || proxyversion_.compare("rfc") == 0) {
      //The "limited" and "restricted" are from the definition in
      //http://dev.globus.org/wiki/Security/ProxyCertTypes#RFC_3820_Proxy_Certificates
      if(policylang_.compare("LIMITED") == 0 || policylang_.compare("limited") == 0) {
        cert_type_ = CERT_TYPE_RFC_LIMITED_PROXY;
      }
      else if(policylang_.compare("RESTRICTED") == 0 || policylang_.compare("restricted") == 0) {
        cert_type_ = CERT_TYPE_RFC_RESTRICTED_PROXY;
      }
      else if(policylang_.compare("INDEPENDENT") == 0 || policylang_.compare("independent") == 0) {
        cert_type_ = CERT_TYPE_RFC_INDEPENDENT_PROXY;
      }
      else if(policylang_.compare("IMPERSONATION") == 0 || policylang_.compare("impersonation") == 0 ||
         policylang_.compare("INHERITALL") == 0 || policylang_.compare("inheritAll") == 0){
        cert_type_ = CERT_TYPE_RFC_IMPERSONATION_PROXY;
        //For RFC here, "impersonation" is the same as the "inheritAll" in openssl version>098
      }
      else if(policylang_.compare("ANYLANGUAGE") == 0 || policylang_.compare("anylanguage") == 0) {
        cert_type_ = CERT_TYPE_RFC_ANYLANGUAGE_PROXY;  //Defined in openssl version>098
      }
    }
    else if (proxyversion_.compare("EEC") == 0 || proxyversion_.compare("eec") == 0) {
      cert_type_ = CERT_TYPE_EEC;
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

    if(cert_type_ != CERT_TYPE_EEC) {
      if(!policy_.empty() && policylang_.empty()) {
        CredentialLogger.msg(ERROR,"If you specify a policy you also need to specify a policy language");
        return;
      }

      if(proxy_cert_info_) {
        PROXYCERTINFO_free(proxy_cert_info_);
        proxy_cert_info_ = NULL;
      }

      proxy_cert_info_ = PROXYCERTINFO_new();
      PROXYPOLICY *   ppolicy =PROXYCERTINFO_get_proxypolicy(proxy_cert_info_);
      PROXYPOLICY_set_policy(ppolicy, NULL, 0);
      ASN1_OBJECT *   policy_object = NULL;

      //set policy language, see definiton in: http://dev.globus.org/wiki/Security/ProxyCertTypes
      switch(cert_type_)
      {
        case CERT_TYPE_GSI_3_IMPERSONATION_PROXY:
        case CERT_TYPE_RFC_IMPERSONATION_PROXY:
          if((policy_object = OBJ_nid2obj(OBJ_txt2nid(IMPERSONATION_PROXY_OID))) != NULL) {
            PROXYPOLICY_set_policy_language(ppolicy, policy_object);
          }
          break;

        case CERT_TYPE_GSI_3_INDEPENDENT_PROXY:
        case CERT_TYPE_RFC_INDEPENDENT_PROXY:
          if((policy_object = OBJ_nid2obj(OBJ_txt2nid(INDEPENDENT_PROXY_OID))) != NULL) {
            PROXYPOLICY_set_policy_language(ppolicy, policy_object);
          }
          break;

        case CERT_TYPE_GSI_3_LIMITED_PROXY:
        case CERT_TYPE_RFC_LIMITED_PROXY:
          if((policy_object = OBJ_nid2obj(OBJ_txt2nid(LIMITED_PROXY_OID))) != NULL) {
            PROXYPOLICY_set_policy_language(ppolicy, policy_object);
          }
          break;
        case CERT_TYPE_RFC_ANYLANGUAGE_PROXY:
          if((policy_object = OBJ_nid2obj(OBJ_txt2nid(ANYLANGUAGE_PROXY_OID))) != NULL) {
            PROXYPOLICY_set_policy_language(ppolicy, policy_object);
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
      if(!policy_.empty()) {
        if(Glib::file_test(policy_,Glib::FILE_TEST_EXISTS)) {
          //If the argument is a location which specifies a file that
          //includes the policy content
          if(Glib::file_test(policy_,Glib::FILE_TEST_IS_REGULAR)) {
            std::ifstream fp;
            fp.open(policy_.c_str());
            if(!fp) {
              CredentialLogger.msg(ERROR,"Error: can't open policy file: %s", policy_.c_str());
              if(proxy_cert_info_) {
                PROXYCERTINFO_free(proxy_cert_info_);
                proxy_cert_info_ = NULL;
              }
              return;
            }
            fp.unsetf(std::ios::skipws);
            char c;
            while(fp.get(c))
              policystring += c;
          }
          else {
            CredentialLogger.msg(ERROR,"Error: policy location: %s is not a regular file", policy_.c_str());
            if(proxy_cert_info_) {
              PROXYCERTINFO_free(proxy_cert_info_);
              proxy_cert_info_ = NULL;
            }
            return;
          }
        }
        else {
          //Otherwise the argument should include the policy content
          policystring = policy_;
        }
      }

      if(policylang_ == "LIMITED" || policylang_ == "limited")
        policylang_ = LIMITED_PROXY_OID;
      else if(policylang_ == "INDEPENDENT" || policylang_ == "independent")
        policylang_ = INDEPENDENT_PROXY_OID;
      else if(policylang_ == "IMPERSONATION" || policylang_ == "impersonation"
          || policylang_ == "inheritAll" || policylang_ == "INHERITALL")
        policylang_ = IMPERSONATION_PROXY_OID;
      else if(policylang_.empty()) {
        if(policy_.empty()) policylang_ = IMPERSONATION_PROXY_OID;
        else policylang_ = GLOBUS_GSI_PROXY_GENERIC_POLICY_OID;
      }

      /**here the definition about policy languange is redundant with above definition,
       * but it could cover "restricted" language which has no explicit definition
       * according to http://dev.globus.org/wiki/Security/ProxyCertTypes
       */
      //OBJ_create((char *)policylang_.c_str(), (char *)policylang_.c_str(), (char *)policylang_.c_str());

      //policy_object = OBJ_nid2obj(OBJ_sn2nid(policylang_.c_str()));

      ppolicy = PROXYCERTINFO_get_proxypolicy(proxy_cert_info_);
      //Here only consider the situation when there is policy specified
      if(policystring.size() > 0) {
        //PROXYPOLICY_set_policy_language(ppolicy, policy_object);
        PROXYPOLICY_set_policy(ppolicy, (unsigned char*)policystring.c_str(), policystring.size());
      }

      //set the version of PROXYCERTINFO
      if(proxyversion_ == "RFC" || proxyversion_ == "rfc")
        PROXYCERTINFO_set_version(proxy_cert_info_, 4);
      else
        PROXYCERTINFO_set_version(proxy_cert_info_, 3);
    }
  }

  Credential::Credential(const std::string& certfile, const std::string& keyfile,
        const std::string& cadir, const std::string& cafile, 
        const std::string& passphrase4key, const bool is_file) :
        cacertfile_(cafile), cacertdir_(cadir), certfile_(certfile), keyfile_(keyfile),
        cert_(NULL), pkey_(NULL), cert_chain_(NULL), proxy_cert_info_(NULL),
        req_(NULL), rsa_key_(NULL), signing_alg_((EVP_MD*)EVP_sha1()),
        keybits_(1024), extensions_(NULL) {

    OpenSSLInit();
    //EVP_add_digest(EVP_sha1());

    InitVerification();

    extensions_ = sk_X509_EXTENSION_new_null();

    if(certfile.empty()) {
      CredentialLogger.msg(ERROR, "Certificate/Proxy path is empty");
      return;
    }

    //Initiate the proxy certificate constant and  method which is required by openssl
    if(!proxy_init_) InitProxyCertInfo();

    if(is_file) {
      try {
        loadCertificateFile(certfile, cert_, &cert_chain_);
        if(cert_) check_cert_type(cert_,cert_type_);
        //std::cout<<"Your identity: "<<GetDN() << std::endl;
        if(keyfile.empty()) {
          //Detect if the certificate file/string contains private key.
          //If the key file is absent, and the private key is not contained inside
          //certificate file/string, then the certificate file will not 
          //be parsed for private key.
          //Note this detection only applies to PEM file
          std::string keystr;
          if(Glib::file_test(certfile,Glib::FILE_TEST_EXISTS)) {      
            std::ifstream in(certfile.c_str(), std::ios::in);
            if (!in) {
              CredentialLogger.msg(ERROR,"Can not read certificate file: %s", certfile);
              throw CredentialError("Can not read certificate file");
            }
            std::getline<char>(in, keystr, 0);
            in.close();
          }
          else {
            keystr = certfile;
          }
          if(keystr.find("BEGIN RSA PRIVATE KEY") != std::string::npos)
            loadKeyFile(certfile, pkey_, passphrase4key);
        }
        else
          loadKeyFile(keyfile, pkey_, passphrase4key);
      } catch(std::exception& err){
        CredentialLogger.msg(ERROR, "ERROR:%s", err.what());
        LogError(); return;
      }
    } else {
      try {
        loadCertificateString(certfile, cert_, &cert_chain_);
        if(cert_) check_cert_type(cert_,cert_type_);
        if(keyfile.empty()) {
          std::string keystr;
          keystr = certfile;
          if(keystr.find("BEGIN RSA PRIVATE KEY") != std::string::npos)
            loadKeyString(certfile, pkey_, passphrase4key);
        }
        else
          loadKeyString(keyfile, pkey_, passphrase4key);
      } catch(std::exception& err){
        CredentialLogger.msg(ERROR, "ERROR:%s", err.what());
        LogError(); return;
      }
    }

    //Get the lifetime of the credential
    getLifetime(cert_chain_, cert_, start_, lifetime_);

    if(!cacertfile_.empty() || !cacertdir_.empty())
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

    bool numberic = true;
    size_t pos1 = 0, pos2;
    do {
      pos2 = name.find(".", pos1);
      if(pos2 != std::string::npos) { //OID: 1.2.3.4.5
        std::string str = name.substr(pos1, pos2 - pos1);
        long int number = strtol(str.c_str(), NULL, 0);
        if(number == 0) { numberic = false; break; }
      }
      else { //The only one (without '.' as sperator, then the name is a single number or a string) or the last number
        std::string str = name.substr(pos1);
        long int number = strtol(str.c_str(), NULL, 0);
        if(number == 0) { numberic = false; break; }
        else break;
      }
      pos1 = pos2 + 1;
    } while(true);

    if(!numberic && !(ext_obj = OBJ_nid2obj(OBJ_txt2nid((char *)(name.c_str()))))) {
      //string format, the OID should have been registered before calling OBJ_nid2obj
      CredentialLogger.msg(ERROR, "Can not convert string into ASN1_OBJECT");
      LogError(); return NULL;
    }
    else if(numberic && !(ext_obj = OBJ_txt2obj(name.c_str(), 1))) {
      //numerical format, the OID will be registered here if it has not been registered before
      CredentialLogger.msg(ERROR, "Can not convert string into ASN1_OBJECT");
      LogError(); return NULL;
    }

    ext_oct = ASN1_OCTET_STRING_new();

    //ASN1_OCTET_STRING_set(ext_oct, data.c_str(), data.size());
    ext_oct->data = (unsigned char*) malloc(data.size());
    memcpy(ext_oct->data, data.c_str(), data.size());
    ext_oct->length = data.size();

    if (!(ext = X509_EXTENSION_create_by_OBJ(NULL, ext_obj, crit, ext_oct))) {
      CredentialLogger.msg(ERROR, "Can not create extension for proxy certificate");
      LogError();
      if(ext_oct) ASN1_OCTET_STRING_free(ext_oct);
      if(ext_obj) ASN1_OBJECT_free(ext_obj);
      return NULL;
    }

#ifndef WIN32

    // TODO: ASN1_OCTET_STRING_free is not working correctly
    //      on Windows Vista, bugreport: 1587

    if(ext_oct) ASN1_OCTET_STRING_free(ext_oct);

#endif
    if(ext_obj) ASN1_OBJECT_free(ext_obj);

    return ext;
  }

  X509_REQ* Credential::GetCertReq(void) {
    return req_;
  }

  bool Credential::GenerateEECRequest(BIO* reqbio, BIO* /*keybio*/, std::string dn) {
    bool res = false;
    RSA* rsa_key = NULL;
    const EVP_MD *digest = signing_alg_;
    EVP_PKEY* pkey;
    int keybits = keybits_;

#ifdef HAVE_OPENSSL_OLDRSA
    unsigned long prime = RSA_F4;
    rsa_key = RSA_generate_key(keybits, prime, keygen_cb, NULL);
    if(!rsa_key) {
      CredentialLogger.msg(ERROR, "RSA_generate_key failed");
      if(rsa_key) RSA_free(rsa_key);
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
          CredentialLogger.msg(ERROR, "RSA_generate_key_ex failed");
          LogError();
          if(prime) BN_free(prime);
          return false;
        }
      }
      else{
        CredentialLogger.msg(ERROR, "BN_set_word failed");
        LogError();
        if(prime) BN_free(prime);
        if(rsa_key) RSA_free(rsa_key);
        return false; 
      }
    }
    else {
      CredentialLogger.msg(ERROR, "BN_new || RSA_new failed");
      LogError();
      if(prime) BN_free(prime);
      if(rsa_key) RSA_free(rsa_key);
      return false;
    }
    if(prime) BN_free(prime);
#endif

    X509_REQ *req = NULL;
    CredentialLogger.msg(VERBOSE, "Created RSA key, proceeding with request");
    pkey = EVP_PKEY_new();

    if (pkey) {
      if (rsa_key) {
        CredentialLogger.msg(VERBOSE, "pkey and rsa_key exist!");
        if (EVP_PKEY_set1_RSA(pkey, rsa_key)) {
          req = X509_REQ_new();
          CredentialLogger.msg(VERBOSE, "Generate new X509 request!");
          if(req) {
            if (X509_REQ_set_version(req,3L)) {
              X509_NAME *name = NULL;
              unsigned long chtype = MBSTRING_ASC;  //TODO
              name = parse_name((char*)(dn.c_str()), chtype, 0);
              CredentialLogger.msg(VERBOSE, "Setting subject name!");

              X509_REQ_set_subject_name(req, name);
              X509_NAME_free(name);

              if(X509_REQ_set_pubkey(req,pkey)) {
                if(X509_REQ_sign(req,pkey,digest)) {
                  if(!(PEM_write_bio_X509_REQ(reqbio,req))){
                    CredentialLogger.msg(ERROR, "PEM_write_bio_X509_REQ failed");
                    LogError();
                    res = false;
                  } 
                  else {
                    rsa_key_ = rsa_key;
                    rsa_key = NULL;
                    pkey_ = pkey;
                    pkey = NULL;
                    req_ = req;
                    res = true;
                  }
                }
              }
            }
          }
        }
      }
    }

    req_ = req;
    return res;
  }

  bool Credential::GenerateEECRequest(std::string &req_content, std::string &key_content, std::string dn) {
    BIO *req_out = BIO_new(BIO_s_mem());
    BIO *key_out = BIO_new(BIO_s_mem());
    if(!req_out || !key_out) {
      CredentialLogger.msg(ERROR, "Can not create BIO for request");
      LogError(); return false;
    }

    if(GenerateEECRequest(req_out, key_out,dn)) {
      int l = 0;
      char s[256];

      while ((l = BIO_read(req_out,s,sizeof(s))) >= 0) {
        req_content.append(s,l);
      }

      l = 0;

      while ((l=BIO_read(key_out,s,sizeof(s))) >= 0) {
        key_content.append(s,l);
      }
    } else {
      CredentialLogger.msg(ERROR, "Failed to write request into string");
      BIO_free_all(req_out);
      BIO_free_all(key_out);
      return false;
    }

    BIO_free_all(req_out);
    BIO_free_all(key_out);
    return true;
  }

  bool Credential::GenerateEECRequest(const char* req_filename, const char* key_filename, std::string dn) {
    BIO *req_out = BIO_new(BIO_s_file());
    BIO *key_out = BIO_new(BIO_s_file());
    if(!req_out || !key_out) {
      CredentialLogger.msg(ERROR, "Can not create BIO for request");
      return false;
    }
    if (!(BIO_write_filename(req_out, (char*)req_filename))) {
      CredentialLogger.msg(ERROR, "Can not set writable file for request BIO");
      BIO_free_all(req_out); return false;
    }

    if (!(BIO_write_filename(key_out, (char*)key_filename))) {
      CredentialLogger.msg(ERROR, "Can not set writable file for request BIO");
      BIO_free_all(key_out);
      return false;
    }

    if(GenerateEECRequest(req_out,key_out, dn)) {
      CredentialLogger.msg(INFO, "Wrote request into a file");
    } else {
      CredentialLogger.msg(ERROR, "Failed to write request into a file");
      BIO_free_all(req_out);
      BIO_free_all(key_out);
      return false;
    }

    BIO_free_all(req_out);
    BIO_free_all(key_out);
    return true;
  }

  //TODO: self request/sign proxy

  bool Credential::GenerateRequest(BIO* reqbio, bool if_der){
    bool res = false;
    RSA* rsa_key = NULL;
    int keybits = keybits_;
    const EVP_MD *digest = signing_alg_;
    EVP_PKEY* pkey;

    if(pkey_) { CredentialLogger.msg(ERROR, "The credential's private key has already been initialized"); return false; };

#ifdef HAVE_OPENSSL_OLDRSA
    unsigned long prime = RSA_F4;
    rsa_key = RSA_generate_key(keybits, prime, keygen_cb, NULL);
    if(!rsa_key) {
      CredentialLogger.msg(ERROR, "RSA_generate_key failed");
      LogError();
      if(rsa_key) RSA_free(rsa_key);
      return false;
    }
#else
    BN_GENCB cb;
    BIGNUM *prime = BN_new();
    rsa_key = RSA_new();

    BN_GENCB_set(&cb,&keygen_cb,NULL);
    if(prime && rsa_key) {
      if(BN_set_word(prime,RSA_F4)) {
        if(RSA_generate_key_ex(rsa_key, keybits, prime, &cb) != 1) {
          CredentialLogger.msg(ERROR, "RSA_generate_key_ex failed");
          LogError();
          if(prime) BN_free(prime);
          return false;
        }
      }
      else{
        CredentialLogger.msg(ERROR, "BN_set_word failed");
        LogError();
        if(prime) BN_free(prime);
        if(rsa_key) RSA_free(rsa_key);
        return false; }
    }
    else {
      CredentialLogger.msg(ERROR, "BN_new || RSA_new failed");
      LogError();
      if(prime) BN_free(prime);
      if(rsa_key) RSA_free(rsa_key);
      return false;
    }
    if(prime) BN_free(prime);
#endif

    X509_REQ *req = NULL;
    pkey = EVP_PKEY_new();
    if(pkey){
      if(rsa_key) {
        if(EVP_PKEY_set1_RSA(pkey, rsa_key)) {
          req = X509_REQ_new();
          if(req) {
            if(X509_REQ_set_version(req,2L)) {
              //set the DN
              X509_NAME* name = NULL;
              X509_NAME_ENTRY* entry = NULL;
              if(cert_) { //self-sign, copy the X509_NAME
                if ((name = X509_NAME_dup(X509_get_subject_name(cert_))) == NULL) {
                  CredentialLogger.msg(ERROR, "Can not duplicate the subject name for the self-signing proxy certificate request");
                  LogError(); res = false;
                  if(pkey) EVP_PKEY_free(pkey);
                  if(rsa_key) RSA_free(rsa_key);
                  return res;
                }
              }
              else { name = X509_NAME_new();}
              if((entry = X509_NAME_ENTRY_create_by_NID(NULL, NID_commonName, V_ASN1_APP_CHOOSE,
                          (unsigned char *) "NULL SUBJECT NAME ENTRY", -1)) == NULL) {
                CredentialLogger.msg(ERROR, "Can not create a new X509_NAME_ENTRY for the proxy certificate request");
                LogError(); res = false; X509_NAME_free(name);
                if(pkey) EVP_PKEY_free(pkey);
                if(rsa_key) RSA_free(rsa_key);
                return res;
              }
              X509_NAME_add_entry(name, entry, X509_NAME_entry_count(name), 0);
              X509_REQ_set_subject_name(req,name);
              X509_NAME_free(name); name = NULL;
              if(entry) { X509_NAME_ENTRY_free(entry); entry = NULL; }

              if(cert_type_ != CERT_TYPE_EEC) {

                // set the default PROXYCERTINFO extension
                std::string certinfo_sn;
                X509_EXTENSION* ext = NULL;
                if (CERT_IS_RFC_PROXY(cert_type_)) certinfo_sn = "PROXYCERTINFO_V4";
                else certinfo_sn = "PROXYCERTINFO_V3";
                //if(proxy_cert_info_->version == 3)

                if(!(certinfo_sn.empty())) {
                  X509V3_EXT_METHOD*  ext_method = NULL;
                  unsigned char* data = NULL;
                  int length;
                  ext_method = X509V3_EXT_get_nid(OBJ_sn2nid(certinfo_sn.c_str()));
                  if(ext_method == NULL) {
                    CredentialLogger.msg(ERROR, "Can get X509V3_EXT_METHOD for %s",certinfo_sn.c_str());
                    LogError(); return false;
                  }
                  length = ext_method->i2d(proxy_cert_info_, NULL);
                  if(length < 0) {
                    CredentialLogger.msg(ERROR, "Can not convert PROXYCERTINFO struct from internal to DER encoded format");
                    LogError();
                  }
                  else {
                    data = (unsigned char*) malloc(length);

                    unsigned char* derdata;
                    derdata = data;
                    length = ext_method->i2d(proxy_cert_info_,  &derdata);

                    if(length < 0) {
                      CredentialLogger.msg(ERROR, "Can not convert PROXYCERTINFO struct from internal to DER encoded format");
                      free(data); data = NULL; LogError();
                    }
                  }
                  if(data) {
                    std::string ext_data((char*)data, length); free(data);
                    std::string certinfo_oid = PROXYCERTINFO_OPENSSL;
                    ext = CreateExtension(certinfo_oid, ext_data, 1);
                  }
                }

                if(ext) {
                  STACK_OF(X509_EXTENSION)* extensions;
                  extensions = sk_X509_EXTENSION_new_null();
                  sk_X509_EXTENSION_push(extensions, ext);
                  X509_REQ_add_extensions(req, extensions);
                  sk_X509_EXTENSION_pop_free(extensions, X509_EXTENSION_free);
                }

              }

              if(X509_REQ_set_pubkey(req,pkey)) {
                if(X509_REQ_sign(req,pkey,digest)  != 0) {
                  if(if_der == false) {
                    if(!(PEM_write_bio_X509_REQ(reqbio,req))){
                      CredentialLogger.msg(ERROR, "PEM_write_bio_X509_REQ failed");
                      LogError(); res = false;
                    }
                    else { rsa_key_ = rsa_key; rsa_key = NULL; pkey_ = pkey; pkey = NULL; res = true; }
                  }
                  else {
                    if(!(i2d_X509_REQ_bio(reqbio,req))){
                      CredentialLogger.msg(ERROR, "Can't convert X509 request from internal to DER encoded format");
                      LogError(); res = false;
                    }
                    else { rsa_key_ = rsa_key; rsa_key = NULL; pkey_ = pkey; pkey = NULL; res = true; }
                  }
                }
              }
            }
            //X509_REQ_free(req);
          }
          else { CredentialLogger.msg(ERROR, "Can not generate X509 request"); LogError(); res = false; }
        }
        else { CredentialLogger.msg(ERROR, "Can not set private key"); LogError(); res = false;}
      }
    }

    req_ = req;
    return res;
  }

  bool Credential::GenerateRequest(std::string &content, bool if_der) {
    BIO *out = BIO_new(BIO_s_mem());
    if(!out) {
      CredentialLogger.msg(ERROR, "Can not create BIO for request");
      LogError(); return false;
    }

    if(GenerateRequest(out,if_der)) {
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

  bool Credential::GenerateRequest(const char* filename, bool if_der) {
    BIO *out = BIO_new(BIO_s_file());
    if(!out) {
      CredentialLogger.msg(ERROR, "Can not create BIO for request");
      LogError(); return false;
    }
    if (!(BIO_write_filename(out, (char*)filename))) {
      CredentialLogger.msg(ERROR, "Can not set writable file for request BIO");
      LogError(); BIO_free_all(out); return false;
    }

    if(GenerateRequest(out,if_der)) {
      CredentialLogger.msg(INFO, "Wrote request into a file");
    }
    else {
      CredentialLogger.msg(ERROR, "Failed to write request into a file");
      BIO_free_all(out); return false;
    }

    BIO_free_all(out);
    return true;
  }

  bool Credential::OutputPrivatekey(std::string &content, bool encryption, const std::string& passphrase) {
    BIO *out = BIO_new(BIO_s_mem());
    EVP_CIPHER *enc = NULL;
    if(!out) return false;
    if(rsa_key_ != NULL) {
      if(!encryption) {
        if(!PEM_write_bio_RSAPrivateKey(out,rsa_key_,enc,NULL,0,NULL,NULL)) {
          BIO_free_all(out); return false;
        }
      }
      else {
        enc = (EVP_CIPHER*)EVP_des_ede3_cbc();
#if 0
        std::string prompt;
        prompt = "Enter passphrase to encrypt the PEM key file: ";
        if(!PEM_write_bio_RSAPrivateKey(out,rsa_key_,enc,NULL,0,passwordcb,
          (passphrase.empty())?NULL:(void*)(passphrase.c_str()))) {
          BIO_free_all(out); return false;
        }
#endif
        PW_CB_DATA cb_data;
        cb_data.password = (passphrase.empty()) ? NULL : (void*)(passphrase.c_str());
        cb_data.prompt_info = NULL;
        if(!PEM_write_bio_RSAPrivateKey(out,rsa_key_,enc,NULL,0, (pem_password_cb *)passwordcb,&cb_data)) {
          BIO_free_all(out); return false;
        }
      }
    }
    else if(pkey_ != NULL) {
      if(!encryption) {
        if(!PEM_write_bio_PrivateKey(out,pkey_,enc,NULL,0,NULL,NULL)) {
          BIO_free_all(out); return false;
        }
      }
      else {
        enc = (EVP_CIPHER*)EVP_des_ede3_cbc();
#if 0
        std::string prompt;
        prompt = "Enter passphrase to encrypt the PEM key file: ";
        if(!PEM_write_bio_PrivateKey(out,pkey_,enc,NULL,0,passwordcb,
          (passphrase.empty())?NULL:(void*)(passphrase.c_str()))) {
          BIO_free_all(out); return false;
        }
#endif
        PW_CB_DATA cb_data;
        cb_data.password = (passphrase.empty()) ? NULL : (void*)(passphrase.c_str());
        cb_data.prompt_info = NULL;
        if(!PEM_write_bio_PrivateKey(out,pkey_,enc,NULL,0, (pem_password_cb *)passwordcb,&cb_data)) {
          BIO_free_all(out); return false;
        }
      }
    }
    else {
      CredentialLogger.msg(ERROR, "Failed to get private key");
      BIO_free_all(out); return false;
    }

    for(;;) {
      char s[256];
      int l = BIO_read(out,s,sizeof(s));
      if(l <= 0) break;
      content.append(s,l);
    }
    BIO_free_all(out);
    return true;
  }

  bool Credential::OutputPublickey(std::string &content) {
    BIO *out = BIO_new(BIO_s_mem());
    if(!out) return false;
    if(rsa_key_ != NULL) {
      if(!PEM_write_bio_RSAPublicKey(out,rsa_key_)) {
        CredentialLogger.msg(ERROR, "Failed to get public key from RSA object");
        BIO_free_all(out); return false;
      };
    }
    else if(cert_ != NULL) {
      EVP_PKEY *pkey = NULL;
      pkey = X509_get_pubkey(cert_);
      if(pkey == NULL) {
        CredentialLogger.msg(ERROR, "Failed to get public key from X509 object");
        BIO_free_all(out); return false;
      };
      PEM_write_bio_PUBKEY(out, pkey);
      EVP_PKEY_free(pkey);
    }
    else {
      CredentialLogger.msg(ERROR, "Failed to get public key");
      BIO_free_all(out); return false;
    }

    for(;;) {
      char s[256];
      int l = BIO_read(out,s,sizeof(s));
      if(l <= 0) break;
      content.append(s,l);
    }
    BIO_free_all(out);
    return true;
  }

  bool Credential::OutputCertificate(std::string &content, bool if_der) {
    BIO *out = BIO_new(BIO_s_mem());
    if(!out) return false;
    if(if_der == false) {
      if(!PEM_write_bio_X509(out,cert_)) { BIO_free_all(out); return false; };
    }
    else {
      if(!i2d_X509_bio(out,cert_)) { BIO_free_all(out); return false; };
    }

    for(;;) {
      char s[256];
      int l = BIO_read(out,s,sizeof(s));
      if(l <= 0) break;
      content.append(s,l);
    }
    BIO_free_all(out);
    return true;
  }

  bool Credential::OutputCertificateChain(std::string &content, bool if_der) {
    BIO *out = BIO_new(BIO_s_mem());
    if(!out) return false;
    X509 *cert;
    CredentialLogger.msg(DEBUG, "Certiticate chain number %d",sk_X509_num(cert_chain_));
    //std::cout<<"+++++ cert chain number: "<<sk_X509_num(cert_chain_)<<std::endl;

    //Out put the cert chain. After the verification the cert_chain_
    //will include the CA certificate and the certificate (which
    //need to be verified here) itself.
    //Those two certificates are excluded when outputing
    for (int n = 1; n < sk_X509_num(cert_chain_) - 1 ; n++) {
      cert = sk_X509_value(cert_chain_, n);
      if(if_der == false) {
        if(!PEM_write_bio_X509(out,cert)) { BIO_free_all(out); return false; };
      }
      else {
        if(!i2d_X509_bio(out,cert)) { BIO_free_all(out); return false; };
      }

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

  //Inquire the input request bio to get PROXYCERTINFO, certType
  bool Credential::InquireRequest(BIO* reqbio, bool if_eec, bool if_der){
    bool res = false;
    if(reqbio == NULL) { CredentialLogger.msg(ERROR, "NULL BIO passed to InquireRequest"); return false; }
    if(req_) {X509_REQ_free(req_); req_ = NULL; }
    if((if_der == false) && (!(PEM_read_bio_X509_REQ(reqbio, &req_, NULL, NULL)))) {
      CredentialLogger.msg(ERROR, "PEM_read_bio_X509_REQ failed");
      LogError(); return false;
    }
    else if((if_der == true) && (!(d2i_X509_REQ_bio(reqbio, &req_)))) {
      CredentialLogger.msg(ERROR, "d2i_X509_REQ_bio failed");
      LogError(); return false;
    }

    STACK_OF(X509_EXTENSION)* req_extensions = NULL;
    X509_EXTENSION* ext;
    PROXYPOLICY*  policy = NULL;
    ASN1_OBJECT*  policy_lang = NULL;
    ASN1_OBJECT*  extension_oid = NULL;
    int certinfo_old_NID, certinfo_NID, nid = NID_undef;
    int i;

    //Get the PROXYCERTINFO from request' extension
    req_extensions = X509_REQ_get_extensions(req_);
    certinfo_old_NID = OBJ_txt2nid(PROXYCERTINFO_V3);
    certinfo_NID = OBJ_txt2nid(PROXYCERTINFO_V4);
    for(i=0;i<sk_X509_EXTENSION_num(req_extensions);i++) {
      ext = sk_X509_EXTENSION_value(req_extensions,i);
      extension_oid = X509_EXTENSION_get_object(ext);
      nid = OBJ_obj2nid(extension_oid);
      if((nid != 0) && (nid == certinfo_old_NID || nid == certinfo_NID)) {
        if(proxy_cert_info_) {
          PROXYCERTINFO_free(proxy_cert_info_);
          proxy_cert_info_ = NULL;
        }
        if((proxy_cert_info_ = (PROXYCERTINFO*)X509V3_EXT_d2i(ext)) == NULL) {
           CredentialLogger.msg(ERROR, "Can not convert DER encoded PROXYCERTINFO extension to internal format");
           LogError(); goto err;
        }
        break;
      }
    }

    if(proxy_cert_info_) {
      if((policy = PROXYCERTINFO_get_proxypolicy(proxy_cert_info_)) == NULL) {
        CredentialLogger.msg(ERROR, "Can not get policy from PROXYCERTINFO extension");
        LogError(); goto err;
      }
      if((policy_lang = PROXYPOLICY_get_policy_language(policy)) == NULL) {
        CredentialLogger.msg(ERROR, "Can not get policy language from PROXYCERTINFO extension");
        LogError(); goto err;
      }
      int policy_nid = OBJ_obj2nid(policy_lang);
      if((nid != 0) && (nid == certinfo_old_NID)) {
        if(policy_nid == OBJ_txt2nid(IMPERSONATION_PROXY_OID)) { cert_type_= CERT_TYPE_GSI_3_IMPERSONATION_PROXY; }
        else if(policy_nid == OBJ_txt2nid(INDEPENDENT_PROXY_OID)) { cert_type_ = CERT_TYPE_GSI_3_INDEPENDENT_PROXY; }
        else if(policy_nid == OBJ_txt2nid(LIMITED_PROXY_OID)) { cert_type_ = CERT_TYPE_GSI_3_LIMITED_PROXY; }
        else { cert_type_ = CERT_TYPE_GSI_3_RESTRICTED_PROXY; }
      }
      else {
        if(policy_nid == OBJ_txt2nid(IMPERSONATION_PROXY_OID)) { cert_type_ = CERT_TYPE_RFC_IMPERSONATION_PROXY; }
        else if(policy_nid == OBJ_txt2nid(INDEPENDENT_PROXY_OID)) { cert_type_ = CERT_TYPE_RFC_INDEPENDENT_PROXY; }
        else if(policy_nid == OBJ_txt2nid(ANYLANGUAGE_PROXY_OID)) { cert_type_ = CERT_TYPE_RFC_ANYLANGUAGE_PROXY; }
        else if(policy_nid == OBJ_txt2nid(LIMITED_PROXY_OID)) { cert_type_ = CERT_TYPE_RFC_LIMITED_PROXY; }
        else { cert_type_ = CERT_TYPE_RFC_RESTRICTED_PROXY; }
      }
    }
    //If can not find proxy_cert_info, depend on the parameters to distinguish the type: if
    //it is not eec request, then treat it as RFC proxy request
    else if(if_eec == false) { cert_type_ =  CERT_TYPE_RFC_INDEPENDENT_PROXY; } //CERT_TYPE_GSI_2_PROXY; }
    else { cert_type_ = CERT_TYPE_EEC; }

    CredentialLogger.msg(DEBUG,"Cert Type: %d",cert_type_);

    res = true;

err:
    if(req_extensions != NULL) { sk_X509_EXTENSION_pop_free(req_extensions, X509_EXTENSION_free); }

    return res;
  }

  bool Credential::InquireRequest(std::string &content, bool if_eec, bool if_der) {
    BIO *in;
    if(!(in = BIO_new_mem_buf((void*)(content.c_str()), content.length()))) {
      CredentialLogger.msg(ERROR, "Can not create BIO for parsing request");
      LogError(); return false;
    }

    if(InquireRequest(in,if_eec,if_der)) {
      CredentialLogger.msg(INFO, "Read request from a string");
    }
    else {
      CredentialLogger.msg(ERROR, "Failed to read request from a string");
      BIO_free_all(in); return false;
    }

    BIO_free_all(in);
    return true;
  }

  bool Credential::InquireRequest(const char* filename, bool if_eec, bool if_der) {
    BIO *in = BIO_new(BIO_s_file());
    if(!in) {
      CredentialLogger.msg(ERROR, "Can not create BIO for parsing request");
      LogError(); return false;
    }
    if (!(BIO_read_filename(in, (char*)filename))) {
      CredentialLogger.msg(ERROR, "Can not set readable file for request BIO");
      LogError(); BIO_free_all(in); return false;
    }

    if(InquireRequest(in,if_eec,if_der)) {
      CredentialLogger.msg(INFO, "Read request from a file");
    }
    else {
      CredentialLogger.msg(ERROR, "Failed to read request from a file");
      BIO_free_all(in); return false;
    }

    BIO_free_all(in);
    return true;
  }

  bool Credential::SetProxyPeriod(X509* tosign, X509* issuer, Time& start, Period& lifetime) {
    ASN1_UTCTIME* notBefore = NULL;
    ASN1_UTCTIME* notAfter = NULL;
    time_t t1 = start.GetTime();
    Time tmp = start + lifetime;
    time_t t2 = tmp.GetTime();

    //Set "notBefore"
    if( X509_cmp_time(X509_get_notBefore(issuer), &t1) < 0) {
      X509_time_adj(X509_get_notBefore(tosign), 0L, &t1);
    }
    else {
      X509_set_notBefore(tosign, X509_get_notBefore(issuer));
    }

    //Set "not After"
#ifdef WIN32  //Different process here for Win, because X509_cmp_time always return 0 on Win when the second calling, which is not as expected
    ASN1_UTCTIME*  atime = X509_get_notAfter(issuer);
    Time end = asn1_to_utctime(atime);
    if(Time(t2) < end)
      X509_gmtime_adj(X509_get_notAfter(tosign), t2-t1);
    else
      X509_gmtime_adj(X509_get_notAfter(tosign), end.GetTime()-t1);
/*
    ASN1_UTCTIME* atime = NULL;
    atime = X509_get_notAfter(tosign);
    Time end = asn1_to_utctime(atime);
    time_t end_t = end.GetTime();
    if( X509_cmp_time(X509_get_notAfter(issuer), &end_t) < 0) {
      X509_set_notAfter(tosign, X509_get_notAfter(issuer));
    } 
*/
#else
    if( X509_cmp_time(X509_get_notAfter(issuer), &t2) > 0) {
      X509_gmtime_adj(X509_get_notAfter(tosign), t2-t1);
    }
    else {
      X509_set_notAfter(tosign, X509_get_notAfter(issuer));
    }
#endif
 
    return true;
  }

  EVP_PKEY* Credential::GetPrivKey(void){
    EVP_PKEY* key = NULL;
    BIO*  bio = NULL;
    int length;
    bio = BIO_new(BIO_s_mem());
    if(pkey_ == NULL) {
      CredentialLogger.msg(ERROR, "Private key of the credential object is NULL");
      BIO_free(bio); return NULL;
    }
    length = i2d_PrivateKey_bio(bio, pkey_);
    if(length <= 0) {
      CredentialLogger.msg(ERROR, "Can not convert private key to DER format");
      LogError(); BIO_free(bio); return NULL;
    }
    key = d2i_PrivateKey_bio(bio, NULL);
    BIO_free(bio);

    return key;
  }

  EVP_PKEY* Credential::GetPubKey(void){
    EVP_PKEY* key = NULL;
    key = X509_get_pubkey(cert_);
    return key;
  }

  X509* Credential::GetCert(void) {
    X509* cert = NULL;
    if(cert_) cert = X509_dup(cert_);
    return cert;
  }

  STACK_OF(X509)* Credential::GetCertChain(void) {
    STACK_OF(X509)* chain = NULL;
    chain = sk_X509_new_null();
    //Return the cert chain (not including this certificate itself)
    for (int i=1; i < sk_X509_num(cert_chain_)-1; i++) {
      X509* tmp = X509_dup(sk_X509_value(cert_chain_,i));
      sk_X509_insert(chain, tmp, i);
    }
    return chain;
  }

  int Credential::GetCertNumofChain(void) {
    //Return the number of certificates
    //in the issuer chain
    return sk_X509_num(cert_chain_) - 2;
  }

  bool Credential::AddExtension(std::string name, std::string data, bool crit) {
    X509_EXTENSION* ext = NULL;
    ext = CreateExtension(name, data, crit);
    if(ext && sk_X509_EXTENSION_push(extensions_, ext)) return true;
    return false;
  }

  bool Credential::AddExtension(std::string name, char** binary, bool) {
    X509_EXTENSION* ext = NULL;
    if(binary == NULL) return false;
    ext = X509V3_EXT_conf_nid(NULL, NULL, OBJ_txt2nid((char*)(name.c_str())), (char*)binary);
    if(ext && sk_X509_EXTENSION_push(extensions_, ext)) return true;
    return false;
  }

  bool Credential::SignRequestAssistant(Credential* proxy, EVP_PKEY* req_pubkey, X509** tosign){

    bool res = false;
    X509* issuer = NULL;

    X509_NAME* subject_name = NULL;
    X509_NAME_ENTRY* name_entry = NULL;

    *tosign = NULL;
    char* CN_name = NULL;
    ASN1_INTEGER* serial_number = NULL;
    unsigned char* certinfo_data = NULL;
    X509_EXTENSION* certinfo_ext = NULL;
    X509_EXTENSION* ext = NULL;
    int certinfo_NID = NID_undef;

    if(cert_ == NULL) {
      CredentialLogger.msg(ERROR, "Credential is not initialized");
      goto err;
    }
    issuer = X509_dup(cert_);
    if((*tosign = X509_new()) == NULL) {
      CredentialLogger.msg(ERROR, "Failed to initialize X509 structure");
      LogError(); goto err;
    }

    //TODO: VOMS
    if(CERT_IS_GSI_3_PROXY(proxy->cert_type_)) { certinfo_NID = OBJ_txt2nid(PROXYCERTINFO_V3); }
    else if(CERT_IS_RFC_PROXY(proxy->cert_type_)) { certinfo_NID = OBJ_txt2nid(PROXYCERTINFO_V4); }

    if(proxy->cert_type_ == CERT_TYPE_GSI_2_LIMITED_PROXY){
      CN_name = const_cast<char*>("limited proxy");
      serial_number = M_ASN1_INTEGER_dup(X509_get_serialNumber(issuer));
    }
    else if(proxy->cert_type_ == CERT_TYPE_GSI_2_PROXY) {
      CN_name = const_cast<char*>("proxy");
      serial_number = M_ASN1_INTEGER_dup(X509_get_serialNumber(issuer));
    }
    else if (certinfo_NID != NID_undef) {
      unsigned char   md[SHA_DIGEST_LENGTH];
      long  sub_hash;
      unsigned int   len;
      X509V3_EXT_METHOD* ext_method = NULL;
      ext_method = X509V3_EXT_get_nid(certinfo_NID);
      if(ext_method == NULL) {
        CredentialLogger.msg(ERROR, "Can get X509V3_EXT_METHOD for %s",OBJ_nid2sn(certinfo_NID));
        LogError(); goto err;
      }

#if (OPENSSL_VERSION_NUMBER >= 0x0090800fL)
      ASN1_digest((int (*)(void*, unsigned char**))i2d_PUBKEY, EVP_sha1(), (char*)req_pubkey,md,&len);
#else
      ASN1_digest((int(*)())i2d_PUBKEY, EVP_sha1(), (char*)req_pubkey,md,&len);
#endif

      sub_hash = md[0] + (md[1] + (md[2] + (md[3] >> 1) * 256) * 256) * 256;

      CN_name = (char*)malloc(sizeof(long)*4 + 1);
      sprintf(CN_name, "%ld", sub_hash);

      //serial_number = ASN1_INTEGER_new();
      //ASN1_INTEGER_set(serial_number, sub_hash);
      //Use the serial number in the certificate as the
      //serial number in the proxy certificate
      serial_number = M_ASN1_INTEGER_dup(X509_get_serialNumber(issuer));

      int length = ext_method->i2d(proxy->proxy_cert_info_, NULL);
      if(length < 0) {
        CredentialLogger.msg(ERROR, "Can not convert PROXYCERTINFO struct from internal to DER encoded format");
        LogError();
      }
      else {certinfo_data = (unsigned char*)malloc(length); }

      unsigned char* derdata; derdata = certinfo_data;
      length = ext_method->i2d(proxy->proxy_cert_info_, &derdata);
      if(length < 0) {
        CredentialLogger.msg(ERROR, "Can not convert PROXYCERTINFO struct from internal to DER encoded format");
        free(certinfo_data); certinfo_data = NULL; LogError();
      }
      if(certinfo_data) {
        std::string certinfo_string((char*)certinfo_data, length); free(certinfo_data);
        certinfo_data = NULL;
        std::string NID_txt = PROXYCERTINFO_OPENSSL;
        certinfo_ext = CreateExtension(NID_txt, certinfo_string, 1);
      }
      if(certinfo_ext != NULL) {
        //if(!X509_add_ext(*tosign, certinfo_ext, 0)) {
        if(!sk_X509_EXTENSION_push(proxy->extensions_, certinfo_ext)) {
          CredentialLogger.msg(ERROR, "Can not add X509 extension to proxy cert");
          LogError(); goto err;
        }
      }
    }

    int position;

    /* Add any keyUsage and extendedKeyUsage extensions present in the issuer cert */
    if((position = X509_get_ext_by_NID(issuer, NID_key_usage, -1)) > -1) {
      ASN1_BIT_STRING*   usage;
      unsigned char *    ku_data = NULL;
      int ku_length;

      if(!(ext = X509_get_ext(issuer, position))) {
        CredentialLogger.msg(ERROR, "Can not get extension from issuer certificate");
        LogError(); goto err;
      }

      if(!(usage = (ASN1_BIT_STRING*)X509_get_ext_d2i(issuer, NID_key_usage, NULL, NULL))) {
        CredentialLogger.msg(ERROR, "Can not convert keyUsage struct from DER encoded format");
        LogError(); goto err;
      }

      /* clear bits specified in draft */
      ASN1_BIT_STRING_set_bit(usage, 1, 0); /* Non Repudiation */
      ASN1_BIT_STRING_set_bit(usage, 5, 0); /* Certificate Sign */

      ku_length = i2d_ASN1_BIT_STRING(usage, NULL);
      if(ku_length < 0) {
        CredentialLogger.msg(ERROR, "Can not convert keyUsage struct from internal to DER format");
        LogError(); ASN1_BIT_STRING_free(usage); goto err;
      }
      ku_data = (unsigned char*) malloc(ku_length);

      unsigned char* derdata; derdata = ku_data;
      ku_length = i2d_ASN1_BIT_STRING(usage, &derdata);
      if(ku_length < 0) {
        CredentialLogger.msg(ERROR, "Can not convert keyUsage struct from internal to DER format");
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
          CredentialLogger.msg(ERROR, "Can not add X509 extension to proxy cert");
          LogError(); X509_EXTENSION_free(ext); ext = NULL; goto err;
        }
      }
    }

    if((position = X509_get_ext_by_NID(issuer, NID_ext_key_usage, -1)) > -1) {
      if(!(ext = X509_get_ext(issuer, position))) {
        CredentialLogger.msg(ERROR, "Can not get extended KeyUsage extension from issuer certificate");
        LogError(); goto err;
      }

      ext = X509_EXTENSION_dup(ext);
      if(ext == NULL) {
        CredentialLogger.msg(ERROR, "Can not copy extended KeyUsage extension");
        LogError(); goto err;
      }

      //if(!X509_add_ext(*tosign, ext, 0)) {
      if(!sk_X509_EXTENSION_push(proxy->extensions_, ext)) {
        CredentialLogger.msg(ERROR, "Can not add X509 extended KeyUsage extension to new proxy certificate");
        LogError(); X509_EXTENSION_free(ext); ext = NULL; goto err;
      }
    }

    /* Create proxy subject name */
    if((subject_name = X509_NAME_dup(X509_get_subject_name(issuer))) == NULL) {
      CredentialLogger.msg(ERROR, "Can not copy the subject name from issuer for proxy certificate");
      goto err;
    }
    if((name_entry = X509_NAME_ENTRY_create_by_NID(&name_entry, NID_commonName, V_ASN1_APP_CHOOSE,
                                     (unsigned char *) CN_name, -1)) == NULL) {
      CredentialLogger.msg(ERROR, "Can not create name entry CN for proxy certificate");
      LogError(); X509_NAME_free(subject_name); goto err;
    }
    if(!X509_NAME_add_entry(subject_name, name_entry, X509_NAME_entry_count(subject_name), 0) ||
       !X509_set_subject_name(*tosign, subject_name)) {
      CredentialLogger.msg(ERROR, "Can not set CN in proxy certificate");
      LogError(); X509_NAME_ENTRY_free(name_entry); goto err;
    }

    X509_NAME_free(subject_name); subject_name = NULL;
    X509_NAME_ENTRY_free(name_entry);

    if(!X509_set_issuer_name(*tosign, X509_get_subject_name(issuer))) {
      CredentialLogger.msg(ERROR, "Can not set issuer's subject for proxy certificate");
      LogError(); goto err;
    }

    if(!X509_set_version(*tosign, 2L)) {
      CredentialLogger.msg(ERROR, "Can not set version number for proxy certificate");
      LogError(); goto err;
    }

    if(!X509_set_serialNumber(*tosign, serial_number)) {
      CredentialLogger.msg(ERROR, "Can not set serial number for proxy certificate");
      LogError(); goto err;
    }

    if(!SetProxyPeriod(*tosign, issuer, proxy->start_, proxy->lifetime_)) {
      CredentialLogger.msg(ERROR, "Can not set the lifetime for proxy certificate"); goto err;
    }

    if(!X509_set_pubkey(*tosign, req_pubkey)) {
      CredentialLogger.msg(ERROR, "Can not set pubkey for proxy certificate");
      LogError(); goto err;
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

  bool Credential::SignRequest(Credential* proxy, BIO* outputbio, bool if_der){
    bool res = false;
    if(proxy == NULL) {CredentialLogger.msg(ERROR, "The credential to be signed is NULL"); return false; }
    if(proxy->req_ == NULL) {CredentialLogger.msg(ERROR, "The credential to be signed contains no request"); return false; }
    if(outputbio == NULL) { CredentialLogger.msg(ERROR, "The BIO for output is NULL"); return false; }

    EVP_PKEY* issuer_priv = NULL;
    EVP_PKEY* issuer_pub = NULL;
    X509*  proxy_cert = NULL;
    X509_CINF*  cert_info = NULL;
    X509_EXTENSION* ext = NULL;
    EVP_PKEY* req_pubkey = NULL;
    req_pubkey = X509_REQ_get_pubkey(proxy->req_);

    if(!req_pubkey) {
      CredentialLogger.msg(ERROR, "Error when extracting public key from request");
      LogError(); return false;
    }

    if(!X509_REQ_verify(proxy->req_, req_pubkey)){
      CredentialLogger.msg(ERROR,"Failed to verify the request"); LogError(); goto err;
    }

    if(!SignRequestAssistant(proxy, req_pubkey, &proxy_cert)) {
      CredentialLogger.msg(ERROR,"Failed to add issuer's extension into proxy");
      LogError(); goto err;
    }

    /*Add the extensions which has just been added by application,
     * into the proxy_cert which will be signed soon.
     * Note here we suppose it is the signer who will add the
     * extension to to-signed proxy and then sign it;
     * it also could be the request who add the extension and put
     * it inside X509 request' extension, but here the situation
     * has not been considered for now
     */
    cert_info = proxy_cert->cert_info;
    if (cert_info->extensions != NULL) {
      sk_X509_EXTENSION_pop_free(cert_info->extensions, X509_EXTENSION_free);
    }

    /*Set the serialNumber*/
    //cert_info->serialNumber = M_ASN1_INTEGER_dup(X509_get_serialNumber(proxy_cert));;

    /*Set the extension*/
    if(sk_X509_EXTENSION_num(proxy->extensions_)) {
      cert_info->extensions = sk_X509_EXTENSION_new_null();
    }

    for (int i=0; i<sk_X509_EXTENSION_num(proxy->extensions_); i++) {
      //ext = X509_EXTENSION_dup(sk_X509_EXTENSION_value(proxy->extensions_, i));
      ext = sk_X509_EXTENSION_value(proxy->extensions_, i);
      if (ext == NULL) {
        //CredentialLogger.msg(ERROR,"Failed to duplicate extension"); LogError(); goto err;
        CredentialLogger.msg(ERROR,"Failed to find extension"); LogError(); goto err;
      }

      if (!sk_X509_EXTENSION_push(cert_info->extensions, ext)) {
        CredentialLogger.msg(ERROR,"Failed to add extension into proxy"); LogError(); goto err;
      }
    }

    /*Clean extensions attached to "proxy" after it has been linked into
    to-signed certificate*/
    sk_X509_EXTENSION_zero(proxy->extensions_);

    /* Now sign the new certificate */
    if(!(issuer_priv = GetPrivKey())) {
      CredentialLogger.msg(ERROR, "Can not get the issuer's private key"); goto err;
    }

    /* Check whether SHA1 isn't requested as the signing algorithm in the request*/
    if(EVP_MD_type(proxy->signing_alg_) != NID_sha1) {
      CredentialLogger.msg(ERROR, "The signing algorithm %s is not allowed,it should be SHA1 to sign certificate requests",
      OBJ_nid2sn(EVP_MD_type(proxy->signing_alg_)));
      goto err;
    }

    if(!X509_sign(proxy_cert, issuer_priv, proxy->signing_alg_)) {
      CredentialLogger.msg(ERROR, "Failed to sign the proxy certificate"); LogError(); goto err;
    }
    else CredentialLogger.msg(INFO, "Succeeded to sign the proxy certificate");

    /*Verify the signature, not needed later*/
    issuer_pub = GetPubKey();
    if((X509_verify(proxy_cert, issuer_pub)) != 1) {
      CredentialLogger.msg(ERROR, "Failed to verify the signed certificate"); LogError(); goto err;
    }
    else CredentialLogger.msg(INFO, "Succeeded to verify the signed certificate");

    /*Output the signed certificate into BIO*/
    if(if_der == false) {
      if(PEM_write_bio_X509(outputbio, proxy_cert)) {
        CredentialLogger.msg(INFO, "Output the proxy certificate"); res = true;
      }
      else {
        CredentialLogger.msg(ERROR, "Can not convert signed proxy cert into PEM format");
        LogError();
      }
    }
    else {
      if(i2d_X509_bio(outputbio, proxy_cert)) {
        CredentialLogger.msg(INFO, "Output the proxy certificate"); res = true;
      }
      else {
        CredentialLogger.msg(ERROR, "Can not convert signed proxy cert into DER format");
        LogError();
      }
    }

err:
    if(issuer_priv) { EVP_PKEY_free(issuer_priv);}
    if(proxy_cert) { X509_free(proxy_cert);}
    if(req_pubkey) { EVP_PKEY_free(req_pubkey); }
    if(issuer_pub) { EVP_PKEY_free(issuer_pub); }
    return res;
  }

  bool Credential::SignRequest(Credential* proxy, std::string &content, bool if_der) {
    BIO *out = BIO_new(BIO_s_mem());
    if(!out) {
      CredentialLogger.msg(ERROR, "Can not create BIO for signed proxy certificate");
      LogError(); return false;
    }

    if(SignRequest(proxy, out, if_der)) {
      for(;;) {
        char s[256];
        int l = BIO_read(out,s,sizeof(s));
        if(l <= 0) break;
        content.append(s,l);
      }
    } else {
      BIO_free_all(out);
      return false;
    }

    BIO_free_all(out);
    return true;
  }

  bool Credential::SignRequest(Credential* proxy, const char* filename, bool if_der) {
    BIO *out = BIO_new(BIO_s_file());
    if(!out) {
      CredentialLogger.msg(ERROR, "Can not create BIO for signed proxy certificate");
      LogError(); return false;
    }
    if (!(BIO_write_filename(out, (char*)filename))) {
      CredentialLogger.msg(ERROR, "Can not set writable file for signed proxy certificate BIO");
      LogError(); BIO_free_all(out); return false;
    }

    if(SignRequest(proxy, out, if_der)) {
      CredentialLogger.msg(INFO, "Wrote signed proxy certificate into a file");
    }
    else {
      CredentialLogger.msg(ERROR, "Failed to write signed proxy certificate into a file");
      BIO_free_all(out); return false;
    }

    BIO_free_all(out);
    return true;
  }

 //The following is the methods about how to use a CA credential to sign an EEC

  Credential::Credential(const std::string& CAcertfile, const std::string& CAkeyfile,
       const std::string& CAserial, bool CAcreateserial, const std::string& extfile,
       const std::string& extsect, const std::string& passphrase4key) : certfile_(CAcertfile), keyfile_(CAkeyfile),
       CAserial_(CAserial), CAcreateserial_(CAcreateserial), extfile_(extfile), extsect_(extsect),
       cert_(NULL), pkey_(NULL), cert_chain_(NULL), proxy_cert_info_(NULL),
       req_(NULL), rsa_key_(NULL), signing_alg_((EVP_MD*)EVP_sha1()), keybits_(1024), extensions_(NULL) {
    OpenSSLInit();

    InitVerification();

    //Initiate the proxy certificate constant and  method which is required by openssl
    if(!proxy_init_) InitProxyCertInfo();

    extensions_ = sk_X509_EXTENSION_new_null();

    try {
      loadCertificateFile(CAcertfile, cert_, &cert_chain_);
      if(cert_) check_cert_type(cert_,cert_type_);
      loadKeyFile(CAkeyfile, pkey_, passphrase4key);
    } catch(std::exception& err){
      CredentialLogger.msg(ERROR, "ERROR:%s", err.what());
      LogError();
    }
  }

  static void print_ssl_errors() {
    unsigned long err;
    while(err = ERR_get_error()) {
      CredentialLogger.msg(ERROR,"SSL error: %s, libs: %s, func: %s, reason: %s",
        ERR_error_string(err, NULL),ERR_lib_error_string(err),
        ERR_func_error_string(err),ERR_reason_error_string(err));
    }
  }

#define SERIAL_RAND_BITS        64
  int rand_serial(BIGNUM *b, ASN1_INTEGER *ai) {
    BIGNUM *btmp;
    int ret = 0;
    if (b)
      btmp = b;
    else
      btmp = BN_new();
    if (!btmp)
      return 0;
    if (!BN_pseudo_rand(btmp, SERIAL_RAND_BITS, 0, 0))
      goto error;
    if (ai && !BN_to_ASN1_INTEGER(btmp, ai))
      goto error;
    ret = 1;
error:
    if (!b)
      BN_free(btmp);
    return ret;
  }

#undef BSIZE
#define BSIZE 256
  BIGNUM *load_serial(char *serialfile, int create, ASN1_INTEGER **retai) {
    BIO *in=NULL;
    BIGNUM *ret=NULL;
    char buf[1024];
    ASN1_INTEGER *ai=NULL;

    ai=ASN1_INTEGER_new();
    if (ai == NULL) goto err;

    if ((in=BIO_new(BIO_s_file())) == NULL) {
      print_ssl_errors();
      goto err;
    }

    if (BIO_read_filename(in,serialfile) <= 0) {
      if (!create) {
        perror(serialfile);
        goto err;
      }
      else {
        ret=BN_new();
        if (ret == NULL || !rand_serial(ret, ai))
          CredentialLogger.msg(ERROR,"Out of memory");
      }
    }
    else {
      if (!a2i_ASN1_INTEGER(in,ai,buf,1024)) {
        CredentialLogger.msg(ERROR,"unable to load number from: %s",serialfile);
        goto err;
      }
      ret=ASN1_INTEGER_to_BN(ai,NULL);
      if (ret == NULL) {
        CredentialLogger.msg(ERROR, "error converting number from bin to BIGNUM");
        goto err;
      }
    }

    if (ret && retai) {
      *retai = ai;
      ai = NULL;
    }
err:
    if (in != NULL) BIO_free(in);
    if (ai != NULL) ASN1_INTEGER_free(ai);
      return(ret);
  }

  int save_serial(char *serialfile, char *suffix, BIGNUM *serial, ASN1_INTEGER **retai) {
    char buf[1][BSIZE];
    BIO *out = NULL;
    int ret=0;
    ASN1_INTEGER *ai=NULL;
    int j;

    if (suffix == NULL)
      j = strlen(serialfile);
    else
      j = strlen(serialfile) + strlen(suffix) + 1;
    if (j >= BSIZE) {
      CredentialLogger.msg(ERROR,"file name too long");
      goto err;
    }

    if (suffix == NULL)
      BUF_strlcpy(buf[0], serialfile, BSIZE);
    else {
#ifndef OPENSSL_SYS_VMS
      j = BIO_snprintf(buf[0], sizeof buf[0], "%s.%s", serialfile, suffix);
#else
      j = BIO_snprintf(buf[0], sizeof buf[0], "%s-%s", serialfile, suffix);
#endif
    }
    out=BIO_new(BIO_s_file());
    if (out == NULL) {
      print_ssl_errors();
      goto err;
    }
    if (BIO_write_filename(out,buf[0]) <= 0) {
      perror(serialfile);
      goto err;
    }
    if ((ai=BN_to_ASN1_INTEGER(serial,NULL)) == NULL) {
      CredentialLogger.msg(ERROR,"error converting serial to ASN.1 format");
      goto err;
    }
    i2a_ASN1_INTEGER(out,ai);
    BIO_puts(out,"\n");
    ret=1;
    if (retai) {
      *retai = ai;
      ai = NULL;
    }
err:
    if (out != NULL) BIO_free_all(out);
    if (ai != NULL) ASN1_INTEGER_free(ai);
    return(ret);
  }

#undef POSTFIX
#define POSTFIX ".srl"
  static ASN1_INTEGER *x509_load_serial(char *CAfile, char *serialfile, int create) {
    char *buf = NULL, *p;
    ASN1_INTEGER *bs = NULL;
    BIGNUM *serial = NULL;
    size_t len;

    len = ((serialfile == NULL)
            ?(strlen(CAfile)+strlen(POSTFIX)+1)
            :(strlen(serialfile)))+1;
    buf=(char*)(OPENSSL_malloc(len));
    if (buf == NULL) { CredentialLogger.msg(ERROR,"out of memory"); goto end; }
    if (serialfile == NULL) {
      BUF_strlcpy(buf,CAfile,len);
      for (p=buf; *p; p++)
        if (*p == '.') {
          *p='\0';
          break;
        }
      BUF_strlcat(buf,POSTFIX,len);
    }
    else
      BUF_strlcpy(buf,serialfile,len);

    serial = load_serial(buf, create, NULL);
    if (serial == NULL) goto end;

    if (!BN_add_word(serial,1)) {
      CredentialLogger.msg(ERROR,"add_word failure"); goto end;
    }

    if (!save_serial(buf, NULL, serial, &bs)) goto end;

 end:
    if (buf) OPENSSL_free(buf);
    BN_free(serial);
    return bs;
  }

  static int x509_certify(X509_STORE *ctx, char *CAfile, const EVP_MD *digest,
             X509 *x, X509 *xca, EVP_PKEY *pkey, char *serialfile, int create,
             long lifetime, int clrext, CONF *conf, char *section, ASN1_INTEGER *sno) {
    int ret=0;
    ASN1_INTEGER *bs=NULL;
    X509_STORE_CTX xsc;
    EVP_PKEY *upkey;

    upkey = X509_get_pubkey(xca);
    EVP_PKEY_copy_parameters(upkey,pkey);
    EVP_PKEY_free(upkey);

    if(!X509_STORE_CTX_init(&xsc,ctx,x,NULL)) {
      CredentialLogger.msg(ERROR,"Error initialising X509 store");
      goto end;
    }
    if (sno) bs = sno;
    else if (!(bs = x509_load_serial(CAfile, serialfile, create)))
      goto end;

    X509_STORE_CTX_set_cert(&xsc,x);
    //if (!X509_verify_cert(&xsc))
    //  goto end;

    if (!X509_check_private_key(xca,pkey)) {
      CredentialLogger.msg(ERROR,"CA certificate and CA private key do not match");
      goto end;
    }

    if (!X509_set_issuer_name(x,X509_get_subject_name(xca))) goto end;
    if (!X509_set_serialNumber(x,bs)) goto end;

    if (X509_gmtime_adj(X509_get_notBefore(x),0L) == NULL)
      goto end;

    /* hardwired expired */
    if (X509_gmtime_adj(X509_get_notAfter(x), lifetime) == NULL)
      goto end;

    if (clrext) {
      while (X509_get_ext_count(x) > 0) X509_delete_ext(x, 0);
    }

    if (conf) {
      X509V3_CTX ctx2;
      X509_set_version(x,2); /* version 3 certificate */
      X509V3_set_ctx(&ctx2, xca, x, NULL, NULL, 0);
      X509V3_set_nconf(&ctx2, conf);
      if (!X509V3_EXT_add_nconf(conf, &ctx2, section, x)) goto end;
    }

    if (!X509_sign(x,pkey,digest)) goto end;
      ret=1;
end:
    X509_STORE_CTX_cleanup(&xsc);
    if (!ret)
      ERR_clear_error();
    if (!sno) ASN1_INTEGER_free(bs);

    return ret;
  }


 /*subject is expected to be in the format /type0=value0/type1=value1/type2=...
 * where characters may be escaped by \
 */
  X509_NAME * Credential::parse_name(char *subject, long chtype, int multirdn) {
    size_t buflen = strlen(subject)+1;
    /* to copy the types and values into. due to escaping, the copy can only become shorter */
    char *buf = (char*)(OPENSSL_malloc(buflen));
    size_t max_ne = buflen / 2 + 1; /* maximum number of name elements */
    char **ne_types =  (char **)(OPENSSL_malloc(max_ne * sizeof (char *)));
    char **ne_values = (char **)(OPENSSL_malloc(max_ne * sizeof (char *)));
    int *mval = (int*)(OPENSSL_malloc (max_ne * sizeof (int)));

    char *sp = subject, *bp = buf;
    int i, ne_num = 0;

    X509_NAME *n = NULL;
    int nid;

    if (!buf || !ne_types || !ne_values) {
      CredentialLogger.msg(ERROR,"malloc error");
      goto error;
    }
    if (*subject != '/') {
      CredentialLogger.msg(ERROR,"Subject does not start with '/'");
      goto error;
    }
    sp++; /* skip leading / */

    /* no multivalued RDN by default */
    mval[ne_num] = 0;
    while (*sp) {
      /* collect type */
      ne_types[ne_num] = bp;
      while (*sp) {
        if (*sp == '\\') /* is there anything to escape in the type...? */
        {
          if (*++sp)
          *bp++ = *sp++;
          else {
            CredentialLogger.msg(ERROR,"escape character at end of string");
            goto error;
          }
        }
        else if (*sp == '=') {
          sp++;
          *bp++ = '\0';
          break;
        }
        else  *bp++ = *sp++;
      }
      if (!*sp) {
        CredentialLogger.msg(ERROR,"end of string encountered while processing type of subject name element #%d",ne_num);
        goto error;
      }
      ne_values[ne_num] = bp;
      while (*sp) {
        if (*sp == '\\') {
          if (*++sp)
            *bp++ = *sp++;
          else {
            CredentialLogger.msg(ERROR,"escape character at end of string");
            goto error;
          }
        }
        else if (*sp == '/') {
          sp++;
          /* no multivalued RDN by default */
          mval[ne_num+1] = 0;
          break;
        }
        else if (*sp == '+' && multirdn) {
          /* a not escaped + signals a mutlivalued RDN */
          sp++;
          mval[ne_num+1] = -1;
          break;
        }
        else
          *bp++ = *sp++;
      }
      *bp++ = '\0';
      ne_num++;
    }

    if (!(n = X509_NAME_new()))
      goto error;

    for (i = 0; i < ne_num; i++) {
      if ((nid=OBJ_txt2nid(ne_types[i])) == NID_undef) {
        CredentialLogger.msg(ERROR,"Subject Attribute %s has no known NID, skipped",ne_types[i]);
        continue;
      }
      if (!*ne_values[i]) {
        CredentialLogger.msg(ERROR,"No value provided for Subject Attribute %s skipped",ne_types[i]);
        continue;
      }

      if (!X509_NAME_add_entry_by_NID(n, nid, chtype, (unsigned char*)ne_values[i], -1,-1,mval[i]))
        goto error;
    }

    OPENSSL_free(mval);
    OPENSSL_free(ne_values);
    OPENSSL_free(ne_types);
    OPENSSL_free(buf);
    return n;

error:
    X509_NAME_free(n);
    if (ne_values)
      OPENSSL_free(ne_values);
    if (ne_types)
      OPENSSL_free(ne_types);
    if (buf)
      OPENSSL_free(buf);
    return NULL;
  }

  bool Credential::SignEECRequest(Credential* eec, const std::string& dn, BIO* outputbio) {
    if(pkey_ == NULL) { CredentialLogger.msg(ERROR, "The private key for signing is not initialized"); return false; }
    bool res = false;
    if(eec == NULL) { CredentialLogger.msg(ERROR, "The credential to be signed is NULL"); return false; }
    if(eec->req_ == NULL) { CredentialLogger.msg(ERROR, "The credential to be signed contains no request"); return false; }
    if(outputbio == NULL) { CredentialLogger.msg(ERROR, "The BIO for output is NULL"); return false; }

    X509*  eec_cert = NULL;
    EVP_PKEY* req_pubkey = NULL;
    req_pubkey = X509_REQ_get_pubkey(eec->req_);
    if(!req_pubkey) { CredentialLogger.msg(ERROR, "Error when extracting public key from request");
      LogError(); return false;
    }
    if(!X509_REQ_verify(eec->req_, req_pubkey)){
      CredentialLogger.msg(ERROR,"Failed to verify the request");
      LogError(); return false;
    }
    eec_cert = X509_new();
    X509_set_pubkey(eec_cert, req_pubkey);
    EVP_PKEY_free(req_pubkey);

    X509_set_issuer_name(eec_cert,eec->req_->req_info->subject);
    //X509_set_subject_name(eec_cert,eec->req_->req_info->subject);

    X509_NAME *subject = NULL;
    unsigned long chtype = MBSTRING_ASC;  //TODO
    subject = parse_name((char*)(dn.c_str()), chtype, 0);
    X509_set_subject_name(eec_cert, subject);
    X509_NAME_free(subject);

    const EVP_MD *digest=EVP_sha1();
#ifndef OPENSSL_NO_DSA
    if (pkey_->type == EVP_PKEY_DSA)
      digest=EVP_dss1();
#endif
/*
#ifndef OPENSSL_NO_ECDSA
    if (pkey_->type == EVP_PKEY_EC)
      digest = EVP_ecdsa();
#endif
*/
    X509_STORE *ctx=NULL;
    ctx=X509_STORE_new();
    //X509_STORE_set_verify_cb_func(ctx,callb);
    if (!X509_STORE_set_default_paths(ctx)) {
      LogError();
    }

    CONF *extconf = NULL;
    if (!extfile_.empty()) {
      long errorline = -1;
      X509V3_CTX ctx2;
      extconf = NCONF_new(NULL);
      //configuration file with X509V3 extensions to add
      if (!NCONF_load(extconf, extfile_.c_str(),&errorline)) {
        if (errorline <= 0) {
          CredentialLogger.msg(ERROR,"Error when loading the extension config file: %s", extfile_.c_str());
          return false;
        }
        else {
          CredentialLogger.msg(ERROR,"Error when loading the extension config file: %s on line: %d",
             extfile_.c_str(), errorline);
        }
      }
      //section from config file with X509V3 extensions to add
      if (extsect_.empty()) {
        extsect_.append(NCONF_get_string(extconf, "default", "extensions"));
        if (extsect_.empty()) {
          ERR_clear_error();
          extsect_ = "default";
        }
      }
      X509V3_set_ctx_test(&ctx2);
      X509V3_set_nconf(&ctx2, extconf);
      if (!X509V3_EXT_add_nconf(extconf, &ctx2, (char*)(extsect_.c_str()), NULL)) {
        CredentialLogger.msg(ERROR,"Error when loading the extension section: %s",
          extsect_.c_str());
        LogError();
      }
    }

    //Add extensions to certificate object
    X509_EXTENSION* ext = NULL;
    X509_CINF*  cert_info = NULL;
    cert_info = eec_cert->cert_info;
    if (cert_info->extensions != NULL) {
      sk_X509_EXTENSION_pop_free(cert_info->extensions, X509_EXTENSION_free);
    }
    if(sk_X509_EXTENSION_num(eec->extensions_)) {
      cert_info->extensions = sk_X509_EXTENSION_new_null();
    }
    for (int i=0; i<sk_X509_EXTENSION_num(eec->extensions_); i++) {
      ext = X509_EXTENSION_dup(sk_X509_EXTENSION_value(eec->extensions_, i));
      if (ext == NULL) {
        CredentialLogger.msg(ERROR,"Failed to duplicate extension"); LogError();
      }

      if (!sk_X509_EXTENSION_push(cert_info->extensions, ext)) {
        CredentialLogger.msg(ERROR,"Failed to add extension into EEC certificate"); LogError();
      }
    }

    long lifetime = 12*60*60; //Default lifetime 12 hours
    if (!x509_certify(ctx,(char*)(certfile_.c_str()), digest, eec_cert, cert_,
                      pkey_, (char*)(CAserial_.c_str()), CAcreateserial_, lifetime, 0,
                      extconf, (char*)(extsect_.c_str()), NULL)) {
      CredentialLogger.msg(ERROR,"Can not sign a EEC"); LogError();
    }

    if(PEM_write_bio_X509(outputbio, eec_cert)) {
      CredentialLogger.msg(INFO, "Output EEC certificate"); res = true;
    }
    else {
      CredentialLogger.msg(ERROR, "Can not convert signed EEC cert into DER format");
      LogError();
    }

    NCONF_free(extconf);
    X509_free(eec_cert);
    X509_STORE_free(ctx);

    return res;
  }

  bool Credential::SignEECRequest(Credential* eec, const std::string& dn, std::string &content) {
    BIO *out = BIO_new(BIO_s_mem());
    if(!out) {
      CredentialLogger.msg(ERROR, "Can not create BIO for signed EEC certificate");
      LogError(); return false;
    }

    if(SignEECRequest(eec, dn, out)) {
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

  bool Credential::SignEECRequest(Credential* eec, const std::string& dn, const char* filename) {
    BIO *out = BIO_new(BIO_s_file());
    if(!out) {
      CredentialLogger.msg(ERROR, "Can not create BIO for signed EEC certificate");
      LogError(); return false;
    }
    if (!(BIO_write_filename(out, (char*)filename))) {
      CredentialLogger.msg(ERROR, "Can not set writable file for signed EEC certificate BIO");
      LogError(); BIO_free_all(out); return false;
    }

    if(SignEECRequest(eec, dn, out)) {
      CredentialLogger.msg(INFO, "Wrote signed EEC certificate into a file");
    }
    else {
      CredentialLogger.msg(ERROR, "Failed to write signed EEC certificate into a file");
      BIO_free_all(out); return false;
    }
    BIO_free_all(out);
    return true;
  }

  Credential::~Credential() {
    if(cert_) X509_free(cert_);
    if(pkey_) EVP_PKEY_free(pkey_);
    if(cert_chain_) sk_X509_pop_free(cert_chain_, X509_free);
    if(proxy_cert_info_) PROXYCERTINFO_free(proxy_cert_info_);
    if(req_) X509_REQ_free(req_);
    if(rsa_key_) RSA_free(rsa_key_);
    if(extensions_) sk_X509_EXTENSION_pop_free(extensions_, X509_EXTENSION_free);
    if(verify_ctx_.cert_chain) sk_X509_pop_free(verify_ctx_.cert_chain, X509_free);
  }

}

