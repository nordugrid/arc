#ifndef __ARC_CHARON_H__
#define __ARC_CHARON_H__

#include <string>
#include <list>

#include <arc/infosys/RegisteredService.h>
#include <arc/security/ArcPDP/Evaluator.h>
#include <arc/security/PDP.h>
#include <arc/infosys/InformationInterface.h>

namespace ArcSec {

///A Service which includes the ArcPDP functionality; it can be deployed as an independent service to provide 
///request evaluation functionality for the other remote services
class Charon: public Arc::RegisteredService {
 protected:
  class PolicyLocation {
   public:
    std::string path;
    bool reload;
    time_t mtime;
    time_t ctime;
    PolicyLocation(const std::string& location,bool reload);
    bool IsModified(void);
  };
  Glib::Mutex lock_;
  Arc::NS ns_;
  Arc::Logger logger_;
  std::string endpoint_;
  std::string expiration_;
  std::list<PolicyLocation> locations_;
  std::string evaluator_name_;
  Evaluator* eval;
  //Arc::InformationContainer infodoc;
  Arc::MCC_Status make_soap_fault(Arc::Message& outmsg, const std::string& msg = "");
  bool load_policies(void);
  bool policies_modified(void);
 public:
  Charon(Arc::Config *cfg);
  virtual ~Charon(void);
  virtual Arc::MCC_Status process(Arc::Message& inmsg,Arc::Message& outmsg);
  bool RegistrationCollector(Arc::XMLNode &doc);
};

} // namespace ArcSec

#endif

