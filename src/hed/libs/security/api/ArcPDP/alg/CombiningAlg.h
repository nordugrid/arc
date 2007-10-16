#ifndef __ARC_SEC_COMBININGALG_H__
#define __ARC_SEC_COMBININGALG_H__

#include <string>
#include <list>
#include "../EvaluationCtx.h"
#include "../policy/Policy.h"

namespace ArcSec {

class CombiningAlg {
public:
  CombiningAlg(){};
  virtual ~CombiningAlg(){};

public:
  virtual Result combine(EvaluationCtx* ctx, std::list<Policy*> policies) = 0;
  virtual std::string& getalgId(void) = 0;
};

} // namespace ArcSec

#endif /* __ARC_SEC_COMBININGALG_H__ */

