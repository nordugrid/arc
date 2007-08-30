#ifndef __ARC_ATTRIBUTEPROXY_H__
#define __ARC_ATTRIBUTEPROXY_H__

#include <list>
#include <fstream>
#include "common/XMLNode.h"
#include "common/Logger.h"
#include "AttributeValue.h"

namespace Arc {

class AttributeProxy {
public:
  AttributeProxy() {};
  virtual ~AttributeProxy(){};
public:
  virtual AttributeValue* getAttribute(const XMLNode& node) = 0;
};

template <class TheAttribute>
class ArcAttributeProxy : public AttributeProxy {
public:
  ArcAttributeProxy(){};
  virtual ~ArcAttributeProxy(){};
public: 
  virtual AttributeValue* getAttribute(const XMLNode& node);
};

template <class TheAttribute>
AttributeValue* ArcAttributeProxy<TheAttribute>::getAttribute(const XMLNode& node){
  std::string value = (std::string)node;
  return new TheAttribute(value);
}

} // namespace Arc

#endif /* __ARC_ATTRIBUTEPROXY_H__ */

