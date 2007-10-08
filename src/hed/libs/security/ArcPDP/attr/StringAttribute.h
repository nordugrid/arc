#ifndef __ARC_SEC_STRINGATTRIBUTE_H__
#define __ARC_SEC_STRINGATTRIBUTE_H__

#include <arc/security/ArcPDP/attr/AttributeValue.h>

namespace ArcSec {
class StringAttribute : public AttributeValue {
private:
  static std::string identifier;
  std::string value;

public:
  StringAttribute(){ };
  StringAttribute(std::string v) : value(v){ };
  virtual ~StringAttribute(){ };

  virtual bool equal(AttributeValue* other);
  virtual std::string encode() {return value;};
  std::string getValue(){ return value; };
  static const std::string& getIdentifier(void) { return identifier; };
 
};

}// namespace ArcSec

#endif /* __ARC_SEC_STRINGATTRIBUTE_H__ */


