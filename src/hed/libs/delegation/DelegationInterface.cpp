#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <openssl/rsa.h>
#include <openssl/bn.h>
#include <openssl/bio.h>
#include <openssl/pem.h>
#include <openssl/err.h>
#include <openssl/x509v3.h>

#include <string>
#include <iostream>
#include <fstream>

#include <arc/GUID.h>
#include <arc/StringConv.h>
#include <arc/DateTime.h>
#include <arc/message/PayloadSOAP.h>
#include <arc/crypto/OpenSSL.h>
#include <arc/ws-addressing/WSA.h>

#include "DelegationInterface.h"

namespace Arc {

#if (OPENSSL_VERSION_NUMBER < 0x10100000L)

#define X509_getm_notAfter X509_get_notAfter
#define X509_getm_notBefore X509_get_notBefore
#define X509_set1_notAfter X509_set_notAfter
#define X509_set1_notBefore X509_set_notBefore

#endif

#define DELEGATION_NAMESPACE "http://www.nordugrid.org/schemas/delegation"
#define GDS10_NAMESPACE "http://www.gridsite.org/ns/delegation.wsdl"
//#define GDS20_NAMESPACE "http://www.gridsite.org/namespaces/delegation-2"
#define EMIES_NAMESPACE "http://www.eu-emi.eu/es/2010/12/delegation/types"
#define EMIES_TYPES_NAMESPACE "http://www.eu-emi.eu/es/2010/12/types"
//#define EMIDS_NAMESPACE "http://www.gridsite.org/namespaces/delegation-21"
// GDS 2.1 was made on wire compatible with GDS 2.0 - so they use same namespace
#define EMIDS_NAMESPACE "http://www.gridsite.org/namespaces/delegation-2"

#define GLOBUS_LIMITED_PROXY_OID "1.3.6.1.4.1.3536.1.1.1.9"

//#define SERIAL_RAND_BITS 64
#define SERIAL_RAND_BITS 31

static int rand_serial(ASN1_INTEGER *ai) {
  int ret = 0;
  BIGNUM *btmp = BN_new();
  if(!btmp) goto error;
  if(!BN_pseudo_rand(btmp, SERIAL_RAND_BITS, 0, 0)) goto error;
  if(ai && !BN_to_ASN1_INTEGER(btmp, ai)) goto error;
  ret = 1;
error:
  if(btmp) BN_free(btmp);
  return ret;
}

static bool x509_to_string(X509* cert,std::string& str) {
  BIO *out = BIO_new(BIO_s_mem());
  if(!out) return false;
  if(!PEM_write_bio_X509(out,cert)) { BIO_free_all(out); return false; };
  for(;;) {
    char s[256];
    int l = BIO_read(out,s,sizeof(s));
    if(l <= 0) break;
    str.append(s,l);;
  };
  BIO_free_all(out);
  return true;
}

/*
static bool x509_to_string(EVP_PKEY* key,std::string& str) {
  BIO *out = BIO_new(BIO_s_mem());
  if(!out) return false;
  EVP_CIPHER *enc = NULL;
  if(!PEM_write_bio_PrivateKey(out,key,enc,NULL,0,NULL,NULL)) { BIO_free_all(out); return false; };
  for(;;) {
    char s[256];
    int l = BIO_read(out,s,sizeof(s));
    if(l <= 0) break;
    str.append(s,l);;
  };
  BIO_free_all(out);
  return true;
}
*/

static bool x509_to_string(RSA* key,std::string& str) {
  BIO *out = BIO_new(BIO_s_mem());
  if(!out) return false;
  EVP_CIPHER *enc = NULL;
  if(!PEM_write_bio_RSAPrivateKey(out,key,enc,NULL,0,NULL,NULL)) { BIO_free_all(out); return false; };
  for(;;) {
    char s[256];
    int l = BIO_read(out,s,sizeof(s));
    if(l <= 0) break;
    str.append(s,l);;
  };
  BIO_free_all(out);
  return true;
}

static int passphrase_callback(char* buf, int size, int, void *arg) {
   std::istream* in = (std::istream*)arg;
   if(in == &std::cin) std::cout<<"Enter passphrase for your private key: ";
   buf[0]=0;
   in->getline(buf,size);
   //if(!(*in)) {
   //  if(in == &std::cin) std::cerr<< "Failed to read passphrase from stdin"<<std::endl;
   //  return -1;
   //};
   return strlen(buf);
}

static bool string_to_x509(const std::string& str,X509* &cert,EVP_PKEY* &pkey,STACK_OF(X509)* &cert_sk) {
  BIO *in = NULL;
  cert=NULL; pkey=NULL; cert_sk=NULL;
  if(str.empty()) return false;
  if(!(in=BIO_new_mem_buf((void*)(str.c_str()),str.length()))) return false;
  if((!PEM_read_bio_X509(in,&cert,NULL,NULL)) || (!cert)) { BIO_free_all(in); return false; };
  if((!PEM_read_bio_PrivateKey(in,&pkey,NULL,NULL)) || (!pkey)) { BIO_free_all(in); return false; };
  if(!(cert_sk=sk_X509_new_null())) { BIO_free_all(in); return false; };
  for(;;) {
    X509* c = NULL;
    if((!PEM_read_bio_X509(in,&c,NULL,NULL)) || (!c)) break;
    sk_X509_push(cert_sk,c);
  };
  BIO_free_all(in);
  return true;
}

static bool string_to_x509(const std::string& cert_file,const std::string& key_file,std::istream* inpwd,X509* &cert,EVP_PKEY* &pkey,STACK_OF(X509)* &cert_sk) {
  BIO *in = NULL;
  cert=NULL; pkey=NULL; cert_sk=NULL;
  if(cert_file.empty()) return false;
  if(!(in=BIO_new_file(cert_file.c_str(),"r"))) return false;
  if((!PEM_read_bio_X509(in,&cert,NULL,NULL)) || (!cert)) { BIO_free_all(in); return false; };
  if(key_file.empty()) {
    if((!PEM_read_bio_PrivateKey(in,&pkey,inpwd?&passphrase_callback:NULL,inpwd)) || (!pkey)) { BIO_free_all(in); return false; };
  };
  if(!(cert_sk=sk_X509_new_null())) { BIO_free_all(in); return false; };
  for(;;) {
    X509* c = NULL;
    if((!PEM_read_bio_X509(in,&c,NULL,NULL)) || (!c)) break;
    sk_X509_push(cert_sk,c);
  };
  ERR_get_error();
  if(!pkey) {
    BIO_free_all(in); in=NULL;
    if(!(in=BIO_new_file(key_file.c_str(),"r"))) return false;
    if((!PEM_read_bio_PrivateKey(in,&pkey,inpwd?&passphrase_callback:NULL,inpwd)) || (!pkey)) { BIO_free_all(in); return false; };
  };
  BIO_free_all(in);
  return true;
}

static bool string_to_x509(const std::string& str,X509* &cert,STACK_OF(X509)* &cert_sk) {
  BIO *in = NULL;
  if(str.empty()) return false;
  if(!(in=BIO_new_mem_buf((void*)(str.c_str()),str.length()))) return false;
  if((!PEM_read_bio_X509(in,&cert,NULL,NULL)) || (!cert)) { BIO_free_all(in); return false; };
  if(!(cert_sk=sk_X509_new_null())) { BIO_free_all(in); return false; };
  for(;;) {
    X509* c = NULL;
    if((!PEM_read_bio_X509(in,&c,NULL,NULL)) || (!c)) break;
    sk_X509_push(cert_sk,c);
  };
  ERR_get_error();
  BIO_free_all(in);
  return true;
}

/*
static unsigned char* ASN1_BIT_STRING_to_data(ASN1_BIT_STRING* str,int* l) {
  *l=0;
  int length = i2d_ASN1_BIT_STRING(str, NULL);
  if(length < 0) return NULL;
  unsigned char* data = (unsigned char*)malloc(length);
  if(!data) return NULL;
  length = i2d_ASN1_BIT_STRING(str,&data);
  if(length < 0) {
    free(data);
    return NULL;
  };
  if(l) *l=length;
  return data;
}

static X509_EXTENSION* data_to_X509_EXTENSION(const char* name,unsigned char* data,int length,bool critical) {
  ASN1_OBJECT* ext_obj = NULL;
  ASN1_OCTET_STRING* ext_oct = NULL;
  X509_EXTENSION* ext = NULL;

  ext_obj=OBJ_nid2obj(OBJ_txt2nid(name));
  if(!ext_obj) goto err;
  ext_oct=ASN1_OCTET_STRING_new();
  if(!ext_oct) goto err;
  ASN1_OCTET_STRING_set(ext_oct,data,length);
  ext=X509_EXTENSION_create_by_OBJ(NULL,ext_obj,critical?1:0,ext_oct);
err:
  if(ext_oct) ASN1_OCTET_STRING_free(ext_oct);
  if(ext_obj) ASN1_OBJECT_free(ext_obj);
  return ext;
}

static X509_EXTENSION* ASN1_OCTET_STRING_to_X509_EXTENSION(const char* name,ASN1_OCTET_STRING* oct,bool critical) {
  ASN1_OBJECT* ext_obj = NULL;
  X509_EXTENSION* ext = NULL;

  ext_obj=OBJ_nid2obj(OBJ_txt2nid(name));
  if(!ext_obj) goto err;
  ext=X509_EXTENSION_create_by_OBJ(NULL,ext_obj,critical?1:0,oct);
err:
  if(ext_obj) ASN1_OBJECT_free(ext_obj);
  return ext;
}
*/

static Time asn1_to_time(const ASN1_UTCTIME *s) {
  if(s != NULL) {
    if(s->type == V_ASN1_UTCTIME) return Time(std::string("20")+((char*)(s->data)));
    if(s->type == V_ASN1_GENERALIZEDTIME) return Time(std::string((char*)(s->data)));
  }
  return Time(Time::UNDEFINED);
}

static bool X509_add_ext_by_nid(X509 *cert,int nid,char *value,int pos) {
  X509_EXTENSION* ext = X509V3_EXT_conf_nid(NULL, NULL, nid, value);
  if(!ext) return false;
  X509_add_ext(cert,ext,pos);
  X509_EXTENSION_free(ext);
  return true;
}

static std::string::size_type find_line(const std::string& val, const char* token, std::string::size_type p = std::string::npos) {
  std::string::size_type l = ::strlen(token);
  if(p == std::string::npos) {
    p = val.find(token);
  } else {
    p = val.find(token,p);
  };
  if(p == std::string::npos) return p;
  if((p > 0) && (val[p-1] != '\r') && (val[p-1] != '\n')) return std::string::npos;
  if(((p+l) < val.length()) && (val[p+l] != '\r') && (val[p+l] != '\n')) return std::string::npos;
  return p;
}

static bool strip_PEM(std::string& val, const char* ts, const char* te) {
  std::string::size_type ps = find_line(val,ts);
  if(ps == std::string::npos) return false;
  ps += ::strlen(ts);
  ps = val.find_first_not_of("\r\n",ps);
  if(ps == std::string::npos) return false;
  std::string::size_type pe = find_line(val,te,ps);
  if(pe == std::string::npos) return false;
  if(pe == 0) return false;
  pe = val.find_last_not_of("\r\n",pe-1);
  if(pe == std::string::npos) return false;
  if(pe < ps) return false;
  val = val.substr(ps,pe-ps+1);
  return true;
}

static void wrap_PEM(std::string& val, const char* ts, const char* te) {
  val = std::string(ts)+"\n"+trim(val,"\r\n")+"\n"+te;
}

static bool strip_PEM_request(std::string& val) {
  const char* ts = "-----BEGIN CERTIFICATE REQUEST-----";
  const char* te = "-----END CERTIFICATE REQUEST-----";
  return strip_PEM(val, ts, te);
}

static bool strip_PEM_cert(std::string& val) {
  const char* ts = "-----BEGIN CERTIFICATE-----";
  const char* te = "-----END CERTIFICATE-----";
  return strip_PEM(val, ts, te);
}

static void wrap_PEM_request(std::string& val) {
  const char* ts = "-----BEGIN CERTIFICATE REQUEST-----";
  const char* te = "-----END CERTIFICATE REQUEST-----";
  wrap_PEM(val, ts, te);
}

static void wrap_PEM_cert(std::string& val) {
  const char* ts = "-----BEGIN CERTIFICATE-----";
  const char* te = "-----END CERTIFICATE-----";
  wrap_PEM(val, ts, te);
}

class cred_info_t {
 public:
  Arc::Time valid_from;
  Arc::Time valid_till;
  std::string identity;
  std::string ca;
  unsigned int deleg_depth;
  unsigned int strength;
};

static bool get_cred_info(const std::string& str,cred_info_t& info) {
  // It shold use Credential class. But so far keeping dependencies simple.
  bool r = false;
  X509* cert = NULL;
  STACK_OF(X509)* cert_sk = NULL;
  if(string_to_x509(str,cert,cert_sk) && cert && cert_sk) {
    info.valid_from=Time(Time::UNDEFINED);
    info.valid_till=Time(Time::UNDEFINED);
    info.deleg_depth=0;
    info.strength=0;
    X509* c = cert;
    for(int idx = 0;;++idx) {
      char* buf = X509_NAME_oneline(X509_get_issuer_name(c),NULL,0);
      if(buf) {
        info.ca = buf;
        OPENSSL_free(buf); buf = NULL;
      } else {
        info.ca = "";
      }
      buf = X509_NAME_oneline(X509_get_subject_name(c),NULL,0);
      if(buf) {
        info.identity=buf;
        OPENSSL_free(buf); buf = NULL;
      } else {
        info.identity="";
      }
      Time from = asn1_to_time(X509_getm_notBefore(c));
      Time till = asn1_to_time(X509_getm_notAfter(c));
      if(from != Time(Time::UNDEFINED)) {
        if((info.valid_from == Time(Time::UNDEFINED)) || (from > info.valid_from)) {
          info.valid_from = from;
        };
      };
      if(till != Time(Time::UNDEFINED)) {
        if((info.valid_till == Time(Time::UNDEFINED)) || (till < info.valid_till)) {
          info.valid_till = till;
        };
      };
      if(X509_get_ext_by_NID(cert,NID_proxyCertInfo,-1) < 0) break;
      if(idx >= sk_X509_num(cert_sk)) break;
      c = sk_X509_value(cert_sk,idx);
    };
    r = true;
  };
  if(cert) X509_free(cert);
  if(cert_sk) {
    for(int i = 0;i<sk_X509_num(cert_sk);++i) {
      X509* v = sk_X509_value(cert_sk,i);
      if(v) X509_free(v);
    };
    sk_X509_free(cert_sk);
  };
  return r;
}

DelegationConsumer::DelegationConsumer(void):key_(NULL) {
  Generate();
}

DelegationConsumer::DelegationConsumer(const std::string& content):key_(NULL) {
  Restore(content);
}

DelegationConsumer::~DelegationConsumer(void) {
  if(key_) RSA_free((RSA*)key_);
}

const std::string& DelegationConsumer::ID(void) {
  static std::string s;
  return s;
}

/*
static int progress_cb(int p, int, BN_GENCB*) {
  char c='*';
  if (p == 0) c='.';
  if (p == 1) c='+';
  if (p == 2) c='*';
  if (p == 3) c='\n';
  std::cerr<<c;
  return 1;
}
*/

static int ssl_err_cb(const char *str, size_t len, void *u) {
  std::string& ssl_err = *((std::string*)u);
  ssl_err.append(str,len);
  return 1;
}

void DelegationConsumer::LogError(void) {
  std::string ssl_err;
  ERR_print_errors_cb(&ssl_err_cb,&ssl_err);
}

bool DelegationConsumer::Backup(std::string& content) {
  bool res = false;
  content.resize(0);
  RSA *rsa = (RSA*)key_;
  if(rsa) {
    BIO *out = BIO_new(BIO_s_mem());
    if(out) {
      EVP_CIPHER *enc = NULL;
      if(PEM_write_bio_RSAPrivateKey(out,rsa,enc,NULL,0,NULL,NULL)) {
        res=true;
        for(;;) {
          char s[256];
          int l = BIO_read(out,s,sizeof(s));
          if(l <= 0) break;
          content.append(s,l);;
        };
      } else {
        LogError();
        std::cerr<<"PEM_write_bio_RSAPrivateKey failed"<<std::endl;
      };
      BIO_free_all(out);
    };
  };
  return res;
}

bool DelegationConsumer::Restore(const std::string& content) {
  RSA *rsa = NULL;
  BIO *in = BIO_new_mem_buf((void*)(content.c_str()),content.length());
  if(in) {
    if(PEM_read_bio_RSAPrivateKey(in,&rsa,NULL,NULL)) {
      if(rsa) {
        if(key_) RSA_free((RSA*)key_);
        key_=rsa;
      };
    };
    BIO_free_all(in);
  };
  return rsa;
}

bool DelegationConsumer::Generate(void) {
  bool res = false;
  int num = 2048;
  //BN_GENCB cb;
  BIGNUM *bn = BN_new();
  RSA *rsa = RSA_new();

  //BN_GENCB_set(&cb,&progress_cb,NULL);
  if(bn && rsa) {
    if(BN_set_word(bn,RSA_F4)) {
      //if(RSA_generate_key_ex(rsa,num,bn,&cb)) {
      if(RSA_generate_key_ex(rsa,num,bn,NULL)) {
        if(key_) RSA_free((RSA*)key_);
        key_=rsa; rsa=NULL; res=true;
      } else {
        LogError();
        std::cerr<<"RSA_generate_key_ex failed"<<std::endl;
      };
    } else {
      LogError();
      std::cerr<<"BN_set_word failed"<<std::endl;
    };
  } else {
    LogError();
    std::cerr<<"BN_new || RSA_new failed"<<std::endl;
  };
  if(bn) BN_free(bn);
  if(rsa) RSA_free(rsa);
  return res;
}

bool DelegationConsumer::Request(std::string& content) {
  bool res = false;
  content.resize(0);
  EVP_PKEY *pkey = EVP_PKEY_new();
  const EVP_MD *digest = EVP_sha256();
  if(pkey) {
    RSA *rsa = (RSA*)key_;
    if(rsa) {
      if(EVP_PKEY_set1_RSA(pkey, rsa)) {
        X509_REQ *req = X509_REQ_new();
        if(req) {
          //if(X509_REQ_set_version(req,0L)) {
          if(X509_REQ_set_version(req,2L)) {
            if(X509_REQ_set_pubkey(req,pkey)) {
              if(X509_REQ_sign(req,pkey,digest)) {
                BIO *out = BIO_new(BIO_s_mem());
                if(out) {
                  if(PEM_write_bio_X509_REQ(out,req)) {
                    res=true;
                    for(;;) {
                      char s[256];
                      int l = BIO_read(out,s,sizeof(s));
                      if(l <= 0) break;
                      content.append(s,l);;
                    };
                  } else {
                    LogError();
                    std::cerr<<"PEM_write_bio_X509_REQ failed"<<std::endl;
                  };
                  BIO_free_all(out);
                };
              };
            };
          };
          X509_REQ_free(req);
        };
      };
    };
    EVP_PKEY_free(pkey);
  };
  return res;
}

bool DelegationConsumer::Acquire(std::string& content) {
  std::string identity;
  return Acquire(content,identity);
}

bool DelegationConsumer::Acquire(std::string& content, std::string& identity) {
  X509 *cert = NULL;
  STACK_OF(X509) *cert_sk = NULL;
  bool res = false;
  std::string subject;

  if(!key_) return false;

  if(!string_to_x509(content,cert,cert_sk)) goto err;

  content.resize(0);
  if(!x509_to_string(cert,content)) goto err;
  {
    char* buf = X509_NAME_oneline(X509_get_subject_name(cert),NULL,0);
    if(buf) {
      subject=buf;
      OPENSSL_free(buf);
    };
  };
  if(X509_get_ext_by_NID(cert,NID_proxyCertInfo,-1) < 0) {
    identity=subject;
  };

  if(!x509_to_string((RSA*)key_,content)) goto err;
  if(cert_sk) {
    for(int n=0;n<sk_X509_num((STACK_OF(X509) *)cert_sk);++n) {
      X509* v = sk_X509_value((STACK_OF(X509) *)cert_sk,n);
      if(!v) goto err;
      if(!x509_to_string(v,content)) goto err;
      if(identity.empty()) {
        if(X509_get_ext_by_NID(v,NID_proxyCertInfo,-1) < 0) {
          char* buf = X509_NAME_oneline(X509_get_subject_name(v),NULL,0);
          if(buf) {
            identity=buf;
            OPENSSL_free(buf);
          };
        };
      };
    };
  };
  if(identity.empty()) identity = subject;

  res=true;
err:
  if(!res) LogError();
  if(cert) X509_free(cert);
  if(cert_sk) {
    for(int i = 0;i<sk_X509_num(cert_sk);++i) {
      X509* v = sk_X509_value(cert_sk,i);
      if(v) X509_free(v);
    };
    sk_X509_free(cert_sk);
  };
  return res;
}

// ---------------------------------------------------------------------------------

DelegationProvider::DelegationProvider(const std::string& credentials):key_(NULL),cert_(NULL),chain_(NULL) {
  EVP_PKEY *pkey = NULL;
  X509 *cert = NULL;
  STACK_OF(X509) *cert_sk = NULL;
  bool res = false;

  OpenSSLInit();
  EVP_add_digest(EVP_sha256());

  if(!string_to_x509(credentials,cert,pkey,cert_sk)) goto err;
  cert_=cert; cert=NULL;
  key_=pkey; pkey=NULL;
  chain_=cert_sk; cert_sk=NULL;
  res=true;
err:
  if(!res) LogError();
  if(pkey) EVP_PKEY_free(pkey);
  if(cert) X509_free(cert);
  if(cert_sk) {
    for(int i = 0;i<sk_X509_num(cert_sk);++i) {
      X509* v = sk_X509_value(cert_sk,i);
      if(v) X509_free(v);
    };
    sk_X509_free(cert_sk);
  };
}

DelegationProvider::DelegationProvider(const std::string& cert_file,const std::string& key_file,std::istream* inpwd):key_(NULL),cert_(NULL),chain_(NULL) {
  EVP_PKEY *pkey = NULL;
  X509 *cert = NULL;
  STACK_OF(X509) *cert_sk = NULL;
  bool res = false;

  
  OpenSSLInit();
  EVP_add_digest(EVP_sha256());

  if(!string_to_x509(cert_file,key_file,inpwd,cert,pkey,cert_sk)) goto err;
  cert_=cert; cert=NULL;
  key_=pkey; pkey=NULL;
  chain_=cert_sk; cert_sk=NULL;
  res=true;
err:
  if(!res) LogError();
  if(pkey) EVP_PKEY_free(pkey);
  if(cert) X509_free(cert);
  if(cert_sk) {
    for(int i = 0;i<sk_X509_num(cert_sk);++i) {
      X509* v = sk_X509_value(cert_sk,i);
      if(v) X509_free(v);
    };
    sk_X509_free(cert_sk);
  };
}

DelegationProvider::~DelegationProvider(void) {
  if(key_) EVP_PKEY_free((EVP_PKEY*)key_);
  if(cert_) X509_free((X509*)cert_);
  if(chain_) {
    for(;;) {
      X509* v = sk_X509_pop((STACK_OF(X509) *)chain_);
      if(!v) break;
      X509_free(v);
    };
    sk_X509_free((STACK_OF(X509) *)chain_);
  };
}

std::string DelegationProvider::Delegate(const std::string& request,const DelegationRestrictions& restrictions) {
  X509 *cert = NULL;
  X509_REQ *req = NULL;
  BIO* in = NULL;
  EVP_PKEY *pkey = NULL;
  ASN1_INTEGER *sno = NULL;
  ASN1_OBJECT *obj= NULL;
  ASN1_OCTET_STRING* policy_string = NULL;
  X509_EXTENSION *ex = NULL;
  PROXY_CERT_INFO_EXTENSION proxy_info;
  PROXY_POLICY proxy_policy;
  const EVP_MD *digest = EVP_sha256();
  X509_NAME *subject = NULL;
  const char* need_ext = "critical,digitalSignature,keyEncipherment";
  std::string proxy_cn;
  std::string res;
  time_t validity_start_adjustment = 300; //  5 minute grace period for unsync clocks
  time_t validity_start = time(NULL);
  time_t validity_end = (time_t)(-1);
  DelegationRestrictions& restrictions_ = (DelegationRestrictions&)restrictions;
  std::string proxyPolicy;
  std::string proxyPolicyFile;

  if(!cert_) {
    std::cerr<<"Missing certificate chain"<<std::endl;
    return "";
  };
  if(!key_) {
    std::cerr<<"Missing private key"<<std::endl;
    return "";
  };

  // Unify format of request
  std::string prequest = request;
  strip_PEM_request(prequest);
  wrap_PEM_request(prequest);

  in = BIO_new_mem_buf((void*)(prequest.c_str()),prequest.length());
  if(!in) goto err;

  if((!PEM_read_bio_X509_REQ(in,&req,NULL,NULL)) || (!req)) goto err;
  BIO_free_all(in); in=NULL;

 
  //subject=X509_REQ_get_subject_name(req);
  //char* buf = X509_NAME_oneline(subject, 0, 0);
  //std::cerr<<"subject="<<buf<<std::endl;
  //OPENSSL_free(buf);

  if((pkey=X509_REQ_get_pubkey(req)) == NULL) goto err;
  if(X509_REQ_verify(req,pkey) <= 0) goto err;

  cert=X509_new();
  if(!cert) goto err;
  //ci=x->cert_info;
  sno = ASN1_INTEGER_new();
  if(!sno) goto err;
  // TODO - serial number must be unique among generated by proxy issuer
  if(!rand_serial(sno)) goto err;
  if (!X509_set_serialNumber(cert,sno)) goto err;
  proxy_cn=tostring(ASN1_INTEGER_get(sno));
  ASN1_INTEGER_free(sno); sno=NULL;
  X509_set_version(cert,2L);

  /*
   Proxy certificates do not need KeyUsage extension. But
   some old software still expects it to be present.

   From RFC3820:

   If the Proxy Issuer certificate has the KeyUsage extension, the
   Digital Signature bit MUST be asserted.
  */

  X509_add_ext_by_nid(cert,NID_key_usage,(char*)need_ext,-1);

  /*
   From RFC3820:

   If a certificate is a Proxy Certificate, then the proxyCertInfo
   extension MUST be present, and this extension MUST be marked as
   critical.
  
   The pCPathLenConstraint field, if present, specifies the maximum
   depth of the path of Proxy Certificates that can be signed by this
   Proxy Certificate. 

   The proxyPolicy field specifies a policy on the use of this
   certificate for the purposes of authorization.  Within the
   proxyPolicy, the policy field is an expression of policy, and the
   policyLanguage field indicates the language in which the policy is
   expressed.

   *  id-ppl-inheritAll indicates that this is an unrestricted proxy
      that inherits all rights from the issuing PI.  An unrestricted
      proxy is a statement that the Proxy Issuer wishes to delegate all
      of its authority to the bearer (i.e., to anyone who has that proxy
      certificate and can prove possession of the associated private
      key).  For purposes of authorization, this an unrestricted proxy
      effectively impersonates the issuing PI.

   *  id-ppl-independent indicates that this is an independent proxy
      that inherits no rights from the issuing PI.  This PC MUST be
      treated as an independent identity by relying parties.  The only
      rights this PC has are those granted explicitly to it.
  */
  /*
  ex=X509V3_EXT_conf_nid(NULL,NULL,NID_proxyCertInfo,"critical,CA:FALSE");
  if(!ex) goto err;
  if(!X509_add_ext(cert,ex,-1)) goto err;
  X509_EXTENSION_free(ex); ex=NULL;
  */
  memset(&proxy_info,0,sizeof(proxy_info));
  memset(&proxy_policy,0,sizeof(proxy_policy));
  proxy_info.pcPathLengthConstraint=NULL;
  proxy_info.proxyPolicy=&proxy_policy;
  proxy_policy.policyLanguage=NULL;
  proxy_policy.policy=NULL;
  proxyPolicy=restrictions_["proxyPolicy"];
  proxyPolicyFile=restrictions_["proxyPolicyFile"];
  if(!proxyPolicyFile.empty()) {
    if(!proxyPolicy.empty()) goto err; // Two policies supplied
    std::ifstream is(proxyPolicyFile.c_str());
    std::getline(is,proxyPolicy,(char)0);
    if(proxyPolicy.empty()) goto err;
  };
  if(!proxyPolicy.empty()) {
    obj=OBJ_nid2obj(NID_id_ppl_anyLanguage);  // Proxy with policy
    if(!obj) goto err;
    policy_string=ASN1_OCTET_STRING_new();
    if(!policy_string) goto err;
    ASN1_OCTET_STRING_set(policy_string,(const unsigned char*)(proxyPolicy.c_str()),proxyPolicy.length());
    proxy_policy.policyLanguage=obj;
    proxy_policy.policy=policy_string;
  } else {
    PROXY_CERT_INFO_EXTENSION *pci =
        (PROXY_CERT_INFO_EXTENSION*)X509_get_ext_d2i((X509*)cert_,NID_proxyCertInfo,NULL,NULL);
    if(pci) {
      if(pci->proxyPolicy && pci->proxyPolicy->policyLanguage) {
        int const bufSize = 255;
        char* buf = new char[bufSize+1];
        int l = OBJ_obj2txt(buf,bufSize,pci->proxyPolicy->policyLanguage,1);
        if(l > 0) {
          if(l > bufSize) l=bufSize;
          buf[l] = 0;
          if(strcmp(GLOBUS_LIMITED_PROXY_OID,buf) == 0) {
            // Gross hack for globus. If Globus marks own proxy as limited
            // it expects every derived proxy to be limited or at least
            // independent. Independent proxies has little sense in Grid
            // world. So here we make our proxy globus-limited to allow
            // it to be used with globus code.
            obj=OBJ_txt2obj(GLOBUS_LIMITED_PROXY_OID,1);
          };
        };
        delete[] buf;
      };
      PROXY_CERT_INFO_EXTENSION_free(pci);
    };
    if(!obj) {
      obj=OBJ_nid2obj(NID_id_ppl_inheritAll);  // Unrestricted proxy
    };
    if(!obj) goto err;
    proxy_policy.policyLanguage=obj;
  };
  if(X509_add1_ext_i2d(cert,NID_proxyCertInfo,&proxy_info,1,X509V3_ADD_REPLACE) != 1) goto err;
  if(policy_string) { ASN1_OCTET_STRING_free(policy_string); policy_string=NULL; }
  ASN1_OBJECT_free(obj); obj=NULL;
  /*
  PROXY_CERT_INFO_EXTENSION *pci = X509_get_ext_d2i(x, NID_proxyCertInfo, NULL, NULL);
  typedef struct PROXY_CERT_INFO_EXTENSION_st {
        ASN1_INTEGER *pcPathLengthConstraint;
        PROXY_POLICY *proxyPolicy;
        } PROXY_CERT_INFO_EXTENSION;
  typedef struct PROXY_POLICY_st {
        ASN1_OBJECT *policyLanguage;
        ASN1_OCTET_STRING *policy;
        } PROXY_POLICY;
  */

  subject=X509_get_subject_name((X509*)cert_);
  if(!subject) goto err;
  subject=X509_NAME_dup(subject);
  if(!subject) goto err;
  if(!X509_set_issuer_name(cert,subject)) goto err;
  if(!X509_NAME_add_entry_by_NID(subject,NID_commonName,MBSTRING_ASC,(unsigned char*)(proxy_cn.c_str()),proxy_cn.length(),-1,0)) goto err;
  if(!X509_set_subject_name(cert,subject)) goto err;
  X509_NAME_free(subject); subject=NULL;
  if(!(restrictions_["validityStart"].empty())) {
    validity_start=Time(restrictions_["validityStart"]).GetTime();
    validity_start_adjustment = 0;
  };
  if(!(restrictions_["validityEnd"].empty())) {
    validity_end=Time(restrictions_["validityEnd"]).GetTime();
  } else if(!(restrictions_["validityPeriod"].empty())) {
    validity_end=validity_start+Period(restrictions_["validityPeriod"]).GetPeriod();
  };
  validity_start -= validity_start_adjustment;
  //Set "notBefore"
  if( X509_cmp_time(X509_getm_notBefore((X509*)cert_), &validity_start) < 0) {
    X509_time_adj(X509_getm_notBefore(cert), 0L, &validity_start);
  }
  else {
    X509_set1_notBefore(cert, X509_getm_notBefore((X509*)cert_));
  }
  //Set "not After"
  if(validity_end == (time_t)(-1)) {
    X509_set1_notAfter(cert,X509_getm_notAfter((X509*)cert_));
  } else {
    X509_gmtime_adj(X509_getm_notAfter(cert), (validity_end-validity_start));
  };
  X509_set_pubkey(cert,pkey);
  EVP_PKEY_free(pkey); pkey=NULL;

  if(!X509_sign(cert,(EVP_PKEY*)key_,digest)) goto err;
  /*
  {
    int pci_NID = NID_undef;
    ASN1_OBJECT * extension_oid = NULL;
    int nid;
    PROXY_CERT_INFO_EXTENSION* proxy_cert_info;
    X509_EXTENSION *                    extension;

    pci_NID = OBJ_sn2nid(SN_proxyCertInfo);
    for(i=0;i<X509_get_ext_count(cert);i++) {
        extension = X509_get_ext(cert,i);
        extension_oid = X509_EXTENSION_get_object(extension);
        nid = OBJ_obj2nid(extension_oid);
        if(nid == pci_NID) {
            CleanError();
            if((proxy_cert_info = (PROXY_CERT_INFO_EXTENSION*)(X509V3_EXT_d2i(extension))) == NULL) {
              goto err;
              std::cerr<<"X509V3_EXT_d2i failed"<<std::endl;
            }
            break;
        }
    }
  }
  */

  if(!x509_to_string(cert,res)) { res=""; goto err; };
  // Append chain of certificates
  if(!x509_to_string((X509*)cert_,res)) { res=""; goto err; };
  if(chain_) {
    for(int n=0;n<sk_X509_num((STACK_OF(X509) *)chain_);++n) {
      X509* v = sk_X509_value((STACK_OF(X509) *)chain_,n);
      if(!v) { res=""; goto err; };
      if(!x509_to_string(v,res)) { res=""; goto err; };
    };
  };

err:
  if(res.empty()) LogError();
  if(in) BIO_free_all(in);
  if(req) X509_REQ_free(req);
  if(pkey) EVP_PKEY_free(pkey);
  if(cert) X509_free(cert);
  if(sno) ASN1_INTEGER_free(sno);
  if(ex) X509_EXTENSION_free(ex);
  if(obj) ASN1_OBJECT_free(obj);
  if(subject) X509_NAME_free(subject);
  if(policy_string) ASN1_OCTET_STRING_free(policy_string);
  return res;
}

void DelegationProvider::LogError(void) {
  std::string ssl_err;
  ERR_print_errors_cb(&ssl_err_cb,&ssl_err);
}

void DelegationProvider::CleanError(void) {
  std::string ssl_err;
  ERR_print_errors_cb(&ssl_err_cb,&ssl_err);
}

// ---------------------------------------------------------------------------------

DelegationConsumerSOAP::DelegationConsumerSOAP(void):DelegationConsumer() {
}

DelegationConsumerSOAP::DelegationConsumerSOAP(const std::string& content):DelegationConsumer(content) {
}

DelegationConsumerSOAP::~DelegationConsumerSOAP(void) {
}

bool DelegationConsumerSOAP::DelegateCredentialsInit(const std::string& id,const SOAPEnvelope& in,SOAPEnvelope& out) {
/*
  DelegateCredentialsInit

  DelegateCredentialsInitResponse
    TokenRequest - Format (=x509)
      Id (string)
      Value (string)
*/
  if(!in["DelegateCredentialsInit"]) return false;
  std::string x509_request;
  Request(x509_request);
  NS ns; ns["deleg"]=DELEGATION_NAMESPACE;
  out.Namespaces(ns);
  XMLNode resp = out.NewChild("deleg:DelegateCredentialsInitResponse");
  XMLNode token = resp.NewChild("deleg:TokenRequest");
  token.NewAttribute("deleg:Format")="x509";
  token.NewChild("deleg:Id")=id;
  token.NewChild("deleg:Value")=x509_request;
  return true;
}

bool DelegationConsumerSOAP::UpdateCredentials(std::string& credentials,const SOAPEnvelope& in,SOAPEnvelope& out) {
  std::string identity;
  return UpdateCredentials(credentials,identity,in,out);
}

bool DelegationConsumerSOAP::UpdateCredentials(std::string& credentials,std::string& identity,const SOAPEnvelope& in,SOAPEnvelope& out) {
/*
  UpdateCredentials
    DelegatedToken - Format (=x509)
      Id (string)
      Value (string)
      Reference (any, optional)

  UpdateCredentialsResponse
*/
  XMLNode req = in["UpdateCredentials"];
  if(!req) return false;
  credentials = (std::string)(req["DelegatedToken"]["Value"]);
  if(credentials.empty()) {
    // TODO: Fault
    return false;
  };
  if(((std::string)(req["DelegatedToken"].Attribute("Format"))) != "x509") {
    // TODO: Fault
    return false;
  };
  if(!Acquire(credentials,identity)) return false;
  NS ns; ns["deleg"]=DELEGATION_NAMESPACE;
  out.Namespaces(ns);
  out.NewChild("deleg:UpdateCredentialsResponse");
  return true;
}

bool DelegationConsumerSOAP::DelegatedToken(std::string& credentials,XMLNode token) {
  std::string identity;
  return DelegatedToken(credentials,identity,token);
}

bool DelegationConsumerSOAP::DelegatedToken(std::string& credentials,std::string& identity,XMLNode token) {
  credentials = (std::string)(token["Value"]);
  if(credentials.empty()) return false;
  if(((std::string)(token.Attribute("Format"))) != "x509") return false;
  if(!Acquire(credentials,identity)) return false;
  return true;
}

// ---------------------------------------------------------------------------------

DelegationProviderSOAP::DelegationProviderSOAP(const std::string& credentials):DelegationProvider(credentials) {
}

DelegationProviderSOAP::DelegationProviderSOAP(const std::string& cert_file,const std::string& key_file, std::istream* inpwd):DelegationProvider(cert_file,key_file, inpwd) {
}

DelegationProviderSOAP::~DelegationProviderSOAP(void) {
}

bool DelegationProviderSOAP::DelegateCredentialsInit(MCCInterface& interface,MessageContext* context,DelegationProviderSOAP::ServiceType stype) {
  MessageAttributes attributes_in;
  MessageAttributes attributes_out;
  return DelegateCredentialsInit(interface,&attributes_in,&attributes_out,context,stype);
}

static PayloadSOAP* do_process(MCCInterface& interface,MessageAttributes* attributes_in,MessageAttributes* attributes_out,MessageContext* context,PayloadSOAP* in) {
  Message req;
  Message resp;
  WSAHeader header(*in);
  if(attributes_in && (attributes_in->count("SOAP:ACTION") > 0)) {
    header.Action(attributes_in->get("SOAP:ACTION"));
    header.To(attributes_in->get("SOAP:ENDPOINT"));
  }
  req.Attributes(attributes_in);
  req.Context(context);
  req.Payload(in);
  resp.Attributes(attributes_out);
  resp.Context(context);
  MCC_Status r = interface.process(req,resp);
  if(r != STATUS_OK) return NULL;
  if(!resp.Payload()) return NULL;
  PayloadSOAP* resp_soap = NULL;
  try {
    resp_soap=dynamic_cast<PayloadSOAP*>(resp.Payload());
  } catch(std::exception& e) { };
  if(!resp_soap) { delete resp.Payload(); return NULL; };
  resp.Payload(NULL);
  return resp_soap;
}


bool DelegationProviderSOAP::DelegateCredentialsInit(MCCInterface& interface,MessageAttributes* attributes_in,MessageAttributes* attributes_out,MessageContext* context,DelegationProviderSOAP::ServiceType stype) {
  if(stype == DelegationProviderSOAP::ARCDelegation) {
    NS ns; ns["deleg"]=DELEGATION_NAMESPACE;
    PayloadSOAP req_soap(ns);
    req_soap.NewChild("deleg:DelegateCredentialsInit");
    PayloadSOAP* resp_soap = do_process(interface,attributes_in,attributes_out,context,&req_soap);
    if(!resp_soap) return false;
    XMLNode token = (*resp_soap)["DelegateCredentialsInitResponse"]["TokenRequest"];
    if(!token) { delete resp_soap; return false; };
    if(((std::string)(token.Attribute("Format"))) != "x509") { delete resp_soap; return false; };
    id_=(std::string)(token["Id"]);
    request_=(std::string)(token["Value"]);
    delete resp_soap;
    if(id_.empty() || request_.empty()) return false;
    return true;
  } else if((stype == DelegationProviderSOAP::GDS10) ||
            (stype == DelegationProviderSOAP::GDS10RENEW)) {
    // Not implemented yet due to problems with id
  /*
  } else if((stype == DelegationProviderSOAP::GDS20) ||
            (stype == DelegationProviderSOAP::GDS20RENEW)) {
    NS ns; ns["deleg"]=GDS20_NAMESPACE;
    PayloadSOAP req_soap(ns);
    req_soap.NewChild("deleg:getNewProxyReq");
    PayloadSOAP* resp_soap = do_process(interface,attributes_in,attributes_out,context,&req_soap);
    if(!resp_soap) return false;
    XMLNode token = (*resp_soap)["getNewProxyReqResponse"]["NewProxyReq"];
    if(!token) { delete resp_soap; return false; };
    id_=(std::string)(token["delegationID"]);
    request_=(std::string)(token["proxyRequest"]);
    delete resp_soap;
    if(id_.empty() || request_.empty()) return false;
    return true;
  */
  } else if((stype == DelegationProviderSOAP::GDS20) ||
            (stype == DelegationProviderSOAP::GDS20RENEW) ||
            (stype == DelegationProviderSOAP::EMIDS) ||
            (stype == DelegationProviderSOAP::EMIDSRENEW)) {
    NS ns; ns["deleg"]=EMIDS_NAMESPACE;
    PayloadSOAP req_soap(ns);
    if((!id_.empty()) && 
       ((stype == DelegationProviderSOAP::GDS20RENEW) ||
        (stype == DelegationProviderSOAP::EMIDSRENEW))) {
      req_soap.NewChild("deleg:renewProxyReq").NewChild("deleg:delegationID") = id_;
      PayloadSOAP* resp_soap = do_process(interface,attributes_in,attributes_out,context,&req_soap);
      if(!resp_soap) return false;
      XMLNode token = (*resp_soap)["renewProxyReqResponse"];
      if(!token) { delete resp_soap; return false; };
      request_=(std::string)(token["renewProxyReqReturn"]);
      delete resp_soap;
    } else {
      req_soap.NewChild("deleg:getNewProxyReq");
      PayloadSOAP* resp_soap = do_process(interface,attributes_in,attributes_out,context,&req_soap);
      if(!resp_soap) return false;
      XMLNode token = (*resp_soap)["getNewProxyReqResponse"];
      if(!token) { delete resp_soap; return false; };
      id_=(std::string)(token["delegationID"]);
      request_=(std::string)(token["proxyRequest"]);
      delete resp_soap;
    }
    if(id_.empty() || request_.empty()) return false;
    return true;
  } else if((stype == DelegationProviderSOAP::EMIES)) {
    NS ns; ns["deleg"]=EMIES_NAMESPACE; ns["estypes"]=EMIES_TYPES_NAMESPACE;
    PayloadSOAP req_soap(ns);
    XMLNode op = req_soap.NewChild("deleg:InitDelegation");
    op.NewChild("deleg:CredentialType") = "RFC3820";
    //op.NewChild("deleg:RenewalID") = "";
    PayloadSOAP* resp_soap = do_process(interface,attributes_in,attributes_out,context,&req_soap);
    if(!resp_soap) return false;
    XMLNode token = (*resp_soap)["InitDelegationResponse"];
    if(!token) { delete resp_soap; return false; };
    id_=(std::string)(token["DelegationID"]);
    request_=(std::string)(token["CSR"]);
    delete resp_soap;
    if(id_.empty() || request_.empty()) return false;
    //wrap_PEM_request(request_);
    return true;
  };
  return false;
}

bool DelegationProviderSOAP::UpdateCredentials(MCCInterface& interface,MessageContext* context,const DelegationRestrictions& /* restrictions */,DelegationProviderSOAP::ServiceType stype) {
  MessageAttributes attributes_in;
  MessageAttributes attributes_out;
  return UpdateCredentials(interface,&attributes_in,&attributes_out,context,DelegationRestrictions(),stype);
}

bool DelegationProviderSOAP::UpdateCredentials(MCCInterface& interface,MessageAttributes* attributes_in,MessageAttributes* attributes_out,MessageContext* context,const DelegationRestrictions& restrictions,DelegationProviderSOAP::ServiceType stype) {
  if(id_.empty()) return false;
  if(request_.empty()) return false;
  if(stype == DelegationProviderSOAP::ARCDelegation) {
    std::string delegation = Delegate(request_,restrictions);
    if(delegation.empty()) return false;
    NS ns; ns["deleg"]=DELEGATION_NAMESPACE;
    PayloadSOAP req_soap(ns);
    XMLNode token = req_soap.NewChild("deleg:UpdateCredentials").NewChild("deleg:DelegatedToken");
    token.NewAttribute("deleg:Format")="x509";
    token.NewChild("deleg:Id")=id_;
    token.NewChild("deleg:Value")=delegation;
    PayloadSOAP* resp_soap = do_process(interface,attributes_in,attributes_out,context,&req_soap);
    if(!resp_soap) return false;
    if(!(*resp_soap)["UpdateCredentialsResponse"]) {
      delete resp_soap;
      return false;
    };
    delete resp_soap;
    return true;
  } else if((stype == DelegationProviderSOAP::GDS10) ||
            (stype == DelegationProviderSOAP::GDS10RENEW)) {
    // No implemented yet due to problems with id
  /*
  } else if((stype == DelegationProviderSOAP::GDS20) ||
            (stype == DelegationProviderSOAP::GDS20RENEW)) {
    std::string delegation = Delegate(request_,restrictions);
    if(delegation.empty()) return false;
    NS ns; ns["deleg"]=GDS20_NAMESPACE;
    PayloadSOAP req_soap(ns);
    XMLNode token = req_soap.NewChild("deleg:putProxy");
    token.NewChild("deleg:delegationID")=id_;
    token.NewChild("deleg:proxy")=delegation;
    PayloadSOAP* resp_soap = do_process(interface,attributes_in,attributes_out,context,&req_soap);
    if(!resp_soap) return false;
    if(!(*resp_soap)["putProxyResponse"]) {
      delete resp_soap;
      return false;
    };
    delete resp_soap;
    return true;
  */
  } else if((stype == DelegationProviderSOAP::GDS20) ||
            (stype == DelegationProviderSOAP::GDS20RENEW) ||
            (stype == DelegationProviderSOAP::EMIDS) ||
            (stype == DelegationProviderSOAP::EMIDSRENEW)) {
    std::string delegation = Delegate(request_,restrictions);
    //strip_PEM_cert(delegation);
    if(delegation.empty()) return false;
    NS ns; ns["deleg"]=EMIDS_NAMESPACE;
    PayloadSOAP req_soap(ns);
    XMLNode token = req_soap.NewChild("deleg:putProxy");
    token.NewChild("delegationID")=id_; // unqualified
    token.NewChild("proxy")=delegation; // unqualified
    PayloadSOAP* resp_soap = do_process(interface,attributes_in,attributes_out,context,&req_soap);
    if(!resp_soap) return false;
    if(resp_soap->Size() > 0) {
      delete resp_soap;
      return false;
    };
    delete resp_soap;
    return true;
  } else if((stype == DelegationProviderSOAP::EMIES)) {
    std::string delegation = Delegate(request_,restrictions);
    //if((!strip_PEM_cert(delegation)) || (delegation.empty())) return false;
    if(delegation.empty()) return false;
    NS ns; ns["deleg"]=EMIES_NAMESPACE; ns["estypes"]=EMIES_TYPES_NAMESPACE;
    PayloadSOAP req_soap(ns);
    XMLNode token = req_soap.NewChild("deleg:PutDelegation");
    //token.NewChild("deleg:CredentialType")="RFC3820";
    token.NewChild("deleg:DelegationId")=id_;
    token.NewChild("deleg:Credential")=delegation;
    PayloadSOAP* resp_soap = do_process(interface,attributes_in,attributes_out,context,&req_soap);
    if(!resp_soap) return false;
    if((std::string)((*resp_soap)["PutDelegationResponse"]) != "SUCCESS") {
      delete resp_soap;
      return false;
    };
    delete resp_soap;
    return true;
  };
  return false;
}

bool DelegationProviderSOAP::DelegatedToken(XMLNode parent) {
  if(id_.empty()) return false;
  if(request_.empty()) return false;
  std::string delegation = Delegate(request_);
  if(delegation.empty()) return false;
  NS ns; ns["deleg"]=DELEGATION_NAMESPACE;
  parent.Namespaces(ns);
  XMLNode token = parent.NewChild("deleg:DelegatedToken");
  token.NewAttribute("deleg:Format")="x509";
  token.NewChild("deleg:Id")=id_;
  token.NewChild("deleg:Value")=delegation;
  return true;
}

// ---------------------------------------------------------------------------------

class DelegationContainerSOAP::Consumer {
 public:
  DelegationConsumerSOAP* deleg;
  unsigned int usage_count;
  unsigned int acquired;
  bool to_remove;
  time_t last_used;
  std::string client_id;
  DelegationContainerSOAP::ConsumerIterator previous;
  DelegationContainerSOAP::ConsumerIterator next;
  Consumer(void):
     deleg(NULL),usage_count(0),acquired(0),to_remove(false),last_used(time(NULL)) { };
  Consumer(DelegationConsumerSOAP* d):
     deleg(d),usage_count(0),acquired(0),to_remove(false),last_used(time(NULL)) { };
  Consumer& operator=(DelegationConsumerSOAP* d) {
    deleg=d; usage_count=0; acquired=0; to_remove=false; last_used=time(NULL); return *this;
  };
};

#define DELEGFAULT(out) { \
  for(XMLNode old = out.Child();(bool)old;old = out.Child()) old.Destroy(); \
  SOAPFault((out),SOAPFault::Receiver,failure_.c_str()); \
}

