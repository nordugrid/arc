#ifndef __ARC_DENYOVERRIDESCOMBININGALG_H__
#define __ARC_DENYOVERRIDESCOMBININGALG_H__

#include "CombiningAlg.h"
#include "../EvaluationCtx.h"

namespace Arc {

class DenyOverridesCombiningAlg : public CombiningAlg {
private:
  static std::string algId;
public:
  DenyOverridesCombiningAlg(){};
  virtual ~DenyOverridesCombiningAlg(){};

public:
  virtual Result combine(EvaluationCtx* ctx, std::list<Policy*> policies);
  static const std::string& Identifier(void) { return algId; };
  virtual std::string& getalgId(void){return algId;};
};

} // namespace Arc

#endif /* __ARC_DENYOVERRIDESCOMBININGALG_H__ */

