#ifndef __ARC_SERVICE_DELEGATION_H__
#define __ARC_SERVICE_DELEGATION_H__

#include <arc/message/Service.h>
#include <arc/delegation/DelegationInterface.h>

namespace ArcSec {

///A Service which launches the proxy certificate request; it accepts the request from

class Service_Delegation: public Arc::Service {
 protected:
  Arc::NS ns_;
  Arc::Logger logger_;
  Arc::MCC_Status make_soap_fault(Arc::Message& outmsg);
 public:
  Service_Delegation(Arc::Config *cfg);
  virtual ~Service_Delegation(void);
  virtual Arc::MCC_Status process(Arc::Message& inmsg,Arc::Message& outmsg);
 private:
  Arc::DelegationContainerSOAP* deleg_service_;
};

} // namespace ArcSec

#endif

