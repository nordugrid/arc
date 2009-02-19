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

#include "ClientX509Delegation.h"

namespace Arc {
  Logger ClientX509Delegation::logger(Logger::getRootLogger(), "ClientX509Delegation");

#define ARC_DELEGATION_NAMESPACE "http://www.nordugrid.org/schemas/delegation"
#define GS_DELEGATION_NAMESPACE "http://www.gridsite.org/namespaces/delegation-2"

  ClientX509Delegation::ClientX509Delegation(const BaseConfig& cfg,
                                                 const URL& url)
    : soap_client_(NULL), signer_(NULL) {
    soap_client_ = new ClientSOAP(cfg, url);

    //Use the certificate and key in the main chain to delegate
    cert_file_ = cfg.cert;
    privkey_file_ = cfg.key;
    proxy_file_ = cfg.proxy;
    trusted_ca_dir_ = cfg.cadir;
    trusted_ca_file_ = cfg.cafile;
    if(!cert_file_.empty() && !privkey_file_.empty())
      signer_ = new Arc::Credential(cert_file_, privkey_file_, trusted_ca_dir_, trusted_ca_file_);
    else if(!proxy_file_.empty())
      signer_ = new Arc::Credential(proxy_file_, "", trusted_ca_dir_, trusted_ca_file_);
  }

  ClientX509Delegation::~ClientX509Delegation() { 
    if(soap_client_) delete soap_client_;
    if(signer_) delete signer_;
  }

