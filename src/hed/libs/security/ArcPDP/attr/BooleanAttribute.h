#ifndef __ARC_SEC_BOOLEANATTRIBUTE_H__
#define __ARC_SEC_BOOlEANATTRIBUTE_H__

#include <arc/security/ArcPDP/attr/AttributeValue.h>

namespace ArcSec {
class BooleanAttribute : public AttributeValue {
private:
  static std::string identifier;
  bool value;
  std::string id;
  
public:
  BooleanAttribute() { };
  BooleanAttribute(const bool& v,const std::string& i = std::string()) : value(v), id(i){ };
  virtual ~BooleanAttribute(){ };

  virtual bool equal(AttributeValue* o);
  virtual std::string encode() { if(value) return std::string("true"); else return std::string("false"); };
  bool getValue(){ return value; };
  std::string getId(){ return id; };
  std::string getType() {return identifier; };
  static const std::string& getIdentifier(void) { return identifier; };
 
};

}// namespace ArcSec

#endif /* __ARC_SEC_BOOLEANATTRIBUTE_H__ */


