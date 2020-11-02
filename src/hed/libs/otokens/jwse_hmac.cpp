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

namespace Arc {

  bool JWSE::VerifyHMAC(char const* digestName, void const* message, unsigned int messageSize,
                                                void const* signature, unsigned int signatureSize) {
    AutoPointer<EVP_MD_CTX> ctx(EVP_MD_CTX_new(),&EVP_MD_CTX_free);
    if(!ctx) return false;
    // EVP_sha256
    const EVP_MD* md = EVP_get_digestbyname(digestName);
    if(!md) return false;
    int rc = EVP_DigestInit_ex(ctx.Ptr(), md, NULL);
    if(rc != 1) return false;
    rc = EVP_DigestUpdate(ctx.Ptr(), message, messageSize);
    if(rc != 1) return false;
    AutoPointer<unsigned char> buffer(reinterpret_cast<unsigned char*>(std::malloc(signatureSize)),
                                      reinterpret_cast<void (*)(unsigned char*)>(&std::free));
    if(!buffer) return false;
    unsigned int size = signatureSize;
    rc = EVP_DigestFinal_ex(ctx.Ptr(), buffer.Ptr(), &size);
    if(rc != 1) return false;
    if(size != signatureSize) {
      std::memset(buffer.Ptr(), 0, signatureSize);
      return false;
    }
    rc = 0;
    for(unsigned int n = 0; n < signatureSize; ++n) rc |= (reinterpret_cast<unsigned char const*>(signature))[n] ^ buffer.Ptr()[n];
    std::memset(buffer.Ptr(), 0, signatureSize);
    if(rc != 1) return false;
    return true;
  }
  
} // namespace Arc
