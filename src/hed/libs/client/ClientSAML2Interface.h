#ifndef __ARC_CLIENTSAML2INTERFACE_H__
#define __ARC_CLIENTSAML2INTERFACE_H__

#include <string>
#include <list>

#include <inttypes.h>

#include <arc/ArcConfig.h>
#include <arc/message/MCC_Status.h>
#include <arc/message/PayloadRaw.h>
#include <arc/message/PayloadSOAP.h>
#include <arc/client/ClientInterface.h>

namespace Arc {

  class ClientHTTPwithSAML2SSO
    : public virtual ClientHTTP {
  public:
    /** Constructor creates MCC chain and connects to server.
        cfg - common configuration,
        host - hostname of remote server,
        port - TCP port of remote server,
        tls - true if connection to use HTTPS, false for HTTP,
        path - internal path of service to be contacted.
    */
    ClientHTTPwithSAML2SSO() {}
    ClientHTTPwithSAML2SSO(const BaseConfig& cfg, const std::string& host, int port,
               bool tls, const std::string& path);
    virtual ~ClientHTTPwithSAML2SSO();
    
    /** */
    MCC_Status process_saml2sso(const std::string& idp_name);

    /** Send HTTP request and receive response. */
    MCC_Status process(const std::string& method, PayloadRawInterface *request,
                       HTTPClientInfo *info, PayloadRawInterface **response);
    MCC_Status process(const std::string& method, const std::string& path, PayloadRawInterface *request,
                       HTTPClientInfo *info, PayloadRawInterface **response);
  };

  class ClientSOAPwithSAML2SSO
    : public ClientSOAP, public ClientHTTPwithSAML2SSO {
  public:
    /** Constructor creates MCC chain and connects to server.*/
    ClientSOAPwithSAML2SSO() {}
    ClientSOAPwithSAML2SSO(const BaseConfig& cfg, const std::string& host, int port,
               bool tls, const std::string& path);
    virtual ~ClientSOAPwithSAML2SSO();
    /** Send SOAP request and receive response. */
    MCC_Status process(PayloadSOAP *request, PayloadSOAP **response);
    /** Send SOAP request with specified SOAP action and receive response. */
    MCC_Status process(const std::string& action, PayloadSOAP *request,
                       PayloadSOAP **response);
  };

} // namespace Arc

#endif // __ARC_CLIENTSAML2INTERFACE_H__

