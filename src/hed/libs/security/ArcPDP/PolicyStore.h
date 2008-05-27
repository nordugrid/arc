#ifndef __ARC_SEC_POLICYSTORE_H__
#define __ARC_SEC_POLICYSTORE_H__

#include <list>
#include <arc/security/ArcPDP/policy/Policy.h>

namespace ArcSec {

class EvaluatorContext;

///Storage place for policy objects
class PolicyStore {

public:

  class PolicyElement {
  private:
    Policy* policy;
    std::string id;
  public:
    PolicyElement(Policy* policy_):policy(policy_) { };
    PolicyElement(Policy* policy_, const std::string& id_):policy(policy_),id(id_) { };
    operator Policy*(void) const { return policy; };
    const std::string& Id(void) const { return id; };
  };

  PolicyStore();

  //plist is the list of policy sources, alg is the combing algorithm between these sources
  PolicyStore(const std::list<std::string>& plist, const std::string& alg, const std::string& policyclassname, EvaluatorContext* ctx);

  virtual ~PolicyStore();
  
  virtual std::list<PolicyElement> findPolicy(EvaluationCtx* context);

  virtual void addPolicy(const std::string& policyfile, EvaluatorContext* ctx,const std::string& id);

  virtual void addPolicy(BasePolicy* policy, EvaluatorContext* ctx,const std::string& id);

  virtual void removePolicies();

  // std::list<std::string> policysrclist;
private:
  std::list<PolicyElement> policies;
  //std::string combalg;
  
  std::string policy_classname;
};

} // namespace ArcSec

#endif /* __ARC_SEC_POLICYSTORE_H__ */

