#ifndef __ARC_VOMSUTIL_H__
#define __ARC_VOMSUTIL_H__

#include <vector>
#include <string>

extern "C" {
#include "VOMSAttribute.h"
}

namespace ArcLib {
  int createVOMSAC(X509 *issuer, STACK_OF(X509) *issuerstack, X509 *holder, EVP_PKEY *pkey, BIGNUM *serialnum,
             std::vector<std::string> &fqan, std::vector<std::string> &targets, std::vector<std::string>& attributes,
             AC **ac, std::string vo, std::string uri, int lifetime);




}// namespace ArcLib

#endif /* __ARC_VOMSUTIL_H__ */

