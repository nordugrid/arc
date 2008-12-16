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
#include <arc/delegation/DelegationInterface.h>

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
  class ClientX509Delegation {
  public:
    /** Constructor creates MCC chain and connects to server.*/
    ClientX509Delegation() {}
    ClientX509Delegation(const BaseConfig& cfg, const URL& url);
    virtual ~ClientX509Delegation();
    /** Send SOAP request and receive response. */
    MCC_Status process(PayloadSOAP *request, PayloadSOAP **response);
    /** Send SOAP request with specified SOAP action and receive response. */
    MCC_Status process(const std::string& action,
                       PayloadSOAP *request, PayloadSOAP **response);
  private:
    ClientSOAP* soap_client_;
    DelegationProviderSOAP* delegator_;
    static Logger logger;
  };

} // namespace Arc

#endif // __ARC_CLIENTX509DELEGATION_H__
