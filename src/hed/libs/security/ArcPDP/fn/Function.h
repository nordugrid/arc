#ifndef __ARC_SEC_FUNCTION_H__
#define __ARC_SEC_FUNCTION_H__

#include <list>
#include <string>
#include <arc/security/ArcPDP/attr/AttributeValue.h>

namespace ArcSec {

//static std::string FUNCTION_NS = "know-arc:function";
//#define FUNCTION_NS "know-arc:function"

///Interface for function, which is in charge of evaluating two AttributeValue
class Function {
public:
  Function(std::string, std::string){};
  virtual ~Function(){};

public:
  /**Evaluate two AttributeValue objects, and return one AttributeValue object */
  virtual AttributeValue* evaluate(AttributeValue* arg0, AttributeValue* arg1) = 0;
  /**Evaluate a list of AttributeValue objects, and return a list of Attribute objects*/
  virtual std::list<AttributeValue*> evaluate(std::list<AttributeValue*> args) = 0;
};

} // namespace ArcSec

#endif /* __ARC_SEC_FUNCTION_H__ */

