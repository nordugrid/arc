#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <iostream>
#include <fstream>

#include <arc/XMLNode.h>
#include <arc/GUID.h>
#include <arc/URL.h>
#include <arc/StringConv.h>
#include <arc/message/MCC.h>
#include <arc/ws-security/SAMLToken.h>
#include <arc/xmlsec/XmlSecUtils.h>
#include <arc/xmlsec/XMLSecNode.h>
#include <arc/credential/Credential.h>
#include <arc/communication/ClientInterface.h>
#include <arc/message/PayloadSOAP.h>

#include "SAMLTokenSH.h"

static Arc::Logger logger(Arc::Logger::rootLogger, "SAMLTokenSH");

Arc::Plugin* ArcSec::SAMLTokenSH::get_sechandler(Arc::PluginArgument* arg) {
  ArcSec::SecHandlerPluginArgument* shcarg =
          arg?dynamic_cast<ArcSec::SecHandlerPluginArgument*>(arg):NULL;
  if(!shcarg) return NULL;
  ArcSec::SAMLTokenSH* plugin = new ArcSec::SAMLTokenSH((Arc::Config*)(*shcarg),(Arc::ChainContext*)(*shcarg),arg);
  if(!plugin) return NULL;
  if(!(*plugin)) { delete plugin; plugin = NULL; };
  return plugin;
}

/*
sechandler_descriptors ARC_SECHANDLER_LOADER = {
    { "samltoken.creator", 0, &get_sechandler},
    { NULL, 0, NULL }
};
*/

