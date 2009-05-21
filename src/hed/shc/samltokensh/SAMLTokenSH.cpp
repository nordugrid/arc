#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <iostream>
#include <fstream>

#include <arc/XMLNode.h>
#include <arc/GUID.h>
#include <arc/URL.h>
#include <arc/message/MCC.h>
#include <arc/ws-security/SAMLToken.h>
#include <arc/xmlsec/XmlSecUtils.h>
#include <arc/xmlsec/XMLSecNode.h>
#include <arc/credential/Credential.h>
#include <arc/client/ClientInterface.h>
#include <arc/message/PayloadSOAP.h>

#include "SAMLTokenSH.h"

static Arc::Logger logger(Arc::Logger::rootLogger, "SAMLTokenSH");

Arc::Plugin* ArcSec::SAMLTokenSH::get_sechandler(Arc::PluginArgument* arg) {
    ArcSec::SecHandlerPluginArgument* shcarg =
            arg?dynamic_cast<ArcSec::SecHandlerPluginArgument*>(arg):NULL;
    if(!shcarg) return NULL;
    return new ArcSec::SAMLTokenSH((Arc::Config*)(*shcarg),(Arc::ChainContext*)(*shcarg));
}

/*
sechandler_descriptors ARC_SECHANDLER_LOADER = {
    { "samltoken.creator", 0, &get_sechandler},
    { NULL, 0, NULL }
};
*/

static std::string convert_dn(const std::string& dn) {
  std::string ret;
  size_t pos1 = std::string::npos;
  size_t pos2;
  do {
    std::string str;
    pos2 = dn.find_last_of("/", pos1);
    if(pos2 != std::string::npos && pos1 == std::string::npos) {
      str = dn.substr(pos2+1);
      ret.append(str);
      pos1 = pos2-1;
    }
    else if (pos2 != std::string::npos && pos1 != std::string::npos) {
      str = dn.substr(pos2+1, pos1-pos2);
      ret.append(str);
      pos1 = pos2-1;
    }
    if(pos2 != (std::string::npos+1)) ret.append(",");
  }while(pos2 != std::string::npos && pos2 != (std::string::npos+1));
  return ret;
}

