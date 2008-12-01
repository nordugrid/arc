#ifndef __ARC_SEC_ARCPDPSERVICEINVOKER_H__
#define __ARC_SEC_ARCPDPSERVICEINVOKER_H__

#include <stdlib.h>

#include <arc/loader/Loader.h>
#include <arc/ArcConfig.h>
#include <arc/client/ClientInterface.h>
#include <arc/security/PDP.h>

namespace ArcSec {

///ArcPDPServiceInvoker - client which will invoke pdpservice
class ArcPDPServiceInvoker : public PDP {
 public:
  static Arc::Plugin* get_pdpservice_invoker(Arc::PluginArgument* arg);
  ArcPDPServiceInvoker(Arc::Config* cfg);
  virtual ~ArcPDPServiceInvoker();
  virtual bool isPermitted(Arc::Message *msg);
 private:
  Arc::ClientSOAP* client;
  std::list<std::string> select_attrs;
  std::list<std::string> reject_attrs;
  std::list<std::string> policy_locations;
 protected:
  static Arc::Logger logger;
};

} // namespace ArcSec

#endif /* __ARC_SEC_ARCPDPSERVICEINVOKER_H__ */

