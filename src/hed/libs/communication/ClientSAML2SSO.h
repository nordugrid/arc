// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_CLIENTSAML2SSO_H__
#define __ARC_CLIENTSAML2SSO_H__

#include <string>
#include <list>

#include <inttypes.h>

#include <arc/ArcConfig.h>
#include <arc/URL.h>
#include <arc/message/MCC_Status.h>
#include <arc/message/PayloadRaw.h>
#include <arc/message/PayloadSOAP.h>
#include <arc/communication/ClientInterface.h>

namespace Arc {

  class ClientHTTPwithSAML2SSO {
  public:
    /** Constructor creates MCC chain and connects to server. */
    ClientHTTPwithSAML2SSO() : http_client_(NULL), authn_(false) {}
    ClientHTTPwithSAML2SSO(const BaseConfig& cfg, const URL& url);
    virtual ~ClientHTTPwithSAML2SSO();

    /** Send HTTP request and receive response. */
    MCC_Status process(const std::string& method, PayloadRawInterface *request,
                       HTTPClientInfo *info, PayloadRawInterface **response,
                       const std::string& idp_name, const std::string& username,
                       const std::string& password, const bool reuse_authn = false);
    MCC_Status process(const std::string& method, const std::string& path,
                       PayloadRawInterface *request,
                       HTTPClientInfo *info, PayloadRawInterface **response,
                       const std::string& idp_name, const std::string& username,
                       const std::string& password, const bool reuse_authn = false);
  private:
    ClientHTTP *http_client_;
    bool authn_;
    //Credential and trusted certificates used to contact IdP
    std::string cert_file_;
    std::string privkey_file_;
    std::string ca_file_;
    std::string ca_dir_;
    static Logger logger;

    std::string cookie;

  private:
    ClientHTTPwithSAML2SSO(ClientHTTPwithSAML2SSO const&);
    ClientHTTPwithSAML2SSO& operator=(ClientHTTPwithSAML2SSO const&);
  };

  class ClientSOAPwithSAML2SSO {
  public:
    /** Constructor creates MCC chain and connects to server.*/
    ClientSOAPwithSAML2SSO() : soap_client_(NULL), authn_(false) {}
    ClientSOAPwithSAML2SSO(const BaseConfig& cfg, const URL& url);
    virtual ~ClientSOAPwithSAML2SSO();
    /** Send SOAP request and receive response. */
    MCC_Status process(PayloadSOAP *request, PayloadSOAP **response,
                       const std::string& idp_name, const std::string& username,
                       const std::string& password, const bool reuse_authn = false);
    /** Send SOAP request with specified SOAP action and receive response. */
    MCC_Status process(const std::string& action,
                       PayloadSOAP *request, PayloadSOAP **response,
                       const std::string& idp_name, const std::string& username,
                       const std::string& password, const bool reuse_authn = false);
  private:
    ClientSOAP *soap_client_;
    bool authn_;
    //Credential and trusted certificates used to contact IdP
    std::string cert_file_;
    std::string privkey_file_;
    std::string ca_file_;
    std::string ca_dir_;
    static Logger logger;

    std::string cookie;

    ClientSOAPwithSAML2SSO(ClientSOAPwithSAML2SSO const&);
    ClientSOAPwithSAML2SSO& operator=(ClientSOAPwithSAML2SSO const&);
  };

} // namespace Arc

#endif // __ARC_CLIENTSAML2SSO_H__
