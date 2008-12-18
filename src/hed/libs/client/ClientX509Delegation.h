#ifndef __ARC_CLIENTX509DELEGATION_H__
#define __ARC_CLIENTX509DELEGATION_H__

#include <string>
#include <list>

#include <inttypes.h>

#include <arc/ArcConfig.h>
#include <arc/URL.h>
#include <arc/message/MCC_Status.h>
#include <arc/message/PayloadRaw.h>
#include <arc/message/PayloadSOAP.h>
#include <arc/client/ClientInterface.h>
#include <arc/credential/Credential.h>

namespace Arc {
  //This class is supposed to be run against the generic ARC delegation service 
  //to delegate itself's X.509 credential to delegation service; afterwards,
  //other functional clients can access the services which is hosted together
  //with the above delegation service.
  //This class can be used in any client utility, and also the service implementation
  //which needs to interoperate with another service.
  //The purpose of this client (together with the delegation service) is that any
  //intermediate service is able to act on behalf of the user.
  //
  //This class will also be extended to interoperate with other delegation service
  //implementaion such as the gridsite implementation which is used by CREAM service.
  //
  //Also, MyProxy could be looked as a delegation service, which will only used for
  //the first-step delegation (user delegates its credential to client). In this case,
  //ClientX509Delegation will only be used for the client utility, not for the inmediate
  //services. 
  //User firstly delegates its credential to MyProxy server (the proxy certificate and
  //related private key will be stored on MyProxy server), then the client (normally it 
  //could be the Web Browser) uses the username/password to acquire the proxy credential
  //from MyProxy server.

  enum DelegationType { DELEG_ARC, DELEG_GRIDSITE, DELEG_GT4, DELEG_MYPROXY, DELEG_UNKNOWN };

  class ClientX509Delegation {
  public:
    /** Constructor creates MCC chain and connects to server.*/
    ClientX509Delegation() {}
    ClientX509Delegation(const BaseConfig& cfg, const URL& url);
    virtual ~ClientX509Delegation();
    /** Process the delegation according to the different remote 
     *  delegation service. 
     *  @param deleg Delegation type
     *  @param delegation_id   For gridsite delegation service, the delegation_id
     *       is supposed to be created by client side, and sent to service side; 
     *       for ARC delegation service, the delegation_id is supposed to be created 
     *       by service side, and returned back. So for gridsite delegation service,
     *       this parameter is treated as input, while for ARC delegation service,
     *       it is treated as output.
     */
    bool createDelegation(DelegationType deleg, std::string& delegation_id);
    //bool deleteDelegation(DelegationType deleg);
    //bool acquireDelegation(DelegationType deleg, std::string& delegation_id, 
    //        std::string username = "", std::string password = "");

  private:
    ClientSOAP* soap_client_;
    std::string cert_file_; //if it is proxy certificate, the privkey_file_ should be empty
    std::string privkey_file_;
    std::string proxy_file_;
    std::string trusted_ca_dir_;
    std::string trusted_ca_file_;
    Arc::Credential* signer_; 
    static Logger logger;
  };

} // namespace Arc

#endif // __ARC_CLIENTX509DELEGATION_H__
