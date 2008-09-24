#ifndef __ARC_SAMLUTIL_H__
#define __ARC_SAMLUTIL_H__

#include <vector>
#include <string>

#include <xmlsec/crypto.h>

namespace Arc {

  typedef enum {
    RSA_SHA1,
    DSA_SHA1
  } SignatureMethod;

  std::string SignQuery(std::string query, SignatureMethod sign_method, std::string& privkey_file);

  bool VerifyQuery(const std::string query, const xmlSecKey *sender_public_key);

}// namespace Arc

#endif /* __ARC_SAMLUTIL_H__ */
