#ifndef __ARC_SERVICE_DELEGATION_H__
#define __ARC_SERVICE_DELEGATION_H__

#include <map>
#include <arc/delegation/DelegationInterface.h>
#include <arc/infosys/RegisteredService.h>
#include <arc/infosys/InformationInterface.h>


namespace ArcSec {

///A Service which launches the proxy certificate request; it accepts the request from

class Service_Delegation: public Arc::RegisteredService {
 private:
  class CredentialCache;
  typedef std::map<std::string,CredentialCache*> ID2CredMap;
  typedef ID2CredMap::iterator ID2CredMapIterator;
  ID2CredMap id2cred_;
  typedef std::multimap<std::string,CredentialCache*> Identity2CredMap;
  typedef Identity2CredMap::iterator Identity2CredMapIterator;
  typedef std::pair<Identity2CredMapIterator,Identity2CredMapIterator> Identity2CredMapReturn;
  Identity2CredMap identity2cred_;
  Glib::Mutex lock_;
  int max_crednum_;
  int max_credlife_;
  std::string trusted_cadir;
  std::string trusted_capath;
 protected:
  Arc::NS ns_;
  Arc::Logger logger_;
  std::string endpoint_;
  std::string expiration_;
  Arc::InformationContainer infodoc;
  Arc::MCC_Status make_soap_fault(Arc::Message& outmsg);
 public:
  Service_Delegation(Arc::Config *cfg);
  virtual ~Service_Delegation(void);
  virtual Arc::MCC_Status process(Arc::Message& inmsg,Arc::Message& outmsg);
  bool RegistrationCollector(Arc::XMLNode &doc);
 private:
  Arc::DelegationContainerSOAP* deleg_service_;
};

} // namespace ArcSec

#endif

