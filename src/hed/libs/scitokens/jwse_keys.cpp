#include <cctype>
#include <cstdlib>
#include <cstring>

#include <arc/Utils.h>
#include <arc/Base64.h>
#include <arc/StringConv.h>
#include <arc/external/cJSON/cJSON.h>
#include <arc/message/MCC.h>
#include <openssl/evp.h>
#include <openssl/x509.h>
#include <openssl/pem.h>
#include <openssl/rsa.h>
#include <openssl/bn.h>

#include "jwse.h"
#include "jwse_private.h"
#include "openid_metadata.h"


static EVP_PKEY* X509_get_privkey(X509*) {
  return NULL;
}

#if OPENSSL_VERSION_NUMBER < 0x10100000L

static int RSA_set0_key(RSA *r, BIGNUM *n, BIGNUM *e, BIGNUM *d) {
  /* If the fields n and e in r are NULL, the corresponding input
   * parameters MUST be non-NULL for n and e.  d may be
   * left NULL (in case only the public key is used).
   */
  if ((r->n == NULL && n == NULL)
      || (r->e == NULL && e == NULL))
    return 0;

  if (n != NULL) {
    BN_free(r->n);
    r->n = n;
  }
  if (e != NULL) {
    BN_free(r->e);
    r->e = e;
  }
  if (d != NULL) {
    BN_free(r->d);
    r->d = d;
  }

  return 1;
}

static RSA *EVP_PKEY_get0_RSA(EVP_PKEY *pkey) {
  if(pkey)
    if((pkey->type == EVP_PKEY_RSA) || (pkey->type == EVP_PKEY_RSA2))
      return pkey->pkey.rsa;
  return NULL;
}

void RSA_get0_key(const RSA *r, const BIGNUM **n, const BIGNUM **e, const BIGNUM **d) {
  if(n) *n = NULL;
  if(e) *e = NULL;
  if(d) *d = NULL;
  if(!r) return;
  if(n) *n = r->n;
  if(e) *e = r->e;
  if(d) *d = r->d;
}

#endif


namespace Arc {
 

  char const * const JWSE::HeaderNameX509CertChain = "x5c";
  char const * const JWSE::HeaderNameJSONWebKey = "jwk";
  char const * const JWSE::HeaderNameJSONWebKeyId = "kid";

  static void sk_x509_deallocate(STACK_OF(X509)* o) {
    sk_X509_pop_free(o, X509_free);
  }

  static void BIO_deallocate(BIO* bio) {
    (void)BIO_free(bio);
  }

  static EVP_PKEY* jwkECParse(cJSON* jwkObject) {
    return NULL;
  }

  static EVP_PKEY* jwkRSAParse(cJSON* jwkObject) {
    cJSON* modulusObject = cJSON_GetObjectItem(jwkObject, "n");
    cJSON* exponentObject = cJSON_GetObjectItem(jwkObject, "e");
    if((modulusObject == NULL) || (exponentObject == NULL)) return NULL;
    if((modulusObject->type != cJSON_String) || (exponentObject->type != cJSON_String)) return NULL;
    std::string modulus = Base64::decodeURLSafe(modulusObject->valuestring);
    std::string exponent = Base64::decodeURLSafe(exponentObject->valuestring);
    AutoPointer<RSA> rsaKey(RSA_new(),&RSA_free);
    if(!rsaKey) return NULL;
    AutoPointer<BIGNUM> n(NULL,&BN_free);
    AutoPointer<BIGNUM> e(NULL,&BN_free);
    if(!(n = BN_bin2bn(reinterpret_cast<unsigned char const *>(modulus.c_str()), modulus.length(), NULL))) return NULL;
    if(!(e = BN_bin2bn(reinterpret_cast<unsigned char const *>(exponent.c_str()), exponent.length(), NULL))) return NULL;
    if(!RSA_set0_key(rsaKey.Ptr(), n.Ptr(), e.Ptr(), NULL)) return NULL;
    n.Release();
    e.Release();
    AutoPointer<EVP_PKEY> evpKey(EVP_PKEY_new(), &EVP_PKEY_free);
    if(!evpKey) return NULL;
    if(EVP_PKEY_assign_RSA(evpKey.Ptr(), rsaKey.Ptr()) != 1) return NULL;
    rsaKey.Release();
    return evpKey.Release();
  }

