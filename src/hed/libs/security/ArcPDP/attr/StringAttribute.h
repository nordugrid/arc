#ifndef __ARC_SEC_STRINGATTRIBUTE_H__
#define __ARC_SEC_STRINGATTRIBUTE_H__

#include <arc/security/ArcPDP/attr/AttributeValue.h>

namespace ArcSec {
class StringAttribute : public AttributeValue {
private:
  static std::string identifier;
  std::string value;
  std::string id;

public:
  StringAttribute(){ };
  StringAttribute(const std::string& v,const std::string& i) : value(v), id(i){ };
  virtual ~StringAttribute(){ };

  virtual bool equal(AttributeValue* other);
  virtual bool inrange(AttributeValue* other);
  virtual std::string encode() {return value;};
  std::string getValue(){ return value; };
  virtual std::string getType() {return identifier; };
  virtual std::string getId() {return id;};
  static const std::string& getIdentifier(void) { return identifier; };
 
};

}// namespace ArcSec

#endif /* __ARC_SEC_STRINGATTRIBUTE_H__ */


