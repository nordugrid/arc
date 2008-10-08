#ifndef __ARC_SEC_COMBININGALG_H__
#define __ARC_SEC_COMBININGALG_H__

#include <string>
#include <list>
#include "../EvaluationCtx.h"
#include "../policy/Policy.h"

namespace ArcSec {
///Interface for combining algrithm
/**This class is used to implement a specific combining algorithm for 
 * combining policies.
 */
class CombiningAlg {
public:
  CombiningAlg(){};
  virtual ~CombiningAlg(){};

public:
 /**Evaluate request against policy, and if there are more than one policies, combine 
 * the evaluation results according to the combing algorithm implemented inside in the 
 * method combine(ctx, policies) itself.
 *@param ctx  The information about request is included
 *@param policies  The "match" and "eval" method inside each policy will be called,
 * and then those results from each policy will be combined according to the combining
 * algorithm inside CombingAlg class.
 */
  virtual Result combine(EvaluationCtx* ctx, std::list<Policy*> policies) = 0;

 /**Get the identifier of the combining algorithm class
 *@return The identity of the algorithm
 */
  virtual const std::string& getalgId(void) const = 0;
};

} // namespace ArcSec

#endif /* __ARC_SEC_COMBININGALG_H__ */