#define GDS10FAULT(out) { \
  for(XMLNode old = out.Child();(bool)old;old = out.Child()) old.Destroy(); \
  SOAPFault((out),SOAPFault::Receiver,failure_.c_str()); \
}

#define GDS20FAULT(out) { \
  for(XMLNode old = out.Child();(bool)old;old = out.Child()) old.Destroy(); \
  XMLNode r = SOAPFault((out),SOAPFault::Receiver,"").Detail(true); \
  XMLNode ex = r.NewChild("DelegationException"); \
  ex.Namespaces(ns); ex.NewChild("msg") = (failure_); \
}

#define EMIDSFAULT(out) { \
  for(XMLNode old = out.Child();(bool)old;old = out.Child()) old.Destroy(); \
  XMLNode r = SOAPFault((out),SOAPFault::Receiver,"").Detail(true); \
  XMLNode ex = r.NewChild("deleg:DelegationException",ns); \
  ex.NewChild("msg") = (failure_); \
}

// InternalServiceDelegationFault
#define EMIESFAULT(out,msg) { \
  for(XMLNode old = out.Child();(bool)old;old = out.Child()) old.Destroy(); \
  XMLNode r = SOAPFault((out),SOAPFault::Receiver,"").Detail(true); \
  XMLNode ex = r.NewChild("InternalServiceDelegationFault"); \
  ex.Namespaces(ns); \
  ex.NewChild("estypes:Message") = (msg); \
  ex.NewChild("estypes:Timestamp") = Time().str(ISOTime); \
  /*ex.NewChild("estypes:Description") = "";*/ \
  /*ex.NewChild("estypes:FailureCode") = "0";*/ \
}

