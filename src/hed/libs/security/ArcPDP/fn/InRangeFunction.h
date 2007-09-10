#ifndef __ARC_INRANGE_FUNCTION_H__
#define __ARC_INRANGE_FUNCTION_H__

#include "Function.h"

namespace Arc {

#define NAME_TIME_IN_RANGE "know-arc:function:time-in-range"

class InRangeFunction : public Function {
public:
  InRangeFunction(std::string functionName, std::string argumentType);

public:
  virtual bool evaluate(AttributeValue* arg0, AttributeValue* arg1);
   //help function specific for existing policy expression because of no exiplicit function defined in policy
  static std::string getFunctionName(std::string datatype);

private:
  std::string fnName;
  std::string argType;
};

} // namespace Arc

#endif /* __ARC_INRANGE_FUNCTION_H__ */

