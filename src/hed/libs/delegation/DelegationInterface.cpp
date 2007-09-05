#include <openssl/rsa.h>
#include <openssl/bn.h>
#include <openssl/bio.h>
#include <openssl/pem.h>
#include <openssl/err.h>

#include <iostream>

#include "DelegationInterface.h"

namespace Arc {

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
          if(X509_REQ_set_version(req,0L)) {
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

DelegationProvider::DelegationProvider(const std::string& credentials):key_(NULL),cert_(NULL) {
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
  if(cert_) {
    STACK_OF(X509) *sv = (STACK_OF(X509) *)cert_;
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

  cert_sk = sk_X509_new_null();
  if(!cert_sk) goto err;

  if((!PEM_read_bio_X509(in,&cert,NULL,NULL)) || (!cert)) goto err;
  sk_X509_push(cert_sk,cert); cert=NULL;

  if((!PEM_read_bio_PrivateKey(in,&pkey,NULL,NULL)) || (!pkey)) goto err;

  for(;;) {
    if((!PEM_read_bio_X509(in,&cert,NULL,NULL)) || (!cert)) {
      LogError();
      break;
    };
    sk_X509_push(cert_sk,cert); cert=NULL;
  };

  cert_=cert_sk; cert_sk=NULL;

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
  if(cert_) {
    for(;;) {
      X509* v = sk_X509_pop((STACK_OF(X509) *)cert_);
      if(!v) break;
      X509_free(v);
    };
    sk_X509_free((STACK_OF(X509) *)cert_);
  };
}

std::string DelegationProvider::Delegate(const std::string& request) {
  X509_REQ *req = NULL;
  BIO* in = NULL;
  EVP_PKEY *pkey = NULL;
  std::string res;

  in = BIO_new_mem_buf((void*)(request.c_str()),request.length());
  if(!in) goto err;

  if((!PEM_read_bio_X509_REQ(in,&req,NULL,NULL)) || (!req)) goto err;
  BIO_free_all(in); in=NULL;

 
  X509_NAME* subject = X509_REQ_get_subject_name(req);
  char* buf = X509_NAME_oneline(subject, 0, 0);
  std::cerr<<"subject="<<buf<<std::endl;
  OPENSSL_free(buf);

  if((pkey=X509_REQ_get_pubkey(req)) == NULL) goto err;
  int i = X509_REQ_verify(req,pkey);
  //EVP_PKEY_free(pkey); pkey=NULL;
  if(i <= 0) goto err;
  //EVP_PKEY_free(pkey); pkey=NULL;

  res = "DDD";
err:
  if(res.empty()) LogError();
  if(in) BIO_free_all(in);
  if(req) X509_REQ_free(req);
  //if(pkey) EVP_PKEY_free(pkey);
  return res;
}

void DelegationProvider::LogError(void) {
  std::string ssl_err;
  ERR_print_errors_cb(&ssl_err_cb,&ssl_err);
std::cerr<<ssl_err<<std::endl;
}





} // namespace Arc
