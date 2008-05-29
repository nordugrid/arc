#ifndef __ARC_SEC_ARCPDP_H__
#define __ARC_SEC_ARCPDP_H__

#include <stdlib.h>

#include <arc/loader/ClassLoader.h>
#include <arc/ArcConfig.h>
#include <arc/security/ArcPDP/Evaluator.h>
#include <arc/security/PDP.h>

namespace ArcSec {

///ArcPDP - PDP which can handle the Arc specific request and policy schema
class ArcPDP : public PDP {
 public:
  static PDP* get_arc_pdp(Arc::Config *cfg, Arc::ChainContext *ctx);
  ArcPDP(Arc::Config* cfg);
  virtual ~ArcPDP();

  /***/
  virtual bool isPermitted(Arc::Message *msg);
 private:
  // Evaluator *eval;
  // Arc::ClassLoader* classloader;
  std::list<std::string> select_attrs;
  std::list<std::string> reject_attrs;
  std::list<std::string> policy_locations;
 protected:
  static Arc::Logger logger;
};

} // namespace ArcSec

#endif /* __ARC_SEC_ARCPDP_H__ */

