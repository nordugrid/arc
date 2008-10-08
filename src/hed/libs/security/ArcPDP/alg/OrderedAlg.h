#ifndef __ARC_SEC_ORDEREDCOMBININGALG_H__
#define __ARC_SEC_ORDEREDCOMBININGALG_H__

#include <arc/security/ArcPDP/alg/CombiningAlg.h>
#include <arc/security/ArcPDP/EvaluationCtx.h>

namespace ArcSec {

#define MAX_OREDERED_PRIORITIES 4

class OrderedCombiningAlg : public CombiningAlg {
public:
  OrderedCombiningAlg() {};
  virtual ~OrderedCombiningAlg() {};
protected:
  Result combine(EvaluationCtx* ctx, std::list<Policy*> policies,const Result priorities[MAX_OREDERED_PRIORITIES]);
};

#define ORDERED_ALG_CLASS(NAME) \
class NAME: public OrderedCombiningAlg { \
private: \
  static std::string algId; \
  static Result priorities[MAX_OREDERED_PRIORITIES]; \
public: \
  NAME(void) {}; \
  virtual ~NAME(void) {}; \
  virtual const std::string& getalgId(void) const { return algId; }; \
  virtual Result combine(EvaluationCtx* ctx, std::list<Policy*> policies) { \
    return OrderedCombiningAlg::combine(ctx,policies,priorities); \
  }; \
}

ORDERED_ALG_CLASS(PermitDenyIndeterminateNotApplicableCombiningAlg);
ORDERED_ALG_CLASS(PermitDenyNotApplicableIndeterminateCombiningAlg);
ORDERED_ALG_CLASS(PermitIndeterminateDenyNotApplicableCombiningAlg);
ORDERED_ALG_CLASS(PermitIndeterminateNotApplicableDenyCombiningAlg);
ORDERED_ALG_CLASS(PermitNotApplicableDenyIndeterminateCombiningAlg);
ORDERED_ALG_CLASS(PermitNotApplicableIndeterminateDenyCombiningAlg);
ORDERED_ALG_CLASS(DenyPermitIndeterminateNotApplicableCombiningAlg);
ORDERED_ALG_CLASS(DenyPermitNotApplicableIndeterminateCombiningAlg);
ORDERED_ALG_CLASS(DenyIndeterminatePermitNotApplicableCombiningAlg);
ORDERED_ALG_CLASS(DenyIndeterminateNotApplicablePermitCombiningAlg);
ORDERED_ALG_CLASS(DenyNotApplicablePermitIndeterminateCombiningAlg);
ORDERED_ALG_CLASS(DenyNotApplicableIndeterminatePermitCombiningAlg);
ORDERED_ALG_CLASS(IndeterminatePermitDenyNotApplicableCombiningAlg);
ORDERED_ALG_CLASS(IndeterminatePermitNotApplicableDenyCombiningAlg);
ORDERED_ALG_CLASS(IndeterminateDenyPermitNotApplicableCombiningAlg);
ORDERED_ALG_CLASS(IndeterminateDenyNotApplicablePermitCombiningAlg);
ORDERED_ALG_CLASS(IndeterminateNotApplicablePermitDenyCombiningAlg);
ORDERED_ALG_CLASS(IndeterminateNotApplicableDenyPermitCombiningAlg);
ORDERED_ALG_CLASS(NotApplicablePermitDenyIndeterminateCombiningAlg);
ORDERED_ALG_CLASS(NotApplicablePermitIndeterminateDenyCombiningAlg);
ORDERED_ALG_CLASS(NotApplicableDenyPermitIndeterminateCombiningAlg);
ORDERED_ALG_CLASS(NotApplicableDenyIndeterminatePermitCombiningAlg);
ORDERED_ALG_CLASS(NotApplicableIndeterminatePermitDenyCombiningAlg);
ORDERED_ALG_CLASS(NotApplicableIndeterminateDenyPermitCombiningAlg);

} // namespace ArcSec

#endif /* __ARC_SEC_ORDEREDCOMBININGALG_H__ */

