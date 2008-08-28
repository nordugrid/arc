#ifndef __ARC_SERVICE_AA_H__
#define __ARC_SERVICE_AA_H__

#include <arc/message/Service.h>

namespace ArcSec {

///A Service which includes the AttributeAuthority functionality; it accepts the <samlp:AttributeQuery> 
///which includes the <Subject> of the principal from the request and <Attribute> which the request would get; 
///it access some local attribute database and returns <samlp:Assertion> which includes the <Attribute>
class Service_AA: public Arc::Service {
 protected:
  Arc::NS ns_;
  Arc::Logger logger_;
  Arc::MCC_Status make_soap_fault(Arc::Message& outmsg);
 public:
  Service_AA(Arc::Config *cfg);
  virtual ~Service_AA(void);
  virtual Arc::MCC_Status process(Arc::Message& inmsg,Arc::Message& outmsg);
 private:
  bool get_roles(std::vector<std::string>& fqans, std::string& userid, std::string& role, Arc::XMLNode& config);
  bool get_attributes(Arc::QueryArrayResult& attributes, std::string& idofsqlset, std::vector<std::string>& sqlargs, Arc::XMLNode& config);
};

} // namespace ArcSec

#endif