  static EVP_PKEY* jwkOctParse(cJSON* jwkObject) {
    cJSON* keyObject = cJSON_GetObjectItem(jwkObject, "k");
    cJSON* algObject = cJSON_GetObjectItem(jwkObject, "alg");
    if((keyObject == NULL) || (algObject == NULL)) return NULL;
    if((keyObject->type != cJSON_String) || (algObject->type != cJSON_String)) return NULL;
    std::string key = Base64::decodeURLSafe(keyObject->valuestring);
    // It looks like RFC does not define any "alg" values with "JWK" usage.
    // TODO: finish implementing
    return NULL;
  }

  static bool jwkParse(cJSON* jwkObject, JWSEKeyHolder& keyHolder) {
    if(jwkObject->type != cJSON_Object) return false;

    cJSON* ktyObject = cJSON_GetObjectItem(jwkObject, "kty");
    if(ktyObject == NULL) return false;
    if(ktyObject->type != cJSON_String) return false;

    EVP_PKEY* publicKey(NULL);
    if(strcmp(ktyObject->valuestring, "EC") == 0) {    
      publicKey = jwkECParse(jwkObject);
    } else if(strcmp(ktyObject->valuestring, "RSA") == 0) {    
      publicKey = jwkRSAParse(jwkObject);
    } else if(strcmp(ktyObject->valuestring, "oct") == 0) {    
      publicKey = jwkOctParse(jwkObject);
    }
    if(publicKey == NULL)
      return false;

    keyHolder.PublicKey(publicKey);

    return true;
  }


  static bool jwkRSASerialize(cJSON* jwkObject, EVP_PKEY const * key) {
    if(!key) return false;
    RSA const* rsaKey = EVP_PKEY_get0_RSA(const_cast<EVP_PKEY*>(key));
    if(!rsaKey) return false;
    BIGNUM const* n(NULL);
    BIGNUM const* e(NULL);
    BIGNUM const* d(NULL);
    RSA_get0_key(const_cast<RSA*>(rsaKey), &n, &e, &d);
    if((!n) || (!e)) return false;

    std::string modulus;
    modulus.resize(BN_num_bytes(n));
    modulus.resize(BN_bn2bin(n, reinterpret_cast<unsigned char*>(const_cast<char*>(modulus.c_str()))));
    std::string exponent;
    exponent.resize(BN_num_bytes(e));
    exponent.resize(BN_bn2bin(e, reinterpret_cast<unsigned char*>(const_cast<char*>(exponent.c_str()))));
    if(modulus.empty() || exponent.empty()) return false;

    cJSON_AddStringToObject(jwkObject, "n", Base64::encodeURLSafe(modulus.c_str()).c_str());
    cJSON_AddStringToObject(jwkObject, "e", Base64::encodeURLSafe(exponent.c_str()).c_str());

    return true;
  }

  static cJSON* jwkSerialize(JWSEKeyHolder const& keyHolder) {
    AutoPointer<cJSON> jwkObject(cJSON_CreateObject(), &cJSON_Delete);
    if(jwkRSASerialize(jwkObject.Ptr(), keyHolder.PublicKey())) {
      return jwkObject.Release();
    };
    return NULL;
  }