namespace ArcSec {
using namespace Arc;

SAMLTokenSH::SAMLTokenSH(Config *cfg,ChainContext*):SecHandler(cfg){
  if(!init_xmlsec()) return;
  process_type_=process_none;
  std::string process_type = (std::string)((*cfg)["Process"]);
  if(process_type == "generate") { 
    cert_file_=(std::string)((*cfg)["CertificatePath"]);
    if(cert_file_.empty()) {
      logger.msg(ERROR,"Missing or empty CertificatePath element");
      return;
    };
    key_file_=(std::string)((*cfg)["KeyPath"]);
    if(key_file_.empty()) {
      logger.msg(ERROR,"Missing or empty KeyPath element");
      return;
    };
    ca_file_=(std::string)((*cfg)["CACertificatePath"]);
    ca_dir_=(std::string)((*cfg)["CACertificatesDir"]);
    if(ca_file_.empty() && ca_dir_.empty()) {
      logger.msg(WARNING,"Both of CACertificatePath and CACertificatesDir elements missing or empty");
    };
    aa_service_ = (std::string)((*cfg)["AAService"]);
    process_type_=process_generate;
  } else if(process_type == "extract") {
    //If ca file does not exist, we can only verify the signature by
    //using the certificate in the incoming wssecurity; we can not authenticate
    //the the message because we can not check the certificate chain without 
    //trusted ca.
    ca_file_=(std::string)((*cfg)["CACertificatePath"]);
    ca_dir_=(std::string)((*cfg)["CACertificatesDir"]);
    if(ca_file_.empty() && ca_dir_.empty()) {
      logger.msg(INFO,"Missing or empty CertificatePath or CACertificatesDir element; will only check the signature, will not do message authentication");
    };
    process_type_=process_extract;
  } else {
    logger.msg(ERROR,"Processing type not supported: %s",process_type);
    return;
  };
}

SAMLTokenSH::~SAMLTokenSH() {
  final_xmlsec();
}

bool SAMLTokenSH::Handle(Arc::Message* msg){
  if(process_type_ == process_extract) {
    try {
      PayloadSOAP* soap = dynamic_cast<PayloadSOAP*>(msg->Payload());
      SAMLToken st(*soap);
      if(!st) {
        logger.msg(ERROR,"Failed to parse SAML Token from incoming SOAP");
        return false;
      };
      if(!st.Authenticate()) {
        logger.msg(ERROR, "Failed to verify SAML Token inside the incoming SOAP");
        return false;
      };
      if((!ca_file_.empty() || !ca_dir_.empty()) && !st.Authenticate(ca_file_, ca_dir_)) {
        logger.msg(ERROR, "Failed to authenticate SAML Token inside the incoming SOAP");
        return false;
      };
      logger.msg(INFO, "Succeeded to authenticate SAMLToken");
    } catch(std::exception) {
      logger.msg(ERROR,"Incoming Message is not SOAP");
      return false;
    }  
  } else if(process_type_ == process_generate) {
    try {
      if(!saml_assertion_) {
        //Contact the AA service to get the saml assertion

        Arc::Credential cred(cert_file_, key_file_, ca_dir_, ca_file_);
        std::string local_dn = convert_dn(cred.GetDN());

        //Compose <samlp:AttributeQuery/>
        Arc::NS ns;
        ns["saml"] = "urn:oasis:names:tc:SAML:2.0:assertion";
        ns["samlp"] = "urn:oasis:names:tc:SAML:2.0:protocol";
        Arc::XMLNode attr_query(ns, "samlp:AttributeQuery");
        std::string query_id = Arc::UUID();
        attr_query.NewAttribute("ID") = query_id;
        Arc::Time t;
        std::string current_time = t.str(Arc::UTCTime);
        attr_query.NewAttribute("IssueInstant") = current_time;
        attr_query.NewAttribute("Version") = std::string("2.0");
        attr_query.NewChild("saml:Issuer") = local_dn;

        Arc::XMLNode subject = attr_query.NewChild("saml:Subject");
        Arc::XMLNode name_id = subject.NewChild("saml:NameID");
        name_id.NewAttribute("Format")=std::string("urn:oasis:names:tc:SAML:1.1:nameid-format:X509SubjectName");
        name_id = local_dn;

        Arc::XMLNode attribute = attr_query.NewChild("saml:Attribute");
        attribute.NewAttribute("Name")=std::string("urn:oid:1.3.6.1.4.1.5923.1.1.1.6");
        attribute.NewAttribute("NameFormat")=std::string("urn:oasis:names:tc:SAML:2.0:attrname-format:uri");
        attribute.NewAttribute("FriendlyName")=std::string("eduPersonPrincipalName");

        Arc::XMLSecNode attr_query_secnd(attr_query);
        std::string attr_query_idname("ID");
        attr_query_secnd.AddSignatureTemplate(attr_query_idname, Arc::XMLSecNode::RSA_SHA1);
        if(attr_query_secnd.SignNode(key_file_, cert_file_)) {
          std::cout<<"Succeeded to sign the signature under <samlp:AttributeQuery/>"<<std::endl;
        }

        Arc::NS soap_ns;
        Arc::SOAPEnvelope envelope(soap_ns);
        envelope.NewChild(attr_query);
        Arc::PayloadSOAP request(envelope);

        // Send request
        Arc::URL aa_service_url(aa_service_);
        Arc::MCCConfig cfg;
        if (!cert_file_.empty()) cfg.AddCertificate(cert_file_);
        if (!key_file_.empty()) cfg.AddPrivateKey(key_file_);
        if (!ca_file_.empty()) cfg.AddCAFile(ca_file_);
        if (!ca_dir_.empty()) cfg.AddCADir(ca_dir_);

        Arc::ClientSOAP client(cfg, aa_service_url);
        Arc::PayloadSOAP *response = NULL;
        Arc::MCC_Status status = client.process(&request,&response);
        if (!response) {
          logger.msg(Arc::ERROR, "No response from AA service %s failed", aa_service_.c_str());
          return false;
        }
        if (!status) {
          logger.msg(Arc::ERROR, "SOAP Request to AA service %s failed", aa_service_.c_str());
          delete response; return false;
        }

        //Consume the response from AA
        Arc::XMLNode attr_resp;
        attr_resp = (*response).Body().Child(0);
        if(!attr_resp) {
          logger.msg(Arc::ERROR, "Cannot find content under response soap message");
          return false;
        }
        if((attr_resp.Name() != "Response") || (attr_resp.Prefix() != "samlp")) {
          logger.msg(Arc::ERROR, "Cannot find <samlp:Response/> under response soap message:");
          std::string tmp; attr_resp.GetXML(tmp);
          logger.msg(Arc::ERROR, "%s", tmp.c_str());
          return false;
        }

        std::string resp_idname = "ID";
        Arc::XMLSecNode attr_resp_secnode(attr_resp);
        if(attr_resp_secnode.VerifyNode(resp_idname, ca_file_, ca_dir_)) {
          logger.msg(Arc::INFO, "Succeeded to verify the signature under <samlp:Response/>");
        }
        else {
          logger.msg(Arc::ERROR, "Failed to verify the signature under <samlp:Response/>");
          delete response; return false;
        }
        std::string responseto_id = (std::string)(attr_resp.Attribute("InResponseTo"));
        if(query_id != responseto_id) {
          logger.msg(Arc::INFO, "The Response is not going to this end");
          delete response; return false;
        }

        std::string resp_time = attr_resp.Attribute("IssueInstant");
        std::string statuscode_value = attr_resp["samlp:Status"]["samlp:StatusCode"];
        if(statuscode_value == "urn:oasis:names:tc:SAML:2.0:status:Success")
          logger.msg(Arc::INFO, "The StatusCode is Success");

        Arc::XMLNode assertion = attr_resp["saml:Assertion"];
        std::string assertion_idname = "ID";
        Arc::XMLSecNode assertion_secnode(assertion);
        if(assertion_secnode.VerifyNode(assertion_idname, ca_file_, ca_dir_)) {
          logger.msg(Arc::INFO, "Succeeded to verify the signature under <saml:Assertion/>");
        }
        else {
          logger.msg(Arc::ERROR, "Failed to verify the signature under <saml:Assertion/>");
          delete response;  return false;
        }
        assertion.New(saml_assertion_);
        delete response;
      }

      //Protect the SOAP message with SAML assertion
      PayloadSOAP* soap = dynamic_cast<PayloadSOAP*>(msg->Payload());
      SAMLToken st(*soap, cert_file_, key_file_, SAMLToken::SAML2, saml_assertion_);
      if(!st) {
        logger.msg(ERROR,"Failed to generate SAML Token for outgoing SOAP");
        return false;
      };
      //Reset the soap message
      (*soap) = st;
    } catch(std::exception) {
      logger.msg(ERROR,"Outgoing Message is not SOAP");
      return false;
    }
  } else {
    logger.msg(ERROR,"SAML Token handler is not configured");
    return false;
  } 
  return true;
}

}


