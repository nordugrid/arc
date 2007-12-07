#ifndef __ARC_SEC_PERMITOVERRIDESCOMBININGALG_H__
#define __ARC_SEC_PERMITOVERRIDESCOMBININGALG_H__

#include <arc/security/ArcPDP/alg/CombiningAlg.h>
#include <arc/security/ArcPDP/EvaluationCtx.h>

namespace ArcSec {
///Implement the "Permit-Overrides" algorithm
class PermitOverridesCombiningAlg : public CombiningAlg {
private:
  static std::string algId;

public:
  PermitOverridesCombiningAlg(){};
  virtual ~PermitOverridesCombiningAlg(){};

public:
  /**If there is one policy which return positive evaluation result, then omit the other policies and return DECISION_PERMIT */
  virtual Result combine(EvaluationCtx* ctx, std::list<Policy*> policies);
  static const std::string& Identifier(void) { return algId; };
  virtual std::string& getalgId(void){return algId;};
};

} // namespace ArcSec

#endif /* __ARC_SEC_PERMITOVERRIDESCOMBININGALG_H__ */

