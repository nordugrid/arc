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

}
