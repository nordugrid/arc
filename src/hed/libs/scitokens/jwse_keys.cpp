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
    std::string modulus = Base64::decodeURLSafe(modulusObject->string);
    std::string exponent = Base64::decodeURLSafe(exponentObject->string);
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
    std::string key = Base64::decodeURLSafe(keyObject->string);
    // It looks like RFC does not define any "alg" values with "JWK" usage.
    return NULL;
  }

  static EVP_PKEY* jwkParse(cJSON* jwkObject) {
    if(jwkObject->type != cJSON_Object) return NULL;

    cJSON* ktyObject = cJSON_GetObjectItem(jwkObject, "kty");
    if(ktyObject == NULL) return NULL;
    if(ktyObject->type != cJSON_String) return NULL;
    if(strcmp(ktyObject->string, "EC") == 0) {    
      return jwkECParse(jwkObject);
    } else if(strcmp(ktyObject->string, "RSA") == 0) {    
      return jwkRSAParse(jwkObject);
    } else if(strcmp(ktyObject->string, "oct") == 0) {    
      return jwkOctParse(jwkObject);
    }
    return NULL;
  }

  static EVP_PKEY* x5cParse(cJSON* x5cObject) {
    if(x5cObject->type != cJSON_Array) return NULL;
    // Collect the chain
    AutoPointer<STACK_OF(X509)> certchain(sk_X509_new_null(), &sk_x509_deallocate);
    int certN = 0;
    while(true) {
      cJSON* certObject = cJSON_GetArrayItem(x5cObject, certN);
      if(certObject == NULL) break;
      if(certObject->type != cJSON_String) return NULL;
      // It should be PEM encoded certificate in single string and without header/footer           
      AutoPointer<BIO> mem(BIO_new(BIO_s_mem()), &BIO_deallocate);
      if(!mem) return NULL;
      BIO_puts(mem.Ptr(), "-----BEGIN CERTIFICATE-----\n");
      BIO_puts(mem.Ptr(), certObject->string);
      BIO_puts(mem.Ptr(), "-----END CERTIFICATE-----\n");
      AutoPointer<X509> cert(PEM_read_bio_X509(mem.Ptr(), NULL, NULL, NULL), *X509_free);
      if(!cert) return NULL;
      if(sk_X509_insert(certchain.Ptr(), cert.Ptr(), certN) == 0) return NULL;
      (void)cert.Release();
    }
    // Verify the chain

    // Remember and return public key of first certificate
    X509* mainCert = sk_X509_value(certchain.Ptr(), 0);
    if(mainCert == NULL) return NULL;
    // Note: following call increases reference count
    return X509_PUBKEY_get(X509_get_X509_PUBKEY(mainCert));
  }

  evp_pkey_st* JWSE::GetPublicKey() const {
    if (publicKey_ != NULL) return publicKey_;
    // So far we are going to support only embedded keys - jwk and x5c
    cJSON* x5cObject = cJSON_GetObjectItem(header_, "x5c");
    cJSON* jwkObject = cJSON_GetObjectItem(header_, "jwk");
    if(x5cObject != NULL) {
      publicKey_ = x5cParse(x5cObject);
    } else if(jwkObject != NULL) {
      publicKey_ = jwkParse(jwkObject);
    }
    return publicKey_;
  }
  
} // namespace Arc
