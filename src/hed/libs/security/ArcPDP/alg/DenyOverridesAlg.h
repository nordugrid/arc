#ifndef __ARC_DENYOVERRIDESCOMBININGALG_H__
#define __ARC_DENYOVERRIDESCOMBININGALG_H__

#include "CombiningAlg.h"
#include "../EvaluationCtx.h"

namespace Arc {

class DenyOverridesCombiningAlg : public CombiningAlg {
public:
  DenyOverridesCombiningAlg(){algId = "Deny-Overrides";};
  virtual ~DenyOverridesCombiningAlg(){};

public:
  virtual Result combine(EvaluationCtx* ctx, std::list<Policy*> policies);
};

} // namespace Arc

#endif /* __ARC_DENYOVERRIDESCOMBININGALG_H__ */

