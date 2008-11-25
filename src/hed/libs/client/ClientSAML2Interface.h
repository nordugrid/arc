#ifndef __ARC_CLIENTSAML2INTERFACE_H__
#define __ARC_CLIENTSAML2INTERFACE_H__

#include <string>
#include <list>

#include <inttypes.h>

#include <arc/ArcConfig.h>
#include <arc/URL.h>
#include <arc/message/MCC_Status.h>
#include <arc/message/PayloadRaw.h>
#include <arc/message/PayloadSOAP.h>
#include <arc/client/ClientInterface.h>

namespace Arc {

  class ClientHTTPwithSAML2SSO {
  public:
    /** Constructor creates MCC chain and connects to server. */
    ClientHTTPwithSAML2SSO() {}
    ClientHTTPwithSAML2SSO(const BaseConfig& cfg, const URL& url);
    virtual ~ClientHTTPwithSAML2SSO();

    /** Send HTTP request and receive response. */
    MCC_Status process(const std::string& method, PayloadRawInterface *request,
                       HTTPClientInfo *info, PayloadRawInterface **response,
                       const std::string& idp_name, const std::string& username,
                       const std::string& password);
    MCC_Status process(const std::string& method, const std::string& path,
                       PayloadRawInterface *request,
                       HTTPClientInfo *info, PayloadRawInterface **response,
                       const std::string& idp_name, const std::string& username,
                       const std::string& password);
  private:
    ClientHTTP* http_client_;
    bool authn_;
    //Credential and trusted certificates used to contact IdP 
    std::string cert_file_;
    std::string privkey_file_;
    std::string ca_file_;
    std::string ca_dir_;
    static Logger logger;
  };

  class ClientSOAPwithSAML2SSO {
  public:
    /** Constructor creates MCC chain and connects to server.*/
    ClientSOAPwithSAML2SSO() {}
    ClientSOAPwithSAML2SSO(const BaseConfig& cfg, const URL& url);
    virtual ~ClientSOAPwithSAML2SSO();
    /** Send SOAP request and receive response. */
    MCC_Status process(PayloadSOAP *request, PayloadSOAP **response, 
                       const std::string& idp_name, const std::string& username,
                       const std::string& password);
    /** Send SOAP request with specified SOAP action and receive response. */
    MCC_Status process(const std::string& action,
                       PayloadSOAP *request, PayloadSOAP **response,
                       const std::string& idp_name, const std::string& username,
                       const std::string& password);
  private:
    ClientSOAP* soap_client_;
    bool authn_;
    //Credential and trusted certificates used to contact IdP
    std::string cert_file_;
    std::string privkey_file_;
    std::string ca_file_;
    std::string ca_dir_;
    static Logger logger;
  };

} // namespace Arc

#endif // __ARC_CLIENTSAML2INTERFACE_H__
