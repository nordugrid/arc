#ifndef __ARC_SEC_ATTRIBUTEPROXY_H__
#define __ARC_SEC_ATTRIBUTEPROXY_H__

#include <list>
#include <fstream>
#include <arc/XMLNode.h>
#include <arc/Logger.h>
#include "AttributeValue.h"

namespace ArcSec {

///Interface for generating the AttributeValue object, it will be used by AttributeFactory
/**the AttributeProxy object will be insert into AttributeFactoty; and the 
getAttribute(node) method will be called inside AttributeFacroty.createvalue(node), 
in order to generate a specific AttributeValue 
*/ 
class AttributeProxy {
public:
  AttributeProxy() {};
  virtual ~AttributeProxy(){};
public:
  virtual AttributeValue* getAttribute(const Arc::XMLNode& node) = 0;
};

///Arc specific AttributeProxy class
template <class TheAttribute>
class ArcAttributeProxy : public AttributeProxy {
public:
  ArcAttributeProxy(){};
  virtual ~ArcAttributeProxy(){};
public: 
  virtual AttributeValue* getAttribute(const Arc::XMLNode& node);
};

///Implementation of getAttribute 
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

#endif /* __ARC_SEC_ATTRIBUTEPROXY_H__ */

