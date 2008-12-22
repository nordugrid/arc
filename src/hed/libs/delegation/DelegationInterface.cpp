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

#include "DelegationInterface.h"

namespace Arc {

#define DELEGATION_NAMESPACE "http://www.nordugrid.org/schemas/delegation"
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
   if(in == &std::cin) std::cout<<"Enter passphrase for your private key: "<<std::endl;
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

DelegationConsumer::DelegationConsumer(void):key_(NULL) {
  Generate();
}

DelegationConsumer::DelegationConsumer(const std::string& content):key_(NULL) {
  Restore(content);
}

DelegationConsumer::~DelegationConsumer(void) {
  if(key_) RSA_free((RSA*)key_);
}
 
#ifdef HAVE_OPENSSL_OLDRSA
static void progress_cb(int p, int, void*) {
  char c='*';
  if (p == 0) c='.';
  if (p == 1) c='+';
  if (p == 2) c='*';
  if (p == 3) c='\n';
  std::cerr<<c;
}
#else
static int progress_cb(int p, int, BN_GENCB*) {
  char c='*';
  if (p == 0) c='.';
  if (p == 1) c='+';
  if (p == 2) c='*';
  if (p == 3) c='\n';
  std::cerr<<c;
  return 1;
}
#endif

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
  bool res = false;
  RSA *rsa = NULL;
  BIO *in = BIO_new_mem_buf((void*)(content.c_str()),content.length());
  if(in) {
    if(PEM_read_bio_RSAPrivateKey(in,&rsa,NULL,NULL)) {
      if(rsa) {
        if(key_) RSA_free((RSA*)key_);
        key_=rsa; res=true;
      };
    };
    BIO_free_all(in);
  };
  return rsa;
}

