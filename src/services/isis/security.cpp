#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "security.h"

using namespace Arc;

namespace ISIS {

ISISSecAttr::ISISSecAttr(const std::string& action) {
  action_=action;
}

ISISSecAttr::~ISISSecAttr(void) {
}

ISISSecAttr::operator bool(void) const {
  return !action_.empty();
}

bool ISISSecAttr::equal(const SecAttr &b) const {
  try {
    const ISISSecAttr& a = (const ISISSecAttr&)b;
    return ((action_ == a.action_) && (context_ == a.context_));
  } catch(std::exception&) { };
  return false;
}

bool ISISSecAttr::Export(SecAttrFormat format,XMLNode &val) const {
  if(format == UNDEFINED) {
  } else if(format == ARCAuth) {
    NS ns;
    ns["ra"]="http://www.nordugrid.org/schemas/request-arc";
    val.Namespaces(ns); val.Name("ra:Request");
    XMLNode item = val.NewChild("ra:RequestItem");
    if(!action_.empty()) {
      XMLNode action = item.NewChild("ra:Action");
      action=action_;
      action.NewAttribute("Type")="string";
      action.NewAttribute("AttributeId")="http://www.nordugrid.org/schemas/policy-arc/types/isis/operation";
    };
    return true;
  } else {
  };
  return false;
}

} // namespace ISIS