// UnknownDelegationIDFault
#define EMIESIDFAULT(out,msg) { \
  XMLNode r = SOAPFault((out),SOAPFault::Receiver,"").Detail(true); \
  XMLNode ex = r.NewChild("UnknownDelegationIDFault"); \
  ex.Namespaces(ns); \
  ex.NewChild("Message") = (msg); \
  ex.NewChild("Timestamp") = Time().str(ISOTime); \
  /*ex.NewChild("Description") = "";*/ \
  /*ex.NewChild("FailureCode") = "0";*/ \
}

// InternalBaseFault
// AccessControlFault

DelegationContainerSOAP::DelegationContainerSOAP(void) {
  max_size_=0;         // unlimited size of container
  max_duration_=30;    // 30 seconds for delegation
  max_usage_=2;        // allow 1 failure
  context_lock_=false;
  consumers_first_=consumers_.end();
  consumers_last_=consumers_.end();
}

DelegationContainerSOAP::~DelegationContainerSOAP(void) {
  lock_.lock();
  ConsumerIterator i = consumers_.begin();
  for(;i!=consumers_.end();++i) {
    if(i->second->deleg) delete i->second->deleg;
    delete i->second;
  };
  lock_.unlock();
}

DelegationConsumerSOAP* DelegationContainerSOAP::AddConsumer(std::string& id,const std::string& client) {
  lock_.lock();
  if(id.empty()) {
    for(int tries = 0;tries<1000;++tries) {
      GUID(id);
      ConsumerIterator i = consumers_.find(id);
      if(i == consumers_.end()) break;
      id.resize(0);
    };
    if(id.empty()) {
      failure_ = "Failed to generate unique identifier";
      lock_.unlock();
      return NULL;
    };
  } else {
    ConsumerIterator i = consumers_.find(id);
    if(i != consumers_.end()) {
      failure_ = "Requested identifier already in use";
      lock_.unlock();
      return NULL;
    };
  };
  Consumer* c = new Consumer();
  c->deleg=new DelegationConsumerSOAP();
  c->client_id=client;
  c->previous=consumers_.end();
  c->next=consumers_first_;
  ConsumerIterator i = consumers_.insert(consumers_.begin(),make_pair(id,c));
  if(consumers_first_ != consumers_.end()) consumers_first_->second->previous=i;
  consumers_first_=i;
  if(consumers_last_ == consumers_.end()) consumers_last_=i;
  i->second->acquired = 1;
  DelegationConsumerSOAP* cs = i->second->deleg;
  lock_.unlock();
  return cs;
}

