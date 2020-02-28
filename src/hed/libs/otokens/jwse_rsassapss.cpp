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

  template<typename T> void ArrayDeleter(T* o) { delete[] o; }

  bool JWSE::VerifyRSASSAPSS(char const* digestName, void const* message, unsigned int messageSize, void const* signature, unsigned int signatureSize) {
    return false;

  }

  bool JWSE::SignRSASSAPSS(char const* digestName, void const* message, unsigned int messageSize, std::string& signature) const {
    return false;
  }

  
} // namespace Arc
