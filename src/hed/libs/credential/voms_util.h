#ifndef __ARC_VOMSUTIL_H__
#define __ARC_VOMSUTIL_H__

#include <vector>
#include <string>

#include "Credential.h"

namespace ArcLib {
  
  void InitVOMSAttribute(void);

  int createVOMSAC(X509 *issuer, STACK_OF(X509) *issuerstack, X509 *holder, EVP_PKEY *pkey, BIGNUM *serialnum,
             std::vector<std::string> &fqan, std::vector<std::string> &targets, std::vector<std::string>& attributes,
             AC **ac, std::string voname, std::string uri, int lifetime);

  bool createVOMSAC(std::string& codedac, Credential& issuer_cred, Credential& holder_cred,
             std::vector<std::string> &fqan, std::vector<std::string> &targets, std::vector<std::string>& attributes, 
             std::string &voname, std::string &uri, int lifetime);

  bool addVOMSAC(AC** &aclist, std::string &codedac);

  bool parseVOMSAC(X509* holder, std::string& ca_cert_dir, std::string& voms_dir, std::vector<std::string>& output);

  bool parseVOMSAC(Credential& holder_cred, std::string& ca_cert_dir, std::string& voms_dir, std::vector<std::string>& output);

}// namespace ArcLib

#endif /* __ARC_VOMSUTIL_H__ */

