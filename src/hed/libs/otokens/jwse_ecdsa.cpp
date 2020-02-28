#include <cctype>
#include <cstdlib>
#include <cstring>

#include <arc/Utils.h>
#include <arc/external/cJSON/cJSON.h>
#include <openssl/evp.h>

#include "otokens.h"
#include "jwse_private.h"

#if OPENSSL_VERSION_NUMBER < 0x10100000L
#define EVP_MD_CTX_new EVP_MD_CTX_create
#define EVP_MD_CTX_free EVP_MD_CTX_destroy
#endif

// EVP_PKEY_get0_EC_KEY

namespace Arc {

  bool JWSE::VerifyECDSA(char const* digestName, void const* message, unsigned int messageSize,
                                                 void const* signature, unsigned int signatureSize) {
    if(!key_) return false;
    AutoPointer<EVP_MD_CTX> ctx(EVP_MD_CTX_new(),&EVP_MD_CTX_free);
    if(!ctx) return false;
    const EVP_MD* md = EVP_get_digestbyname(digestName);
    if(!md) return false;
    int rc = EVP_DigestVerifyInit(ctx.Ptr(), NULL, md, NULL, const_cast<EVP_PKEY*>(key_->PublicKey()));
    if(rc != 1) return false;
    rc = EVP_DigestVerifyUpdate(ctx.Ptr(), message, messageSize);
    if(rc != 1) return false;
    // const_cast is only for older openssl
    rc = EVP_DigestVerifyFinal(ctx.Ptr(), const_cast<unsigned char*>(reinterpret_cast<unsigned char const*>(signature)), signatureSize);
    if(rc != 1) return false;
    return true;
  }
  
  bool JWSE::SignECDSA(char const* digestName, void const* message, unsigned int messageSize, std::string& signature) const {
    if(!key_) return false;
    AutoPointer<EVP_MD_CTX> ctx(EVP_MD_CTX_new(),&EVP_MD_CTX_free);
    if(!ctx) return false;
    const EVP_MD* md = EVP_get_digestbyname(digestName);
    if(!md) return false;
    int rc = EVP_DigestSignInit(ctx.Ptr(), NULL, md, NULL, const_cast<EVP_PKEY*>(key_->PrivateKey()));
    if(rc != 1) return false;
    rc = EVP_DigestSignUpdate(ctx.Ptr(), message, messageSize);
    if(rc != 1) return false;
    size_t signatureSize = 0;
    rc = EVP_DigestSignFinal(ctx.Ptr(), NULL, &signatureSize);
    if((rc != 1) || (signatureSize <= 0)) return false;
    signature.resize(signatureSize);
    rc = EVP_DigestSignFinal(ctx.Ptr(), const_cast<unsigned char*>(reinterpret_cast<unsigned char const*>(signature.c_str())), &signatureSize);
    if((rc != 1) || (signatureSize <= 0)) return false;
    return true;
  }

} // namespace Arc
