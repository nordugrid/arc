#ifndef __ARC_PERMITOVERRIDESCOMBININGALG_H__
#define __ARC_PERMITOVERRIDESCOMBININGALG_H__

#include "CombiningAlg.h"
#include "../EvaluationCtx.h"

namespace Arc {

class PermitOverridesCombiningAlg : public CombiningAlg {
private:
  static std::string algId;

public:
  PermitOverridesCombiningAlg(){};
  virtual ~PermitOverridesCombiningAlg(){};

public:
  virtual Result combine(EvaluationCtx* ctx, std::list<Policy*> policies);
  static const std::string& Identifier(void) { return algId; };
  virtual std::string& getalgId(void){return algId;};
};

} // namespace Arc

#endif /* __ARC_PERMITOVERRIDESCOMBININGALG_H__ */

