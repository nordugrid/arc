#ifndef __ARC_COMBININGALG_H__
#define __ARC_COMBININGALG_H__

#include <string>
#include <list>
#include "../EvaluationCtx.h"
#include "../policy/Policy.h"

namespace Arc {

class CombiningAlg {
public:
  CombiningAlg(){};
  virtual ~CombiningAlg(){};

public:
  virtual Result combine(EvaluationCtx* ctx, std::list<Policy*> policies){};
  virtual std::string& getalgId(void){};
};

} // namespace Arc

#endif /* __ARC_COMBININGALG_H__ */

