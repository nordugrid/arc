#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "LegacySecAttr.h"


namespace ArcSHCLegacy {


LegacySecAttr::LegacySecAttr(Arc::Logger& logger):logger_(logger) {
}

LegacySecAttr::~LegacySecAttr(void) {
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
