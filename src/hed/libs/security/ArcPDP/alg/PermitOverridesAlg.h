#ifndef __ARC_PERMITOVERRIDESCOMBININGALG_H__
#define __ARC_PERMITOVERRIDESCOMBININGALG_H__

#include "CombiningAlg.h"
#include "../EvaluationCtx.h"

namespace Arc {

class PermitOverridesCombiningAlg : public CombiningAlg {
public:
  PermitOverridesCombiningAlg(){algId = "Permit-Overrides";};
  virtual ~PermitOverridesCombiningAlg(){};

public:
  virtual Result combine(EvaluationCtx* ctx, std::list<Policy*> policies);
};

} // namespace Arc

#endif /* __ARC_PERMITOVERRIDESCOMBININGALG_H__ */

