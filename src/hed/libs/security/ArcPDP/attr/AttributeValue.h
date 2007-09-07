#ifndef __ARC_ATTRIBUTEVALUE_H__
#define __ARC_ATTRIBUTEVALUE_H__

#include <string>

namespace Arc {

class AttributeValue {
public:
  AttributeValue(){};
  virtual ~AttributeValue(){};

public:
  virtual bool equal(AttributeValue* other){};
  //virtual int compare(AttributeValue* other){};
  //encode the value in a string format, which is suitable for printing
  virtual std::string encode(){};
};

} // namespace Arc

#endif /* __ARC_ATTRIBUTEVALUE_H__ */

