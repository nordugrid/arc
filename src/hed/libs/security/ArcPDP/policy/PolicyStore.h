#ifndef __ARC_POLICYSTORE_H__
#define __ARC_POLICYSTORE_H__

#include <arc/loader/LoadableClass.h>
#include <list>
#include "Policy.h"

namespace Arc {

class EvaluatorContext;

/**Storage place for policy objects */

class PolicyStore : public LoadableClass {

public:
  PolicyStore(){};

  //plist is the list of policy sources, alg is the combing algorithm between these sources
  PolicyStore(const std::list<std::string>& plist, const std::string& alg, EvaluatorContext* ctx);
  
  virtual std::list<Arc::Policy*> findPolicy(EvaluationCtx* context);

private:
  std::list<std::string> policysrclist;
  std::list<Arc::Policy*> policies;
  std::string combalg;
 
};

} // namespace Arc

#endif /* __ARC_POLICYSTORE_H__ */