  static bool x5cParse(cJSON* x5cObject, JWSEKeyHolder& keyHolder) {
    if(x5cObject->type != cJSON_Array) return false;
    // Collect the chain
    AutoPointer<X509> maincert(NULL, *X509_free);
    AutoPointer<STACK_OF(X509)> certchain(sk_X509_new_null(), &sk_x509_deallocate);
    for(int certN = 0; ; ++certN) {
      cJSON* certObject = cJSON_GetArrayItem(x5cObject, certN);
      if(certObject == NULL) break;
      if(certObject->type != cJSON_String) return NULL;
      // It should be PEM encoded certificate in single string and without header/footer           
      AutoPointer<BIO> mem(BIO_new(BIO_s_mem()), &BIO_deallocate);
      if(!mem) return NULL;
      BIO_puts(mem.Ptr(), "-----BEGIN CERTIFICATE-----\n");
      BIO_puts(mem.Ptr(), certObject->valuestring);
      BIO_puts(mem.Ptr(), "\n-----END CERTIFICATE-----");
      AutoPointer<X509> cert(PEM_read_bio_X509(mem.Ptr(), NULL, NULL, NULL), *X509_free);
      if(!cert) return false;
      if(certN == 0) {
        maincert = cert.Release();
      } else {
        if(sk_X509_insert(certchain.Ptr(), cert.Release(), certN) == 0) return false; // does sk_X509_insert increase reference?
        (void)cert.Release();
      }
    }
    // Verify the chain





    // Remember collected information
    keyHolder.Certificate(maincert.Release());
    if(sk_X509_num(certchain.Ptr()) > 0)
      keyHolder.CertificateChain(certchain.Release());

    return true;
  }

  static cJSON* x5cSerialize(JWSEKeyHolder const& keyHolder) {
    X509 const* cert = keyHolder.Certificate();
    if(!cert) return NULL;
    const STACK_OF(X509)* chain = keyHolder.CertificateChain();

    AutoPointer<cJSON> x5cObject(cJSON_CreateArray(), &cJSON_Delete);
    int certN = 0;
    while(true) {
      AutoPointer<BIO> mem(BIO_new(BIO_s_mem()), &BIO_deallocate);
      if(!PEM_write_bio_X509(mem.Ptr(), const_cast<X509*>(cert)))
        return NULL;
      std::string certStr;
      std::string::size_type certStrSize = 0;
      while(true) {
        certStr.resize(certStrSize+256);
        int l = BIO_gets(mem.Ptr(), const_cast<char*>(certStr.c_str()+certStrSize), certStr.length()-certStrSize);
        if(l < 0) l = 0;
        certStrSize+=l;
        certStr.resize(certStrSize);
        if(l == 0) break;
      };

      // Must remove header, footer and line breaks
      std::vector<std::string> lines;
      Arc::tokenize(certStr, lines, "\n");
      if(lines.size() < 2) return NULL;
      if(lines[0].find("BEGIN CERTIFICATE") == std::string::npos) return NULL;
      lines.erase(lines.begin());
      if(lines[lines.size()-1].find("END CERTIFICATE") == std::string::npos) return NULL;
      certStr = Arc::join(lines, "");

      cJSON_AddItemToArray(x5cObject.Ptr(), cJSON_CreateString(certStr.c_str()));
      if(!chain) break;
      if(certN >= sk_X509_num(chain)) break;
      cert = sk_X509_value(keyHolder.CertificateChain(), certN);
      if(!cert) break;
      ++certN;
    };

    return x5cObject.Release();
  }

