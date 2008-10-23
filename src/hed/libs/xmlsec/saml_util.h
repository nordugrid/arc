#ifndef __ARC_SAMLUTIL_H__
#define __ARC_SAMLUTIL_H__

#include <vector>
#include <string>

#include <xmlsec/crypto.h>

#include <arc/XMLNode.h>

namespace Arc {

  typedef enum {
    RSA_SHA1,
    DSA_SHA1
  } SignatureMethod;

  std::string SignQuery(std::string query, SignatureMethod sign_method, std::string& privkey_file);

  bool VerifyQuery(const std::string query, const xmlSecKey *sender_public_key);

  std::string BuildDeflatedQuery(const Arc::XMLNode& node);

  bool BuildNodefromMsg(const std::string msg, Arc::XMLNode& node);

}// namespace Arc

#endif /* __ARC_SAMLUTIL_H__ */
