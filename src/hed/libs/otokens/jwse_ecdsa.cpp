#include <cctype>
#include <cstdlib>
#include <cstring>

#include <arc/Utils.h>
#include <arc/external/cJSON/cJSON.h>
#include <arc/crypto/OpenSSL.h>
#include <openssl/evp.h>
#include <openssl/err.h>
#include <openssl/bn.h>

#include "otokens.h"
#include "jwse_private.h"


namespace Arc {

  bool JWSE::VerifyECDSA(char const* digestName, void const* message, unsigned int messageSize,
                                                 void const* signature, unsigned int signatureSize) {
    if(!key_) {
      logger_.msg(DEBUG, "JWSE::VerifyECDSA: missing key");
      return false;
    }
    if(signatureSize != 64) {
      logger_.msg(DEBUG, "JWSE::VerifyECDSA: wrong signature size");
      return false;
    }

    // Key
    EC_KEY* key = const_cast<EC_KEY*>(EVP_PKEY_get0_EC_KEY(const_cast<EVP_PKEY*>(key_->PublicKey())));
    // Signature
    AutoPointer<ECDSA_SIG> sig(ECDSA_SIG_new(),&ECDSA_SIG_free);
    if(!sig) {
      logger_.msg(DEBUG, "JWSE::VerifyECDSA: failed to create ECDSA signature");
      HandleOpenSSLError();
      return false;
    }
    AutoPointer<BIGNUM> r(BN_bin2bn(reinterpret_cast<unsigned char const *>(signature)+0,32,NULL), &BN_free);
    AutoPointer<BIGNUM> s(BN_bin2bn(reinterpret_cast<unsigned char const *>(signature)+32,32,NULL), &BN_free);
    if(!r || !s) {
      logger_.msg(DEBUG, "JWSE::VerifyECDSA: failed to parse signature");
      HandleOpenSSLError();
      return false;
    }
    int rc = ECDSA_SIG_set0(sig.Ptr(), r.Ptr(), s.Ptr());
    if(rc != 1) {
      logger_.msg(DEBUG, "JWSE::VerifyECDSA: failed to assign ECDSA signature: %i",rc);
      HandleOpenSSLError();
      return false;
    }
    r.Release();
    s.Release();
    // Hash
    AutoPointer<EVP_MD_CTX> ctx(EVP_MD_CTX_new(),&EVP_MD_CTX_free);
    if(!ctx) {
      logger_.msg(DEBUG, "JWSE::VerifyECDSA: failed to create EVP context");
      return false;
    }
    const EVP_MD* md = EVP_get_digestbyname(digestName);
    if(!md) {
      logger_.msg(DEBUG, "JWSE::VerifyECDSA: failed to recognize digest: %s",digestName);
      return false;
    }
    rc = EVP_DigestInit(ctx.Ptr(),md);
    if(rc != 1) {
      logger_.msg(DEBUG, "JWSE::VerifyECDSA: failed to initialize hash: %i",rc);
      HandleOpenSSLError();
      return false;
    }
    rc = EVP_DigestUpdate(ctx.Ptr(), message, messageSize);
    if(rc != 1) {
      logger_.msg(DEBUG, "JWSE::VerifyECDSA: failed to add message to hash: %i",rc);
      HandleOpenSSLError();
      return false;
    }
    unsigned char hash[EVP_MAX_MD_SIZE];
    unsigned int hashSize = 0;
    rc = EVP_DigestFinal(ctx.Ptr(), hash, &hashSize);
    if(rc != 1) {
      logger_.msg(DEBUG, "JWSE::VerifyECDSA: failed to finalize hash: %i",rc);
      HandleOpenSSLError();
      return false;
    }
    // Verification
    rc = ECDSA_do_verify(hash,hashSize,sig.Ptr(),key);
    if(rc != 1) {
      logger_.msg(DEBUG, "JWSE::VerifyECDSA: failed to verify: %i",rc);
      HandleOpenSSLError();
      return false;
    }
    return true;
  }
  
  bool JWSE::SignECDSA(char const* digestName, void const* message, unsigned int messageSize, std::string& signature) const {
    if(!key_) {
      logger_.msg(DEBUG, "JWSE::SignECDSA: missing key");
      return false;
    }
    // Key
    EC_KEY* key = const_cast<EC_KEY*>(EVP_PKEY_get0_EC_KEY(const_cast<EVP_PKEY*>(key_->PrivateKey())));
    // Hash
    AutoPointer<EVP_MD_CTX> ctx(EVP_MD_CTX_new(),&EVP_MD_CTX_free);
    if(!ctx) {
      logger_.msg(DEBUG, "JWSE::SignECDSA: failed to create EVP context");
      return false;
    }
    const EVP_MD* md = EVP_get_digestbyname(digestName);
    if(!md) {
      logger_.msg(DEBUG, "JWSE::SignECDSA: failed to recognize digest: %s",digestName);
      return false;
    }
    int rc = EVP_DigestInit(ctx.Ptr(),md);
    if(rc != 1) {
      logger_.msg(DEBUG, "JWSE::SignECDSA: failed to initialize hash: %i",rc);
      HandleOpenSSLError();
      return false;
    }
    rc = EVP_DigestUpdate(ctx.Ptr(), message, messageSize);
    if(rc != 1) {
      logger_.msg(DEBUG, "JWSE::SignECDSA: failed to add message to hash: %i",rc);
      HandleOpenSSLError();
      return false;
    }
    unsigned char hash[EVP_MAX_MD_SIZE];
    unsigned int hashSize = 0;
    rc = EVP_DigestFinal(ctx.Ptr(), hash, &hashSize);
    if(rc != 1) {
      logger_.msg(DEBUG, "JWSE::SignECDSA: failed to finalize hash: %i",rc);
      HandleOpenSSLError();
      return false;
    }
    // Signature
    AutoPointer<ECDSA_SIG> sig(ECDSA_do_sign(hash,hashSize,key),&ECDSA_SIG_free);
    if(!sig) {
      logger_.msg(DEBUG, "JWSE::SignECDSA: failed to create ECDSA signature");
      HandleOpenSSLError();
      return false;
    }
    BIGNUM const * r = NULL;
    BIGNUM const * s = NULL;
    ECDSA_SIG_get0(sig.Ptr(), &r, &s);
    if(!r || !s) {
      logger_.msg(DEBUG, "JWSE::SignECDSA: failed to parse signature");
      HandleOpenSSLError();
      return false;
    }
    int rSize = BN_num_bytes(r);
    int sSize = BN_num_bytes(s);
    if((rSize != 32) || (sSize != 32)) {
      logger_.msg(DEBUG, "JWSE::SignECDSA: wrong signature size: %i + %i",rSize,sSize);
      return false;
    }
    signature.resize(64);
    if((BN_bn2bin(r,reinterpret_cast<unsigned char*>(const_cast<char*>(signature.c_str()+0))) != 32) ||
       (BN_bn2bin(r,reinterpret_cast<unsigned char*>(const_cast<char*>(signature.c_str()+32))) != 32)) {
      logger_.msg(DEBUG, "JWSE::SignECDSA: wrong signature size written");
      return false;
    }
    return true;
  }

} // namespace Arc
