#ifndef __ARC_FUNCTION_H__
#define __ARC_FUNCTION_H__

#include <string>
#include <arc/security/ArcPDP/attr/AttributeValue.h>

namespace Arc {

//static std::string FUNCTION_NS = "know-arc:function";
#define FUNCTION_NS "know-arc:function"

class Function {
public:
  Function(std::string, std::string){};
  virtual ~Function(){};

public:
  virtual bool evaluate(AttributeValue* arg0, AttributeValue* arg1) = 0;

};

} // namespace Arc

#endif /* __ARC_FUNCTION_H__ */

