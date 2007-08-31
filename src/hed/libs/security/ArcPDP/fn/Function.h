#ifndef __ARC_FUNCTION_H__
#define __ARC_FUNCTION_H__

#include <string>
#include "../attr/AttributeValue.h"

namespace Arc {

#define FUNCTION_NS "know-arc:function"

class Function {
public:
  Function(){};
  virtual ~Function();

public:
  virtual bool evaluate(AttributeValue* arg0, AttributeValue* arg1){};

};

} // namespace Arc

#endif /* __ARC_FUNCTION_H__ */

