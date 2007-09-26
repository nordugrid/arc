#include <istream>
#include <string>
#include <arc/XMLNode.h>

namespace ARex {

bool LDIFtoXML(std::istream& ldif,const std::string& ldif_base,Arc::XMLNode xml);

} // namespace ARex

