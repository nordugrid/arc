#include <cctype>
#include <cstdlib>
#include <cstring>

#include <arc/Utils.h>
#include <arc/external/cJSON/cJSON.h>
#include <openssl/evp.h>

#include "jwse.h"

#if OPENSSL_VERSION_NUMBER < 0x10100000L
#define EVP_MD_CTX_new EVP_MD_CTX_create
#define EVP_MD_CTX_free EVP_MD_CTX_destroy
#endif

// EVP_PKEY_get0_EC_KEY

namespace Arc {

  bool JWSE::VerifyECDSA(char const* digestName, void const* message, unsigned int messageSize,
                                                 void const* signature, unsigned int signatureSize) {
    AutoPointer<EVP_PKEY> pkey(GetPublicKey(), &EVP_PKEY_free);
    if(!pkey) return false;
    AutoPointer<EVP_MD_CTX> ctx(EVP_MD_CTX_new(),&EVP_MD_CTX_free);
    if(!ctx) return false;
    const EVP_MD* md = EVP_get_digestbyname(digestName);
    if(!md) return false;
    int rc = EVP_DigestVerifyInit(ctx.Ptr(), NULL, md, NULL, pkey.Ptr());
    if(rc != 1) return false;
    rc = EVP_DigestVerifyUpdate(ctx.Ptr(), message, messageSize);
    if(rc != 1) return false;
    // const_cast is only for older openssl
    rc = EVP_DigestVerifyFinal(ctx.Ptr(), const_cast<unsigned char*>(reinterpret_cast<unsigned char const*>(signature)), signatureSize);
    if(rc != 1) return false;
    return true;
  }
  
} // namespace Arc
