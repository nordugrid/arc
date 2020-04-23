/**Some of the following code is compliant to OpenSSL license*/
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <cstdlib>
#include <cstring>
#include <vector>

#include <fstream>
#include <fcntl.h>
#include <iostream>
#include <openssl/x509v3.h>

#include <glibmm/fileutils.h>

#include <arc/Thread.h>
#include <arc/Utils.h>
#include <arc/User.h>
#include <arc/crypto/OpenSSL.h>

#include <arc/credential/VOMSUtil.h>

#include "Credential.h"

using namespace ArcCredential;

namespace Arc {

#if (OPENSSL_VERSION_NUMBER < 0x10100000L)

static BN_GENCB* BN_GENCB_new(void) {
  BN_GENCB* bn = (BN_GENCB*)std::malloc(sizeof(BN_GENCB));
  if(bn) std::memset(bn, 0, sizeof(BN_GENCB));
  return bn;
}

static void BN_GENCB_free(BN_GENCB* bn) {
  if(bn) std::free(bn);
}

#define X509_getm_notAfter X509_get_notAfter
#define X509_getm_notBefore X509_get_notBefore
#define X509_set1_notAfter X509_set_notAfter
#define X509_set1_notBefore X509_set_notBefore

static const unsigned char* ASN1_STRING_get0_data(const ASN1_STRING* x) {
    return x->data;
}

#endif


#if (OPENSSL_VERSION_NUMBER < 0x10002000L)

static int X509_get_signature_nid(const X509 *x) {
    return OBJ_obj2nid(x->sig_alg->algorithm);
}

static void X509_get0_signature(ASN1_BIT_STRING **psig, X509_ALGOR **palg, const X509 *x) {
    if (psig) *psig = x->signature;
    if (palg) *palg = x->sig_alg;
}

#endif



  #define DEFAULT_DIGEST   ((EVP_MD*)EVP_sha1())
  #define DEFAULT_KEYBITS  (2048)

  CredentialError::CredentialError(const std::string& what) : std::runtime_error(what) { }

  Logger CredentialLogger(Logger::rootLogger, "Credential");

  static int ssl_err_cb(const char *str, size_t, void *u) {
    Logger& logger = *((Logger*)u);
    logger.msg(DEBUG, "OpenSSL error string: %s", str);
    return 1;
  }

#define PASS_MIN_LENGTH (0)
  typedef struct pw_cb_data {
    PasswordSource *password;
  } PW_CB_DATA;

  static int passwordcb(char *buf, int bufsiz, int verify, void *cb_tmp) {
    PW_CB_DATA *cb_data = (PW_CB_DATA *)cb_tmp;

    if (bufsiz <= 1) return 0;
    if (cb_data) {
      if (cb_data->password) {
        std::string password;
        if(cb_data->password->Get(password,PASS_MIN_LENGTH,bufsiz) != PasswordSource::PASSWORD) {
          // It was requested to have key encrypted and no password was provided
          return 0;
        }
        if(buf) strncpy(buf, password.c_str(), bufsiz);
        int len = password.length();
        if(len > bufsiz) len = bufsiz;
        return len;
      }
    }
    // Password is needed but no source defined for password
    return 0;
  }

