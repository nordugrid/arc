#ifndef __ARC_POLICY_H__
#define __ARC_POLICY_H__

#include <list>
#include "../alg/CombiningAlg.h"
#include "XMLNode.h"

namespace Arc {

/**Base class for Policy, PolicySet, or Rule*/

class Policy {

protected:
  std::list<Policy*> subelements;

public:
  Policy(){};
  Policy(const XMLNode& node){};  
  virtual ~Policy();
 
  vitual MatchResult match(const Evaluation* ctx){};

  virtual Result eval(const EvaluationCtx* ctx){};

  virtual void addPolicy(Policy* pl){subelements.push_back(pl);};

  //Distinguish ArcRule and ArcPolicy
  virtual boolean hasDirectEffect() const {return hasDirectEffect };

protected:
  boolean hasDirectEffect = false;
};

} // namespace Arc

#endif /* __ARC_POLICY_H__ */

