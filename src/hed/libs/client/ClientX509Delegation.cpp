#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

// This define is needed to have maximal values for types with fixed size
#define __STDC_LIMIT_MACROS
#include <stdlib.h>
#include <map>

#include <arc/StringConv.h>
#include <arc/URL.h>
#include <arc/message/PayloadRaw.h>
#include <arc/message/MCC.h>
#include <arc/credential/Credential.h>

#include "ClientX509Delegation.h"

namespace Arc {
  Logger ClientX509Delegation::logger(Logger::getRootLogger(), "ClientX509Delegation");

#define DELEGATION_NAMESPACE "http://www.nordugrid.org/schemas/delegation"

  ClientX509Delegation::ClientX509Delegation(const BaseConfig& cfg,
                                                 const URL& url)
    : soap_client_(NULL), delegator_(NULL), signer_(NULL) {
    soap_client_ = new ClientSOAP(cfg, url);

    //Use the certificate and key in the main chain to delegate
    cert_file_ = cfg.cert;
    privkey_file_ = cfg.key;
    trusted_ca_dir_ = cfg.cadir;
    trusted_ca_file_ = cfg.cafile;
    delegator_ = new Arc::DelegationProviderSOAP(cert_file_, privkey_file_, &(std::cin));
    signer_ = new Arc::Credential(cert_file_, privkey_file_, trusted_ca_dir_, trusted_ca_file_);
  }

  ClientX509Delegation::~ClientX509Delegation() { 
    if(soap_client_) delete soap_client_;
    if(delegator_) delete delegator_;
    if(signer_) delete signer_;
  }

  static void set_cream_namespaces(NS& ns) {
    ns["SOAP-ENV"] = "http://schemas.xmlsoap.org/soap/envelope/";
    ns["SOAP-ENC"] = "http://schemas.xmlsoap.org/soap/encoding/";
    ns["xsi"] = "http://www.w3.org/2001/XMLSchema-instance";
    ns["xsd"] = "http://www.w3.org/2001/XMLSchema";
    ns["ns1"] = "http://www.gridsite.org/namespaces/delegation-2";
    ns["ns2"] = "http://glite.org/2007/11/ce/cream/types";
    ns["ns3"] = "http://glite.org/2007/11/ce/cream";
  }

  bool ClientX509Delegation::createDelegation(DelegationType deleg, std::string& delegation_id) {

    if(deleg == DELEG_ARC) {
      //Use the DelegationInterface class for ARC delegation service
      logger.msg(INFO, "Creating delegation to ARC delegation service");
      if(soap_client_ != NULL) {
        MCCInterface* soap_entry = soap_client_->GetEntry();
        Arc::MessageContext context;
        if(!delegator_->DelegateCredentialsInit(*soap_entry,&context)) {
          logger.msg(Arc::ERROR, "DelegateCredentialsInit failed");
          return false;
        };
        if(!delegator_->UpdateCredentials(*soap_entry,&context)) {
          logger.msg(Arc::ERROR, "UpdateCredentials failed");
          return false;
        };
      }
      else {
        logger.msg(ERROR, "There is no SOAP connection chain configured");
        return false;
      }
    }
    else if(deleg == DELEG_GRIDSITE) {
      //Move the current delegation related code in CREAMClient class to here 
      logger.msg(INFO, "Creating delegation to CREAM delegation service");
      NS cream_ns;
      set_cream_namespaces(cream_ns);

      PayloadSOAP request(cream_ns);
      //NS ns1;
      //ns1["ns1"] = "http://www.gridsite.org/namespaces/delegation-2";
      //XMLNode getProxyReqRequest = request.NewChild("ns1:getProxyReq", ns1);
      //XMLNode delegid = getProxyReqRequest.NewChild("delegationID", ns1);
      XMLNode getProxyReqRequest = request.NewChild("ns1:getProxyReq");
      XMLNode delegid = getProxyReqRequest.NewChild("ns1:delegationID");
      delegid.Set(delegation_id);
      PayloadSOAP *response = NULL;
      //Send the getProxyReq request
      if (soap_client_) {
        MCC_Status status = soap_client_->process("", &request, &response);
        if (!status) {
          logger.msg(ERROR, "Delegation getProxyReq request failed");
          return false;
        }
        if (response == NULL) {
          logger.msg(ERROR, "There is no SOAP response");
          return false;
        }
      }
      else {
        logger.msg(ERROR, "There is no SOAP connection chain configured");
        return false;
      }
      std::string getProxyReqReturnValue;
      if ((bool)(*response) &&
          (bool)((*response)["getProxyReqResponse"]["getProxyReqReturn"]) &&
          ((std::string)(*response)["getProxyReqResponse"]["getProxyReqReturn"]!= ""))
        getProxyReqReturnValue =
          (std::string)(*response)["getProxyReqResponse"]["getProxyReqReturn"];
      else {
        logger.msg(ERROR, "Creating delegation to CREAM delegation service failed");
        return false;
      }
      delete response;

      //Sign the proxy certificate
      Arc::Credential proxy;
      std::string signedcert;
      proxy.InquireRequest(getProxyReqReturnValue);
      if(!(signer_->SignRequest(&proxy, signedcert))) {
        logger.msg(ERROR, "DelegateProxy failed");
        return false;
      } 

      PayloadSOAP request2(cream_ns);
      //XMLNode putProxyRequest = request2.NewChild("ns1:putProxy", ns1);
      //XMLNode delegid_node = putProxyRequest.NewChild("delegationID", ns1);
      XMLNode putProxyRequest = request2.NewChild("ns1:putProxy");
      XMLNode delegid_node = putProxyRequest.NewChild("delegationID");
      delegid_node.Set(delegation_id);
      //XMLNode proxy_node = putProxyRequest.NewChild("proxy", ns1);
      XMLNode proxy_node = putProxyRequest.NewChild("proxy");
      proxy_node.Set(signedcert);
      response = NULL;

      //Send the putProxy request
      if (soap_client_) {
        MCC_Status status = soap_client_->process("", &request2, &response);
        if (!status) {
          logger.msg(ERROR, "Delegation putProxy request failed");
          return false;
        }
        if (response == NULL) {
          logger.msg(ERROR, "There is no SOAP response");
          return false;
        }
      }
      else {
        logger.msg(ERROR, "There is no SOAP connection chain configured");
        return false;
      }
      if (!(bool)(*response) || !(bool)((*response)["putProxyResponse"])) {
        logger.msg(ERROR, "Creating delegation to CREAM delegation failed");
        return false;
      }
      delete response;
    }
    else  if(deleg == DELEG_MYPROXY) {

    }

    return true; 
  }

} // namespace Arc
