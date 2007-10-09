#ifndef __ARC_SEC_POLICY_H__
#define __ARC_SEC_POLICY_H__

#include <list>
#include <arc/XMLNode.h>
#include <arc/Logger.h>
#include "../Result.h"
#include "../EvaluationCtx.h"


namespace ArcSec {

/**Base class for Policy, PolicySet, or Rule*/

class Policy {

protected:
  std::list<Policy*> subelements;
  static Arc::Logger logger; 
 
public:
  Policy(Arc::XMLNode&){};  
  virtual ~Policy(){};
 
  virtual MatchResult match(EvaluationCtx* ctx) = 0;

  virtual Result eval(EvaluationCtx* ctx) = 0;

  virtual void addPolicy(Policy* pl){subelements.push_back(pl);};

  virtual std::string getEffect() = 0;

  virtual EvalResult& getEvalResult() = 0;

};

} // namespace ArcSec

#endif /* __ARC_SEC_POLICY_H__ */

