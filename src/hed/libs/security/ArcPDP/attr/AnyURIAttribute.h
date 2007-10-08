#ifndef __ARC_SEC_ANYURIATTRIBUTE_H__
#define __ARC_SEC_ANYURIATTRIBUTE_H__

#include <arc/security/ArcPDP/attr/AttributeValue.h>

namespace ArcSec {
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

}// namespace ArcSec

#endif /* __ARC_SEC_ANYURIATTRIBUTE_H__ */


