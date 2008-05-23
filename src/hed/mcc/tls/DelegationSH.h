#ifndef __ARC_SEC_DELEGATIONSH_H__
#define __ARC_SEC_DELEGATIONSH_H__

#include <stdlib.h>

#include <arc/security/SecHandler.h>
#include <arc/ArcConfig.h>
#include <arc/loader/Loader.h>
/*
#include <arc/security/PDP.h>
#include <arc/message/Message.h>
*/

namespace ArcSec {

class DelegationSH : public SecHandler {
 public:
  DelegationSH(Arc::Config *cfg, Arc::ChainContext* ctx);
  virtual ~DelegationSH(void);
  virtual bool Handle(Arc::Message* msg);  
  static SecHandler* get_sechandler(Arc::Config *cfg, Arc::ChainContext* ctx);
};

} // namespace ArcSec

#endif /* __ARC_SEC_DELEGATIONSH_H__ */