bool DelegationContainerSOAP::TouchConsumer(DelegationConsumerSOAP* c,const std::string& /*credentials*/) {
  Glib::Mutex::Lock lock(lock_);
  ConsumerIterator i = find(c);
  if(i == consumers_.end()) { failure_ = "Delegation not found"; return false; };
  i->second->last_used=time(NULL);
  if(((++(i->second->usage_count)) > max_usage_) && (max_usage_ > 0)) {
    i->second->to_remove=true;
  } else {
    i->second->to_remove=false;
  };
  if(i == consumers_first_) return true;
  ConsumerIterator previous = i->second->previous;
  ConsumerIterator next = i->second->next;
  if(previous != consumers_.end()) previous->second->next=next;
  if(next != consumers_.end()) next->second->previous=previous;
  i->second->previous=consumers_.end();
  i->second->next=consumers_first_;
  if(consumers_first_ != consumers_.end()) consumers_first_->second->previous=i;
  consumers_first_=i;
  return true;
}

bool DelegationContainerSOAP::QueryConsumer(DelegationConsumerSOAP* c,std::string& credentials) {
  Glib::Mutex::Lock lock(lock_);
  ConsumerIterator i = find(c);
  if(i == consumers_.end()) { failure_ = "Delegation not found"; return false; };
  if(i->second->deleg) i->second->deleg->Backup(credentials); // only key is available
  return true;
}

