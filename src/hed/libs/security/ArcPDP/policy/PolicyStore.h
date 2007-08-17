#ifndef __ARC_POLICYSTORE_H__
#define __ARC_POLICYSTORE_H__

#include <list>
#include "XMLNode.h"
#include "Policy.h"

namespace Arc {

/**Storage place for policy objects */

class PolicyStore {

public:
  PolicyStore(){};

  //plist is the list of policy sources, alg is the combing algorithm between these sources
  PolicyStore(const std::list<std::string>& plist, const std::string& alg);
  
  virtual Policy* findPolicy(EvaluationCtx context);

private:
  std::list<std::string> policysrclist;
  std::list<Policy*> policies;
  std::string combalg;
 
};

} // namespace Arc

#endif /* __ARC_POLICYSTORE_H__ */