  void Credential::LogError(void) const {
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

  ASN1_UTCTIME* utc_to_asn1time(const Time& t) {
    // Using ASN1_UTCTIME instead of ASN1_GENERALIZEDTIME because of dCache
    std::string t_str = t.str(MDSTime);
    if(t_str.length() < 2) return NULL; // paranoic
    ASN1_UTCTIME* s = ASN1_UTCTIME_new();
    if(!s) return NULL;
    if(ASN1_UTCTIME_set_string(s,(char*)(t_str.c_str()+2))) return s;
    ASN1_UTCTIME_free(s);
    return NULL;
    //ASN1_GENERALIZEDTIME* s = ASN1_GENERALIZEDTIME_new();
    //if(!s) return NULL;
    //std::string t_str = t.str(MDSTime);
    //if(ASN1_GENERALIZEDTIME_set_string(s,(char*)t_str.c_str())) return s;
    //ASN1_GENERALIZEDTIME_free(s);
    //return NULL;
  }

  class AutoBIO {
   private:
    BIO* bio_;
   public:
    AutoBIO(BIO* bio):bio_(bio) { };
    ~AutoBIO(void) { if(bio_) { BIO_set_close(bio_,BIO_CLOSE); BIO_free_all(bio_); } };
    operator bool(void) const { return (bio_ != NULL); };
    operator BIO*(void) const { return bio_; };
    BIO& operator*(void) const { return *bio_; };
    BIO* operator->(void) const { return bio_; };
  };

  //Get the life time of the credential
  static void getLifetime(STACK_OF(X509)* certchain, X509* cert, Time& start, Period &lifetime) {
    Time start_time(-1), end_time(-1);
    ASN1_UTCTIME* atime = NULL;

    if(cert == NULL) {
      start = Time();
      lifetime = Period();
      return;
    }

    if(certchain) for (int n = 0; n < sk_X509_num(certchain); n++) {
      X509* tmp_cert = sk_X509_value(certchain, n);

      atime = X509_getm_notAfter(tmp_cert);
      Time e = asn1_to_utctime(atime);
      if (end_time == Time(-1) || e < end_time) { end_time = e; }

      atime = X509_getm_notBefore(tmp_cert);
      Time s = asn1_to_utctime(atime);
      if (start_time == Time(-1) || s > start_time) { start_time = s; }
    }

    atime = X509_getm_notAfter(cert);
    Time e = asn1_to_utctime(atime);
    if (end_time == Time(-1) || e < end_time) { end_time = e; }

    atime = X509_getm_notBefore(cert);
    Time s = asn1_to_utctime(atime);
    if (start_time == Time(-1) || s > start_time) { start_time = s; }

    start = start_time;
    lifetime = end_time - start_time;
  }

  //Parse the BIO for certificate and get the format of it
  Credformat Credential::getFormat_BIO(BIO* bio, const bool is_file) const {
    Credformat format = CRED_UNKNOWN;
    if(bio == NULL) return format;
    if(is_file) {
      char buf[1];
      char firstbyte;
      int position;
      if((position = BIO_tell(bio))<0 || BIO_read(bio, buf, 1)<=0 || BIO_seek(bio, position)<0) {
        LogError();
        CredentialLogger.msg(ERROR,"Can't get the first byte of input to determine its format");
        return format;
      }
      firstbyte = buf[0];
      // DER-encoded structure (including PKCS12) will start with ASCII 048.
      // Otherwise, it's PEM.
      if(firstbyte==48) {
        //DER-encoded, PKCS12 or DER? firstly parse it as PKCS12 ASN.1,
        //if can not parse it, then it is DER format
        PKCS12* pkcs12 = NULL;
        if((pkcs12 = d2i_PKCS12_bio(bio,NULL)) == NULL){ format=CRED_DER; }
        else { format = CRED_PKCS; PKCS12_free(pkcs12); }
        if( BIO_seek(bio, position) < 0 ) {
          LogError();
          CredentialLogger.msg(ERROR,"Can't reset the input");
          return format;
        }
      }
      else { format = CRED_PEM; }
    }
    else {
      unsigned char* bio_str;
      int len;
      len = BIO_get_mem_data(bio, (unsigned char *) &bio_str);
      if(len>0) {
        char firstbyte = bio_str[0];
        if(firstbyte==48)  {
          //DER-encoded, PKCS12 or DER? firstly parse it as PKCS12 ASN.1,
          //if can not parse it, then it is DER format
          AutoBIO pkcs12bio(BIO_new_mem_buf(bio_str,len));
          PKCS12* pkcs12 = NULL;
          if((pkcs12 = d2i_PKCS12_bio(pkcs12bio,NULL)) == NULL){
            format=CRED_DER;
          } else {
            format = CRED_PKCS; PKCS12_free(pkcs12);
          }
        } else { format = CRED_PEM; }
      }
      else {
        CredentialLogger.msg(ERROR,"Can't get the first byte of input BIO to get its format");
        return format;
      }
    }
    return format;
  }

  //Parse the string for certificate and get the format of it
  Credformat Credential::getFormat_str(const std::string& source) const {
    Credformat format = CRED_UNKNOWN;
    AutoBIO bio(BIO_new_mem_buf((void*)(source.c_str()), source.length()));
    if(!bio){
      CredentialLogger.msg(ERROR,"Can not read certificate/key string");
      LogError();
    }
    if(!bio) return format;

    unsigned char* bio_str;
    int len;
    len = BIO_get_mem_data(bio, (unsigned char *) &bio_str);
    if(len>0) {
      char firstbyte = bio_str[0];
      if(firstbyte==48)  {
        //DER-encoded, PKCS12 or DER? firstly parse it as PKCS12 ASN.1,
        //if can not parse it, then it is DER format
        PKCS12* pkcs12 = NULL;
        unsigned char* source_chr = (unsigned char*)(source.c_str());
        if((pkcs12 = d2i_PKCS12(NULL, (const unsigned char**)&source_chr, source.length())) != NULL){
          format=CRED_PKCS; PKCS12_free(pkcs12);
        } else {
          format = CRED_DER;
        }
      }
      else { format = CRED_PEM; }
    }
    else {
      CredentialLogger.msg(ERROR,"Can't get the first byte of input BIO to get its format");
      return format;
    }

    return format;
  }

  std::string Credential::GetDN(void) const {
    X509_NAME *subject = NULL;
    if(!cert_) return "";
    subject = X509_get_subject_name(cert_);
    std::string str;
    if(subject!=NULL) {
      char* buf = X509_NAME_oneline(subject,NULL,0);
      if(buf) {
        str.append(buf);
        OPENSSL_free(buf);
      }
    }
    return str;
  }

  std::string Credential::GetIdentityName(void) const {
    // TODO: it is more correct to go through chain till first non-proxy cert
    X509_NAME *subject = NULL;
    if(!cert_) return "";
    subject = X509_NAME_dup(X509_get_subject_name(cert_));

    ASN1_STRING* entry;
    std::string entry_str;
    for(;;) {
      X509_NAME_ENTRY *ne = X509_NAME_get_entry(subject, X509_NAME_entry_count(subject)-1);
      if (!OBJ_cmp(X509_NAME_ENTRY_get_object(ne),OBJ_nid2obj(NID_commonName))) {
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
    if(subject!=NULL) {
      char* buf = X509_NAME_oneline(subject,NULL,0);
      if(buf) {
        str.append(buf);
        OPENSSL_free(buf);
      }
      X509_NAME_free(subject);
    }
    return str;
  }

  certType Credential::GetType(void) const {
    return cert_type_;
  }

  std::string Credential::GetIssuerName(void) const {
    X509_NAME *issuer = NULL;
    if(!cert_) return "";
    issuer = X509_get_issuer_name(cert_);
    std::string str;
    if(issuer!=NULL) {
      char* buf = X509_NAME_oneline(issuer,NULL,0);
      if(buf) {
        str.append(buf);
        OPENSSL_free(buf);
      }
    }
    return str;
  }

  std::string Credential::GetCAName(void) const {
    if(!cert_chain_) return "";
    int num = sk_X509_num(cert_chain_);
    std::string str;
    if(num > 0) {
      // This works even if last cert on chain is CA
      // itself because CA is self-signed.
      X509 *cacert = sk_X509_value(cert_chain_, num-1);
      X509_NAME *caname = X509_get_issuer_name(cacert);
      if(caname!=NULL) {
        char* buf = X509_NAME_oneline(caname,NULL,0);
        if(buf) {
          str.append(buf);
          OPENSSL_free(buf);
        }
      }
    }
    return str;
  }

  std::string Credential::GetProxyPolicy(void) const {
    return verification_proxy_policy;
  }

  Period Credential::GetLifeTime(void) const {
    return lifetime_;
  }

  Time Credential::GetStartTime() const {
    return start_;
  }

  Time Credential::GetEndTime() const {
    return start_+lifetime_;
  }

  Signalgorithm Credential::GetSigningAlgorithm(void) const {
    Signalgorithm signing_algorithm = SIGN_DEFAULT;
    if(!cert_) return signing_algorithm;
    int sig_nid = X509_get_signature_nid(cert_);
    switch(sig_nid) {
      case NID_sha1WithRSAEncryption: signing_algorithm = SIGN_SHA1; break;
      case NID_sha224WithRSAEncryption: signing_algorithm = SIGN_SHA224; break;
      case NID_sha256WithRSAEncryption: signing_algorithm = SIGN_SHA256; break;
      case NID_sha384WithRSAEncryption: signing_algorithm = SIGN_SHA384; break;
      case NID_sha512WithRSAEncryption: signing_algorithm = SIGN_SHA512; break;
    }
    return signing_algorithm;
  }

  int Credential::GetKeybits(void) const {
    int keybits = 0;
    if(!cert_) return keybits;
    EVP_PKEY* pkey = X509_get_pubkey(cert_);
    if(!pkey) return keybits;
    keybits = EVP_PKEY_bits(pkey);
    return keybits;
  }

  void Credential::SetLifeTime(const Period& period) {
    lifetime_ = period;
  }

  void Credential::SetStartTime(const Time& start_time) {
    start_ = start_time;
  }

  void Credential::SetSigningAlgorithm(Signalgorithm signing_algorithm) {
    switch(signing_algorithm) {
      case SIGN_SHA1: signing_alg_ = ((EVP_MD*)EVP_sha1()); break;
      case SIGN_SHA224: signing_alg_ = ((EVP_MD*)EVP_sha224()); break;
      case SIGN_SHA256: signing_alg_ = ((EVP_MD*)EVP_sha256()); break;
      case SIGN_SHA384: signing_alg_ = ((EVP_MD*)EVP_sha384()); break;
      case SIGN_SHA512: signing_alg_ = ((EVP_MD*)EVP_sha512()); break;
      default: signing_alg_ = NULL; break;
    }
  }

  void Credential::SetKeybits(int keybits) {
    keybits_ = keybits;
  }

  bool Credential::IsCredentialsValid(const UserConfig& usercfg) {
    return
    Credential(!usercfg.ProxyPath().empty() ? usercfg.ProxyPath() : usercfg.CertificatePath(),
               !usercfg.ProxyPath().empty() ? ""                  : usercfg.KeyPath(),
               usercfg.CACertificatesDirectory(), usercfg.CACertificatePath()).IsValid();
  }

  bool Credential::IsValid(void) {
    Time t;
    return verification_valid && (GetStartTime() <= t) && (t < GetEndTime());
  }

  static BIO* OpenFileBIO(const std::string& file) {
    if(!Glib::file_test(file,Glib::FILE_TEST_IS_REGULAR)) return NULL;
    return  BIO_new_file(file.c_str(), "r");
  }

  void Credential::loadCertificateFile(const std::string& certfile, X509* &x509, STACK_OF(X509) **certchain) {
    BIO* b = OpenFileBIO(certfile);
    if(!b) {
        CredentialLogger.msg(ERROR,"Can not find certificate file: %s", certfile);
        throw CredentialError("Can not find certificate file");
    }
    AutoBIO certbio(b);
    if(!certbio){
      CredentialLogger.msg(ERROR,"Can not read certificate file: %s", certfile);
      LogError();
      throw CredentialError("Can not read certificate file");
    }

    std::string certstr;
    for(;;) {
      char s[256];
      int l = BIO_read(certbio,s,sizeof(s));
      if(l <= 0) break;
      certstr.append(s,l);
    }

    loadCertificateString(certstr,x509,certchain);
  }

  static bool matchCertificate(X509* tmp, X509* x509) {
    if((X509_cmp(tmp, x509) == 0) && // only hash is checked by X509_cmp
       (X509_issuer_and_serial_cmp(tmp, x509) == 0) &&
       (X509_subject_name_cmp(tmp, x509) == 0)) return true;
    return false;
  }

  static bool matchCertificate(X509* tmp, STACK_OF(X509) *certchain) {
    int nn = 0;
    for(; nn < sk_X509_num(certchain) ; nn++) {
      X509* ccert = sk_X509_value(certchain, nn);
      if((X509_cmp(tmp, ccert) == 0) && // only hash is checked by X509_cmp
         (X509_issuer_and_serial_cmp(tmp, ccert) == 0) &&
         (X509_subject_name_cmp(tmp, ccert) == 0)) break;
    }
    if(nn < sk_X509_num(certchain)) return true;
    return false;
  }

  void Credential::loadCertificateString(const std::string& cert, X509* &x509, STACK_OF(X509) **certchain) {
    AutoBIO certbio(BIO_new_mem_buf((void*)(cert.c_str()), cert.length()));
    if(!certbio){
      CredentialLogger.msg(ERROR,"Can not read certificate string");
      LogError();
      throw CredentialError("Can not read certificate string");
    }

    //Parse the certificate
    Credformat format = CRED_UNKNOWN;

    if(!certbio) return;
    format = getFormat_str(cert);
    int n;
    if(*certchain) {
      sk_X509_pop_free(*certchain, X509_free);
      *certchain = NULL;
    }

    unsigned char* pkcs_chr;

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
          // Gross hack - fight users which concatenate their certificates in loop
          // Filter out certificates which are already present.
          if(matchCertificate(tmp, *certchain) || matchCertificate(tmp, x509)) continue; // duplicate - skip
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
        x509 = d2i_X509_bio(certbio, NULL);
        if(!x509){
          throw CredentialError("Unable to read DER credential from BIO");
        }
        //Get the issuer chain
        *certchain = sk_X509_new_null();
        n = 0;
        while(!BIO_eof(certbio)){
          X509 * tmp = NULL;
          if(!(tmp = d2i_X509_bio(certbio, NULL))){
            ERR_clear_error(); break;
          }
          // Gross hack - fight users which concatenate their certificates in loop
          // Filter out certificates which are already present.
          if(matchCertificate(tmp, *certchain) || matchCertificate(tmp, x509)) continue; // duplicate - skip
          if(!sk_X509_insert(*certchain, tmp, n)) {
            //std::string str(X509_NAME_oneline(X509_get_subject_name(tmp),0,0));
            X509_free(tmp);
            throw CredentialError("Can not insert cert into certificate's issuer chain");
          }
          ++n;
        }
        break;

      case CRED_PKCS:
        {
          PKCS12* pkcs12 = NULL;
          STACK_OF(X509)* pkcs12_certs = NULL;
          CredentialLogger.msg(DEBUG,"Certificate format is PKCS");
          pkcs_chr = (unsigned char*)(cert.c_str());
          pkcs12 = d2i_PKCS12(NULL, (const unsigned char**)&pkcs_chr, cert.length());
          if(pkcs12){
            char password[100];
            EVP_read_pw_string(password, 100, "Enter Password for PKCS12 certificate:", 0);
            if(!PKCS12_parse(pkcs12, password, &pkey_, &x509, &pkcs12_certs)) {
              if(pkcs12) PKCS12_free(pkcs12);
              throw CredentialError("Can not parse PKCS12 file");
            }
          }
          else{
            throw CredentialError("Can not read PKCS12 credential from BIO");
          }
          if (pkcs12_certs && sk_X509_num(pkcs12_certs)){
            for (n = 0; n < sk_X509_num(pkcs12_certs); n++) {
              X509* tmp = X509_dup(sk_X509_value(pkcs12_certs, n));
              sk_X509_insert(*certchain, tmp, n);
            }
          }
          if(pkcs12) { PKCS12_free(pkcs12); }
          if(pkcs12_certs) { sk_X509_pop_free(pkcs12_certs, X509_free); }
        }
        break;

      default:
        CredentialLogger.msg(DEBUG,"Certificate format is unknown");
        break;
     } // end switch
  }

  void Credential::loadKeyFile(const std::string& keyfile, EVP_PKEY* &pkey, PasswordSource& passphrase) {
    BIO* b = OpenFileBIO(keyfile);
    if(!b) {
        CredentialLogger.msg(ERROR,"Can not find key file: %s", keyfile);
        throw CredentialError("Can not find key file");
    }
    AutoBIO keybio(b);
    if(!keybio){
      CredentialLogger.msg(ERROR,"Can not open key file %s", keyfile);
      LogError();
      throw CredentialError("Can not open key file " + keyfile);
    }

    std::string keystr;
    for(;;) {
      char s[256];
      int l = BIO_read(keybio,s,sizeof(s));
      if(l <= 0) break;
      keystr.append(s,l);
    }

    loadKeyString(keystr,pkey,passphrase);
  }

  void Credential::loadKeyString(const std::string& key, EVP_PKEY* &pkey, PasswordSource& passphrase) {
    AutoBIO keybio(BIO_new_mem_buf((void*)(key.c_str()), key.length()));
    if(!keybio){
      CredentialLogger.msg(ERROR,"Can not read key string");
      LogError();
      throw CredentialError("Can not read key string");
    }

    //Read key
    Credformat format;

    if(!keybio) return;
    format = getFormat_str(key);

    unsigned char* key_chr;

    switch(format){
      case CRED_PEM: {
        PW_CB_DATA cb_data;
        cb_data.password = &passphrase;
        if(!(pkey = PEM_read_bio_PrivateKey(keybio, NULL, passwordcb, &cb_data))) {
          int reason = ERR_GET_REASON(ERR_peek_error());
          if(reason == PEM_R_BAD_BASE64_DECODE) 
            throw CredentialError("Can not read PEM private key: probably bad password");
          if(reason == PEM_R_BAD_DECRYPT)
            throw CredentialError("Can not read PEM private key: failed to decrypt");
          if(reason == PEM_R_BAD_PASSWORD_READ)
            throw CredentialError("Can not read PEM private key: failed to obtain password");
          if(reason == PEM_R_PROBLEMS_GETTING_PASSWORD)
            throw CredentialError("Can not read PEM private key: failed to obtain password");
          throw CredentialError("Can not read PEM private key");
        }
        } break;

      case CRED_DER:
        key_chr = (unsigned char*)(key.c_str());
        pkey=d2i_PrivateKey(EVP_PKEY_RSA, NULL, (const unsigned char**)&key_chr, key.length());
        break;

      default:
        break;
    }
  }

  static bool proxy_init_ = false;

  void Credential::InitProxyCertInfo(void) {
    static Glib::Mutex lock_;

    // At least in some versions of OpenSSL functions manupulating
    // global lists seems to be not thread-safe despite locks
    // installed (tested for 0.9.7). Hence it is safer to protect
    // such calls.
    // It is also good idea to protect proxy_init_ too.

    Glib::Mutex::Lock lock(lock_);
    if(proxy_init_) return;

    /* Proxy Certificate Extension's related objects */

    // none

    // This library provides methods and objects which when registred in
    // global OpenSSL lists can't be unregistred anymore. Hence it must not
    // be allowed to unload.
    if(!PersistentLibraryInit("modcredential")) {
      CredentialLogger.msg(WARNING, "Failed to lock arccredential library in memory");
    };
    proxy_init_=true;
  }

  void Credential::AddCertExtObj(std::string& sn, std::string& oid) {
    OBJ_create(oid.c_str(), sn.c_str(), sn.c_str());
  }

  bool Credential::Verify(void) {
    verification_proxy_policy.clear();
    if(verify_cert_chain(cert_, &cert_chain_, cacertfile_, cacertdir_, verification_proxy_policy)) {
      CredentialLogger.msg(VERBOSE, "Certificate verification succeeded");
      verification_valid = true;
      return true;
    }
    else { CredentialLogger.msg(INFO, "Certificate verification failed"); LogError(); return false;}
  }

  Credential::Credential() : verification_valid(false), cert_(NULL), pkey_(NULL),
        cert_chain_(NULL), proxy_cert_info_(NULL), format(CRED_UNKNOWN),
        start_(Time()), lifetime_(Period("PT12H")),
        req_(NULL), rsa_key_(NULL), signing_alg_(NULL), keybits_(0),
        proxyver_(0), pathlength_(0), extensions_(NULL) {

    OpenSSLInit();

    extensions_ = sk_X509_EXTENSION_new_null();
    if(!extensions_) {
      CredentialLogger.msg(ERROR, "Failed to initialize extensions member for Credential");
      return;
    }

    //Initiate the proxy certificate constant and  method which is required by openssl
    if(!proxy_init_) InitProxyCertInfo();
  }

  Credential::Credential(const int keybits) : verification_valid(false),
    cert_(NULL), pkey_(NULL), cert_chain_(NULL), proxy_cert_info_(NULL),
    start_(Time()), lifetime_(Period("PT12H")),
    req_(NULL), rsa_key_(NULL), signing_alg_(NULL), keybits_(keybits),
    extensions_(NULL) {

    OpenSSLInit();

    extensions_ = sk_X509_EXTENSION_new_null();
    if(!extensions_) {
      CredentialLogger.msg(ERROR, "Failed to initialize extensions member for Credential");
      return;
    }

    //Initiate the proxy certificate constant and  method which is required by openssl
    if(!proxy_init_) InitProxyCertInfo();
  }

  Credential::Credential(Time start, Period lifetime, int keybits, std::string proxyversion,
        std::string policylang, std::string policy, int pathlength) :
        verification_valid(false), cert_(NULL), pkey_(NULL), cert_chain_(NULL), proxy_cert_info_(NULL),
        start_(start), lifetime_(lifetime), req_(NULL), rsa_key_(NULL),
        signing_alg_(NULL), keybits_(keybits), extensions_(NULL) {

    OpenSSLInit();

    extensions_ = sk_X509_EXTENSION_new_null();
    if(!extensions_) {
      CredentialLogger.msg(ERROR, "Failed to initialize extensions member for Credential");
      return;
    }

    //Initiate the proxy certificate constant and  method which is required by openssl
    if(!proxy_init_) InitProxyCertInfo();

    SetProxyPolicy(proxyversion, policylang, policy, pathlength);
  }

  void Credential::SetProxyPolicy(const std::string& proxyversion, const std::string& policylang,
        const std::string& policy, int pathlength) {

    proxyversion_ = proxyversion;
    policy_ = policy;
    pathlength_ = pathlength;
    if(proxy_cert_info_) {
      PROXY_CERT_INFO_EXTENSION_free(proxy_cert_info_);
      proxy_cert_info_ = NULL;
    }

    //Get certType
    if (proxyversion_.compare("RFC") == 0 || proxyversion_.compare("rfc") == 0) {
      //The "limited" and "restricted" are from the definition in
      //http://dev.globus.org/wiki/Security/ProxyCertTypes#RFC_3820_Proxy_Certificates
      if(policylang.compare("LIMITED") == 0 || policylang.compare("limited") == 0) {
        cert_type_ = CERT_TYPE_RFC_LIMITED_PROXY;
      }
      else if(policylang.compare("RESTRICTED") == 0 || policylang.compare("restricted") == 0) {
        cert_type_ = CERT_TYPE_RFC_RESTRICTED_PROXY;
      }
      else if(policylang.compare("INDEPENDENT") == 0 || policylang.compare("independent") == 0) {
        cert_type_ = CERT_TYPE_RFC_INDEPENDENT_PROXY;
      }
      else if(policylang.compare("IMPERSONATION") == 0 || policylang.compare("impersonation") == 0 ||
         policylang.compare("INHERITALL") == 0 || policylang.compare("inheritAll") == 0){
        cert_type_ = CERT_TYPE_RFC_IMPERSONATION_PROXY;
        //For RFC here, "impersonation" is the same as the "inheritAll" in openssl version>098
      }
      else if(policylang.compare("ANYLANGUAGE") == 0 || policylang.compare("anylanguage") == 0) {
        cert_type_ = CERT_TYPE_RFC_ANYLANGUAGE_PROXY;  //Defined in openssl version>098
      }
      else {
        CredentialLogger.msg(ERROR,"Unsupported proxy policy language is requested - %s", policylang);
        proxyversion_.clear();
        policy_.clear();
        pathlength_ = -1;
        cert_type_ = CERT_TYPE_CA;
        return;
      }
    }
    else if (proxyversion_.compare("EEC") == 0 || proxyversion_.compare("eec") == 0) {
      cert_type_ = CERT_TYPE_EEC;
    }
    else {
      CredentialLogger.msg(ERROR,"Unsupported proxy version is requested - %s", proxyversion_);
      proxyversion_.clear();
      policy_.clear();
      pathlength_ = -1;
      cert_type_ = CERT_TYPE_CA;
      return;
    }

    if(cert_type_ != CERT_TYPE_EEC) {
      // useless check but keep for future extensions
      if(!policy_.empty() && policylang.empty()) {
        CredentialLogger.msg(ERROR,"If you specify a policy you also need to specify a policy language");
        return;
      }

      proxy_cert_info_ = PROXY_CERT_INFO_EXTENSION_new();
      PROXY_POLICY *   ppolicy =PROXY_CERT_INFO_EXTENSION_get_proxypolicy(proxy_cert_info_);
      PROXY_POLICY_set_policy(ppolicy, NULL, 0);
      ASN1_OBJECT *   policy_object = NULL;

      //set policy language, see definiton in: http://dev.globus.org/wiki/Security/ProxyCertTypes
      switch(cert_type_)
      {
        case CERT_TYPE_RFC_IMPERSONATION_PROXY:
          if((policy_object = OBJ_nid2obj(IMPERSONATION_PROXY_NID)) != NULL) {
            PROXY_POLICY_set_policy_language(ppolicy, policy_object);
          }
          break;

        case CERT_TYPE_RFC_INDEPENDENT_PROXY:
          if((policy_object = OBJ_nid2obj(INDEPENDENT_PROXY_NID)) != NULL) {
            PROXY_POLICY_set_policy_language(ppolicy, policy_object);
          }
          break;

        case CERT_TYPE_RFC_ANYLANGUAGE_PROXY:
          if((policy_object = OBJ_nid2obj(ANYLANGUAGE_PROXY_NID)) != NULL) {
            PROXY_POLICY_set_policy_language(ppolicy, policy_object);
          }
          break;
        default:
          break;
      }

      //set path length constraint
      if(pathlength >= 0)
        PROXY_CERT_INFO_EXTENSION_set_path_length(proxy_cert_info_, pathlength_);

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
                PROXY_CERT_INFO_EXTENSION_free(proxy_cert_info_);
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
              PROXY_CERT_INFO_EXTENSION_free(proxy_cert_info_);
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

      ppolicy = PROXY_CERT_INFO_EXTENSION_get_proxypolicy(proxy_cert_info_);
      //Here only consider the situation when there is policy specified
      if(policystring.size() > 0) {
        //PROXYPOLICY_set_policy_language(ppolicy, policy_object);
        PROXY_POLICY_set_policy(ppolicy, (unsigned char*)policystring.c_str(), policystring.size());
      }

    }
  }

  Credential::Credential(const std::string& certfile, const std::string& keyfile,
        const std::string& cadir, const std::string& cafile,
        PasswordSource& passphrase4key, const bool is_file) {
    InitCredential(certfile,keyfile,cadir,cafile,passphrase4key,is_file);
  }

  Credential::Credential(const std::string& certfile, const std::string& keyfile,
        const std::string& cadir, const std::string& cafile,
        const std::string& passphrase4key, const bool is_file) {
    PasswordSource* pass = NULL;
    if(passphrase4key.empty()) {
      pass = new PasswordSourceInteractive("private key", false);
    } else if(passphrase4key[0] == '\0') {
      pass = new PasswordSourceString("");
    } else {
      pass = new PasswordSourceString(passphrase4key);
    }
    InitCredential(certfile,keyfile,cadir,cafile,*pass,is_file);
    delete pass;
  }

  Credential::Credential(const UserConfig& usercfg, PasswordSource& passphrase4key) {
    if (usercfg.CredentialString().empty()) {
      InitCredential(!usercfg.ProxyPath().empty() ? usercfg.ProxyPath() : usercfg.CertificatePath(),
                     !usercfg.ProxyPath().empty() ? ""                  : usercfg.KeyPath(),
                     usercfg.CACertificatesDirectory(), usercfg.CACertificatePath(),
                     passphrase4key, true);
    } else {
      InitCredential(usercfg.CredentialString(), "",
                     usercfg.CACertificatesDirectory(), usercfg.CACertificatePath(),
                     passphrase4key, false);
    }
  }

  Credential::Credential(const UserConfig& usercfg, const std::string& passphrase4key) {
    PasswordSource* pass = NULL;
    if(passphrase4key.empty()) {
      pass = new PasswordSourceInteractive("private key", false);
    } else if(passphrase4key[0] == '\0') {
      pass = new PasswordSourceString("");
    } else {
      pass = new PasswordSourceString(passphrase4key);
    }
    if (usercfg.CredentialString().empty()) {
      InitCredential(!usercfg.ProxyPath().empty() ? usercfg.ProxyPath() : usercfg.CertificatePath(),
                     !usercfg.ProxyPath().empty() ? ""                  : usercfg.KeyPath(),
                     usercfg.CACertificatesDirectory(), usercfg.CACertificatePath(),
                     *pass, true);
    } else {
      InitCredential(usercfg.CredentialString(), "",
                     usercfg.CACertificatesDirectory(), usercfg.CACertificatePath(),
                     *pass, false);
    }
    delete pass;
  }

  void Credential::InitCredential(const std::string& certfile, const std::string& keyfile,
        const std::string& cadir, const std::string& cafile,
        PasswordSource& passphrase4key, const bool is_file) {

    cacertfile_ = cafile;
    cacertdir_ = cadir;
    certfile_ = certfile;
    keyfile_ = keyfile;
    verification_valid = false;
    cert_ = NULL;
    pkey_ = NULL;
    cert_chain_ = NULL;
    proxy_cert_info_ = NULL;
    req_ = NULL;
    rsa_key_ = NULL;
    signing_alg_ = NULL;
    keybits_ = 0;
    proxyver_ = 0;
    pathlength_ = 0;
    extensions_ = NULL;

    OpenSSLInit();

    extensions_ = sk_X509_EXTENSION_new_null();
    if(!extensions_) {
      CredentialLogger.msg(ERROR, "Failed to initialize extensions member for Credential");
      return;
    }

    if(certfile.empty()) {
      CredentialLogger.msg(ERROR, "Certificate/Proxy path is empty");
      return;
    }
    
    //Initiate the proxy certificate constant and  method which is required by openssl
    if(!proxy_init_) InitProxyCertInfo();

    try {
      if(is_file) {
        loadCertificateFile(certfile, cert_, &cert_chain_);
        if(cert_) check_cert_type(cert_,cert_type_);
        if(keyfile.empty()) {
          //Detect if the certificate file/string contains private key.
          //If the key file is absent, and the private key is not contained inside
          //certificate file/string, then the certificate file will not
          //be parsed for private key.
          //Note this detection only applies to PEM file
          std::string keystr;
          // Since the certfile file has been loaded in the call to
          // loadCertificateFile, it is redundant to check if it exist.
          // loadCertificateFile will throw an exception if the file does not
          // exist.
          std::ifstream in(certfile.c_str(), std::ios::in);
          std::getline<char>(in, keystr, 0);
          in.close();
          if(keystr.find("BEGIN RSA PRIVATE KEY") != std::string::npos) {
            loadKeyFile(certfile, pkey_, passphrase4key);
          }
        }
        else {
          loadKeyFile(keyfile, pkey_, passphrase4key);
        }
      } else {
        loadCertificateString(certfile, cert_, &cert_chain_);
        if(cert_) check_cert_type(cert_,cert_type_);
        if(keyfile.empty()) {
          std::string keystr;
          keystr = certfile;
          if(keystr.find("BEGIN RSA PRIVATE KEY") != std::string::npos) {
            loadKeyString(certfile, pkey_, passphrase4key);
          }
        }
        else {
          loadKeyString(keyfile, pkey_, passphrase4key);
        }
      }
    } catch(std::exception& err){
      CredentialLogger.msg(ERROR, "%s", err.what());
      LogError(); //return;
    }

    //Get the lifetime of the credential
    getLifetime(cert_chain_, cert_, start_, lifetime_);

    if(cert_) {
      for (int i=0; i<X509_get_ext_count(cert_); i++) {
        X509_EXTENSION* ext = X509_EXTENSION_dup(X509_get_ext(cert_, i));
        if (ext == NULL) {
          CredentialLogger.msg(ERROR,"Failed to duplicate extension");
          LogError(); break; //return;
        }
        if (!sk_X509_EXTENSION_push(extensions_, ext)) {
          CredentialLogger.msg(ERROR,"Failed to add extension into credential extensions"); 
          X509_EXTENSION_free(ext);
          LogError(); break;
        }
      }
    }

    if(!cacertfile_.empty() || !cacertdir_.empty()) {
      Verify();
    } else {
      if(!collect_cert_chain(cert_, &cert_chain_, verification_proxy_policy)) {
        CredentialLogger.msg(INFO, "Certificate information collection failed");
        LogError();
      }
    }
  }

  static int keygen_cb(int p, int, BN_GENCB*) {
    char c='*';
    if (p == 0) c='.';
    if (p == 1) c='+';
    if (p == 2) c='*';
    if (p == 3) c='\n';
    std::cerr<<c;
    return 1;
  }


  X509_EXTENSION* Credential::CreateExtension(const std::string& name, const std::string& data, bool crit) {
    bool numberic = true;
    size_t pos1 = 0;
    do {
      size_t pos2 = name.find(".", pos1);
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

    ASN1_OBJECT* ext_obj = NULL;
    if(!numberic && !(ext_obj = OBJ_nid2obj(OBJ_txt2nid(name.c_str())))) {
      //string format, the OID should have been registered before calling OBJ_nid2obj
      CredentialLogger.msg(ERROR, "Can not convert string into ASN1_OBJECT");
      LogError(); return NULL;
    }
    else if(numberic && !(ext_obj = OBJ_txt2obj(name.c_str(), 1))) {
      //numerical format, the OID will be registered here if it has not been registered before
      CredentialLogger.msg(ERROR, "Can not convert string into ASN1_OBJECT");
      LogError();
      return NULL;
    }

    ASN1_OCTET_STRING* ext_oct = ASN1_OCTET_STRING_new();
    if(!ext_oct) {
      CredentialLogger.msg(ERROR, "Can not create ASN1_OCTET_STRING");
      LogError();
      if(ext_obj) ASN1_OBJECT_free(ext_obj);
      return NULL;
    }

    //ASN1_OCTET_STRING_set(ext_oct, data.c_str(), data.size());
    ext_oct->data = (unsigned char*) malloc(data.size());
    if(!(ext_oct->data)) {
      CredentialLogger.msg(ERROR, "Can not allocate memory for extension for proxy certificate");
      if(ext_oct) ASN1_OCTET_STRING_free(ext_oct);
      if(ext_obj) ASN1_OBJECT_free(ext_obj);
      return NULL;
    }
    memcpy(ext_oct->data, data.c_str(), data.size());
    ext_oct->length = data.size();

    X509_EXTENSION* ext = NULL;
    if (!(ext = X509_EXTENSION_create_by_OBJ(NULL, ext_obj, crit, ext_oct))) {
      CredentialLogger.msg(ERROR, "Can not create extension for proxy certificate");
      LogError();
      if(ext_oct) ASN1_OCTET_STRING_free(ext_oct);
      if(ext_obj) ASN1_OBJECT_free(ext_obj);
      return NULL;
    }

    // TODO: ASN1_OCTET_STRING_free is not working correctly
    //      on Windows Vista, bugreport: 1587

    if(ext_oct) ASN1_OCTET_STRING_free(ext_oct);

    if(ext_obj) ASN1_OBJECT_free(ext_obj);

    return ext;
  }

  X509_REQ* Credential::GetCertReq(void) const {
    return req_;
  }

  bool Credential::GenerateEECRequest(BIO* reqbio, BIO* /*keybio*/, const std::string& dn) {
    bool res = false;
    RSA* rsa_key = NULL;
    const EVP_MD *digest = signing_alg_?signing_alg_:DEFAULT_DIGEST;
    EVP_PKEY* pkey;
    int keybits = keybits_?keybits_:DEFAULT_KEYBITS;

    BN_GENCB* cb = BN_GENCB_new();
    BIGNUM *prime = BN_new();
    rsa_key = RSA_new();

    BN_GENCB_set(cb,&keygen_cb,NULL);
    if(prime && rsa_key) {
      int val1 = BN_set_word(prime,RSA_F4);
      if(val1 != 1) {
        CredentialLogger.msg(ERROR, "BN_set_word failed");
        LogError();
        if(cb) BN_GENCB_free(cb);
        if(prime) BN_free(prime);
        if(rsa_key) RSA_free(rsa_key);
        return false;
      }
      int val2 = RSA_generate_key_ex(rsa_key, keybits, prime, cb);
      if(val2 != 1) {
        CredentialLogger.msg(ERROR, "RSA_generate_key_ex failed");
        LogError();
        if(cb) BN_GENCB_free(cb);
        if(prime) BN_free(prime);
        if(rsa_key) RSA_free(rsa_key);
        return false;
      }
    }
    else {
      CredentialLogger.msg(ERROR, "BN_new || RSA_new failed");
      LogError();
      if(cb) BN_GENCB_free(cb);
      if(prime) BN_free(prime);
      if(rsa_key) RSA_free(rsa_key);
      return false;
    }
    if(cb) BN_GENCB_free(cb);
    if(prime) BN_free(prime);

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
              name = parse_name((char*)(dn.c_str()), MBSTRING_ASC, 0);
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

    if(rsa_key) RSA_free(rsa_key);

    req_ = req;
    return res;
  }

  bool Credential::GenerateEECRequest(std::string& req_content, std::string& key_content, const std::string& dn) {
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

  static int BIO_write_filename_User(BIO *b, const char* file) {
    return BIO_write_filename(b, (char*)file);
  }

  static int BIO_read_filename_User(BIO *b, const char* file) {
    return BIO_read_filename(b, (char*)file);
  }

  bool Credential::GenerateEECRequest(const char* req_filename, const char* key_filename, const std::string& dn) {
    BIO *req_out = BIO_new(BIO_s_file());
    BIO *key_out = BIO_new(BIO_s_file());
    if(!req_out || !key_out) {
      CredentialLogger.msg(ERROR, "Can not create BIO for request");
      return false;
    }
    if (!(BIO_write_filename_User(req_out, req_filename))) {
      CredentialLogger.msg(ERROR, "Can not set writable file for request BIO");
      BIO_free_all(req_out); return false;
    }

    if (!(BIO_write_filename_User(key_out, key_filename))) {
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
    int keybits = keybits_?keybits_:DEFAULT_KEYBITS;
    const EVP_MD *digest = signing_alg_?signing_alg_:DEFAULT_DIGEST;
    EVP_PKEY* pkey;

    if(pkey_) { CredentialLogger.msg(ERROR, "The credential's private key has already been initialized"); return false; };

    //BN_GENCB cb;
    BIGNUM *prime = BN_new();
    rsa_key = RSA_new();

    //BN_GENCB_set(&cb,&keygen_cb,NULL);
    if(prime && rsa_key) {
      int val1 = BN_set_word(prime,RSA_F4);
      if(val1 != 1) {
        CredentialLogger.msg(ERROR, "BN_set_word failed");
        LogError();
        if(prime) BN_free(prime);
        if(rsa_key) RSA_free(rsa_key);
        return false;
      }
      //int val2 = RSA_generate_key_ex(rsa_key, keybits, prime, &cb);
      int val2 = RSA_generate_key_ex(rsa_key, keybits, prime, NULL);
      if(val2 != 1) {
        CredentialLogger.msg(ERROR, "RSA_generate_key_ex failed");
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

                // set the default PROXY_CERT_INFO_EXTENSION extension
                X509_EXTENSION* ext = NULL;
                std::string data;
                int length = i2d_PROXY_CERT_INFO_EXTENSION(proxy_cert_info_, NULL);
                if(length < 0) {
                  CredentialLogger.msg(ERROR, "Can not convert PROXY_CERT_INFO_EXTENSION struct from internal to DER encoded format");
                  LogError();
                } else {
                  data.resize(length);
                  unsigned char* derdata = reinterpret_cast<unsigned char*>(const_cast<char*>(data.c_str()));
                  length = i2d_PROXY_CERT_INFO_EXTENSION(proxy_cert_info_, &derdata);
                  if(length < 0) {
                    CredentialLogger.msg(ERROR, "Can not convert PROXY_CERT_INFO_EXTENSION struct from internal to DER encoded format");
                    LogError();
                  } else {
                    std::string certinfo_sn = SN_proxyCertInfo;
                    ext = CreateExtension(certinfo_sn, data, true);
                  }
                }
                if(ext) {
                  STACK_OF(X509_EXTENSION)* extensions = sk_X509_EXTENSION_new_null();
                  if(extensions && sk_X509_EXTENSION_push(extensions, ext)) {
                    X509_REQ_add_extensions(req, extensions);
                    sk_X509_EXTENSION_pop_free(extensions, X509_EXTENSION_free);
                  } else {
                    X509_EXTENSION_free(ext);
                  }
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

    if(rsa_key) RSA_free(rsa_key);

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
    if (!(BIO_write_filename_User(out, filename))) {
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
    if(passphrase.empty()) {

      PasswordSourceInteractive pass("", true);
      return OutputPrivatekey(content, encryption, pass);
    }
    PasswordSourceString pass(passphrase);
    return OutputPrivatekey(content, encryption, pass);
  }

  bool Credential::OutputPrivatekey(std::string &content, bool encryption, PasswordSource& passphrase) {
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
        PW_CB_DATA cb_data;
        cb_data.password = &passphrase;
        if(!PEM_write_bio_RSAPrivateKey(out,rsa_key_,enc,NULL,0, &passwordcb,&cb_data)) {
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
        PW_CB_DATA cb_data;
        cb_data.password = &passphrase;
        if(!PEM_write_bio_PrivateKey(out,pkey_,enc,NULL,0, &passwordcb,&cb_data)) {
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
    if(!cert_) return false;
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
    CredentialLogger.msg(DEBUG, "Certiticate chain number %d",sk_X509_num(cert_chain_));

    //Output the cert chain. After the verification the cert_chain_
    //will include the CA certificate. Having CA in proxy does not
    //harm. So we output it too.
    if(cert_chain_) for (int n = 0; n < sk_X509_num(cert_chain_) ; n++) {
      X509 *cert = sk_X509_value(cert_chain_, n);
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

  //Inquire the input request bio to get PROXY_CERT_INFO_EXTENSION, certType
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
    PROXY_POLICY*  policy = NULL;
    ASN1_OBJECT*  policy_lang = NULL;
    int i;

    //Get the PROXY_CERT_INFO_EXTENSION from request' extension
    req_extensions = X509_REQ_get_extensions(req_);
    for(i=0;i<sk_X509_EXTENSION_num(req_extensions);i++) {
      X509_EXTENSION* ext = sk_X509_EXTENSION_value(req_extensions,i);
      ASN1_OBJECT* extension_oid = X509_EXTENSION_get_object(ext);
      int nid = OBJ_obj2nid(extension_oid);
      if(nid == NID_proxyCertInfo) {
        if(proxy_cert_info_) {
          PROXY_CERT_INFO_EXTENSION_free(proxy_cert_info_);
          proxy_cert_info_ = NULL;
        }
        ASN1_OCTET_STRING* data = X509_EXTENSION_get_data(ext);
        if(!data) {
           CredentialLogger.msg(ERROR, "Missing data in DER encoded PROXY_CERT_INFO_EXTENSION extension");
           LogError(); goto err;
        }
        unsigned char const * buf = ASN1_STRING_get0_data(data);
        long int buf_len = ASN1_STRING_length(data);
        if(buf_len > 0) {
           if((proxy_cert_info_ = d2i_PROXY_CERT_INFO_EXTENSION(NULL, &buf, buf_len)) == NULL) {
               CredentialLogger.msg(ERROR, "Can not convert DER encoded PROXY_CERT_INFO_EXTENSION extension to internal format");
               LogError(); goto err;
            }
        } else {
           if((proxy_cert_info_ = PROXY_CERT_INFO_EXTENSION_new()) == NULL) {
               CredentialLogger.msg(ERROR, "Can not create PROXY_CERT_INFO_EXTENSION extension");
               LogError(); goto err;
           }
        }
        break;
      }
    }

    if(proxy_cert_info_) {
      if((policy = PROXY_CERT_INFO_EXTENSION_get_proxypolicy(proxy_cert_info_)) == NULL) {
        CredentialLogger.msg(ERROR, "Can not get policy from PROXY_CERT_INFO_EXTENSION extension");
        LogError(); goto err;
      }
      if((policy_lang = PROXY_POLICY_get_policy_language(policy)) == NULL) {
        CredentialLogger.msg(ERROR, "Can not get policy language from PROXY_CERT_INFO_EXTENSION extension");
        LogError(); goto err;
      }
      int policy_nid = OBJ_obj2nid(policy_lang);
      {
        if(policy_nid == IMPERSONATION_PROXY_NID) { cert_type_ = CERT_TYPE_RFC_IMPERSONATION_PROXY; }
        else if(policy_nid == INDEPENDENT_PROXY_NID) { cert_type_ = CERT_TYPE_RFC_INDEPENDENT_PROXY; }
        else if(policy_nid == ANYLANGUAGE_PROXY_NID) { cert_type_ = CERT_TYPE_RFC_ANYLANGUAGE_PROXY; }
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
    if (!BIO_read_filename_User(in, filename)) {
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

  bool Credential::SetProxyPeriod(X509* tosign, X509* issuer, const Time& start, const Period& lifetime) {
    Time pt1 = start;
    Time pt2 = start + lifetime;
    Time it1 = asn1_to_utctime(X509_getm_notBefore(issuer));
    Time it2 = asn1_to_utctime(X509_getm_notAfter(issuer));
    if(pt1 < it1) pt1 = it1;
    if(pt2 > it2) pt2 = it2;
    ASN1_UTCTIME* not_before = utc_to_asn1time(pt1);
    ASN1_UTCTIME* not_after = utc_to_asn1time(pt2);
    if((!not_before) || (!not_after)) {
      if(not_before) ASN1_UTCTIME_free(not_before);
      if(not_after) ASN1_UTCTIME_free(not_after);
      return false;
    }
    X509_set1_notBefore(tosign, not_before);
    X509_set1_notAfter(tosign, not_after);
    ASN1_UTCTIME_free(not_before);
    ASN1_UTCTIME_free(not_after);
    return true;
  }

  EVP_PKEY* Credential::GetPrivKey(void) const {
    EVP_PKEY* key = NULL;
    BIO*  bio = NULL;
    int length;
    bio = BIO_new(BIO_s_mem());
    if(pkey_ == NULL) {
      //CredentialLogger.msg(ERROR, "Private key of the credential object is NULL");
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

  EVP_PKEY* Credential::GetPubKey(void) const {
    EVP_PKEY* key = NULL;
    if(cert_) key = X509_get_pubkey(cert_);
    return key;
  }

  X509* Credential::GetCert(void) const {
    X509* cert = NULL;
    if(cert_) cert = X509_dup(cert_);
    return cert;
  }

  STACK_OF(X509)* Credential::GetCertChain(void) const {
    STACK_OF(X509)* chain = NULL;
    chain = sk_X509_new_null();
    //Return the cert chain (not including this certificate itself)
    if(cert_chain_) for (int i=0; i < sk_X509_num(cert_chain_); i++) {
      X509* tmp = X509_dup(sk_X509_value(cert_chain_,i));
      sk_X509_insert(chain, tmp, i);
    }
    return chain;
  }

  int Credential::GetCertNumofChain(void) const {
    //Return the number of certificates
    //in the issuer chain
    if(!cert_chain_) return 0;
    return sk_X509_num(cert_chain_);
  }

  static std::string MakeExtensionData(int type, std::string const& value) {
    std::string data;
    GENERAL_NAMES* gens = sk_GENERAL_NAME_new_null();
    if(gens) {
      GENERAL_NAME* gen = GENERAL_NAME_new();
      if(gen) {
        ASN1_IA5STRING* ia5 = ASN1_IA5STRING_new();
        if(ia5) {
          if(ASN1_STRING_set(ia5, value.c_str(), value.length())) {
            GENERAL_NAME_set0_value(gen, type, ia5);
            ia5 = NULL;
            sk_GENERAL_NAME_push(gens, gen);
            gen = NULL;
            int length = i2d_GENERAL_NAMES(gens, NULL);
            if(length > 0) {
              data.resize(length);
              unsigned char* data_ptr = reinterpret_cast<unsigned char*>(const_cast<char*>(data.c_str()));
              length = i2d_GENERAL_NAMES(gens, &data_ptr);
              if (length > 0) {
                data.resize(length);
              } else {
                data.clear();
              };
            };
          };
          if(ia5) ASN1_IA5STRING_free(ia5);
        };
        if(gen) GENERAL_NAME_free(gen);
      };
      if(gen) GENERAL_NAMES_free(gens);
    };
    return data;
  }

  bool Credential::AddExtension(const std::string& name, const std::string& data, bool crit, int type) {
    X509_EXTENSION* ext = NULL;
    
    if(type == -1) { // -1 - raw
      ext = CreateExtension(name, data, crit);
    } else {
      std::string ext_data = MakeExtensionData(type, data);
      if(ext_data.empty() && !data.empty())
        return false;
      ext = CreateExtension(name, ext_data, crit);
    }

    if(!ext)
      return false;

    if(sk_X509_EXTENSION_push(extensions_, ext)) return true;

    X509_EXTENSION_free(ext);
    return false;
  }

  bool Credential::AddExtension(const std::string& name, char** binary) {
    X509_EXTENSION* ext = NULL;
    if(binary == NULL) return false;
    ext = X509V3_EXT_conf_nid(NULL, NULL, OBJ_txt2nid(name.c_str()), (char*)binary);
    if(ext) {
      if(sk_X509_EXTENSION_push(extensions_, ext)) return true;
      X509_EXTENSION_free(ext);
    }
    return false;
  }

  std::string Credential::GetExtension(const std::string& name) {
    std::string res;
    if(cert_ == NULL) return res;
    int num;
    if ((num = X509_get_ext_count(cert_)) > 0) {
      for (int i = 0; i < num; i++) {
        X509_EXTENSION *ext;
        const char *extname;

        ext = X509_get_ext(cert_, i);
        extname = OBJ_nid2sn(OBJ_obj2nid(X509_EXTENSION_get_object(ext)));

        if (strcmp(extname, name.c_str()) == 0) {
          X509V3_EXT_METHOD *method;
          STACK_OF(CONF_VALUE) *val;
          CONF_VALUE *nval;
          void *extstr = NULL;
          const unsigned char *ext_value_data;

          //Get x509 extension method structure
          if (!(method = (X509V3_EXT_METHOD *)(X509V3_EXT_get(ext)))) break;

          ASN1_OCTET_STRING* extvalue = X509_EXTENSION_get_data(ext);
          ext_value_data = extvalue->data;

          //Decode ASN1 item in data
          if (method->it) {
               //New style ASN1
               extstr = ASN1_item_d2i(NULL, &ext_value_data, extvalue->length,
                                      ASN1_ITEM_ptr(method->it));
          } 
          else {
               //Old style ASN1
               extstr = method->d2i(NULL, &ext_value_data, extvalue->length);
          }
          
          val = method->i2v(method, extstr, NULL);
          for (int j = 0; j < sk_CONF_VALUE_num(val); j++) {
            nval = sk_CONF_VALUE_value(val, j);
            std::string name = nval->name;
            std::string val = nval->value;
            if(!val.empty()) res = name + ":" + val;
            else res = name;   
          }
        }
      }
    }
    return res;
  }

  bool Credential::SignRequestAssistant(Credential* proxy, EVP_PKEY* req_pubkey, X509** tosign){

    bool res = false;
    X509* issuer = NULL;
    int position = -1;

    *tosign = NULL;

    if(cert_ == NULL) {
      CredentialLogger.msg(ERROR, "Credential is not initialized");
      goto err;
    }

    issuer = X509_dup(cert_);
    if(!issuer) {
      CredentialLogger.msg(ERROR, "Failed to duplicate X509 structure");
      LogError(); goto err;
    }

    if((*tosign = X509_new()) == NULL) {
      CredentialLogger.msg(ERROR, "Failed to initialize X509 structure");
      LogError(); goto err;
    }

    //TODO: VOMS
    {
      int length = i2d_PROXY_CERT_INFO_EXTENSION(proxy->proxy_cert_info_, NULL);
      if (length < 0) {
        CredentialLogger.msg(ERROR, "Can not convert PROXY_CERT_INFO_EXTENSION struct from internal to DER encoded format");
        LogError(); goto err;
      } else {
        std::string certinfo_data;
        certinfo_data.resize(length);
        unsigned char* derdata = reinterpret_cast<unsigned char*>(const_cast<char*>(certinfo_data.c_str()));
        length = i2d_PROXY_CERT_INFO_EXTENSION(proxy->proxy_cert_info_, &derdata);
        if (length < 0) {
          CredentialLogger.msg(ERROR, "Can not convert PROXY_CERT_INFO_EXTENSION struct from internal to DER encoded format");
          LogError(); goto err;
        } else {
          certinfo_data.resize(length);
          std::string NID_txt = SN_proxyCertInfo;
          X509_EXTENSION* certinfo_ext = CreateExtension(NID_txt, certinfo_data, true);
          if(certinfo_ext == NULL) {
            CredentialLogger.msg(ERROR, "Can not create extension for PROXY_CERT_INFO");
            LogError(); goto err;
          } else {
            if(!sk_X509_EXTENSION_push(proxy->extensions_, certinfo_ext)) {
              CredentialLogger.msg(ERROR, "Can not add X509 extension to proxy cert");
              X509_EXTENSION_free(certinfo_ext);
              LogError(); goto err;
            }
          }
        }
      }
    }

    /* Add any keyUsage and extendedKeyUsage extensions present in the issuer cert */

    if(X509_get_ext_by_NID(issuer, NID_key_usage, -1) > -1) {
      // Extension is present - transfer it

      ASN1_BIT_STRING* usage = (ASN1_BIT_STRING*)X509_get_ext_d2i(issuer, NID_key_usage, NULL, NULL);
      if(!usage) {
        CredentialLogger.msg(ERROR, "Can not convert keyUsage struct from DER encoded format");
        LogError(); goto err;
      }

      /* clear bits specified in draft */
      ASN1_BIT_STRING_set_bit(usage, 1, 0); /* Non Repudiation */
      ASN1_BIT_STRING_set_bit(usage, 5, 0); /* Certificate Sign */

      X509_EXTENSION* ext = NULL;

      int ku_length = i2d_ASN1_BIT_STRING(usage, NULL);
      if(ku_length < 0) {
        CredentialLogger.msg(ERROR, "Can not convert keyUsage struct from internal to DER format");
        LogError(); ASN1_BIT_STRING_free(usage); goto err;
      }

      std::string ku_data;
      ku_data.resize(ku_length);
      unsigned char* derdata = reinterpret_cast<unsigned char*>(const_cast<char*>(ku_data.c_str()));
      ku_length = i2d_ASN1_BIT_STRING(usage, &derdata);
      if(ku_length < 0) {
        CredentialLogger.msg(ERROR, "Can not convert keyUsage struct from internal to DER format");
        LogError(); ASN1_BIT_STRING_free(usage); goto err;
      }
      ASN1_BIT_STRING_free(usage);
      std::string name = "keyUsage";
      ext = CreateExtension(name, ku_data, true);
      if(!ext) {
        CredentialLogger.msg(ERROR, "Can not create extension for keyUsage");
        LogError(); goto err;
      }
      if(!sk_X509_EXTENSION_push(proxy->extensions_, ext)) {
        CredentialLogger.msg(ERROR, "Can not add X509 extension to proxy cert");
        LogError(); X509_EXTENSION_free(ext); ext = NULL; goto err;
      }
    }

    position = X509_get_ext_by_NID(issuer, NID_ext_key_usage, -1);
    if(position > -1) {
      X509_EXTENSION* ext = NULL;
      if(!(ext = X509_get_ext(issuer, position))) {
        CredentialLogger.msg(ERROR, "Can not get extended KeyUsage extension from issuer certificate");
        LogError(); goto err;
      }
      ext = X509_EXTENSION_dup(ext);
      if(!ext) {
        CredentialLogger.msg(ERROR, "Can not copy extended KeyUsage extension");
        LogError(); goto err;
      }

      if(!sk_X509_EXTENSION_push(proxy->extensions_, ext)) {
        CredentialLogger.msg(ERROR, "Can not add X509 extended KeyUsage extension to new proxy certificate");
        LogError(); X509_EXTENSION_free(ext); ext = NULL; goto err;
      }
    }

    {
      std::stringstream CN_name;
      unsigned char md[SHA_DIGEST_LENGTH];
      unsigned int len = sizeof(md);
      if(!ASN1_digest((int(*)(void*, unsigned char**))&i2d_PUBKEY, EVP_sha1(), (char*)req_pubkey, md, &len)) {
        CredentialLogger.msg(ERROR, "Can not compute digest of public key");
        goto err;
      }
      // SHA_DIGEST_LENGTH=20 < 4
      uint32_t sub_hash = md[0] + (md[1] + (md[2] + (md[3] >> 1) * 256) * 256) * 256;
      CN_name<<sub_hash;

      X509_NAME* subject_name = NULL;
      X509_NAME_ENTRY* name_entry = NULL;
      /* Create proxy subject name */
      if((subject_name = X509_NAME_dup(X509_get_subject_name(issuer))) == NULL) {
        CredentialLogger.msg(ERROR, "Can not copy the subject name from issuer for proxy certificate");
        goto err;
      }

      if((name_entry = X509_NAME_ENTRY_create_by_NID(&name_entry, NID_commonName, V_ASN1_APP_CHOOSE,
                        reinterpret_cast<unsigned char*>(const_cast<char*>(CN_name.str().c_str())), -1)) == NULL) {
        CredentialLogger.msg(ERROR, "Can not create name entry CN for proxy certificate");
        LogError(); X509_NAME_free(subject_name); goto err;
      }
      if(!X509_NAME_add_entry(subject_name, name_entry, X509_NAME_entry_count(subject_name), 0) ||
         !X509_set_subject_name(*tosign, subject_name)) {
        CredentialLogger.msg(ERROR, "Can not set CN in proxy certificate");
        LogError(); X509_NAME_free(subject_name); X509_NAME_ENTRY_free(name_entry); goto err;
      }
      X509_NAME_free(subject_name);
      X509_NAME_ENTRY_free(name_entry);
    }

    if(!X509_set_issuer_name(*tosign, X509_get_subject_name(issuer))) {
      CredentialLogger.msg(ERROR, "Can not set issuer's subject for proxy certificate");
      LogError(); goto err;
    }

    if(!X509_set_version(*tosign, 2L)) {
      CredentialLogger.msg(ERROR, "Can not set version number for proxy certificate");
      LogError(); goto err;
    }

    //Use the serial number in the certificate as the serial number in the proxy certificate
    if(ASN1_INTEGER* serial_number = X509_get_serialNumber(issuer)) {
      if((serial_number = ASN1_INTEGER_dup(serial_number))) {
        if(!X509_set_serialNumber(*tosign, serial_number)) {
          CredentialLogger.msg(ERROR, "Can not set serial number for proxy certificate");
          ASN1_INTEGER_free(serial_number);
          LogError(); goto err;
        }
        ASN1_INTEGER_free(serial_number);
      } else {
        CredentialLogger.msg(ERROR, "Can not duplicate serial number for proxy certificate");
        LogError(); goto err;
      }
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
    if((!res) && *tosign) { X509_free(*tosign); *tosign = NULL;}

    return res;
  }

  bool Credential::SignRequest(Credential* proxy, BIO* outputbio, bool if_der){
    bool res = false;
    if(proxy == NULL) {
      CredentialLogger.msg(ERROR, "The credential to be signed is NULL");
      return false;
    }
    if(proxy->req_ == NULL) {
      CredentialLogger.msg(ERROR, "The credential to be signed contains no request");
      return false;
    }
    if(outputbio == NULL) {
      CredentialLogger.msg(ERROR, "The BIO for output is NULL");
      return false;
    }

    int md_nid;
    const EVP_MD* dgst_alg  =NULL;
    EVP_PKEY* issuer_priv = NULL;
    EVP_PKEY* issuer_pub = NULL;
    X509*  proxy_cert = NULL;
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
    for(X509_EXTENSION* ext = X509_delete_ext(proxy_cert,0); ext; ext = X509_delete_ext(proxy_cert,0)) {
    	X509_EXTENSION_free(ext);
    }; 
    
    /*Set the serialNumber*/
    //cert_info->serialNumber = M_ASN1_INTEGER_dup(X509_get_serialNumber(proxy_cert));;

    /*Set the extension*/
    for (int i=0; i<sk_X509_EXTENSION_num(proxy->extensions_); i++) {
      ext = sk_X509_EXTENSION_value(proxy->extensions_, i);
      if (ext == NULL) {
        //CredentialLogger.msg(ERROR,"Failed to duplicate extension"); LogError(); goto err;
        CredentialLogger.msg(ERROR,"Failed to find extension"); LogError(); goto err;
      }
      X509_add_ext(proxy_cert, ext, -1);
    }

    /*Clean extensions attached to "proxy" after it has been linked into to-signed certificate*/
    while(X509_EXTENSION* ext = sk_X509_EXTENSION_pop(proxy->extensions_)) {
      X509_EXTENSION_free(ext);
    }

    /* Now sign the new certificate */
    if(!(issuer_priv = GetPrivKey())) {
      CredentialLogger.msg(ERROR, "Can not get the issuer's private key"); goto err;
    }

    /* Use the signing algorithm in the signer's priv key */
    {
      int dgst_err = EVP_PKEY_get_default_digest_nid(issuer_priv, &md_nid);
      if(dgst_err <= 0) {
        CredentialLogger.msg(INFO, "There is no digest in issuer's private key object");
      } else if((dgst_err == 2) || (!proxy->signing_alg_)) { // mandatory or no digest specified
        char* md_str = (char *)OBJ_nid2sn(md_nid);
        if(md_str) {
          if((dgst_alg = EVP_get_digestbyname(md_str)) == NULL) {
            CredentialLogger.msg(INFO, "%s is an unsupported digest type", md_str);
            // TODO: if digest is mandatory then probably there must be error.
          }
        }
      }
    }
    if(dgst_alg == NULL) dgst_alg = proxy->signing_alg_?proxy->signing_alg_:DEFAULT_DIGEST;

    /* Check whether the digest algorithm is SHA1 or SHA2*/
    md_nid = EVP_MD_type(dgst_alg);
    if((md_nid != NID_sha1) && (md_nid != NID_sha224) && (md_nid != NID_sha256) && (md_nid != NID_sha384) && (md_nid != NID_sha512)) {
      CredentialLogger.msg(ERROR, "The signing algorithm %s is not allowed,it should be SHA1 or SHA2 to sign certificate requests",
      OBJ_nid2sn(md_nid));
      goto err;
    }

    if(!X509_sign(proxy_cert, issuer_priv, dgst_alg)) {
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
    if (!BIO_write_filename_User(out, filename)) {
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
       const std::string& CAserial, const std::string& extfile,
       const std::string& extsect, PasswordSource& passphrase4key) :
       certfile_(CAcertfile), keyfile_(CAkeyfile), verification_valid(false),
       cert_(NULL), pkey_(NULL), cert_chain_(NULL), proxy_cert_info_(NULL),
       req_(NULL), rsa_key_(NULL), signing_alg_(NULL), keybits_(0),
       proxyver_(0), pathlength_(0), extensions_(NULL),
       CAserial_(CAserial), extfile_(extfile), extsect_(extsect) {
    OpenSSLInit();

    //Initiate the proxy certificate constant and  method which is required by openssl
    if(!proxy_init_) InitProxyCertInfo();

    extensions_ = sk_X509_EXTENSION_new_null();
    if(!extensions_) {
      CredentialLogger.msg(ERROR, "Failed to initialize extensions member for Credential");
      return;
    }

    try {
      loadCertificateFile(CAcertfile, cert_, &cert_chain_);
      if(cert_) check_cert_type(cert_,cert_type_);
      loadKeyFile(CAkeyfile, pkey_, passphrase4key);
    } catch(std::exception& err){
      CredentialLogger.msg(ERROR, "ERROR: %s", err.what());
      LogError();
    }
  }

  Credential::Credential(const std::string& CAcertfile, const std::string& CAkeyfile,
       const std::string& CAserial, const std::string& extfile,
       const std::string& extsect, const std::string& passphrase4key) :
       certfile_(CAcertfile), keyfile_(CAkeyfile), verification_valid(false),
       cert_(NULL), pkey_(NULL), cert_chain_(NULL), proxy_cert_info_(NULL),
       req_(NULL), rsa_key_(NULL), signing_alg_(NULL), keybits_(0),
       proxyver_(0), pathlength_(0), extensions_(NULL),
       CAserial_(CAserial), extfile_(extfile), extsect_(extsect) {
    OpenSSLInit();

    //Initiate the proxy certificate constant and  method which is required by openssl
    if(!proxy_init_) InitProxyCertInfo();

    extensions_ = sk_X509_EXTENSION_new_null();
    if(!extensions_) {
      CredentialLogger.msg(ERROR, "Failed to initialize extensions member for Credential");
      return;
    }

    try {
      PasswordSource* pass = NULL;
      if(passphrase4key.empty()) {
        pass = new PasswordSourceInteractive("private key", false);
      } else if(passphrase4key[0] == '\0') {
        pass = new PasswordSourceString("");
      } else {
        pass = new PasswordSourceString(passphrase4key);
      }
      loadCertificateFile(CAcertfile, cert_, &cert_chain_);
      if(cert_) check_cert_type(cert_,cert_type_);
      loadKeyFile(CAkeyfile, pkey_, *pass);
      delete pass;
    } catch(std::exception& err){
      CredentialLogger.msg(ERROR, "ERROR: %s", err.what());
      LogError();
    }
  }

  static void print_ssl_errors() {
    unsigned long err;
    while((err = ERR_get_error())) {
      CredentialLogger.msg(DEBUG,"SSL error: %s, libs: %s, func: %s, reason: %s",
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
  BIGNUM *load_serial(const std::string& serialfile, ASN1_INTEGER **retai) {
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

    if (BIO_read_filename_User(in,serialfile.c_str()) > 0) {
      if (!a2i_ASN1_INTEGER(in,ai,buf,sizeof(buf))) {
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

  int save_serial(const std::string& serialfile, char *suffix, BIGNUM *serial, ASN1_INTEGER **retai) {
    char buf[1][BSIZE];
    BIO *out = NULL;
    int ret=0;
    ASN1_INTEGER *ai=NULL;
    int j;

    if (suffix == NULL)
      j = strlen(serialfile.c_str());
    else
      j = strlen(serialfile.c_str()) + strlen(suffix) + 1;
    if (j >= BSIZE) {
      CredentialLogger.msg(ERROR,"file name too long");
      goto err;
    }

    if (suffix == NULL)
      BUF_strlcpy(buf[0], serialfile.c_str(), BSIZE);
    else {
#ifndef OPENSSL_SYS_VMS
      j = BIO_snprintf(buf[0], sizeof buf[0], "%s.%s", serialfile.c_str(), suffix);
#else
      j = BIO_snprintf(buf[0], sizeof buf[0], "%s-%s", serialfile.c_str(), suffix);
#endif
    }
    out=BIO_new(BIO_s_file());
    if (out == NULL) {
      print_ssl_errors();
      goto err;
    }
    if (BIO_write_filename_User(out,buf[0]) <= 0) {
      perror(serialfile.c_str());
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
  static ASN1_INTEGER *x509_load_serial(const std::string& CAfile, const std::string& serialfile) {
    ASN1_INTEGER *bs = NULL;
    BIGNUM *serial = NULL;

    std::string serial_f;
    if(!serialfile.empty()) serial_f = serialfile;
    else if(!CAfile.empty()){
      std::size_t pos; pos = CAfile.rfind(".");
      if(pos != std::string::npos) serial_f = CAfile.substr(0, pos); 
      serial_f.append(".srl"); 
    }
    else{ return bs;}

    serial = load_serial(serial_f, NULL);
    if (serial == NULL) {
      CredentialLogger.msg(ERROR,"load serial from %s failure",serial_f.c_str());
      return bs;
    }

    if (!BN_add_word(serial,1)) {
      CredentialLogger.msg(ERROR,"add_word failure");
      BN_free(serial); return bs;
    }

    if(!save_serial(serial_f, NULL, serial, &bs)) {
      CredentialLogger.msg(ERROR,"save serial to %s failure",serial_f.c_str());
      BN_free(serial); return bs;
    }
    BN_free(serial);
    return bs;
  }

  static int x509_certify(X509_STORE *ctx, const std::string& CAfile, const EVP_MD *digest,
             X509 *x, X509 *xca, EVP_PKEY *pkey, const std::string& serialfile,
             time_t start, time_t lifetime, int clrext, CONF *conf, char *section, ASN1_INTEGER *sno) {
    int ret=0;
    ASN1_INTEGER *bs=NULL;
    X509_STORE_CTX* xsc = X509_STORE_CTX_new();
    EVP_PKEY *upkey;

    upkey = X509_get_pubkey(xca);
    EVP_PKEY_copy_parameters(upkey,pkey);
    EVP_PKEY_free(upkey);

    if(!X509_STORE_CTX_init(xsc,ctx,x,NULL)) {
      CredentialLogger.msg(ERROR,"Error initialising X509 store");
      goto end;
    }
    if (sno) bs = sno;
    else if (!(bs = x509_load_serial(CAfile, serialfile))) {
      bs = ASN1_INTEGER_new();
      if( bs == NULL || !rand_serial(NULL,bs)) {
        CredentialLogger.msg(ERROR,"Out of memory when generate random serial");
        goto end;
      } 
      //bs = s2i_ASN1_INTEGER(NULL, "1"); 
    }

    //X509_STORE_CTX_set_cert(&xsc,x);
    //X509_STORE_CTX_set_flags(&xsc, X509_V_FLAG_CHECK_SS_SIGNATURE);
    //if (!X509_verify_cert(&xsc))
    //  goto end;

    if (!X509_check_private_key(xca,pkey)) {
      CredentialLogger.msg(ERROR,"CA certificate and CA private key do not match");
      goto end;
    }

    if (!X509_set_issuer_name(x,X509_get_subject_name(xca))) goto end;
    if (!X509_set_serialNumber(x,bs)) goto end;

    if (X509_gmtime_adj(X509_getm_notBefore(x), start) == NULL)
      goto end;

    /* hardwired expired */
    if (X509_gmtime_adj(X509_getm_notAfter(x), lifetime) == NULL)
      goto end;

    if (clrext) {
      while (X509_get_ext_count(x) > 0) X509_delete_ext(x, 0);
    }

    if (conf) {
      X509V3_CTX ctx2;
      X509_set_version(x,2);
      X509V3_set_ctx(&ctx2, xca, x, NULL, NULL, 0);
      X509V3_set_nconf(&ctx2, conf);
      if (!X509V3_EXT_add_nconf(conf, &ctx2, section, x)) {
        CredentialLogger.msg(ERROR,"Failed to load extension section: %s", section);
        goto end;
      }
    }

    if (!X509_sign(x,pkey,digest)) goto end;
      ret=1;
end:
    X509_STORE_CTX_cleanup(xsc);
    X509_STORE_CTX_free(xsc);
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

  bool Credential::SelfSignEECRequest(const std::string& dn, const char* extfile, const std::string& extsect, const char* certfile) {
    if(extfile != NULL){ extfile_ = extfile; }
    if(!extsect.empty()){ extsect_ = extsect; }
    cert_ = X509_new(); 
    if(!dn.empty()) {
      X509_NAME *name = parse_name((char*)(dn.c_str()), MBSTRING_ASC, 0);
      X509_set_subject_name(cert_, name);
      X509_NAME_free(name);
    }
    else {
      X509_set_subject_name(cert_, X509_REQ_get_subject_name(req_));
    }

    EVP_PKEY* tmpkey;
    tmpkey = X509_REQ_get_pubkey(req_);
    if(!tmpkey || !X509_set_pubkey(cert_, tmpkey)) {
      CredentialLogger.msg(ERROR,"Failed to set the pubkey for X509 object by using pubkey from X509_REQ");
      LogError(); return false;
    }
    EVP_PKEY_free(tmpkey);

    return(SignEECRequest(this, dn, certfile));
  }

  bool Credential::SignEECRequest(Credential* eec, const std::string& dn, BIO* outputbio) {
    if(pkey_ == NULL) {
      CredentialLogger.msg(ERROR, "The private key for signing is not initialized");
      return false;
    }
    bool res = false;
    if(eec == NULL) {
      CredentialLogger.msg(ERROR, "The credential to be signed is NULL");
      return false;
    }
    if(eec->req_ == NULL) {
      CredentialLogger.msg(ERROR, "The credential to be signed contains no request");
      return false;
    }
    if(outputbio == NULL) {
      CredentialLogger.msg(ERROR, "The BIO for output is NULL");
      return false;
    }

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

    if(!dn.empty()) {
      X509_NAME *subject = parse_name((char*)(dn.c_str()), MBSTRING_ASC, 0);
      X509_set_subject_name(eec_cert, subject);
      X509_NAME_free(subject);
    }
    else {
      X509_set_subject_name(eec_cert, X509_REQ_get_subject_name(eec->req_));
    }
/*
    const EVP_MD *digest=EVP_sha1();
#ifndef OPENSSL_NO_DSA
    if (pkey_->type == EVP_PKEY_DSA)
      digest=EVP_dss1();
#endif
*/
/*
#ifndef OPENSSL_NO_ECDSA
    if (pkey_->type == EVP_PKEY_EC)
      digest = EVP_ecdsa();
#endif
*/
    const EVP_MD* digest = NULL;
    int md_nid;
    char* md_str;
    if(EVP_PKEY_get_default_digest_nid(pkey_, &md_nid) <= 0) {
      CredentialLogger.msg(INFO, "There is no digest in issuer's private key object");
    }
    md_str = (char *)OBJ_nid2sn(md_nid);
    if((digest = EVP_get_digestbyname(md_str)) == NULL) {
      CredentialLogger.msg(INFO, "%s is an unsupported digest type", md_str);
    }
    if(digest == NULL) digest = EVP_sha1();


    X509_STORE *ctx = NULL;
    ctx = X509_STORE_new();
    //X509_STORE_set_verify_cb_func(ctx,callb);
    if (!X509_STORE_set_default_paths(ctx)) {
      LogError();
    }

    CONF *extconf = NULL;
    if (!extfile_.empty()) {
      long errorline = -1;
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
        //if the extension section/group has not been specified, use the group name in
        //the openssl.cnf file which is set by default when openssl is installed
        char* str = NCONF_get_string(extconf, "usr_cert", "basicConstraints");
        if(str != NULL) extsect_ = "usr_cert";
      }

/*
      X509V3_set_ctx_test(&ctx2);
      X509V3_set_nconf(&ctx2, extconf);
      if (!X509V3_EXT_add_nconf(extconf, &ctx2, (char*)(extsect_.c_str()), NULL)) {
        CredentialLogger.msg(ERROR,"Error when loading the extension section: %s",
          extsect_.c_str());
        LogError();
      }
*/
    }

    //Add extensions to certificate object
    for(X509_EXTENSION* ext = X509_delete_ext(eec_cert, 0); ext; ext = X509_delete_ext(eec_cert, 0)) {
    	X509_EXTENSION_free(ext);
    }
   
    for (int i=0; i<sk_X509_EXTENSION_num(eec->extensions_); i++) {
      X509_EXTENSION* ext = sk_X509_EXTENSION_value(eec->extensions_, i);
      if (ext == NULL) {
        CredentialLogger.msg(ERROR,"Failed to duplicate extension"); LogError();
      }
      X509_add_ext(eec_cert, ext, -1);
    }

    X509_set_version(cert_,2);
    time_t lifetime = (eec->GetLifeTime()).GetPeriod();
    Time t1 = eec->GetStartTime();
    Time t2;
    time_t start;
    if(t1 > t2) start = t1.GetTime() - t2.GetTime();
    else start = 0;

    if (!x509_certify(ctx, certfile_, digest, eec_cert, cert_,
                      pkey_, CAserial_, start, lifetime, 0,
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
    if (!(BIO_write_filename_User(out, filename))) {
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
    if(proxy_cert_info_) PROXY_CERT_INFO_EXTENSION_free(proxy_cert_info_);
    if(req_) X509_REQ_free(req_);
    if(rsa_key_) RSA_free(rsa_key_);
    if(extensions_) sk_X509_EXTENSION_pop_free(extensions_, X509_EXTENSION_free);
  }

}