void DelegationContainerSOAP::ReleaseConsumer(DelegationConsumerSOAP* c) {
  lock_.lock();
  ConsumerIterator i = find(c);
  if(i == consumers_.end()) { lock_.unlock(); return; };
  if(i->second->acquired > 0) --(i->second->acquired);
  remove(i);
  lock_.unlock();
  return;
}

bool DelegationContainerSOAP::RemoveConsumer(DelegationConsumerSOAP* c) {
  lock_.lock();
  ConsumerIterator i = find(c);
  if(i == consumers_.end()) { lock_.unlock(); return false; };
  if(i->second->acquired > 0) --(i->second->acquired);
  i->second->to_remove=true;
  remove(i);
  lock_.unlock();
  return true;
}

DelegationContainerSOAP::ConsumerIterator DelegationContainerSOAP::find(DelegationConsumerSOAP* c) {
  ConsumerIterator i = consumers_first_;
  for(;i!=consumers_.end();i=i->second->next) {
    if(i->second->deleg == c) break;
  };
  return i;
}

bool DelegationContainerSOAP::remove(ConsumerIterator i) {
  if(i->second->acquired > 0) return false;
  if(!i->second->to_remove) return false;
  ConsumerIterator previous = i->second->previous;
  ConsumerIterator next = i->second->next;
  if(previous != consumers_.end()) previous->second->next=next;
  if(next != consumers_.end()) next->second->previous=previous;
  if(consumers_first_ == i) consumers_first_=next; 
  if(consumers_last_ == i) consumers_last_=previous; 
  if(i->second->deleg) delete i->second->deleg;
  delete i->second;
  consumers_.erase(i);
  return true;
}

