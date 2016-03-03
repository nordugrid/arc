#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string>
#include <vector>

#include <arc/URL.h>
#include <arc/Logger.h>

#include "auth.h"

namespace ArcSHCLegacy {

#define LDAP_CONNECT_TIMEOUT 10
#define LDAP_QUERY_TIMEOUT 20
#define LDAP_RESULT_TIMEOUT 60

static Arc::Logger logger(Arc::Logger::getRootLogger(),"AuthUserLDAP");

AuthResult AuthUser::match_ldap(const char* line) {
  logger.msg(Arc::ERROR, "LDAP authorization is not supported anymore");
  return AAA_FAILURE;
}

} // namespace ArcSHCLegacy

