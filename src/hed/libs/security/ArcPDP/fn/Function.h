#ifndef __ARC_FUNCTION_H__
#define __ARC_FUNCTION_H__

#include "../attr/AttributeValue.h"

namespace Arc {

static std::string FUNCTION_NS = "know-arc:function";

class Function {
public:
  virtual ~Function();

public:
  virtual bool evaluate(AttributeValue* arg0, AttributeValue* arg1){};

};

} // namespace Arc

#endif /* __ARC_FUNCTION_H__ */

