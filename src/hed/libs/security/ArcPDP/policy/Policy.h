#ifndef __ARC_POLICY_H__
#define __ARC_POLICY_H__

#include <list>
#include "common/XMLNode.h"
#include "common/Logger.h"
#include "../Result.h"
#include "../EvaluationCtx.h"


namespace Arc {

/**Base class for Policy, PolicySet, or Rule*/

class Policy {

protected:
  std::list<Policy*> subelements;
  static Logger logger; 
 
public:
  Policy(XMLNode& node){};  
  virtual ~Policy(){};
 
  virtual MatchResult match(EvaluationCtx* ctx){};

  virtual Result eval(EvaluationCtx* ctx){};

  virtual void addPolicy(Policy* pl){subelements.push_back(pl);};

  virtual std::string getEffect(){};

};

} // namespace Arc

#endif /* __ARC_POLICY_H__ */

