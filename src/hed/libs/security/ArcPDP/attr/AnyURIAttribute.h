#ifndef __ARC_SEC_ANYURIATTRIBUTE_H__
#define __ARC_SEC_ANYURIATTRIBUTE_H__

#include <arc/security/ArcPDP/attr/AttributeValue.h>

namespace ArcSec {
class AnyURIAttribute : public AttributeValue {
private:
  static std::string identifier;
  std::string value;
  std::string id;
  
public:
  AnyURIAttribute() { };
  AnyURIAttribute(const std::string& v,const std::string& i) : value(v), id(i){ };
  virtual ~AnyURIAttribute(){ };

  virtual bool equal(AttributeValue* other);
  virtual std::string encode() {return value;};
  std::string getValue(){ return value; };
  std::string getId(){ return id; };
  virtual std::string getType() {return identifier; };
  static const std::string& getIdentifier(void) { return identifier; };
 
};

}// namespace ArcSec

#endif /* __ARC_SEC_ANYURIATTRIBUTE_H__ */


