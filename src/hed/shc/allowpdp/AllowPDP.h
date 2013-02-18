#ifndef __ARC_SEC_ALLOWPDP_H__
#define __ARC_SEC_ALLOWPDP_H__

#include <stdlib.h>

#include <arc/ArcConfig.h>
#include <arc/security/PDP.h>

namespace ArcSec {

/// This PDP always return true (allow)
class AllowPDP : public PDP {
 public:
  static Arc::Plugin* get_allow_pdp(Arc::PluginArgument *arg);
  AllowPDP(Arc::Config* cfg, Arc::PluginArgument* parg);
  virtual ~AllowPDP() {};
  virtual PDPStatus isPermitted(Arc::Message *msg) const;
};

} // namespace ArcSec

#endif /* __ARC_SEC_ALLOWPDP_H__ */
