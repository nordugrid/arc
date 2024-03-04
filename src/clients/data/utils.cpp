// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/ArcConfig.h>
#include <arc/credential/Credential.h>

#include "utils.h"


bool checkproxy(const Arc::UserConfig& uc)
{
  if (!uc.ProxyPath().empty() ) {
    Arc::Credential holder(uc.ProxyPath(), "", "", "", false);
    if (holder.GetEndTime() < Arc::Time()){
      std::cout << Arc::IString("Proxy expired. Please run 'arcproxy'!") << std::endl;
      return false;
    }
  }

  return true;
}

bool checktoken(const Arc::UserConfig& uc) {
  if(uc.OToken().empty()) {
    std::cout << Arc::IString("Cannot find any token. Please run 'oidc-token' or use similar\n"
		              "  utility to obtain authentication token!") << std::endl;
    return false;
  }
  return true;
}

bool getAuthenticationType(Arc::Logger& logger, Arc::UserConfig const& usercfg,
                           bool no_authentication, bool x509_authentication, bool token_authentication,
                           AuthenticationType& authentication_type) {
  authentication_type = UndefinedAuthentication;
  if(no_authentication) {
    if(authentication_type != UndefinedAuthentication) {
      logger.msg(Arc::ERROR, "Conflicting authentication types specified.");
      return false;
    }
    authentication_type = NoAuthentication;
  }
  if(x509_authentication) {
    if(authentication_type != UndefinedAuthentication) {
      logger.msg(Arc::ERROR, "Conflicting authentication types specified.");
      return false;
    }
    authentication_type = X509Authentication;
  }
  if(token_authentication) {
    if(authentication_type != UndefinedAuthentication) {
      logger.msg(Arc::ERROR, "Conflicting authentication types specified.");
      return false;
    }
    authentication_type = TokenAuthentication;
  }

  if(authentication_type == X509Authentication) {
    if (!checkproxy(usercfg)) {
      return 1;
    }
  } else if(authentication_type == TokenAuthentication) {
    if (!checktoken(usercfg)) {
      return 1;
    }
  }

  return true;
}

