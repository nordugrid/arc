#ifndef __ARC_STRINGATTRIBUTE_H__
#define __ARC_STRINGATTRIBUTE_H__

#include <arc/security/ArcPDP/attr/AttributeValue.h>

namespace Arc {
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

}// namespace Arc

#endif /* __ARC_STRINGATTRIBUTE_H__ */


