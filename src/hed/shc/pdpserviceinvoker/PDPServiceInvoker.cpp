#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdexcept>

#include <arc/XMLNode.h>
#include <arc/Thread.h>
#include <arc/ArcConfig.h>
#include <arc/Logger.h>
#include <arc/URL.h>
#include <arc/message/MCC.h>
#include <arc/StringConv.h>
#include <arc/GUID.h>
#include <arc/credential/Credential.h>
//#include <arc/security/ArcPDP/Response.h>
//#include <arc/security/ArcPDP/attr/AttributeValue.h>

#include "PDPServiceInvoker.h"

Arc::Logger ArcSec::PDPServiceInvoker::logger(Arc::Logger::getRootLogger(), "ArcSec.PDPServiceInvoker");

#define SAML_NAMESPACE "urn:oasis:names:tc:SAML:2.0:assertion"
#define SAMLP_NAMESPACE "urn:oasis:names:tc:SAML:2.0:protocol"
#define XACML_SAMLP_NAMESPACE "urn:oasis:xacml:2.0:saml:protocol:schema:os"

using namespace Arc;

namespace ArcSec {
Plugin* PDPServiceInvoker::get_pdpservice_invoker(PluginArgument* arg) {
    PDPPluginArgument* pdparg =
            arg?dynamic_cast<PDPPluginArgument*>(arg):NULL;
    if(!pdparg) return NULL;
    return new PDPServiceInvoker((Config*)(*pdparg),arg);
}

PDPServiceInvoker::PDPServiceInvoker(Config* cfg,Arc::PluginArgument* parg):PDP(cfg,parg), client(NULL), 
  default_ca(false), is_xacml(false), is_saml(false) {
  XMLNode filter = (*cfg)["Filter"];
  if((bool)filter) {
    XMLNode select_attr = filter["Select"];
    XMLNode reject_attr = filter["Reject"];
    for(;(bool)select_attr;++select_attr) select_attrs.push_back((std::string)select_attr);
    for(;(bool)reject_attr;++reject_attr) reject_attrs.push_back((std::string)reject_attr);
  };

  //Create a SOAP client
  logger.msg(Arc::INFO, "Creating a pdpservice client");

  std::string url_str;
  url_str = (std::string)((*cfg)["Endpoint"]);
  Arc::URL url(url_str);
  
  std::cout<<"URL: "<<url_str<<std::endl;

  Arc::MCCConfig mcc_cfg;
  std::cout<<"Keypath: "<<(std::string)((*cfg)["KeyPath"])<<"CertificatePath: "<<(std::string)((*cfg)["CertificatePath"])<<"CAPath: "<<(std::string)((*cfg)["CACertificatePath"])<<std::endl;
  key_path = (std::string)((*cfg)["KeyPath"]);
  cert_path = (std::string)((*cfg)["CertificatePath"]);
  proxy_path = (std::string)((*cfg)["ProxyPath"]);
  ca_dir = (std::string)((*cfg)["CACertificatesDir"]);
  ca_file = (std::string)((*cfg)["CACertificatePath"]);
  default_ca = (((std::string)((*cfg)["DefaultCA"])) == "true");
  mcc_cfg.AddPrivateKey(key_path);
  mcc_cfg.AddCertificate(cert_path);
  mcc_cfg.AddProxy(proxy_path);
  mcc_cfg.AddCAFile(ca_file);
  mcc_cfg.AddCADir(ca_dir);

  std::string format = (std::string)((*cfg)["RequestFormat"]);
  if(format=="XACML" || format=="xacml") is_xacml = true;

  std::string protocol = (std::string)((*cfg)["TransferProtocol"]);
  if(protocol=="SAML" || protocol=="saml") is_saml = true;

  client = new Arc::ClientSOAP(mcc_cfg,url,60);
}

PDPStatus PDPServiceInvoker::isPermitted(Message *msg) const {
  if((!is_xacml) && is_saml) {
    logger.msg(ERROR,"Arc policy can not been carried by SAML2.0 profile of XACML");
    return false;
  }
  //Compose the request
  MessageAuth* mauth = msg->Auth()->Filter(select_attrs,reject_attrs);
  MessageAuth* cauth = msg->AuthContext()->Filter(select_attrs,reject_attrs);
  if((!mauth) && (!cauth)) {
    logger.msg(ERROR,"Missing security object in message");
    return false;
  };
  NS ns;
  XMLNode requestxml(ns,"");
  if(mauth) {
    if(!mauth->Export(is_xacml? SecAttr::XACML : SecAttr::ARCAuth,requestxml)) {
      delete mauth;
      logger.msg(ERROR,"Failed to convert security information to ARC request");
      return false;
    };
    delete mauth;
  };
  if(cauth) {
    if(!cauth->Export(is_xacml? SecAttr::XACML : SecAttr::ARCAuth,requestxml)) {
      delete cauth;
      logger.msg(ERROR,"Failed to convert security information to ARC request");
      return false;
    };
    delete cauth;
  };
  {
    std::string s;
    requestxml.GetXML(s);
    logger.msg(DEBUG,"ARC Auth. request: %s",s);
  };
  if(requestxml.Size() <= 0) {
    logger.msg(ERROR,"No requested security information was collected");
    return false;
  };

  //Invoke the remote pdp service
  if(is_saml) {
    Arc::NS ns;
    ns["saml"] = SAML_NAMESPACE;
    ns["samlp"] = SAMLP_NAMESPACE;
    ns["xacml-samlp"] = XACML_SAMLP_NAMESPACE;
    Arc::XMLNode authz_query(ns, "xacml-samlp:XACMLAuthzDecisionQuery");
    std::string query_id = Arc::UUID();
    authz_query.NewAttribute("ID") = query_id;
    Arc::Time t;
    std::string current_time = t.str(Arc::UTCTime);
    authz_query.NewAttribute("IssueInstant") = current_time;
    authz_query.NewAttribute("Version") = std::string("2.0");

    Arc::Credential cred(cert_path.empty() ? proxy_path : cert_path,
         cert_path.empty() ? proxy_path : key_path, ca_dir, ca_file, default_ca);
    std::string local_dn_str = cred.GetDN();
    std::string local_dn = Arc::convert_to_rdn(local_dn_str);
    std::string issuer_name = local_dn;
    authz_query.NewChild("saml:Issuer") = issuer_name;

    authz_query.NewAttribute("InputContextOnly") = std::string("false");
    authz_query.NewAttribute("ReturnContext") = std::string("true");

    authz_query.NewChild(requestxml);

    Arc::NS req_ns;
    Arc::SOAPEnvelope req_env(req_ns);
    req_env.NewChild(authz_query);
    Arc::PayloadSOAP req(req_env);
    Arc::PayloadSOAP* resp = NULL;
    if(client) {
      Arc::MCC_Status status = client->process(&req,&resp);
      if(!status) {
        logger.msg(Arc::ERROR, "Policy Decision Service invocation failed");
      }
      if(resp == NULL) {
        logger.msg(Arc::ERROR,"There was no SOAP response");
      }
    }

    std::string authz_res;
    if(resp) {
      std::string str;
      resp->GetXML(str);
      logger.msg(Arc::INFO, "Response: %s", str);

      std::string authz_res = (std::string)((*resp)["samlp:Response"]["saml:Assertion"]["xacml-saml:XACMLAuthzDecisionStatement"]["xacml-context:Response"]["xacml-context:Result"]["xacml-context:Decision"]);

      delete resp;
    }

    if(authz_res == "Permit") { logger.msg(Arc::INFO,"Authorized from remote pdp service"); return true; }
    else { logger.msg(Arc::INFO,"Unauthorized from remote pdp service"); return false; }

  } else {
    Arc::NS req_ns;
    //req_ns["ra"] = "http://www.nordugrid.org/schemas/request-arc";
    req_ns["pdp"] = "http://www.nordugrid.org/schemas/pdp";
    Arc::PayloadSOAP req(req_ns);
    Arc::XMLNode reqnode = req.NewChild("pdp:GetPolicyDecisionRequest");
    reqnode.NewChild(requestxml);

    Arc::PayloadSOAP* resp = NULL;
    if(client) {
      Arc::MCC_Status status = client->process(&req,&resp);
      if(!status) {
        logger.msg(Arc::ERROR, "Policy Decision Service invocation failed");
      }
      if(resp == NULL) {
        logger.msg(Arc::ERROR,"There was no SOAP response");
      }
    }

    std::string authz_res;
    if(resp) {
      std::string str;
      resp->GetXML(str);
      logger.msg(Arc::INFO, "Response: %s", str);
  
      // TODO: Fix namespaces
      authz_res=(std::string)((*resp)["pdp:GetPolicyDecisionResponse"]["response:Response"]["response:AuthZResult"]);

      delete resp;
    } 

    if(authz_res == "PERMIT") { logger.msg(Arc::INFO,"Authorized from remote pdp service"); return true; }
    else { logger.msg(Arc::INFO,"Unauthorized from remote pdp service"); return false; }
  }

}

PDPServiceInvoker::~PDPServiceInvoker(){
  if(client != NULL) delete client;
}

} // namespace ArcSec

