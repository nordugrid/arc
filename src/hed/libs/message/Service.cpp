#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "Service.h"

namespace Arc {

  Logger Service::logger(Logger::rootLogger, "Service");

void Service::handle(Handler* handler,const std::string& label) {
    if(handler) handlers_[label].push_back(handler);
}
/*
void Service::AuthN(AuthNHandler* authn,const std::string& label) {
    if(authn) authn_[label].push_back(authn);
}

void Service::AuthZ(AuthZHandler* authz,const std::string& label) {
    if(authz) authz_[label].push_back(authz);
}
*/
} // namespace Arc
