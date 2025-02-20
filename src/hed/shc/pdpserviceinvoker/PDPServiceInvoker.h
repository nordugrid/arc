#ifndef __ARC_SEC_PDPSERVICEINVOKER_H__
#define __ARC_SEC_PDPSERVICEINVOKER_H__

#include <stdlib.h>

#include <arc/loader/Loader.h>
#include <arc/ArcConfig.h>
#include <arc/communication/ClientInterface.h>
#include <arc/security/PDP.h>

namespace ArcSec {

///PDPServiceInvoker - client which will invoke pdpservice
class PDPServiceInvoker : public PDP {
 public:
  static Arc::Plugin* get_pdpservice_invoker(Arc::PluginArgument* arg);
  PDPServiceInvoker(Arc::Config* cfg, Arc::PluginArgument* parg);
  virtual ~PDPServiceInvoker();
  virtual PDPStatus isPermitted(Arc::Message *msg) const;
 private:
  Arc::ClientSOAP* client;
  std::string proxy_path;
  std::string cert_path;
  std::string key_path;
  std::string ca_dir;
  std::string ca_file;
  bool system_ca;
  std::list<std::string> select_attrs;
  std::list<std::string> reject_attrs;
  std::list<std::string> policy_locations;
  bool is_xacml; //If the policy decision request is with XACML format
  bool is_saml; //If the "SAML2.0 profile of XACML v2.0" is used
 protected:
  static Arc::Logger logger;
};

} // namespace ArcSec

#endif /* __ARC_SEC_PDPSERVICEINVOKER_H__ */