namespace ArcSec {
using namespace Arc;

class SAMLAssertionSecAttr: public Arc::SecAttr {
 public:
  SAMLAssertionSecAttr(XMLNode& node);
  SAMLAssertionSecAttr(std::string& str);
  virtual ~SAMLAssertionSecAttr(void);
  virtual operator bool(void) const;
  virtual bool Export(SecAttrFormat format,XMLNode &val) const;
  virtual bool Import(SecAttrFormat format, const XMLNode& val);
  virtual std::string get(const std::string& id) const;
  virtual std::map< std::string,std::list<std::string> > getAll() const;
 protected:
  virtual bool equal(const SecAttr &b) const;
 private:
  XMLNode saml_assertion_node_;
};

SAMLAssertionSecAttr::SAMLAssertionSecAttr(XMLNode& node) {
  Import(SAML, node);
}

SAMLAssertionSecAttr::SAMLAssertionSecAttr(std::string& node_str) {
  Import(SAML, node_str);
}

SAMLAssertionSecAttr::~SAMLAssertionSecAttr(){}

std::string SAMLAssertionSecAttr::get(const std::string& id) const {
  // TODO: do some dissection of saml_assertion_node_
  return "";
}

std::map<std::string, std::list<std::string> > SAMLAssertionSecAttr::getAll() const {
 return std::map<std::string, std::list<std::string> >();
}

bool SAMLAssertionSecAttr::equal(const SecAttr& b) const {
  try {
    const SAMLAssertionSecAttr& a = dynamic_cast<const SAMLAssertionSecAttr&>(b);
    if (!a) return false;
    // ...
    return false;
  } catch(std::exception&) { };
  return false;
}

SAMLAssertionSecAttr::operator bool() const {
  return true;
}

static void add_subject_attribute(XMLNode item,const std::string& subject,const char* id) {
   XMLNode attr = item.NewChild("ra:SubjectAttribute");
   attr=subject; attr.NewAttribute("Type")="string";
   attr.NewAttribute("AttributeId")=id;
}

bool SAMLAssertionSecAttr::Export(Arc::SecAttrFormat format, XMLNode& val) const {
  if(format == UNDEFINED) {
  } else if(format == SAML) {
    saml_assertion_node_.New(val);
    return true;
  } else if(format == ARCAuth) { 
    //Parse the attributes inside saml assertion, 
    //and compose it into Arc request
    NS ns;
    ns["ra"]="http://www.nordugrid.org/schemas/request-arc";
    val.Namespaces(ns); val.Name("ra:Request");
    XMLNode item = val.NewChild("ra:RequestItem");
    XMLNode subj = item.NewChild("ra:Subject");

    Arc::XMLNode subject_nd = saml_assertion_node_["Subject"]["NameID"];
    add_subject_attribute(subj,subject_nd,"http://www.nordugrid.org/schemas/policy-arc/types/wss-saml/subject");

    Arc::XMLNode issuer_nd = saml_assertion_node_["Issuer"];
    add_subject_attribute(subj,issuer_nd,"http://www.nordugrid.org/schemas/policy-arc/types/wss-saml/issuer");

    Arc::XMLNode attr_statement = saml_assertion_node_["AttributeStatement"];
    Arc::XMLNode attr_nd;
    for(int i=0;;i++) {
      attr_nd = attr_statement["Attribute"][i];
      if(!attr_nd) break;
      std::string attr_name = attr_nd.Attribute("Name");
      //std::string attr_nameformat = attr_nd.Attribute("NameFormat");
      //std::string attr_friendname = attribute.Attribute("FriendlyName");
      Arc::XMLNode attrval_nd;
      for(int j=0;;j++) {
        attrval_nd = attr_nd["AttributeValue"][j];
        if(!attrval_nd) break;
        std::string tmp = "http://www.nordugrid.org/schemas/policy-arc/types/wss-saml/"+attr_name;
        add_subject_attribute(subj,attrval_nd,tmp.c_str());
      }
    }
  }
  else {};
  return true;
}

bool SAMLAssertionSecAttr::Import(Arc::SecAttrFormat format, const XMLNode& val) {
  if(format == UNDEFINED) {
  } else if(format == SAML) {
    val.New(saml_assertion_node_);
    return true;
  }
  else {};
  return false;
}

SAMLTokenSH::SAMLTokenSH(Config *cfg,ChainContext*,Arc::PluginArgument* parg):SecHandler(cfg,parg),system_ca_(false),valid_(false){
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
    system_ca_ = (((std::string)((*cfg)["SystemCA"])) == "true");
    if(ca_file_.empty() && ca_dir_.empty() && !system_ca_) {
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
    system_ca_ = (((std::string)((*cfg)["SystemCA"])) == "true");
    if(ca_file_.empty() && ca_dir_.empty() && !system_ca_) {
      logger.msg(INFO,"Missing or empty CertificatePath or CACertificatesDir element; will only check the signature, will not do message authentication");
    };
    process_type_=process_extract;
  } else {
    logger.msg(ERROR,"Processing type not supported: %s",process_type);
    return;
  };
  if(!cert_file_.empty()) {
    Arc::Credential cred(cert_file_, key_file_, ca_dir_, ca_file_, system_ca_);
    local_dn_ = convert_to_rdn(cred.GetDN());
  }
  valid_ = true;
}

SAMLTokenSH::~SAMLTokenSH() {
  final_xmlsec();
}

SecHandlerStatus SAMLTokenSH::Handle(Arc::Message* msg) const {
  if(process_type_ == process_extract) {
    try {
      PayloadSOAP* soap = dynamic_cast<PayloadSOAP*>(msg->Payload());
      SAMLToken st(*soap);
      if(!st) {
        logger.msg(ERROR,"Failed to parse SAML Token from incoming SOAP");
        return false;
      };
/*
      if(!st.Authenticate()) {
        logger.msg(ERROR, "Failed to verify SAML Token inside the incoming SOAP");
        return false;
      };
*/
      if((!ca_file_.empty() || !ca_dir_.empty()) && !st.Authenticate(ca_file_, ca_dir_, system_ca_)) {
        logger.msg(ERROR, "Failed to authenticate SAML Token inside the incoming SOAP");
        return false;
      };
      logger.msg(INFO, "Succeeded to authenticate SAMLToken");

      //Store the saml assertion into message context
      Arc::XMLNode assertion_nd = st["Assertion"];
      SAMLAssertionSecAttr* sattr = new SAMLAssertionSecAttr(assertion_nd);
      msg->Auth()->set("SAMLAssertion", sattr);

    } catch(std::exception&) {
      logger.msg(ERROR,"Incoming Message is not SOAP");
      return false;
    } 
  } else if(process_type_ == process_generate) {
    try {
      if(!saml_assertion_) {
        //Contact the AA service to get the saml assertion

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
        attr_query.NewChild("saml:Issuer") = local_dn_;

        Arc::XMLNode subject = attr_query.NewChild("saml:Subject");
        Arc::XMLNode name_id = subject.NewChild("saml:NameID");
        name_id.NewAttribute("Format")=std::string("urn:oasis:names:tc:SAML:1.1:nameid-format:X509SubjectName");
        name_id = local_dn_;

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
	cfg.SetSystemCA(system_ca_);

        Arc::ClientSOAP client(cfg, aa_service_url);
        Arc::PayloadSOAP *response = NULL;
        Arc::MCC_Status status = client.process(&request,&response);
        if (!response) {
          logger.msg(Arc::ERROR, "No response from AA service %s", aa_service_.c_str());
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
        if(attr_resp_secnode.VerifyNode(resp_idname, ca_file_, ca_dir_, system_ca_, true)) {
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
        if(assertion_secnode.VerifyNode(assertion_idname, ca_file_, ca_dir_, system_ca_, true)) {
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
    } catch(std::exception&) {
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


