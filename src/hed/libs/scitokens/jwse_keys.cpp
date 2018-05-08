#include <cctype>
#include <cstdlib>
#include <cstring>

#include <arc/Utils.h>
#include <arc/Base64.h>
#include <arc/external/cJSON/cJSON.h>
#include <openssl/evp.h>
#include <openssl/x509.h>
#include <openssl/pem.h>
#include <openssl/rsa.h>
#include <openssl/bn.h>

#include "jwse.h"
#include "jwse_private.h"


#if OPENSSL_VERSION_NUMBER < 0x10100000L

static int RSA_set0_key(RSA *r, BIGNUM *n, BIGNUM *e, BIGNUM *d)
{
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

#endif


namespace Arc {
 
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
    BIGNUM* n(NULL);
    BIGNUM* e(NULL);
    if((n = BN_bin2bn(reinterpret_cast<unsigned char const *>(modulus.c_str()), modulus.length(), NULL)) == NULL) return NULL;
    if((e = BN_bin2bn(reinterpret_cast<unsigned char const *>(exponent.c_str()), exponent.length(), NULL)) == NULL) return NULL;
    RSA_set0_key(rsaKey.Ptr(), n, e, NULL);
    AutoPointer<EVP_PKEY> evpKey(EVP_PKEY_new(), &EVP_PKEY_free);
    if(!evpKey) return NULL;
    if(EVP_PKEY_set1_RSA(evpKey.Ptr(), rsaKey.Ptr()) != 1) return NULL;
    return evpKey.Release();
  }

  static EVP_PKEY* jwkOctParse(cJSON* jwkObject) {
    cJSON* keyObject = cJSON_GetObjectItem(jwkObject, "k");
    cJSON* algObject = cJSON_GetObjectItem(jwkObject, "alg");
    if((keyObject == NULL) || (algObject == NULL)) return NULL;
    if((keyObject->type != cJSON_String) || (algObject->type != cJSON_String)) return NULL;
    std::string key = Base64::decodeURLSafe(keyObject->valuestring);
    // It looks like RFC does not define any "alg" values with "JWK" usage.
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
      BIO_puts(mem.Ptr(), "-----END CERTIFICATE-----\n");
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
    keyHolder.PublicKey(X509_PUBKEY_get(X509_get_X509_PUBKEY(maincert.Ptr()))); // call increases reference count
    keyHolder.Certificate(maincert.Release());
    keyHolder.CertificateChain(certchain.Release());

    return true;
  }

  bool JWSE::ExtractPublicKey() const {
    key_.Release();
    AutoPointer<JWSEKeyHolder> key(new JWSEKeyHolder());

    // So far we are going to support only embedded keys - jwk and x5c
    cJSON* x5cObject = cJSON_GetObjectItem(header_.Ptr(), "x5c");
    cJSON* jwkObject = cJSON_GetObjectItem(header_.Ptr(), "jwk");
    if(x5cObject != NULL) {
      if(x5cParse(x5cObject, *key)) {
        key_ = key.Release();
        return true;
      }
    } else if(jwkObject != NULL) {
      if(jwkParse(jwkObject, *key)) {
        key_ = key.Release();
        return true;
      }
    } else {
      return true; // No key - no processing
    };

    return false;
  }
  

  JWSEKeyHolder::JWSEKeyHolder(): certificate_(NULL), certificateChain_(NULL), publicKey_(NULL) {
  }

  JWSEKeyHolder::~JWSEKeyHolder() {
    if(publicKey_) EVP_PKEY_free(publicKey_);
    publicKey_ = NULL;
    if(certificate_) X509_free(certificate_);
    certificate_ = NULL;
    if(certificateChain_) sk_x509_deallocate(certificateChain_);
    certificateChain_ = NULL;
  }

  EVP_PKEY* JWSEKeyHolder::PublicKey() {
    return publicKey_;
  }

  void JWSEKeyHolder::PublicKey(EVP_PKEY* publicKey) {
    if(publicKey_) EVP_PKEY_free(publicKey_);
    publicKey_ = publicKey;
  }

  X509* JWSEKeyHolder::Certificate() {
    return certificate_;
  }

  void JWSEKeyHolder::Certificate(X509* certificate) {
    if(certificate_) X509_free(certificate_);
    certificate_ = certificate;
  }

  STACK_OF(X509)* JWSEKeyHolder::CertificateChain() {
    return certificateChain_;
  }

  void JWSEKeyHolder::CertificateChain(STACK_OF(X509)* certificateChain) {
    if(certificateChain_) sk_x509_deallocate(certificateChain_);
    certificateChain_ = certificateChain;
  }


} // namespace Arc
