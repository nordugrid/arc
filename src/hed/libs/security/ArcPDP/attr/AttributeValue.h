#ifndef __ARC_SEC_ATTRIBUTEVALUE_H__
#define __ARC_SEC_ATTRIBUTEVALUE_H__

#include <string>

namespace ArcSec {

class AttributeValue {
public:
  AttributeValue(){};
  virtual ~AttributeValue(){};

public:
  virtual bool equal(AttributeValue* value) = 0;
  //virtual int compare(AttributeValue* other){};
  //encode the value in a string format, which is suitable for printing
  virtual std::string encode() = 0;
  virtual std::string getType() = 0;

};

} // namespace ArcSec

#endif /* __ARC_SEC_ATTRIBUTEVALUE_H__ */

