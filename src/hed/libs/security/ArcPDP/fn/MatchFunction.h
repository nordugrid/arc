#ifndef __ARC_MATCH_FUNCTION_H__
#define __ARC_MATCH_FUNCTION_H__

#include "Function.h"

namespace Arc {

class MatchFunction {
public:
  MatchFunction();
  virtual ~MatchFunction();

public:
  virtual boolean evaluate(){};
};

} // namespace Arc

#endif /* __ARC_MATCH_FUNCTION_H__ */

