#ifndef __ARC_SEC_PERMITOVERRIDESCOMBININGALG_H__
#define __ARC_SEC_PERMITOVERRIDESCOMBININGALG_H__

#include <arc/security/ArcPDP/alg/CombiningAlg.h>
#include <arc/security/ArcPDP/EvaluationCtx.h>

namespace ArcSec {

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

} // namespace ArcSec

#endif /* __ARC_SEC_PERMITOVERRIDESCOMBININGALG_H__ */

