#ifndef __ARC_VOMSUTIL_H__
#define __ARC_VOMSUTIL_H__

#include <vector>
#include <string>

#include "Credential.h"

extern "C" {
#include "VOMSAttribute.h"
}

namespace ArcLib {
  
  void InitVOMSAttribute(void);

  int createVOMSAC(X509 *issuer, STACK_OF(X509) *issuerstack, X509 *holder, EVP_PKEY *pkey, BIGNUM *serialnum,
             std::vector<std::string> &fqan, std::vector<std::string> &targets, std::vector<std::string>& attributes,
             AC **ac, std::string voname, std::string uri, int lifetime);

  bool createVOMSAC(std::string &codedac, Credential &issuer_cred, Credential &holder_cred,
             std::vector<std::string> &fqan, std::vector<std::string> &targets, std::vector<std::string>& attributes, 
             std::string &voname, std::string &uri, int lifetime);

  bool addVOMSAC(AC** &aclist, std::string &codedac);


}// namespace ArcLib

#endif /* __ARC_VOMSUTIL_H__ */

