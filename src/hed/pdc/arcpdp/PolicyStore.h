#ifndef __ARC_SEC_POLICYSTORE_H__
#define __ARC_SEC_POLICYSTORE_H__

#include <arc/loader/LoadableClass.h>
#include <list>
#include <arc/security/ArcPDP/policy/Policy.h>

namespace ArcSec {

class EvaluatorContext;

///Storage place for policy objects, no dynamically loadable now
class PolicyStore : public Arc::LoadableClass {

public:
  PolicyStore();

  //plist is the list of policy sources, alg is the combing algorithm between these sources
  PolicyStore(const std::list<std::string>& plist, const std::string& alg, EvaluatorContext* ctx);

  virtual ~PolicyStore();
  
  virtual std::list<Policy*> findPolicy(EvaluationCtx* context);

  virtual void addPolicy(std::string& policyfile, EvaluatorContext* ctx);

private:
  std::list<std::string> policysrclist;
  std::list<Policy*> policies;
  std::string combalg;
 
};

} // namespace ArcSec

#endif /* __ARC_SEC_POLICYSTORE_H__ */

