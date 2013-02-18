#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

// SecHandler.cpp

#include "SecHandler.h"

namespace ArcSec{
  
  Arc::Logger SecHandler::logger(Arc::Logger::rootLogger, "SecHandler");

  SecHandlerConfig::SecHandlerConfig(const std::string& name,const std::string& event,const std::string& id):Arc::XMLNode("<SecHandler xmlns=\"http://www.nordugrid.org/schemas/ArcConfig/2007\"/>") {
    NewAttribute("name")=name;
    if(!event.empty()) NewAttribute("event")=event;
    if(!id.empty()) NewAttribute("id")=id;
  }

  SecHandlerStatus::SecHandlerStatus(void):
    code(STATUS_DENY) {
  }

  SecHandlerStatus::SecHandlerStatus(bool positive):
    code(positive?STATUS_ALLOW:STATUS_DENY) {
  }

  SecHandlerStatus::SecHandlerStatus(int code_):
    code(code_) {
  }

  SecHandlerStatus::SecHandlerStatus(int code_, const std::string& explanation_):
    code(code_),explanation(explanation_) {
  }

  SecHandlerStatus::SecHandlerStatus(int code_,const std::string& origin_,const std::string& explanation_):
    code(code_),origin(origin_),explanation(explanation_) {
  }

  int SecHandlerStatus::getCode(void) const {
    return code;
  }

  const std::string& SecHandlerStatus::getOrigin(void) const {
    return origin;
  }

  const std::string& SecHandlerStatus::getExplanation(void) const {
    return explanation;
  }

}