bool DelegationConsumer::Generate(void) {
  bool res = false;
  int num = 1024;
#ifdef HAVE_OPENSSL_OLDRSA
  unsigned long bn = RSA_F4;
  RSA *rsa=RSA_generate_key(num,bn,&progress_cb,NULL);
  if(rsa) {
    if(key_) RSA_free((RSA*)key_);
    key_=rsa; rsa=NULL; res=true;
  } else {
    LogError();
    std::cerr<<"RSA_generate_key failed"<<std::endl;
  };
  if(rsa) RSA_free(rsa);
#else
  BN_GENCB cb;
  BIGNUM *bn = BN_new();
  RSA *rsa = RSA_new();

  BN_GENCB_set(&cb,&progress_cb,NULL);
  if(bn && rsa) {
    if(BN_set_word(bn,RSA_F4)) {
      if(RSA_generate_key_ex(rsa,num,bn,&cb)) {
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
#endif
  return res;
}

bool DelegationConsumer::Request(std::string& content) {
  bool res = false;
  content.resize(0);
  EVP_PKEY *pkey = EVP_PKEY_new();
  const EVP_MD *digest = EVP_md5();
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

bool DelegationConsumer::Acquire(std::string& content, std::string& identity) {
  X509 *cert = NULL;
  STACK_OF(X509) *cert_sk = NULL;
  bool res = false;
  char buf[100];
  std::string subject;

  if(!key_) return false;

  if(!string_to_x509(content,cert,cert_sk)) goto err;

  content.resize(0);
  if(!x509_to_string(cert,content)) goto err;

  X509_NAME_oneline(X509_get_subject_name(cert),buf,sizeof(buf));
  subject=buf;
#ifdef HAVE_OPENSSL_PROXY
  if(X509_get_ext_by_NID(cert,NID_proxyCertInfo,-1) < 0) {
    identity=subject;
  };
#endif

  if(!x509_to_string((RSA*)key_,content)) goto err;
  if(cert_sk) {
    for(int n=0;n<sk_X509_num((STACK_OF(X509) *)cert_sk);++n) {
      X509* v = sk_X509_value((STACK_OF(X509) *)cert_sk,n);
      if(!v) goto err;
      if(!x509_to_string(v,content)) goto err;
      if(identity.empty()) {
        memset(buf,0,100);
        X509_NAME_oneline(X509_get_subject_name(v),buf,sizeof(buf));
#ifdef HAVE_OPENSSL_PROXY
        if(X509_get_ext_by_NID(v,NID_proxyCertInfo,-1) < 0) {
          identity=buf;
        };
#endif
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

  //OpenSSL_add_all_algorithms();
  EVP_add_digest(EVP_md5());

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

  //OpenSSL_add_all_algorithms();
  EVP_add_digest(EVP_md5());

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
#ifdef HAVE_OPENSSL_PROXY
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
  const EVP_MD *digest = EVP_md5();
  X509_NAME *subject = NULL;
  std::string proxy_cn;
  std::string res;
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

  in = BIO_new_mem_buf((void*)(request.c_str()),request.length());
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
    obj=OBJ_nid2obj(NID_id_ppl_inheritAll);  // Unrestricted proxy
    if(!obj) goto err;
    proxy_policy.policyLanguage=obj;
  };
  if(X509_add1_ext_i2d(cert,NID_proxyCertInfo,&proxy_info,1,X509V3_ADD_REPLACE) != 1) goto err;
  if(policy_string) ASN1_OCTET_STRING_free(policy_string); policy_string=NULL;
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
  };
  if(!(restrictions_["validityEnd"].empty())) {
    validity_end=Arc::Time(restrictions_["validityEnd"]).GetTime();
  } else if(!(restrictions_["validityPeriod"].empty())) {
    validity_end=validity_start+Arc::Period(restrictions_["validityPeriod"]).GetPeriod();
  };
  ASN1_TIME_set(X509_get_notBefore(cert),validity_start);
  if(validity_end == (time_t)(-1)) {
    X509_set_notAfter(cert,X509_get_notAfter((X509*)cert_));
  } else {
    ASN1_TIME_set(X509_get_notAfter(cert),validity_end);
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
#else
  return "";
#endif
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
  std::string identity;
  if(!Acquire(credentials,identity)) return false;
  NS ns; ns["deleg"]=DELEGATION_NAMESPACE;
  out.Namespaces(ns);
  out.NewChild("deleg:UpdateCredentialsResponse");
  return true;
}

bool DelegationConsumerSOAP::DelegatedToken(std::string& credentials,const XMLNode& token) {
  credentials = (std::string)(token["Value"]);
  if(credentials.empty()) return false;
  if(((std::string)(token.Attribute("Format"))) != "x509") return false;
  std::string identity;
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

bool DelegationProviderSOAP::DelegateCredentialsInit(MCCInterface& interface,MessageContext* context) {
  MessageAttributes attributes_in;
  MessageAttributes attributes_out;
  return DelegateCredentialsInit(interface,&attributes_in,&attributes_out,context);
}

bool DelegationProviderSOAP::DelegateCredentialsInit(MCCInterface& interface,MessageAttributes* attributes_in,MessageAttributes* attributes_out,MessageContext* context) {
  NS ns; ns["deleg"]=DELEGATION_NAMESPACE;
  PayloadSOAP req_soap(ns);
  req_soap.NewChild("deleg:DelegateCredentialsInit");
  Message req;
  Message resp;
  req.Attributes(attributes_in);
  req.Context(context);
  req.Payload(&req_soap);
  resp.Attributes(attributes_out);
  resp.Context(context);
  MCC_Status r = interface.process(req,resp);
  if(r != STATUS_OK) return false;
  if(!resp.Payload()) return false;
  PayloadSOAP* resp_soap = NULL;
  try {
    resp_soap=dynamic_cast<PayloadSOAP*>(resp.Payload());
  } catch(std::exception& e) { };
  if(!resp_soap) { delete resp.Payload(); return false; };
  XMLNode token = (*resp_soap)["DelegateCredentialsInitResponse"]["TokenRequest"];
  if(!token) { delete resp_soap; return false; };
  if(((std::string)(token.Attribute("Format"))) != "x509") { delete resp_soap; return false; };
  id_=(std::string)(token["Id"]);
  request_=(std::string)(token["Value"]);
  delete resp_soap;
  if(id_.empty() || request_.empty()) return false;
  return true;
}

bool DelegationProviderSOAP::UpdateCredentials(MCCInterface& interface,MessageContext* context) {
  MessageAttributes attributes_in;
  MessageAttributes attributes_out;
  return UpdateCredentials(interface,&attributes_in,&attributes_out,context);
}

bool DelegationProviderSOAP::UpdateCredentials(MCCInterface& interface,MessageAttributes* attributes_in,MessageAttributes* attributes_out,MessageContext* context) {
  if(id_.empty()) return false;
  if(request_.empty()) return false;
  std::string delegation = Delegate(request_);
  if(delegation.empty()) return false;
  NS ns; ns["deleg"]=DELEGATION_NAMESPACE;
  PayloadSOAP req_soap(ns);
  XMLNode token = req_soap.NewChild("deleg:UpdateCredentials").NewChild("deleg:DelegatedToken");
  token.NewAttribute("deleg:Format")="x509";
  token.NewChild("deleg:Id")=id_;
  token.NewChild("deleg:Value")=delegation;
  Message req;
  Message resp;
  req.Attributes(attributes_in);
  req.Context(context);
  req.Payload(&req_soap);
  resp.Attributes(attributes_out);
  resp.Context(context);
  MCC_Status r = interface.process(req,resp);
  if(r != STATUS_OK) return false;
  if(!resp.Payload()) return false;
  PayloadSOAP* resp_soap = NULL;
  try {
    resp_soap=dynamic_cast<PayloadSOAP*>(resp.Payload());
  } catch(std::exception& e) { };
  if(!resp_soap) { delete resp.Payload(); return false; };
  if(!(*resp_soap)["UpdateCredentialsResponse"]) 
  delete resp_soap;
  return true;
}

bool DelegationProviderSOAP::DelegatedToken(XMLNode& parent) {
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
// TODO:
// 1. Add access control by assigning user's DN to id.

class DelegationContainerSOAP::Consumer {
 public:
  DelegationConsumerSOAP* deleg;
  int usage_count;
  time_t last_used;
  std::string dn;
  DelegationContainerSOAP::ConsumerIterator previous;
  DelegationContainerSOAP::ConsumerIterator next;
  Consumer(void):deleg(NULL),usage_count(0),last_used(time(NULL)) {
  };
  Consumer(DelegationConsumerSOAP* d):deleg(d),usage_count(0),last_used(time(NULL)) {
  };
  Consumer& operator=(DelegationConsumerSOAP* d) {
    deleg=d; usage_count=0; last_used=time(NULL); return *this;
  };
};

DelegationContainerSOAP::DelegationContainerSOAP(void) {
  max_size_=0;         // unlimited size of container
  max_duration_=30;    // 30 seconds for delegation
  max_usage_=2;        // allow 1 failure
  context_lock_=false;
  restricted_=true;
  consumers_first_=consumers_.end();
  consumers_last_=consumers_.end();
}

DelegationContainerSOAP::~DelegationContainerSOAP(void) {
  lock_.lock();
  ConsumerIterator i = consumers_.begin();
  for(;i!=consumers_.end();++i) {
    if(i->second.deleg) delete i->second.deleg;
  };
  lock_.unlock();
}

void DelegationContainerSOAP::AddConsumer(const std::string& id,DelegationConsumerSOAP* consumer) {
  Consumer c;
  c.deleg=consumer; 
  c.previous=consumers_.end();
  c.next=consumers_first_;
  ConsumerIterator i = consumers_.insert(consumers_.begin(),make_pair(id,c)); 
  if(consumers_first_ != consumers_.end()) consumers_first_->second.previous=i;
  consumers_first_=i;
  if(consumers_last_ == consumers_.end()) consumers_last_=i;
std::cerr<<"Consumer added: "<<consumers_.size()<<std::endl;
}

void DelegationContainerSOAP::TouchConsumer(ConsumerIterator i) {
  i->second.last_used=time(NULL);
  if(i == consumers_first_) return;
  ConsumerIterator previous = i->second.previous;
  ConsumerIterator next = i->second.next;
  if(previous != consumers_.end()) previous->second.next=next;
  if(next != consumers_.end()) next->second.previous=previous;
  i->second.previous=consumers_.end();
  i->second.next=consumers_first_;
  if(consumers_first_ != consumers_.end()) consumers_first_->second.previous=i;
  consumers_first_=i;
std::cerr<<"Consumer touched: "<<consumers_.size()<<std::endl;
}

DelegationContainerSOAP::ConsumerIterator DelegationContainerSOAP::RemoveConsumer(ConsumerIterator i) {
  ConsumerIterator previous = i->second.previous;
  ConsumerIterator next = i->second.next;
  if(previous != consumers_.end()) previous->second.next=next;
  if(next != consumers_.end()) next->second.previous=previous;
  if(consumers_first_ == i) consumers_first_=next; 
  if(consumers_last_ == i) consumers_last_=previous; 
  if(i->second.deleg) delete i->second.deleg;
  consumers_.erase(i);
std::cerr<<"Consumer removed: "<<consumers_.size()<<std::endl;
  return next;
}

void DelegationContainerSOAP::CheckConsumers(void) {
  if(max_size_ > 0) {
    while(consumers_.size() > max_size_) {
      RemoveConsumer(consumers_last_);
    };
  };
  if(max_duration_ > 0) {
    time_t t = time(NULL);
    for(ConsumerIterator i = consumers_last_;i!=consumers_.end();) {
      if(((unsigned int)(t - i->second.last_used)) > max_duration_) {
        i=RemoveConsumer(i);
      } else {
        break;
      };
    };
  };
}

bool DelegationContainerSOAP::DelegateCredentialsInit(const SOAPEnvelope& in,SOAPEnvelope& out) {
  lock_.lock();
  std::string id;
  for(int tries = 0;tries<1000;++tries) {
    GUID(id);
    ConsumerIterator i = consumers_.find(id);
    if(i == consumers_.end()) break;
    id.resize(0);
  };
  if(id.empty()) { lock_.unlock(); return false; };
  DelegationConsumerSOAP* consumer = new DelegationConsumerSOAP();
  if(!(consumer->DelegateCredentialsInit(id,in,out))) { lock_.unlock(); delete consumer; return false; };
  AddConsumer(id,consumer);
  CheckConsumers();
  lock_.unlock();
  return true;
}

bool DelegationContainerSOAP::UpdateCredentials(std::string& credentials,const SOAPEnvelope& in,SOAPEnvelope& out) {
  lock_.lock();
  std::string id = (std::string)(in["UpdateCredentials"]["DelegatedToken"]["Id"]);
  ConsumerIterator i = consumers_.find(id);
  if(i == consumers_.end()) { lock_.unlock(); return false; };
  if(!(i->second.deleg)) { lock_.unlock(); return false; };
  if(restricted_) {


  };
  bool r = i->second.deleg->UpdateCredentials(credentials,in,out);
  if(((++(i->second.usage_count)) > max_usage_) && (max_usage_ > 0)) {
    RemoveConsumer(i);
  } else {
    TouchConsumer(i);
  };
  lock_.unlock();
  return r;
}


bool DelegationContainerSOAP::DelegatedToken(std::string& credentials,const XMLNode& token) {
  lock_.lock();
  std::string id = (std::string)(token["Id"]);
  ConsumerIterator i = consumers_.find(id);
  if(i == consumers_.end()) { lock_.unlock(); return false; };
  if(!(i->second.deleg)) { lock_.unlock(); return false; };
  bool r = i->second.deleg->DelegatedToken(credentials,token);
  if(((++(i->second.usage_count)) > max_usage_) && (max_usage_ > 0)) {
    RemoveConsumer(i);
  } else {
    TouchConsumer(i);
  };
  lock_.unlock();
  return r;
}

} // namespace Arc

