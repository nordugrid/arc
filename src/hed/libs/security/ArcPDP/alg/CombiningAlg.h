#ifndef __ARC_SEC_COMBININGALG_H__
#define __ARC_SEC_COMBININGALG_H__

#include <string>
#include <list>
#include "../EvaluationCtx.h"
#include "../policy/BasePolicy.h"

namespace ArcSec {
///Interface for combining algrithm
class CombiningAlg {
public:
  CombiningAlg(){};
  virtual ~CombiningAlg(){};

public:
  /**Evaluate request against policy, and if there are more than one policies, combine the evaluation results according to the 
  combing algorithm implemented inside in the method combine(ctx, policies) itself.
  @param ctx  The information about request is included
  @param policies  The "match" and "eval" method inside policy will be called
  */
  virtual Result combine(EvaluationCtx* ctx, std::list<BasePolicy*> policies) = 0;
  virtual std::string& getalgId(void) = 0;
};

} // namespace ArcSec

#endif /* __ARC_SEC_COMBININGALG_H__ */

