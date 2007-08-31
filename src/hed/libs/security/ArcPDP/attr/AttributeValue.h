#ifndef __ARC_ATTRIBUTEVALUE_H__
#define __ARC_ATTRIBUTEVALUE_H__

namespace Arc {

class AttributeValue {
public:
  AttributeValue(){};
  virtual ~AttributeValue(){};

public:
  virtual bool equal(AttributeValue* other){};
  //virtual int compare(AttributeValue* other){};
};

} // namespace Arc

#endif /* __ARC_ATTRIBUTEVALUE_H__ */

