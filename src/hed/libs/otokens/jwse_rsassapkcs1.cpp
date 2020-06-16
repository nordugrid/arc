#include <cctype>
#include <cstdlib>
#include <cstring>

#include <arc/Utils.h>
#include <arc/external/cJSON/cJSON.h>
#include <openssl/evp.h>
#include <openssl/x509.h>
#include <openssl/pem.h>
#include <openssl/rsa.h>
#include <openssl/bn.h>

#include "otokens.h"
#include "jwse_private.h"

#if OPENSSL_VERSION_NUMBER < 0x10100000L
#define EVP_MD_CTX_new EVP_MD_CTX_create
#define EVP_MD_CTX_free EVP_MD_CTX_destroy
#endif

namespace Arc {

  template<typename T> void ArrayDeleter(T* o) { delete[] o; }

  bool JWSE::VerifyRSASSAPKCS1(char const* digestName, void const* message, unsigned int messageSize, void const* signature, unsigned int signatureSize) {
    if(!key_) return false;
    AutoPointer<EVP_MD_CTX> ctx(EVP_MD_CTX_new(),&EVP_MD_CTX_free);
    if(!ctx) return false;

    const EVP_MD* md = EVP_get_digestbyname(digestName);
    if(!md) return false;
    int rc = EVP_DigestInit_ex(ctx.Ptr(), md, NULL);
    if(rc != 1) return false;
    rc = EVP_DigestUpdate(ctx.Ptr(), message, messageSize);
    if(rc != 1) return false;
    AutoPointer<unsigned char> digestBuffer(new unsigned char[EVP_MAX_MD_SIZE], &(ArrayDeleter<unsigned char>));
    if(!digestBuffer) return false;
    unsigned int digestSize = EVP_MAX_MD_SIZE;
    rc = EVP_DigestFinal_ex(ctx.Ptr(), digestBuffer.Ptr(), &digestSize);
    if(rc != 1) return false;

    EVP_PKEY* privateKey = const_cast<EVP_PKEY*>(key_->PublicKey());
    if(!privateKey) return false;
    // EVP_PKEY_get0_RSA is more convenient. But is not available in all versions.
    AutoPointer<RSA> rsa(EVP_PKEY_get1_RSA(privateKey), &RSA_free);
    if(!rsa) return false;

    rc = RSA_verify(EVP_MD_type(md), digestBuffer.Ptr(), digestSize, (unsigned char const*)signature, signatureSize, rsa.Ptr());
    if(rc != 1) return false;

    return true;
  }

  bool JWSE::SignRSASSAPKCS1(char const* digestName, void const* message, unsigned int messageSize, std::string& signature) const {
    if(!key_) return false;
    AutoPointer<EVP_MD_CTX> ctx(EVP_MD_CTX_new(),&EVP_MD_CTX_free);
    if(!ctx) return false;

    const EVP_MD* md = EVP_get_digestbyname(digestName);
    if(!md) return false;
    int rc = EVP_DigestInit_ex(ctx.Ptr(), md, NULL);
    if(rc != 1) return false;
    rc = EVP_DigestUpdate(ctx.Ptr(), message, messageSize);
    if(rc != 1) return false;
    AutoPointer<unsigned char> digestBuffer(new unsigned char[EVP_MAX_MD_SIZE], &(ArrayDeleter<unsigned char>));
    if(!digestBuffer) return false;
    unsigned int digestSize = EVP_MAX_MD_SIZE;
    rc = EVP_DigestFinal_ex(ctx.Ptr(), digestBuffer.Ptr(), &digestSize);
    if(rc != 1) return false;
    
    EVP_PKEY* privateKey = const_cast<EVP_PKEY*>(key_->PrivateKey());
    if(!privateKey) return false;
    // EVP_PKEY_get0_RSA is more convenient. But is not available in all versions.
    AutoPointer<RSA> rsa(EVP_PKEY_get1_RSA(privateKey), &RSA_free);
    if(!rsa) return false;
    int rsaSize = RSA_size(rsa.Ptr());
    if(rsaSize <= 0) return false;
    AutoPointer<unsigned char> signatureBuffer(new unsigned char[rsaSize], &ArrayDeleter<unsigned char>);
    if(!signatureBuffer) return false;
    unsigned int signatureSize(rsaSize);

    rc = RSA_sign(EVP_MD_type(md), digestBuffer.Ptr(), digestSize, signatureBuffer.Ptr(), &signatureSize, rsa.Ptr());
    if(rc != 1) return false;
    if(signatureSize <= 0) return false;

    signature.assign((char*)signatureBuffer.Ptr(), signatureSize);
    return true;
  }

  
} // namespace Arc
