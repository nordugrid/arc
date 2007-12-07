#ifndef __ARC_SEC_FUNCTION_H__
#define __ARC_SEC_FUNCTION_H__

#include <string>
#include <arc/security/ArcPDP/attr/AttributeValue.h>

namespace ArcSec {

//static std::string FUNCTION_NS = "know-arc:function";
#define FUNCTION_NS "know-arc:function"

///Interface for function, which is in charge of evaluating two AttributeValue
class Function {
public:
  Function(std::string, std::string){};
  virtual ~Function(){};

public:
  virtual bool evaluate(AttributeValue* arg0, AttributeValue* arg1) = 0;

};

} // namespace ArcSec

#endif /* __ARC_SEC_FUNCTION_H__ */

