#ifndef __ARC_ANYURIATTRIBUTE_H__
#define __ARC_ANYURIATTRIBUTE_H__

#include "AttributeValue.h"

namespace Arc {
class AnyURIAttribute : public AttributeValue {
private:
  static std::string identifier;
  std::string value;

public:
  AnyURIAttribute(){ };
  AnyURIAttribute(std::string v) : value(v){ };
  virtual ~AnyURIAttribute(){ };

  virtual bool equal(AttributeValue* other);
  virtual std::string encode() {return value;};
  std::string getValue(){ return value; };
  static const std::string& getIdentifier(void) { return identifier; };
 
};

}// namespace Arc

#endif /* __ARC_AnyURIATTRIBUTE_H__ */


