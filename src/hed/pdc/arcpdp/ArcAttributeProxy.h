#ifndef __ARC_SEC_ARCATTRIBUTEPROXY_H__
#define __ARC_SEC_ARCATTRIBUTEPROXY_H__

#include <list>
#include <fstream>
#include <arc/XMLNode.h>
#include <arc/Logger.h>
#include <arc/security/ArcPDP/attr/AttributeValue.h>
#include <arc/security/ArcPDP/attr/AttributeProxy.h>

namespace ArcSec {
///Arc specific AttributeProxy class
template <class TheAttribute>
class ArcAttributeProxy : public AttributeProxy {
public:
  ArcAttributeProxy(){};
  virtual ~ArcAttributeProxy(){};
public: 
  virtual AttributeValue* getAttribute(const Arc::XMLNode& node);
};

///Implementation of getAttribute method 
template <class TheAttribute>
AttributeValue* ArcAttributeProxy<TheAttribute>::getAttribute(const Arc::XMLNode& node){
  Arc::XMLNode x = node;
  std::string value = (std::string)x;
  if(value.empty()) x=x.Child(0); // ???
  value = (std::string)x;
  std::string attrid = (std::string)(x.Attribute("AttributeId"));
  if(attrid.empty())
    attrid = (std::string)(x.Attribute("Id"));
  return new TheAttribute(value, attrid);
}

} // namespace ArcSec

#endif /* __ARC_SEC_ARCATTRIBUTEPROXY_H__ */

