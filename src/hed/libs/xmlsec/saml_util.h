#ifndef __ARC_SAMLUTIL_H__
#define __ARC_SAMLUTIL_H__

#include <vector>
#include <string>

#include <xmlsec/crypto.h>

#include <arc/XMLNode.h>


#define SAML_NAMESPACE "urn:oasis:names:tc:SAML:2.0:assertion"
#define SAMLP_NAMESPACE "urn:oasis:names:tc:SAML:2.0:protocol"

#define XENC_NAMESPACE   "http://www.w3.org/2001/04/xmlenc#"
#define DSIG_NAMESPACE   "http://www.w3.org/2000/09/xmldsig#"

namespace Arc {

  typedef enum {
    RSA_SHA1,
    DSA_SHA1
  } SignatureMethod;

  std::string DeflateData(const std::string& data);
  
  std::string InflateData(const std::string& data);

  std::string URIEscape(const std::string& data);

  std::string URIUnEscape(const std::string& data);

  std::string Base64Encode(const std::string& data);

  std::string Base64Decode(const std::string& data);


  std::string SignQuery(std::string query, SignatureMethod sign_method, std::string& privkey_file);

  //bool VerifyQuery(const std::string query, const xmlSecKey *sender_public_key);

  bool VerifyQuery(const std::string query, const std::string& sender_cert_str);

  std::string BuildDeflatedQuery(const Arc::XMLNode& node);

  bool BuildNodefromMsg(const std::string msg, Arc::XMLNode& node);

}// namespace Arc

#endif /* __ARC_SAMLUTIL_H__ */
