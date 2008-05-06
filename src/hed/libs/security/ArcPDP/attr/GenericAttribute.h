#ifndef __ARC_SEC_GENERICATTRIBUTE_H__
#define __ARC_SEC_GENERICATTRIBUTE_H__

#include <arc/security/ArcPDP/attr/AttributeValue.h>

namespace ArcSec {
class GenericAttribute : public AttributeValue {
private:
  std::string value;
  std::string type;
  std::string id;
  static std::string identifier;

public:
  GenericAttribute() { };
  GenericAttribute(const std::string& v,const std::string& i) : value(v), id(i) { };
  virtual ~GenericAttribute() { };

  virtual bool equal(AttributeValue* other);
  virtual std::string encode() { return value; };
  std::string getValue() { return value; };
  virtual std::string getType() { return type; };
  virtual std::string getId() { return id; };
  void setType(const std::string& new_type) { type=new_type; };
  static const std::string& getIdentifier(void) { return identifier; };
 
};

}// namespace ArcSec

#endif /* __ARC_SEC_GENERICATTRIBUTE_H__ */


