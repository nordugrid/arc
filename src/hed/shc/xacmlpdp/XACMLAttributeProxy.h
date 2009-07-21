#ifndef __ARC_SEC_XACMLATTRIBUTEPROXY_H__
#define __ARC_SEC_XACMLATTRIBUTEPROXY_H__

#include <list>
#include <fstream>
#include <arc/XMLNode.h>
#include <arc/Logger.h>
#include <arc/security/ArcPDP/attr/AttributeValue.h>
#include <arc/security/ArcPDP/attr/AttributeProxy.h>

namespace ArcSec {
///XACML specific AttributeProxy class
template <class TheAttribute>
class XACMLAttributeProxy : public AttributeProxy {
public:
  XACMLAttributeProxy(){};
  virtual ~XACMLAttributeProxy(){};
public: 
  virtual AttributeValue* getAttribute(const Arc::XMLNode& node);
};

///Implementation of getAttribute method 
template <class TheAttribute>
AttributeValue* XACMLAttributeProxy<TheAttribute>::getAttribute(const Arc::XMLNode& node){
  Arc::XMLNode x;
  std::string value;
  if((bool)(node.Child())) x=node.Child(0); 
  else x=node;
  value = (std::string)x;
  std::string attrid = (std::string)(node.Attribute("AttributeId"));
  std::size_t start;
  start = value.find_first_not_of(" \n\r\t");
  value = value.substr(start);
  std::size_t end;
  end = value.find_last_not_of(" \n\r\t");
  value = value.substr(0, end+1);
  return new TheAttribute(value, attrid);
}

} // namespace ArcSec

#endif /* __ARC_SEC_XACMLATTRIBUTEPROXY_H__ */