  bool JWSE::ExtractPublicKey() const {
    key_.Release();

    // So far we are going to support only embedded keys - jwk and x5c
    cJSON* x5cObject = cJSON_GetObjectItem(header_.Ptr(), HeaderNameX509CertChain);
    cJSON* jwkObject = cJSON_GetObjectItem(header_.Ptr(), HeaderNameJSONWebKey);
    cJSON* kidObject = cJSON_GetObjectItem(header_.Ptr(), HeaderNameJSONWebKeyId);
    if(x5cObject != NULL) {
      AutoPointer<JWSEKeyHolder> key(new JWSEKeyHolder());
      logger_.msg(DEBUG, "JWSE::ExtractPublicKey: x5c key");
      if(x5cParse(x5cObject, *key)) {
        key_ = key;
        return true;
      }
    } else if(jwkObject != NULL) {
      AutoPointer<JWSEKeyHolder> key(new JWSEKeyHolder());
      logger_.msg(DEBUG, "JWSE::ExtractPublicKey: jwk key");
      if(jwkParse(jwkObject, *key)) {
        key_ = key;
        return true;
      }
    } else if(kidObject != NULL) {
      logger_.msg(DEBUG, "JWSE::ExtractPublicKey: external jwk key");
      if(kidObject->type != cJSON_String)
        return false;
      // TODO: authority validation
      cJSON* issuerObj = cJSON_GetObjectItem(content_.Ptr(), ClaimNameIssuer);
      if(!issuerObj || (issuerObj->type != cJSON_String))
        return false;
      OpenIDMetadata serviceMetadata;
      OpenIDMetadataFetcher metadataFetcher(issuerObj->valuestring);
      if(!metadataFetcher.Fetch(serviceMetadata))
        return false;
      char const * jwksUri = serviceMetadata.JWKSURI();
      logger_.msg(DEBUG, "JWSE::ExtractPublicKey: fetching jwl key from %s", jwksUri);
      JWSEKeyFetcher keyFetcher(jwksUri);
      JWSEKeyHolderList keys;
      if(!keyFetcher.Fetch(keys))
        return false;
      for(JWSEKeyHolderList::iterator keyIt = keys.begin(); keyIt != keys.end(); ++keyIt) {
        if(strcmp(kidObject->valuestring, (*keyIt)->Id()) == 0) {
          key_ = *keyIt;
          return true;
        }
      }
    } else {
      logger_.msg(ERROR, "JWSE::ExtractPublicKey: no supported key");
      return true; // No key - no processing
    };
    logger_.msg(ERROR, "JWSE::ExtractPublicKey: key parsing error");

    return false;
  }
  
  bool JWSE::InsertPublicKey(bool& keyAdded) const {
    keyAdded = false;
    cJSON_DeleteItemFromObject(header_.Ptr(), HeaderNameX509CertChain);
    cJSON_DeleteItemFromObject(header_.Ptr(), HeaderNameJSONWebKey);
    if(key_ && key_->Certificate()) {
      cJSON* x5cObject = x5cSerialize(*key_);
      if(x5cObject) {
        cJSON_AddItemToObject(header_.Ptr(), HeaderNameX509CertChain, x5cObject);
        keyAdded = true;
        return true;
      }
    } else if(key_ && key_->PublicKey()) {
      cJSON* jwkObject = jwkSerialize(*key_);
      if(jwkObject) {
        cJSON_AddItemToObject(header_.Ptr(), HeaderNameJSONWebKey, jwkObject);
        keyAdded = true;
        return true;
      }
    } else {
      return true; // No key - no processing
    };

    return false;
  }

  void JWSE::Certificate(char const* certificate) {
    key_.Release();
    key_ = new JWSEKeyHolder(certificate);
    if(!key_->PublicKey()) // Expect at least public key
      key_.Release();
  }


  // ---------------------------------------------------------------------------------------------

  JWSEKeyHolder::JWSEKeyHolder(): certificate_(NULL), certificateChain_(NULL), publicKey_(NULL), privateKey_(NULL) {
  }

  JWSEKeyHolder::JWSEKeyHolder(char const* certificate): certificate_(NULL), certificateChain_(NULL), publicKey_(NULL), privateKey_(NULL) {
    if(certificate != NULL) {
      AutoPointer<BIO> mem(BIO_new(BIO_s_mem()), &BIO_deallocate);
      BIO_puts(mem.Ptr(), certificate);
      certificate_ = PEM_read_bio_X509(mem.Ptr(), NULL, NULL, NULL);
      if(certificate_ == NULL) return;
      publicKey_ = X509_get_pubkey(certificate_);
      privateKey_ = PEM_read_bio_PrivateKey(mem.Ptr(), NULL, NULL, NULL);
      if(privateKey_ == NULL) return;
      // Optional chain
      AutoPointer<STACK_OF(X509)> certchain(sk_X509_new_null(), &sk_x509_deallocate);
      int certN(0);
      while(!BIO_eof(mem.Ptr())) {
        AutoPointer<X509> cert(PEM_read_bio_X509(mem.Ptr(), NULL, NULL, NULL), *X509_free);
        if(sk_X509_insert(certchain.Ptr(), cert.Ptr(), certN) == 0) break;
        (void)cert.Release();
        ++certN;
      }
      if(certN > 0) certificateChain_ = certchain.Release();
    }
  }

