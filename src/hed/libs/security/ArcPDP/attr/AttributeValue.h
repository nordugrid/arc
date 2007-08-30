#ifndef __ARC_ATTRIBUTEVALUE_H__
#define __ARC_ATTRIBUTEVALUE_H__

namespace Arc {

class AttributeValue {
public:
  AttributeValue();
  virtual ~AttributeValue();

public:
  bool equal(const AttributeValue* other){};
  int compare(const AttributeValue* other){};
};

} // namespace Arc

#endif /* __ARC_ATTRIBUTEVALUE_H__ */

