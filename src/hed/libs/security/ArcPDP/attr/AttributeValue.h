#ifndef __ARC_SEC_ATTRIBUTEVALUE_H__
#define __ARC_SEC_ATTRIBUTEVALUE_H__

#include <string>

namespace ArcSec {
///Interface for containing different type of <Attribute/> node for both policy and request
/**<Attribute/> contains different "Type" definition; Each type of <Attribute/> needs
 *different approach to compare the value.
 *Any specific class which is for processing specific "Type" shoud inherit this class. 
 *The "Type" supported so far is: StringAttribute,
 *DateAttribute, TimeAttribute, DurationAttribute, PeriodAttribute, AnyURIAttribute,
 *X500NameAttribute 
 */
class AttributeValue {
public:
  AttributeValue(){};
  virtual ~AttributeValue(){};

  /**Evluate whether "this" equale to the parameter value */
  virtual bool equal(AttributeValue* value) = 0;

  //virtual int compare(AttributeValue* other){};

  /**encode the value in a string format*/
  virtual std::string encode() = 0;

  /**Get the type of the <Attribute>*/
  virtual std::string getType() = 0;

  /**Get the identifier of the <Attribute>*/
  virtual std::string getId() = 0;

};

} // namespace ArcSec

#endif /* __ARC_SEC_ATTRIBUTEVALUE_H__ */

