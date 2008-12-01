#ifndef __ARC_SEC_COUNTPDP_H__
#define __ARC_SEC_COUNTPDP_H__

#include <stdlib.h>

#include <arc/security/ClassLoader.h>
#include <arc/ArcConfig.h>
#include <arc/security/ArcPDP/Evaluator.h>
#include <arc/security/PDP.h>

namespace ArcSec {

///CountPDP - PDP which can handle the Arc specific request and policy schema
class CountPDP : public PDP {
 public:
  static Arc::Plugin* get_count_pdp(Arc::PluginArgument* arg);
  CountPDP(Arc::Config* cfg);
  virtual ~CountPDP();

  /***/
  virtual bool isPermitted(Arc::Message *msg);
 private:
  Evaluator *eval;
  Arc::ClassLoader* classloader;
 protected:
  static Arc::Logger logger;
};

} // namespace ArcSec

#endif /* __ARC_SEC_COUNTPDP_H__ */


