#ifndef __ARC_POLICY_H__
#define __ARC_POLICY_H__

#include <list>
#include "../alg/CombiningAlg.h"
#include "XMLNode.h"

namespace Arc {

/**Base class for Policy, PolicySet, or Rule*/

class Policy {

public:
  Policy(){};
  Policy(const XMLNode& node){};  
  virtual ~Policy();

  virtual Result eval(EvalCtx ctx){};

};

} // namespace Arc

#endif /* __ARC_POLICY_H__ */

