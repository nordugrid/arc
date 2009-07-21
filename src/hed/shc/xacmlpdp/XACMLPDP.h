#ifndef __ARC_SEC_XACMLPDP_H__
#define __ARC_SEC_XACMLPDP_H__

#include <stdlib.h>

//#include <arc/loader/ClassLoader.h>
#include <arc/ArcConfig.h>
#include <arc/security/ArcPDP/Evaluator.h>
#include <arc/security/PDP.h>

namespace ArcSec {

///XACMLPDP - PDP which can handle the XACML specific request and policy schema
class XACMLPDP : public PDP {
 public:
  static Arc::Plugin* get_xacml_pdp(Arc::PluginArgument* arg);
  XACMLPDP(Arc::Config* cfg);
  virtual ~XACMLPDP();

  virtual bool isPermitted(Arc::Message *msg);
 private:
  // Evaluator *eval;
  // Arc::ClassLoader* classloader;
  std::list<std::string> select_attrs;
  std::list<std::string> reject_attrs;
  std::list<std::string> policy_locations;
  Arc::XMLNodeContainer policies;
  std::string policy_combining_alg;
 protected:
  static Arc::Logger logger;
};

} // namespace ArcSec

#endif /* __ARC_SEC_XACMLPDP_H__ */

