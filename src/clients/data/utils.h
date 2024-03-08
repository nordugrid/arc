#ifndef __ARC_CLIENT_DATA_UTILS_H_
#define __ARC_CLIENT_DATA_UTILS_H_

#include <arc/Logger.h>
#include <arc/UserConfig.h>

bool checkproxy(const Arc::UserConfig& uc);

bool checktoken(const Arc::UserConfig& uc);

enum AuthenticationType {
  UndefinedAuthentication,
  NoAuthentication,
  X509Authentication,
  TokenAuthentication
};

bool getAuthenticationType(Arc::Logger& logger, Arc::UserConfig const& usercfg,
                           bool no_authentication, bool x509_authentication, bool token_authentication,
                           AuthenticationType& authentication_type);

#endif // __ARC_CLIENT_DATA_UTILS_H_
