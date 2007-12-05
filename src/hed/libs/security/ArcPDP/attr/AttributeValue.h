#ifndef __ARC_SEC_ATTRIBUTEVALUE_H__
#define __ARC_SEC_ATTRIBUTEVALUE_H__

#include <string>

namespace ArcSec {

///Interface for different type of Attribute, e.g. StringAttribute
/**<Attribute> uses different "Type" definition; Each type of <Attribute> will have different approach to compare */
class AttributeValue {
public:
  AttributeValue(){};
  virtual ~AttributeValue(){};

public:
  /**evluate whether "this" equale to the parameter value */
  virtual bool equal(AttributeValue* value) = 0;

  //virtual int compare(AttributeValue* other){};

  /**encode the value in a string format*/
  virtual std::string encode() = 0;

  /**get the type of the <Attribute>*/
  virtual std::string getType() = 0;

};

} // namespace ArcSec

#endif /* __ARC_SEC_ATTRIBUTEVALUE_H__ */