void DelegationContainerSOAP::CheckConsumers(void) {
  if(max_size_ > 0) {
    lock_.lock();
    ConsumerIterator i = consumers_last_;
    unsigned int count = consumers_.size();
    while(count > max_size_) {
      if(i == consumers_.end()) break;
      ConsumerIterator prev = i->second->previous;
      i->second->to_remove=true;
      remove(i);
      i=prev;
      --count;
    };
    lock_.unlock();
  };
  if(max_duration_ > 0) {
    lock_.lock();
    time_t t = time(NULL);
    for(ConsumerIterator i = consumers_last_;i!=consumers_.end();) {
      ConsumerIterator next = i->second->next;
      if(((unsigned int)(t - i->second->last_used)) > max_duration_) {
        i->second->to_remove=true;
        remove(i);
        i=next;
      } else {
        break;
      };
    };
    lock_.unlock();
  };
  return;
}

bool DelegationContainerSOAP::DelegateCredentialsInit(const SOAPEnvelope& in,SOAPEnvelope& out,const std::string& client) {
  std::string id;
  DelegationConsumerSOAP* consumer = AddConsumer(id,client);
  if(!consumer) {
    DELEGFAULT(out);
    return true;
  };
  if(!(consumer->DelegateCredentialsInit(id,in,out))) {
    RemoveConsumer(consumer);
    failure_ = "Failed to generate credentials request"; DELEGFAULT(out);
    return true;
  };
  ReleaseConsumer(consumer);
  CheckConsumers();
  return true;
}

bool DelegationContainerSOAP::UpdateCredentials(std::string& credentials,const SOAPEnvelope& in,SOAPEnvelope& out,const std::string& client) {
  std::string identity;
  return UpdateCredentials(credentials,identity,in,out,client);
}

#define ClientAuthorized(consumer,client) \
  ( ((consumer)->client_id.empty()) || ((consumer)->client_id == (client)) )

DelegationConsumerSOAP* DelegationContainerSOAP::FindConsumer(const std::string& id,const std::string& client) {
  Glib::Mutex::Lock lock(lock_);
  ConsumerIterator i = consumers_.find(id);
  if(i == consumers_.end()) { failure_ = "Identifier not found"; return NULL; };
  if(!(i->second->deleg)) { failure_ = "Identifier has no delegation associated"; return NULL; };
  if(!ClientAuthorized(i->second,client)) { failure_ = "Client not authorized for this identifier"; return NULL; };
  ++(i->second->acquired);
  DelegationConsumerSOAP* cs = i->second->deleg;
  return cs;
}

bool DelegationContainerSOAP::UpdateCredentials(std::string& credentials,std::string& identity, const SOAPEnvelope& in,SOAPEnvelope& out,const std::string& client) {
  std::string id = (std::string)(const_cast<SOAPEnvelope&>(in)["UpdateCredentials"]["DelegatedToken"]["Id"]);
  if(id.empty()) {
    failure_ = "Credentials identifier is missing"; DELEGFAULT(out);
    return true;
  };
  DelegationConsumerSOAP* c = FindConsumer(id,client);
  if(!c) {
    DELEGFAULT(out);
    return true;
  };
  if(!c->UpdateCredentials(credentials,identity,in,out)) {
    ReleaseConsumer(c);
    failure_ = "Failed to acquire credentials"; DELEGFAULT(out);
    return true;
  };
  if(!TouchConsumer(c,credentials)) {
    ReleaseConsumer(c);
    DELEGFAULT(out);
    return true;
  };
  ReleaseConsumer(c);
  return true;
}

bool DelegationContainerSOAP::DelegatedToken(std::string& credentials,XMLNode token,const std::string& client) {
  std::string identity;
  return DelegatedToken(credentials,identity,token,client);
}

bool DelegationContainerSOAP::DelegatedToken(std::string& credentials,std::string& identity,XMLNode token,const std::string& client) {
  std::string id = (std::string)(token["Id"]);
  if(id.empty()) return false;
  DelegationConsumerSOAP* c = FindConsumer(id,client);
  if(!c) return false;
  bool r = c->DelegatedToken(credentials,identity,token);
  if(!TouchConsumer(c,credentials)) r = false;
  ReleaseConsumer(c);
  return r;
}

