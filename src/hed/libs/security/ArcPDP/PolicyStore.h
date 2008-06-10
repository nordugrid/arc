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

  /// Creates policy store with specified combing algorithm (alg - not used yet), policy name 
  /// (policyclassname) and context (ctx)
  PolicyStore(const std::string& alg, const std::string& policyclassname, EvaluatorContext* ctx);

  virtual ~PolicyStore();
  
  virtual std::list<PolicyElement> findPolicy(EvaluationCtx* context);

  virtual void addPolicy(const Source& policy, EvaluatorContext* ctx,const std::string& id);

  virtual void addPolicy(Policy* policyobj, EvaluatorContext* ctx,const std::string& id);

  virtual void removePolicies();

  // std::list<std::string> policysrclist;
private:
  std::list<PolicyElement> policies;
  //std::string combalg;
  
  std::string policy_classname;
};

} // namespace ArcSec

#endif /* __ARC_SEC_POLICYSTORE_H__ */

