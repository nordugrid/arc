#ifndef __ARC_SERVICE_PDP_H__
#define __ARC_SERVICE_PDP_H__

#include <arc/message/Service.h>

#include <arc/security/ArcPDP/Evaluator.h>
#include <arc/security/PDP.h>

namespace ArcSec {

class Service_PDP: public Arc::Service {
 protected:
  Arc::NS ns_;
  Arc::Logger logger_;
  Arc::MCC_Status make_soap_fault(Arc::Message& outmsg);
 public:
  Service_PDP(Arc::Config *cfg);
  virtual ~Service_PDP(void);
  virtual Arc::MCC_Status process(Arc::Message& inmsg,Arc::Message& outmsg);
 private:
  Evaluator* eval;
};

} // namespace ArcSec

#endif

