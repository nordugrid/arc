#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <openssl/rsa.h>
#include <openssl/bn.h>
#include <openssl/bio.h>
#include <openssl/pem.h>
#include <openssl/err.h>
#include <openssl/x509v3.h>

#include <iostream>

#include <arc/GUID.h>
#include <arc/StringConv.h>

#include "DelegationInterface.h"

namespace Arc {

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

static bool cert_to_string(X509* cert,std::string& str) {
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

DelegationConsumer::DelegationConsumer(void):key_(NULL) {
  Generate();
}

DelegationConsumer::DelegationConsumer(const std::string& content):key_(NULL) {
  Restore(content);
}

DelegationConsumer::~DelegationConsumer(void) {
  if(key_) RSA_free((RSA*)key_);
}
 
static int progress_cb(int p, int n, BN_GENCB *cb) {
  char c='*';
  if (p == 0) c='.';
  if (p == 1) c='+';
  if (p == 2) c='*';
  if (p == 3) c='\n';
  std::cerr<<c;
  return 1;
}

static int ssl_err_cb(const char *str, size_t len, void *u) {
  std::string& ssl_err = *((std::string*)u);
  ssl_err.append(str,len);
  return 1;
}

void DelegationConsumer::LogError(void) {
  std::string ssl_err;
  ERR_print_errors_cb(&ssl_err_cb,&ssl_err);
std::cerr<<ssl_err<<std::endl;
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
  BN_GENCB cb;
  int num = 1024;
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
  return res;
}

bool DelegationConsumer::Request(std::string& content) {
  bool res = false;
  content.resize(0);
  EVP_PKEY *pkey = EVP_PKEY_new();
  const EVP_MD *digest = EVP_md5();
  BN_GENCB cb;
  BN_GENCB_set(&cb,&progress_cb,NULL);
  if(pkey) {
    RSA *rsa = (RSA*)key_;
    if(rsa) {
      if(EVP_PKEY_set1_RSA(pkey, rsa)) {
        X509_REQ *req = X509_REQ_new();
        if(req) {
          //if(X509_REQ_set_version(req,0L)) {
          if(X509_REQ_set_version(req,2L)) {
            if(X509_REQ_set_pubkey(req,pkey)) {
//X509_NAME *n = parse_name(subject, chtype, multirdn);
//X509_REQ_set_subject_name(req, n);
//X509_NAME_free(n);
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

DelegationProvider::DelegationProvider(const std::string& credentials):key_(NULL),cert_(NULL),chain_(NULL) {
  EVP_PKEY *pkey = NULL;
  X509 *cert = NULL;
  STACK_OF(X509) *cert_sk = NULL;
  BIO *in = NULL;
  bool res = false;

  //OpenSSL_add_all_algorithms();
  EVP_add_digest(EVP_md5());

  if(credentials.empty()) {
    return;
  };

  if(key_) { EVP_PKEY_free((EVP_PKEY*)key_); key_=NULL; };
  if(cert_) { X509_free((X509*)cert_); cert_=NULL; };
  if(chain_) {
    STACK_OF(X509) *sv = (STACK_OF(X509) *)chain_;
    for(;;) {
      X509* v = sk_X509_pop(sv);
      if(!v) break;
      X509_free(v);
    };
    sk_X509_free(sv);
    cert_=NULL;
  };

  in = BIO_new_mem_buf((void*)(credentials.c_str()),credentials.length());
  if(!in) goto err;

  if((!PEM_read_bio_X509(in,&cert,NULL,NULL)) || (!cert)) goto err;
  cert_=cert; cert=NULL;

  if((!PEM_read_bio_PrivateKey(in,&pkey,NULL,NULL)) || (!pkey)) goto err;
  key_=pkey; pkey=NULL;

  cert_sk = sk_X509_new_null();
  if(!cert_sk) goto err;
  for(;;) {
    if((!PEM_read_bio_X509(in,&cert,NULL,NULL)) || (!cert)) {
      CleanError();
      break;
    };
    sk_X509_push(cert_sk,cert); cert=NULL;
  };
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
  if(in) BIO_free_all(in);
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

std::string DelegationProvider::Delegate(const std::string& request) {
  X509 *cert = NULL;
  X509_REQ *req = NULL;
  BIO* in = NULL;
  EVP_PKEY *pkey = NULL;
  ASN1_INTEGER *sno = NULL;
  ASN1_OBJECT *obj= NULL;
  X509_EXTENSION *ex = NULL;
  PROXY_CERT_INFO_EXTENSION proxy_info;
  PROXY_POLICY proxy_policy;
  const EVP_MD *digest = EVP_md5();
  X509_NAME *subject = NULL;
  std::string proxy_cn;
  std::string res;

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
  int i = X509_REQ_verify(req,pkey);
  if(i <= 0) goto err;

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
  obj=OBJ_nid2obj(NID_id_ppl_inheritAll);  // Unrestricted proxy
  if(!obj) goto err;
  proxy_policy.policyLanguage=obj;
  if(X509_add1_ext_i2d(cert,NID_proxyCertInfo,&proxy_info,1,X509V3_ADD_REPLACE) != 1) goto err;
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
  //ASN1_TIME* t = ASN1_TIME_new();
  //ASN1_TIME_set(t,time(NUL));
  X509_gmtime_adj(X509_get_notBefore(cert),0);
  X509_gmtime_adj(X509_get_notAfter(cert),(long)60*60*24);
  /*
  int             X509_set_notBefore(X509 *x, ASN1_TIME *tm);
  int             X509_set_notAfter(X509 *x, ASN1_TIME *tm);
  */

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

  if(!cert_to_string(cert,res)) { res=""; goto err; };
  // Append chain of certificates
  if(!cert_to_string((X509*)cert_,res)) { res=""; goto err; };
  if(chain_) {
    for(int n=0;n<sk_X509_num((STACK_OF(X509) *)chain_);++n) {
      X509* v = sk_X509_value((STACK_OF(X509) *)chain_,n);
      if(!v) { res=""; goto err; };
      if(!cert_to_string(v,res)) { res=""; goto err; };
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
  return res;
}

void DelegationProvider::LogError(void) {
  std::string ssl_err;
  ERR_print_errors_cb(&ssl_err_cb,&ssl_err);
std::cerr<<ssl_err<<std::endl;
}

void DelegationProvider::CleanError(void) {
  std::string ssl_err;
  ERR_print_errors_cb(&ssl_err_cb,&ssl_err);
}


} // namespace Arc
