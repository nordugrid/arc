#ifndef __ARC_SEC_DELEGATIONPDP_H__
#define __ARC_SEC_DELEGATIONPDP_H__

#include <stdlib.h>

#include <arc/ArcConfig.h>
#include <arc/security/ArcPDP/Evaluator.h>
#include <arc/security/PDP.h>

namespace ArcSec {

///DeleagtionPDP - PDP which can handle the Arc specific request and policy
/// provided as identity delegation policy.
class DelegationPDP : public PDP {
 public:
  static Arc::Plugin* get_delegation_pdp(Arc::PluginArgument *arg);
  DelegationPDP(Arc::Config* cfg);
  virtual ~DelegationPDP();
  virtual bool isPermitted(Arc::Message *msg);
 private:
  std::list<std::string> select_attrs;
  std::list<std::string> reject_attrs;
 protected:
  static Arc::Logger logger;
};

} // namespace ArcSec

#endif /* __ARC_SEC_DELEGATIONPDP_H__ */