bool DelegationContainerSOAP::Process(const SOAPEnvelope& in,SOAPEnvelope& out,const std::string& client) {
  std::string credentials;
  return Process(credentials,in,out,client);
}

bool DelegationContainerSOAP::Process(std::string& credentials,const SOAPEnvelope& in,SOAPEnvelope& out,const std::string& client) {
  credentials.resize(0);
  XMLNode op = ((SOAPEnvelope&)in).Child(0);
  if(!op) return false;
  std::string op_ns = op.Namespace();
  std::string op_name = op.Name();
  if(op_ns == DELEGATION_NAMESPACE) {
    // ARC Delegation
    if(op_name == "DelegateCredentialsInit") {
      return DelegateCredentialsInit(in,out,client);
    } else if(op_name == "UpdateCredentials") {
      return UpdateCredentials(credentials,in,out,client);
    };
  } else if(op_ns == GDS10_NAMESPACE) {
    // Original GDS
    NS ns("",GDS10_NAMESPACE);
    if(op_name == "getProxyReq") {
      Arc::XMLNode r = out.NewChild("getProxyReqResponse",ns);
      std::string id = op["delegationID"];
      if(id.empty()) {
        failure_ = "No identifier specified"; GDS10FAULT(out);
        return true;
      };
      // check if new id or id belongs to this client
      bool found = true;
      DelegationConsumerSOAP* c = FindConsumer(id,client);
      if(!c) {
        found=false;
        if(!(c = AddConsumer(id,client))) {
          GDS10FAULT(out);
          return true;
        };
      };
      std::string x509_request;
      c->Request(x509_request);
      if(x509_request.empty()) {
        if(found) ReleaseConsumer(c);
        else RemoveConsumer(c);
        failure_ = "Failed to generate request"; GDS10FAULT(out);
        return true;
      };
      ReleaseConsumer(c);
      r.NewChild("request") = x509_request;
      return true;
    } else if(op_name == "putProxy") {
      Arc::XMLNode r = out.NewChild("putProxyResponse",ns);
      std::string id = op["delegationID"];
      std::string cred = op["proxy"];
      if(id.empty()) {
        failure_ = "Identifier is missing"; GDS10FAULT(out);
        return true;
      };
      if(cred.empty()) {
        failure_ = "proxy is missing"; GDS10FAULT(out);
        return true;
      };
      DelegationConsumerSOAP* c = FindConsumer(id,client);
      if(!c) {
        GDS10FAULT(out);
        return true;
      };
      if(!c->Acquire(cred)) {
        ReleaseConsumer(c);
        failure_ = "Failed to acquire credentials"; GDS10FAULT(out);
        return true;
      };
      if(!TouchConsumer(c,cred)) {
        ReleaseConsumer(c);
        GDS10FAULT(out);
        return true;
      };
      ReleaseConsumer(c);
      credentials = cred;
      return true;
    };
  /*
  } else if(op_ns == GDS20_NAMESPACE) {
    // Glite GDS
    NS ns("",GDS20_NAMESPACE);
    if(op_name == "getVersion") {
      Arc::XMLNode r = out.NewChild("getVersionResponse",ns);
      r.NewChild("getVersionReturn")="0";
      return true;
    } else if(op_name == "getInterfaceVersion") {
      Arc::XMLNode r = out.NewChild("getInterfaceVersionResponse",ns);
      r.NewChild("getInterfaceVersionReturn")="2";
      return true;
    } else if(op_name == "getServiceMetadata") {
      //Arc::XMLNode r = out.NewChild("getServiceMetadataResponse",ns);
      GDS20FAULT(out,"Service has no metadata");
      return true;
    } else if(op_name == "getProxyReq") {
      Arc::XMLNode r = out.NewChild("getProxyReqResponse",ns);
      std::string id = op["delegationID"];
      if(id.empty()) {
        GDS20FAULT(out,"No identifier specified");
        return true;
      };
      // check if new id or id belongs to this client
      bool found = true;
      DelegationConsumerSOAP* c = FindConsumer(id,client);
      if(!c) {
        found = false;
        if(!(c = AddConsumer(id,client))) {
          GDS20FAULT(out,"Wrong identifier"); // ?
          return true;
        };
      };
      std::string x509_request;
      c->Request(x509_request);
      if(x509_request.empty()) {
        if(found) ReleaseConsumer(c);
        else RemoveConsumer(c);
        GDS20FAULT(out,"Failed to generate request");
        return true;
      };
      ReleaseConsumer(c);
      r.NewChild("getProxyReqReturn") = x509_request;
      return true;
    } else if(op_name == "getNewProxyReq") {
      Arc::XMLNode r = out.NewChild("getNewProxyReqResponse",ns);
      std::string id;
      DelegationConsumerSOAP* c = AddConsumer(id,client);
      if(!c) {
        GDS20FAULT(out,"Failed to generate identifier");
        return true;
      };
      std::string x509_request;
      c->Request(x509_request);
      if(x509_request.empty()) {
        RemoveConsumer(c);
        GDS20FAULT(out,"Failed to generate request");
        return true;
      };
      ReleaseConsumer(c);
      CheckConsumers();
      Arc::XMLNode ret = r.NewChild("NewProxyReq");
      ret.NewChild("proxyRequest") = x509_request;
      ret.NewChild("delegationID") = id;
      return true;
    } else if(op_name == "putProxy") {
      Arc::XMLNode r = out.NewChild("putProxyResponse",ns);
      std::string id = op["delegationID"];
      std::string cred = op["proxy"];
      if(id.empty()) {
        GDS20FAULT(out,"Identifier is missing");
        return true;
      };
      if(cred.empty()) {
        GDS20FAULT(out,"proxy is missing");
        return true;
      };
      DelegationConsumerSOAP* c = FindConsumer(id,client);
      if(!c) {
        GDS20FAULT(out,"Failed to find identifier");
        return true;
      };
      if(!c->Acquire(cred)) {
        ReleaseConsumer(c);
        GDS20FAULT(out,"Failed to acquire credentials");
        return true;
      };
      if(!TouchConsumer(c,cred)) {
        ReleaseConsumer(c);
        GDS20FAULT(out,"Failed to process credentials");
        return true;
      };
      ReleaseConsumer(c);
      credentials = cred;
      return true;
    } else if(op_name == "renewProxyReq") {
      Arc::XMLNode r = out.NewChild("renewProxyReqResponse",ns);
      std::string id = op["delegationID"];
      if(id.empty()) {
        GDS20FAULT(out,"No identifier specified");
        return true;
      };
      // check if new id or id belongs to this client
      bool found = true;
      DelegationConsumerSOAP* c = FindConsumer(id,client);
      if(!c) {
        found=false;
        // Probably it is wrong to create new delegation if
        // client explicitly requests to renew.
        //if(!(c = AddConsumer(id,client))) {
          GDS20FAULT(out,"Wrong identifier"); // ?
          return true;
        //};
      };
      std::string x509_request;
      c->Request(x509_request);
      if(x509_request.empty()) {
        if(found) ReleaseConsumer(c);
        else RemoveConsumer(c);
        GDS20FAULT(out,"Failed to generate request");
        return true;
      };
      ReleaseConsumer(c);
      r.NewChild("renewProxyReqReturn") = x509_request;
      return true;
    } else if(op_name == "getTerminationTime") {
      Arc::XMLNode r = out.NewChild("getTerminationTimeResponse",ns);
      std::string id = op["delegationID"];
      if(id.empty()) {
        GDS20FAULT(out,"No identifier specified");
        return true;
      };
      DelegationConsumerSOAP* c = FindConsumer(id,client);
      if(!c) {
        GDS20FAULT(out,"Wrong identifier");
        return true;
      };
      std::string credentials;
      if((!QueryConsumer(c,credentials)) || credentials.empty()) {
        ReleaseConsumer(c);
        GDS20FAULT(out,"Delegated credentials missing");
        return true;
      };
      ReleaseConsumer(c);
      cred_info_t info;
      if(!get_cred_info(credentials,info)) {
        GDS20FAULT(out,"Delegated credentials missing");
        return true;
      };
      if(info.valid_till == Time(Time::UNDEFINED)) info.valid_till = Time();
      r.NewChild("getTerminationTimeReturn") = info.valid_till.str(ISOTime);
      return true;
    } else if(op_name == "destroy") {
      Arc::XMLNode r = out.NewChild("destroyResponse",ns);
      std::string id = op["delegationID"];
      if(id.empty()) {
        GDS20FAULT(out,"No identifier specified");
        return true;
      };
      DelegationConsumerSOAP* c = FindConsumer(id,client);
      if(c) RemoveConsumer(c);
      return true;
    };
  */
  } else if(op_ns == EMIDS_NAMESPACE) {
    // EMI GDS == gLite GDS
    NS ns("deleg",EMIDS_NAMESPACE);
    if(op_name == "getVersion") {
      // getVersion
      //
      // getVersionResponse
      //   getVersionReturn
      // DelegationException
      Arc::XMLNode r = out.NewChild("deleg:getVersionResponse",ns);
      r.NewChild("getVersionReturn")="0";
      return true;
    } else if(op_name == "getInterfaceVersion") {
      // getInterfaceVersion
      //
      // getInterfaceVersionResponse
      //   getInterfaceVersionReturn
      // DelegationException
      Arc::XMLNode r = out.NewChild("deleg:getInterfaceVersionResponse",ns);
      r.NewChild("getInterfaceVersionReturn")="2.1";
      return true;
    } else if(op_name == "getServiceMetadata") {
      // getServiceMetadata
      //   key
      //
      // getServiceMetadataResponse
      //   getServiceMetadataReturn
      // DelegationException
      //Arc::XMLNode r = out.NewChild("getServiceMetadataResponse");
      //r.Namespaces(ns);
      failure_ = "Service has no metadata"; EMIDSFAULT(out);
      return true;
    } else if(op_name == "getProxyReq") {
      // getProxyReq
      //   delegationID
      //
      // getProxyReqResponse
      //   getProxyReqReturn
      // DelegationException
      Arc::XMLNode r = out.NewChild("deleg:getProxyReqResponse",ns);
      std::string id = op["delegationID"];
      // check if new id or id belongs to this client
      bool found = true;
      DelegationConsumerSOAP* c = id.empty()?NULL:FindConsumer(id,client);
      if(!c) {
        found = false;
        if(!(c = AddConsumer(id,client))) {
          EMIDSFAULT(out);
          return true;
        };
      };
      std::string x509_request;
      c->Request(x509_request);
      if(x509_request.empty()) {
        if(found) ReleaseConsumer(c);
        else RemoveConsumer(c);
        failure_ = "Failed to generate request"; EMIDSFAULT(out);
        return true;
      };
      ReleaseConsumer(c);
      r.NewChild("getProxyReqReturn") = x509_request;
      return true;
    } else if(op_name == "getNewProxyReq") {
      // getNewProxyReq
      //
      // getNewProxyReqResponse
      //   proxyRequest
      //   delegationID
      // DelegationException
      Arc::XMLNode r = out.NewChild("deleg:getNewProxyReqResponse",ns);
      std::string id;
      DelegationConsumerSOAP* c = AddConsumer(id,client);
      if(!c) {
        EMIDSFAULT(out);
        return true;
      };
      std::string x509_request;
      c->Request(x509_request);
      if(x509_request.empty()) {
        RemoveConsumer(c);
        failure_ = "Failed to generate request"; EMIDSFAULT(out);
        return true;
      };
      ReleaseConsumer(c);
      CheckConsumers();
      r.NewChild("proxyRequest") = x509_request;
      r.NewChild("delegationID") = id;
      return true;
    } else if(op_name == "putProxy") {
      // putProxy
      //   delegationID
      //   proxy
      //
      // DelegationException
      std::string id = op["delegationID"];
      std::string cred = op["proxy"];
      if(id.empty()) {
        failure_ = "Identifier is missing"; EMIDSFAULT(out);
        return true;
      };
      if(cred.empty()) {
        failure_ = "proxy is missing"; EMIDSFAULT(out);
        return true;
      };
      DelegationConsumerSOAP* c = FindConsumer(id,client);
      if(!c) {
        EMIDSFAULT(out);
        return true;
      };
      if(!c->Acquire(cred)) {
        ReleaseConsumer(c);
        failure_ = "Failed to acquire credentials"; EMIDSFAULT(out);
        return true;
      };
      if(!TouchConsumer(c,cred)) {
        ReleaseConsumer(c);
        EMIDSFAULT(out);
        return true;
      };
      ReleaseConsumer(c);
      credentials = cred;
      return true;
    } else if(op_name == "renewProxyReq") {
      // renewProxyReq
      //   delegationID
      //
      // renewProxyReqResponse
      //   renewProxyReqReturn
      // DelegationException
      Arc::XMLNode r = out.NewChild("deleg:renewProxyReqResponse",ns);
      std::string id = op["delegationID"];
      if(id.empty()) {
        failure_ = "No identifier specified"; EMIDSFAULT(out);
        return true;
      };
      // check if new id or id belongs to this client
      bool found = true;
      DelegationConsumerSOAP* c = FindConsumer(id,client);
      if(!c) {
        found=false;
        if(!(c = AddConsumer(id,client))) {
          EMIDSFAULT(out);
          return true;
        };
      };
      std::string x509_request;
      c->Request(x509_request);
      if(x509_request.empty()) {
        if(found) ReleaseConsumer(c);
        else RemoveConsumer(c);
        failure_ = "Failed to generate request"; EMIDSFAULT(out);
        return true;
      };
      ReleaseConsumer(c);
      r.NewChild("renewProxyReqReturn") = x509_request;
      return true;
    } else if(op_name == "getTerminationTime") {
      // getTerminationTime
      //   delegationID
      //
      // getTerminationTimeResponse
      //   getTerminationTimeReturn (dateTime)
      // DelegationException
      Arc::XMLNode r = out.NewChild("deleg:getTerminationTimeResponse",ns);
      std::string id = op["delegationID"];
      if(id.empty()) {
        failure_ = "No identifier specified"; EMIDSFAULT(out);
        return true;
      };
      DelegationConsumerSOAP* c = FindConsumer(id,client);
      if(!c) {
        EMIDSFAULT(out);
        return true;
      };
      std::string credentials;
      if((!QueryConsumer(c,credentials)) || credentials.empty()) {
        ReleaseConsumer(c);
        if(failure_.empty()) failure_ = "Delegated credentials missing";
        EMIDSFAULT(out);
        return true;
      };
      ReleaseConsumer(c);
      cred_info_t info;
      if(!get_cred_info(credentials,info)) {
        failure_ = "Delegated credentials missing"; EMIDSFAULT(out);
        return true;
      };
      if(info.valid_till == Time(Time::UNDEFINED)) info.valid_till = Time();
      r.NewChild("getTerminationTimeReturn") = info.valid_till.str(ISOTime);
      return true;
    } else if(op_name == "destroy") {
      // destroy
      //   delegationID
      //
      // DelegationException
      Arc::XMLNode r = out.NewChild("deleg:destroyResponse",ns);
      std::string id = op["delegationID"];
      if(id.empty()) {
        failure_ = "No identifier specified"; EMIDSFAULT(out);
        return true;
      };
      DelegationConsumerSOAP* c = FindConsumer(id,client);
      if(c) RemoveConsumer(c);
      return true;
    };
  } else if(op_ns == EMIES_NAMESPACE) {
    // EMI Execution Service own delegation interface
    NS ns("",EMIES_NAMESPACE);
    ns["estypes"] = EMIES_TYPES_NAMESPACE;
    if(op_name == "InitDelegation") {
      // InitDelegation
      //   CredentialType [RFC3820]
      //   RenewalID 0-
      //   InitDelegationLifetime 0-
      // InitDelegationResponse
      //   DelegationID
      //   CSR
      // InternalServiceDelegationFault
      // AccessControlFault
      // InternalBaseFault
      // Need  UnknownDelegationIDFault for reporting bad RenewalID
      Arc::XMLNode r = out.NewChild("InitDelegationResponse",ns);
      if((std::string)op["CredentialType"] != "RFC3820") {
        EMIESFAULT(out,"Unsupported credential type requested");
        return true;
      }
      unsigned long long int lifetime = 0;
      if((bool)op["InitDelegationLifetime"]) {
        if(!stringto((std::string)op["InitDelegationLifetime"],lifetime)) {
          EMIESFAULT(out,"Unsupported credential lifetime requested");
          return true;
        }
      }
      std::string id = op["RenewalID"];
      DelegationConsumerSOAP* c = NULL;
      bool renew = false;
      if(!id.empty()) {
        c = FindConsumer(id,client);
        if(!c) {
          EMIESFAULT(out,"Failed to find identifier");
          return true;
        };
        renew = true;
      } else {
        c = AddConsumer(id,client);
        if(!c) {
          EMIESFAULT(out,"Failed to generate identifier");
          return true;
        };
      };
      std::string x509_request;
      // TODO: use lifetime
      c->Request(x509_request);
      //if((!strip_PEM_request(x509_request)) || (x509_request.empty())) {
      if(x509_request.empty()) {
        if(renew) ReleaseConsumer(c);
        else RemoveConsumer(c);
        EMIESFAULT(out,"Failed to generate request");
        return true;
      };
      ReleaseConsumer(c);
      CheckConsumers();
      r.NewChild("DelegationID") = id;
      r.NewChild("CSR") = x509_request;
      return true;
    } else if(op_name == "PutDelegation") {
      // PutDelegation
      //   DelegationID
      //   Credential
      // PutDelegationResponse [SUCCESS]
      // UnknownDelegationIDFault
      // AccessControlFault
      // InternalBaseFault
      Arc::XMLNode r = out.NewChild("PutDelegationResponse",ns);
      std::string id = op["DelegationId"];
      std::string cred = op["Credential"];
      if(id.empty()) {
        EMIESFAULT(out,"Identifier is missing");
        return true;
      };
      if(cred.empty()) {
        EMIESFAULT(out,"Delegated credential is missing");
        return true;
      };
      //wrap_PEM_cert(cred);
      DelegationConsumerSOAP* c = FindConsumer(id,client);
      if(!c) {
        EMIESIDFAULT(out,"Failed to find identifier");
        return true;
      };
      if(!c->Acquire(cred)) {
        ReleaseConsumer(c);
        EMIESFAULT(out,"Failed to acquire credentials");
        return true;
      };
      if(!TouchConsumer(c,cred)) {
        ReleaseConsumer(c);
        EMIESFAULT(out,"Failed to process credentials");
        return true;
      };
      ReleaseConsumer(c);
      credentials = cred;
      r = "SUCCESS";
      return true;
    } else if(op_name == "GetDelegationInfo") {
      // GetDelegationInfo
      //   DelegationID
      // GetDelegationInfoResponse
      //   Lifetime
      //   Issuer 0-
      //   Subject 0-
      // UnknownDelegationIDFault
      // AccessControlFault
      // InternalBaseFault
      Arc::XMLNode r = out.NewChild("GetDelegationInfoResponse",ns);
      std::string id = op["DelegationID"];
      if(id.empty()) {
        EMIESFAULT(out,"Identifier is missing");
        return true;
      };
      DelegationConsumerSOAP* c = FindConsumer(id,client);
      if(!c) {
        EMIESIDFAULT(out,"Wrong identifier");
        return true;
      };
      std::string credentials;
      if((!QueryConsumer(c,credentials)) || credentials.empty()) {
        ReleaseConsumer(c);
        EMIESFAULT(out,"Delegated credentials missing");
        return true;
      };
      ReleaseConsumer(c);
      cred_info_t info;
      if(!get_cred_info(credentials,info)) {
        EMIESFAULT(out,"Delegated credentials missing");
        return true;
      };
      if(info.valid_till == Time(Time::UNDEFINED)) info.valid_till = Time();
      r.NewChild("Lifetime") = info.valid_till.str(ISOTime);
      r.NewChild("Issuer") = info.ca;
      r.NewChild("Subject") = info.identity;
      return true;
    };
  };
  return false;
}

bool DelegationContainerSOAP::MatchNamespace(const SOAPEnvelope& in) {
  XMLNode op = ((SOAPEnvelope&)in).Child(0);
  if(!op) return false;
  std::string op_ns = op.Namespace();
  return ((op_ns == DELEGATION_NAMESPACE) ||
          (op_ns == GDS10_NAMESPACE) ||
          /*(op_ns == GDS20_NAMESPACE) ||*/
          (op_ns == EMIDS_NAMESPACE) ||
          (op_ns == EMIES_NAMESPACE));
}

std::string DelegationContainerSOAP::GetFailure(void) {
  return failure_;
}

} // namespace Arc

