#ifndef __ARC_SEC_ATTRIBUTEVALUE_H__
#define __ARC_SEC_ATTRIBUTEVALUE_H__

#include <string>

namespace ArcSec {
///Interface for different type of <Attribute> for both policy and request
/**<Attribute> uses different "Type" definition; Each type of <Attribute> needs 
different approach to compare. The "Type" supported so far is: StringAttribute, 
DateAttribute, TimeAttribute, DurationAttribute, PeriodAttribute, AnyURIAttribute, 
X500NameAttribute */
class AttributeValue {
public:
  AttributeValue(){};
  virtual ~AttributeValue(){};

  /**evluate whether "this" equale to the parameter value */
  virtual bool equal(AttributeValue* value) = 0;

  //virtual int compare(AttributeValue* other){};

  /**encode the value in a string format*/
  virtual std::string encode() = 0;

  /**get the type of the <Attribute>*/
  virtual std::string getType() = 0;

  /**get the id of the <Attribute>*/
  virtual std::string getId() = 0;

};

} // namespace ArcSec

#endif /* __ARC_SEC_ATTRIBUTEVALUE_H__ */

