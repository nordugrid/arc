#ifndef __ARC_SEC_DELEGATIONSH_H__
#define __ARC_SEC_DELEGATIONSH_H__

#include <stdlib.h>

#include <arc/security/SecHandler.h>
#include <arc/ArcConfig.h>
#include <arc/loader/Plugin.h>

namespace ArcSec {

class DelegationSH : public SecHandler {
 public:
  DelegationSH(Arc::Config *cfg);
  virtual ~DelegationSH(void);
  virtual bool Handle(Arc::Message* msg);  
  static Arc::Plugin* get_sechandler(Arc::PluginArgument* arg);
};

} // namespace ArcSec

#endif /* __ARC_SEC_DELEGATIONSH_H__ */

