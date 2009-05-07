#ifndef __ARC_CHARON_H__
#define __ARC_CHARON_H__

#include <arc/infosys/RegisteredService.h>
#include <arc/security/ArcPDP/Evaluator.h>
#include <arc/security/PDP.h>
#include <arc/infosys/InformationInterface.h>

namespace ArcSec {

///A Service which includes the ArcPDP functionality; it can be deployed as an independent service to provide 
///request evaluation functionality for the other remote services
class Charon: public Arc::RegisteredService {
 protected:
  Arc::NS ns_;
  Arc::Logger logger_;
  std::string endpoint_;
  std::string expiration_;
  Arc::InformationContainer infodoc;
  Arc::MCC_Status make_soap_fault(Arc::Message& outmsg);
 public:
  Charon(Arc::Config *cfg);
  virtual ~Charon(void);
  virtual Arc::MCC_Status process(Arc::Message& inmsg,Arc::Message& outmsg);
  bool RegistrationCollector(Arc::XMLNode &doc);
 private:
  Evaluator* eval;
};

} // namespace ArcSec

#endif

