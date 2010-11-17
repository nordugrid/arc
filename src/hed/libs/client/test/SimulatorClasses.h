#ifndef __ARC_SIMULATORCLASSES_H__
#define __ARC_SIMULATORCLASSES_H__

#include <arc/message/PayloadSOAP.h>
#include <arc/delegation/DelegationInterface.h>

#ifdef ClientSOAP
#undef ClientSOAP
#endif

#ifdef DelegationProviderSOAP
#undef DelegationProviderSOAP
#endif

namespace Arc {

class ClientSOAPTest :
  public ClientSOAP {
  public:
    /** Constructor creates MCC chain and connects to server. */
    ClientSOAPTest()
      : ClientSOAP() {}
    ClientSOAPTest(const BaseConfig& cfg, const URL& url, int timeout = -1)
      : ClientSOAP(cfg, url, timeout) {}

    /** Send SOAP request and receive response. */
    MCC_Status process(PayloadSOAP *request, PayloadSOAP **response);
    /** Send SOAP request with specified SOAP action and receive response. */
    MCC_Status process(const std::string& action, PayloadSOAP *request,
                            PayloadSOAP **response);

    /** Instantiates pluggable elements according to generated configuration */
    virtual bool Load() { return true; }

    /** Returns entry point to SOAP MCC in configured chain.
        To initialize entry point Load() method must be called. */
    MCC* GetEntry() { return soap_entry; }

    MessageContext& GetContext() { return context; }

    static std::string action;
    static Arc::PayloadSOAP  request;
    static Arc::PayloadSOAP* response;
    static Arc::MCC_Status status;

    MCC *soap_entry;
    MessageContext context;
};

class DelegationProviderSOAPTest :
  public DelegationProviderSOAP {
  public:
    /** Creates instance from provided credentials.
        Credentials are used to sign delegated credentials. */
    DelegationProviderSOAPTest(const std::string& credentials)
      : DelegationProviderSOAP(credentials) {}

    DelegationProviderSOAPTest(const std::string& cert_file,const std::string& key_file,std::istream* inpwd = NULL)
      : DelegationProviderSOAP(cert_file,key_file,inpwd) {}

    /** Performs DelegateCredentialsInit SOAP operation.
        As result request for delegated credentials is received by this instance and stored 
        internally. Call to UpdateCredentials should follow. */
    bool DelegateCredentialsInit(MCCInterface& mcc_interface,MessageContext* context);

    /** Generates DelegatedToken element.
        Element is created as child of provided XML element and contains structure
        described in delegation.wsdl. */
    bool DelegatedToken(XMLNode parent);

    static std::string action;
    static Arc::PayloadSOAP  request;
    static Arc::PayloadSOAP* response;
    static Arc::MCC_Status status;
};


}

#ifndef ClientSOAP
#define ClientSOAP ClientSOAPTest
#endif

#ifndef DelegationProviderSOAP
#define DelegationProviderSOAP DelegationProviderSOAPTest
#endif


#endif // __ARC_SIMULATORCLASSES_H__