  bool ClientX509Delegation::createDelegation(DelegationType deleg, std::string& delegation_id) {

    if(deleg == DELEG_ARC) {
      //Use the DelegationInterface class for ARC delegation service
      logger.msg(INFO, "Creating delegation credential to ARC delegation service");
      if(soap_client_ != NULL) {
        NS ns; ns["deleg"]=ARC_DELEGATION_NAMESPACE;
        PayloadSOAP request(ns);
        request.NewChild("deleg:DelegateCredentialsInit");
        PayloadSOAP* response = NULL;
        //Send DelegateCredentialsInit request
        MCC_Status status = soap_client_->process(&request, &response);
        if(status != Arc::STATUS_OK) { 
          logger.msg(Arc::ERROR, "DelegateCredentialsInit failed"); 
          return false;
        }
        if(!response) {
          logger.msg(Arc::ERROR, "There is no SOAP response");
          return false;
        }
        XMLNode token = (*response)["DelegateCredentialsInitResponse"]["TokenRequest"];
        if(!token) { 
          logger.msg(Arc::ERROR, "There is no X509 request in the response"); 
          delete response; return false; 
        };
        if(((std::string)(token.Attribute("Format"))) != "x509") { 
          logger.msg(Arc::ERROR, "There is no Format request in the response");
          delete response; return false; 
        };
        delegation_id = (std::string)(token["Id"]);
        std::string x509request = (std::string)(token["Value"]);
        delete response;
        if(delegation_id.empty() || x509request.empty()) {
          logger.msg(Arc::ERROR, "There is no Id or X509 request value in the response");
          return false;
        };
   
        std::cout<<"X509 Request: \n"<<x509request<<std::endl;
 
        //Sign the proxy certificate
        Arc::Time start;
        Arc::Credential proxy(start,Arc::Period(12*3600), 0, "rfc", "inheritall","",-1);
        //Set proxy path length to be -1, which means infinit length
        std::string signedcert;
        proxy.InquireRequest(x509request);
        if(!(signer_->SignRequest(&proxy, signedcert))) {
          logger.msg(ERROR, "DelegateProxy failed");
          return false;
        }
        std::string signercert_str;
        std::string signercertchain_str;
        signer_->OutputCertificate(signercert_str);
        signer_->OutputCertificateChain(signercertchain_str);
        signedcert.append(signercert_str);
        signedcert.append(signercertchain_str);

        PayloadSOAP request2(ns);
        XMLNode token2 = request2.NewChild("deleg:UpdateCredentials").NewChild("deleg:DelegatedToken");
        token2.NewAttribute("deleg:Format")="x509";
        token2.NewChild("deleg:Id")=delegation_id;
        token2.NewChild("deleg:Value")=signedcert;
        //Send UpdateCredentials request
        status = soap_client_->process(&request2, &response);
        if(status != Arc::STATUS_OK) {
          logger.msg(Arc::ERROR, "UpdateCredentials failed");
          return false;
        }
        if(!response) {
          logger.msg(Arc::ERROR, "There is no SOAP response");
          return false;
        }
        if(!(*response)["UpdateCredentialsResponse"]) {
          logger.msg(Arc::ERROR, "There is no UpdateCredentialsResponse in response");
          delete response; return false;
        }
        delete response;
        return true;
      }
      else {
        logger.msg(ERROR, "There is no SOAP connection chain configured");
        return false;
      }
    }
    else if(deleg == DELEG_GRIDSITE) {
      //Move the current delegation related code in CREAMClient class to here 
      logger.msg(INFO, "Creating delegation to CREAM delegation service");

      NS ns; ns["deleg"]=GS_DELEGATION_NAMESPACE;
      PayloadSOAP request(ns);
      XMLNode getProxyReqRequest = request.NewChild("deleg:getProxyReq");
      XMLNode delegid = getProxyReqRequest.NewChild("deleg:delegationID");
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
      std::cout<<"X509 Request: \n"<<getProxyReqReturnValue<<std::endl;
      proxy.InquireRequest(getProxyReqReturnValue);
      if(!(signer_->SignRequest(&proxy, signedcert))) {
        logger.msg(ERROR, "DelegateProxy failed");
        return false;
      } 

      std::string signerstr;
      signer_->OutputCertificate(signerstr);
      signedcert.append(signerstr);

      PayloadSOAP request2(ns);
      XMLNode putProxyRequest = request2.NewChild("deleg:putProxy");
      XMLNode delegid_node = putProxyRequest.NewChild("deleg:delegationID");
      delegid_node.Set(delegation_id);
      XMLNode proxy_node = putProxyRequest.NewChild("deleg:proxy");
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

  bool ClientX509Delegation::acquireDelegation(DelegationType deleg, std::string& delegation_cred, std::string& delegation_id,
            const std::string cred_identity, const std::string cred_delegator_ip,
            const std::string username, const std::string password) {
    if(deleg == DELEG_ARC) {
      //Use the DelegationInterface class for ARC delegation service
      logger.msg(INFO, "Getting delegation credential from ARC delegation service");
      if(soap_client_ != NULL) {
        NS ns; ns["deleg"]=ARC_DELEGATION_NAMESPACE;
        PayloadSOAP request(ns);
        XMLNode tokenlookup = request.NewChild("deleg:AcquireCredentials").NewChild("deleg:DelegatedTokenLookup");
        //Use delegation ID to acquire delegation credential from 
        //delegation service, if the delegation ID is presented;
        if(!delegation_id.empty())
          tokenlookup.NewChild("deleg:Id")=delegation_id;
        //If delegation ID is not presented, use the credential identity
        //credential delegator's IP to acquire delegation credential 
        else {
          tokenlookup.NewChild("deleg:CredIdentity")=cred_identity;
          tokenlookup.NewChild("deleg:CredDelegatorIP")=cred_delegator_ip;
        }

        PayloadSOAP* response = NULL;
        //Send AcquireCredentials request
        MCC_Status status = soap_client_->process(&request, &response);
        if(status != Arc::STATUS_OK) {
          logger.msg(Arc::ERROR, "DelegateCredentialsInit failed");
          return false;
        }
        if(!response) {
          logger.msg(Arc::ERROR, "There is no SOAP response");
          return false;
        }
        XMLNode token = (*response)["AcquireCredentialsResponse"]["DelegatedToken"];
        if(!token) {
          logger.msg(Arc::ERROR, "There is no Delegated X509 token in the response");
          delete response; return false;
        };
        if(((std::string)(token.Attribute("Format"))) != "x509") {
          logger.msg(Arc::ERROR, "There is no Format delegated token in the response");
          delete response; return false;
        };
        delegation_id = (std::string)(token["Id"]);
        delegation_cred = (std::string)(token["Value"]);
        delete response;
        if(delegation_id.empty() || delegation_cred.empty()) {
          logger.msg(Arc::ERROR, "There is no Id or X509 token value in the response");
          return false;
        };
        std::cout<<"Get delegated X509 Token: \n"<<delegation_cred<<std::endl;
        return true;
      }
      else {
        logger.msg(ERROR, "There is no SOAP connection chain configured");
        return false;
      }
    }
    else {

    }
  }


} // namespace Arc
