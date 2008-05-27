#ifndef __ARC_SEC_DENYOVERRIDESCOMBININGALG_H__
#define __ARC_SEC_DENYOVERRIDESCOMBININGALG_H__

#include <arc/security/ArcPDP/alg/CombiningAlg.h>
#include <arc/security/ArcPDP/EvaluationCtx.h>

namespace ArcSec {
///Implement the "Deny-Overrides" algorithm
class DenyOverridesCombiningAlg : public CombiningAlg {
private:
  static std::string algId;
public:
  DenyOverridesCombiningAlg(){};
  virtual ~DenyOverridesCombiningAlg(){};

public:
  /**If there is one policy which return negative evaluation result, then omit the other policies and return DECISION_DENY */
  virtual Result combine(EvaluationCtx* ctx, std::list<BasePolicy*> policies);
  static const std::string& Identifier(void) { return algId; };
  virtual std::string& getalgId(void){return algId;};
};

} // namespace ArcSec

#endif /* __ARC_SEC_DENYOVERRIDESCOMBININGALG_H__ */

