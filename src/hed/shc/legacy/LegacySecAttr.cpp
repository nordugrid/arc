#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "LegacySecAttr.h"


namespace ArcSHCLegacy {


LegacySecAttr::LegacySecAttr(Arc::Logger& logger):logger_(logger) {
}

LegacySecAttr::~LegacySecAttr(void) {
}

std::string LegacySecAttr::get(const std::string& id) const {
  if(id == "GROUP") {
    if(groups_.size() > 0) return *groups_.begin();
    return "";
  };
  if(id == "VO") {
    if(vos_.size() > 0) return *vos_.begin();
    return "";
  };
  return "";
}

std::list<std::string> LegacySecAttr::getAll(const std::string& id) const {
  if(id == "GROUP") return groups_;
  if(id == "VO") return vos_;
  return std::list<std::string>();
}


bool LegacySecAttr::Export(Arc::SecAttrFormat format,Arc::XMLNode &val) const {
  // No need to export information yet.
  return true;
}

bool LegacySecAttr::equal(const SecAttr &b) const {
  try {
    const LegacySecAttr& a = dynamic_cast<const LegacySecAttr&>(b);
    if (!a) return false;
    // ...
    return false;
  } catch(std::exception&) { };
  return false;
}

LegacySecAttr::operator bool(void) const {
  return true;
}


} // namespace ArcSHCLegacy