  JWSEKeyHolder::~JWSEKeyHolder() {
    if(publicKey_) EVP_PKEY_free(publicKey_);
    publicKey_ = NULL;
    if(certificate_) X509_free(certificate_);
    certificate_ = NULL;
    if(certificateChain_) sk_x509_deallocate(certificateChain_);
    certificateChain_ = NULL;
  }

  char const* JWSEKeyHolder::Id() const {
    return id_.c_str();
  }

  void JWSEKeyHolder::Id(char const* id) {
    id_.assign(id ? id : "");
  }

  EVP_PKEY const* JWSEKeyHolder::PublicKey() const {
    return publicKey_;
  }

  void JWSEKeyHolder::PublicKey(EVP_PKEY* publicKey) {
    if(publicKey_) EVP_PKEY_free(publicKey_);
    publicKey_ = publicKey;
    if(certificate_) X509_free(certificate_);
    certificate_ = NULL;
  }

  EVP_PKEY const* JWSEKeyHolder::PrivateKey() const {
    return privateKey_;
  }

  void JWSEKeyHolder::PrivateKey(EVP_PKEY* privateKey) {
    if(privateKey_) EVP_PKEY_free(privateKey_);
    privateKey_ = privateKey;
    if(certificate_) X509_free(certificate_);
    certificate_ = NULL;
  }

  X509 const* JWSEKeyHolder::Certificate() const {
    return certificate_;
  }

  void JWSEKeyHolder::Certificate(X509* certificate) {
    if(publicKey_) EVP_PKEY_free(publicKey_);
    // call increases reference count
    publicKey_ = certificate ? X509_get_pubkey(certificate) : NULL;
    if(privateKey_) EVP_PKEY_free(privateKey_);
    // call increases reference count
    privateKey_ = certificate ? X509_get_privkey(certificate) : NULL;
    if(certificate_) X509_free(certificate_);
    certificate_ = certificate;
  }

  STACK_OF(X509) const* JWSEKeyHolder::CertificateChain() const {
    return certificateChain_;
  }

  void JWSEKeyHolder::CertificateChain(STACK_OF(X509)* certificateChain) {
    if(certificateChain_) sk_x509_deallocate(certificateChain_);
    certificateChain_ = certificateChain;
  }


  JWSEKeyFetcher::JWSEKeyFetcher(char const * endpoint_url) : url_(endpoint_url), client_(Arc::MCCConfig(), url_) {
  }

  bool JWSEKeyFetcher::Fetch(JWSEKeyHolderList& keys) {
    HTTPClientInfo info;
    PayloadRaw request;
    PayloadRawInterface* response(NULL);
    MCC_Status status = client_.process("GET", &request, &info, &response);
    if(!status)
      return false;
    if(!response)
      return false;
    if(!(response->Content()))
      return false;
    AutoPointer<cJSON> content(cJSON_Parse(response->Content()), &cJSON_Delete);
    if(!content)
      return false;
    cJSON* keysObj = cJSON_GetObjectItem(content.Ptr(), "keys");
    if(!keysObj || (keysObj->type != cJSON_Array))
      return false;
    for(int idx = 0; idx < cJSON_GetArraySize(keysObj); ++idx) {
      cJSON* keyObj = cJSON_GetArrayItem(keysObj, idx);
      if(!keyObj || (keyObj->type != cJSON_Object))
        continue;
      cJSON* kidObj = cJSON_GetObjectItem(keyObj, "kid");
      std::string id;
      if(kidObj && (kidObj->type == cJSON_String))
        id.assign(kidObj->valuestring);
      Arc::AutoPointer<JWSEKeyHolder> key(new JWSEKeyHolder());
      if(!key) continue;
      if(!jwkParse(keyObj, *key.Ptr()))
        continue;
      key->Id(id.c_str());
      keys.add(key);
    };
    return true;
  }


} // namespace Arc
