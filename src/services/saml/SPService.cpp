#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <iostream>
#include <fstream>

#include <arc/DateTime.h>
#include <arc/GUID.h>
#include <arc/URL.h>
#include <arc/XMLNode.h>
#include <arc/message/PayloadRaw.h>
#include <arc/credential/Credential.h>
#include <arc/client/ClientInterface.h>

#include <arc/xmlsec/XmlSecUtils.h>
#include <arc/xmlsec/XMLSecNode.h>
#include <arc/xmlsec/saml_util.h>

#include "SPService.h"

static Arc::Plugin* get_service(Arc::PluginArgument* arg) {
    Arc::ServicePluginArgument* srvarg =
            arg?dynamic_cast<Arc::ServicePluginArgument*>(arg):NULL;
    if(!srvarg) return NULL;
    return new SPService::Service_SP((Arc::Config*)(*srvarg));
}

Arc::PluginDescriptor PLUGINS_TABLE_NAME[] = {
    { "saml.sp", "HED:SERVICE", 0, &get_service },
    { NULL, NULL, 0, NULL }
};

namespace SPService {

using namespace Arc;

class SAMLAssertionSecAttr: public Arc::SecAttr {
 public:
  SAMLAssertionSecAttr(XMLNode& node);
  SAMLAssertionSecAttr(std::string& str);
  virtual ~SAMLAssertionSecAttr(void);
  virtual operator bool(void) const;
  virtual bool Export(SecAttrFormat format,XMLNode &val) const;
  virtual bool Import(SecAttrFormat format, const XMLNode& val);
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

static void add_arc_subject_attribute(XMLNode item,const std::string& subject,const std::string& id) {
   XMLNode attr = item.NewChild("ra:SubjectAttribute");
   attr=subject; attr.NewAttribute("Type")="string";
   attr.NewAttribute("AttributeId")=id;
}

static void add_xacml_subject_attribute(XMLNode item,const std::string& subject,const std::string& id) {
   XMLNode attr = item.NewChild("ra:Attribute");
   attr.NewAttribute("DataType")="xs:string";
   attr.NewAttribute("AttributeId")=id;
   attr.NewChild("ra:AttributeValue") = subject;
}

bool SAMLAssertionSecAttr::Export(Arc::SecAttrFormat format, XMLNode& val) const {
  if(format == UNDEFINED) {
  } else if(format == SAML) {
    saml_assertion_node_.New(val);
    return true;
  } else if(format == ARCAuth) {
    NS ns;
    ns["ra"]="http://www.nordugrid.org/schemas/request-arc";
    val.Namespaces(ns); val.Name("ra:Request");
    XMLNode item = val.NewChild("ra:RequestItem");
    XMLNode subj = item.NewChild("ra:Subject");

    for(int i=0;;i++) {
      XMLNode attr_statement = const_cast<XMLNode&>(saml_assertion_node_)["AttributeStatement"][i];
      if(!attr_statement) break;
      for(int j=0;;j++) {
        XMLNode attr = attr_statement["Attribute"][j];
        if(!attr) break;
        std::string friendlyname = (std::string)(attr.Attribute("FriendlyName"));
        std::string attr_val = (std::string)(attr["AttributeValue"]);
        //Use the "FriendlyName" as the "AttributeId"
        add_arc_subject_attribute(subj, attr_val, friendlyname);
      };
    };
  } else if(format == XACML) {
    NS ns;
    ns["ra"]="urn:oasis:names:tc:xacml:2.0:context:schema:os";
    val.Namespaces(ns); val.Name("ra:Request");
    XMLNode subj = val.NewChild("ra:Subject");

    for(int i=0;;i++) {
      XMLNode attr_statement = const_cast<XMLNode&>(saml_assertion_node_)["AttributeStatement"][i];
      if(!attr_statement) break;
      for(int j=0;;j++) {
        XMLNode attr = attr_statement["Attribute"][j];
        if(!attr) break;
        std::string friendlyname = (std::string)(attr.Attribute("FriendlyName"));
        std::string attr_val = (std::string)(attr["AttributeValue"]);
        //Use the "FriendlyName" as the "AttributeId"
        add_xacml_subject_attribute(subj, attr_val, friendlyname);
      };
    };
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

Service_SP::Service_SP(Arc::Config *cfg):RegisteredService(cfg),logger(Arc::Logger::rootLogger, "SAML2SP") {
  Arc::XMLNode chain_node = (*cfg).Parent();
  Arc::XMLNode tls_node;
  for(int i = 0; ;i++) {
    tls_node = chain_node.Child(i);
    if(!tls_node) break;
    std::string tls_name = (std::string)(tls_node.Attribute("name"));
    if(tls_name == "tls.service") break;
  }
  //Use the private key file of the main chain 
  //for signing samlp:AuthnRequest
  cert_file_ = (std::string)(tls_node["CertificatePath"]);
  privkey_file_ = (std::string)(tls_node["KeyPath"]);
/*
  cert_file_ = (std::string)((*cfg)["CertificatePath"]);
  privkey_file_ = (std::string)((*cfg)["KeyPath"]);
*/
  sp_name_ = (std::string)((*cfg)["ServiceProviderName"]);
  logger.msg(Arc::INFO, "SP Service name is %s", sp_name_);
  std::string metadata_file = (std::string)((*cfg)["MetaDataLocation"]);
  logger.msg(Arc::INFO, "SAML Metadata is from %s", metadata_file);
  metadata_node_.ReadFromFile(metadata_file);

  if(!(Arc::init_xmlsec())) return;
}

Service_SP::~Service_SP(void) {
  Arc::final_xmlsec();
}

Arc::MCC_Status Service_SP::process(Arc::Message& inmsg,Arc::Message& outmsg) {

  // Check authentication and authorization
  if(!ProcessSecHandlers(inmsg, "incoming")) {
    logger.msg(Arc::ERROR, "saml2SP: Unauthorized");
    return Arc::MCC_Status(Arc::GENERIC_ERROR);
  };
  // Both input and output are supposed to be HTTP 
  // Extracting payload
  std::string msg_content;
  if(!inmsg.Payload()) {
    logger.msg(Arc::WARNING, "no input payload");
  } else {
    const char* payload_content = ContentFromPayload(*inmsg.Payload());
    if(!payload_content) {
      logger.msg(Arc::WARNING, "empty input payload");
    } else {
      msg_content = payload_content;
    }
  }

  //Analyzing http request from user agent

  //SP service is supposed to get two types of http content from user agent:
  //1. The IdP name, which user agent sends to SP, and then SP uses to generate AuthnRequest
  //2. The saml assertion, which user agent gets from IdP, and then sends to SP 
  
  if(msg_content.substr(0,4) == "http") { //If IdP name is given from client/useragent
    //Get the IdP name from the request
    //Here we require the user agent to provide the idp name instead of the 
    //WAYF(where are you from) or Discovery Service in some other implementation of SP
    //like Shibboleth

    std::string idp_name(msg_content);

    //Compose <samlp:AuthnRequest/>
    Arc::NS saml_ns;
    saml_ns["saml"] = SAML_NAMESPACE;
    saml_ns["samlp"] = SAMLP_NAMESPACE;
    Arc::XMLNode authn_request(saml_ns, "samlp:AuthnRequest");
    //std::string sp_name("https://squark.uio.no/shibboleth-sp");
    std::string sp_name = sp_name_;
    std::string req_id = Arc::UUID();
    authn_request.NewAttribute("ID") = req_id;
    Arc::Time t1;
    std::string current_time1 = t1.str(Arc::UTCTime);
    authn_request.NewAttribute("IssueInstant") = current_time1;
    authn_request.NewAttribute("Version") = std::string("2.0");

    //Get url of assertion consumer service from metadata
    std::string assertion_consumer_url;
    for(int i = 0;;i++) {
      Arc::XMLNode nd = metadata_node_.Child(i);
      if(!nd) break;
      if(sp_name == (std::string)(nd.Attribute("entityID"))) {
        for(int j = 0;; j++) {
          Arc::XMLNode sp_nd = nd.Child(j);
          if(!sp_nd) break;
          if(MatchXMLName(sp_nd,"SPSSODescriptor")) {
            for(int k = 0;;k++) {
              Arc::XMLNode assertionconsumer_nd = sp_nd.Child(k);
              if(!assertionconsumer_nd) break;        
              if(MatchXMLName(assertionconsumer_nd, "AssertionConsumerService")) {
                if((std::string)(assertionconsumer_nd.Attribute("Binding")) == "urn:oasis:names:tc:SAML:2.0:bindings:HTTP-POST")
                  assertion_consumer_url = (std::string)(assertionconsumer_nd.Attribute("Location"));
              }
              if(!assertion_consumer_url.empty()) break;
            }
          }
          if(!assertion_consumer_url.empty()) break;
        }
      }
    }
    authn_request.NewAttribute("AssertionConsumerServiceURL") = assertion_consumer_url;

    //Get url of sso service from metadata
    std::string sso_url;
    for(int i = 0;;i++) {
      Arc::XMLNode nd = metadata_node_.Child(i);
      if(!nd) break;
      if(idp_name == (std::string)(nd.Attribute("entityID"))) {
        for(int j = 0;; j++) {
          Arc::XMLNode idp_nd = nd.Child(j);
          if(!idp_nd) break;
          if(MatchXMLName(idp_nd,"IDPSSODescriptor")) {
            for(int k = 0;;k++) {
              Arc::XMLNode sso_nd = idp_nd.Child(k);
              if(!sso_nd) break;
              if(MatchXMLName(sso_nd, "SingleSignOnService")) {
                if((std::string)(sso_nd.Attribute("Binding")) == "urn:oasis:names:tc:SAML:2.0:bindings:HTTP-Redirect")
                  sso_url = (std::string)(sso_nd.Attribute("Location"));
              }
              if(!sso_url.empty()) break;
            }
          }
          if(!sso_url.empty()) break;
        }
      }
    }
    authn_request.NewAttribute("Destination") = sso_url;
    authn_request.NewAttribute("ProtocolBinding") = std::string("urn:oasis:names:tc:SAML:2.0:bindings:HTTP-POST");
    authn_request.NewChild("saml:Issuer") = sp_name;

    Arc::XMLNode nameid_policy = authn_request.NewChild("samlp:NameIDPolicy");
    nameid_policy.NewAttribute("AllowCreate") = std::string("1");

    bool must_signed = true; //TODO: get the information from metadata
    std::string authnRequestQuery;
    std::string query = "SAMLRequest=" + BuildDeflatedQuery(authn_request);
    logger.msg(Arc::DEBUG,"AuthnRequest after deflation: %s", query.c_str());
    if(must_signed) {
      //SP service uses it's private key to sign the AuthnRequest,
      //then after User Agent redirecting AuthnRequest to IdP, 
      //IdP will verify the signature by picking up SP's certificate
      //from IdP's metadata.
      logger.msg(Arc::DEBUG,"Using private key file to sign: %s", privkey_file_.c_str());
      authnRequestQuery = SignQuery(query, Arc::RSA_SHA1, privkey_file_);
      logger.msg(Arc::DEBUG,"After signature: %s", authnRequestQuery.c_str());
    }
    else authnRequestQuery = query;

#if 0
    //Verify the signature
    std::string cert_str = get_cert_str(cert_file_.c_str());
    std::cout<<"Cert to sign AuthnRequest: "<<cert_str<<std::endl;
    if(VerifyQuery(authnRequestQuery, cert_str)) {
      std::cout<<"Succeeded to verify the signature on AuthnRequest"<<std::endl;
    }
    else { std::cout<<"Failed to verify the signature on AuthnRequest"<<std::endl; }
#endif

    std::string authnRequestUrl;
    authnRequestUrl = sso_url + "?" + authnRequestQuery;

    //Return the composed url back to user agent through http
    Arc::PayloadRaw* outpayload = NULL;
    outpayload = new Arc::PayloadRaw;
    outpayload->Insert(authnRequestUrl.c_str(),0, authnRequestUrl.size());
    //outmsg.Attributes()->set("HTTP:CODE","302");
    //outmsg.Attributes()->set("HTTP:REASON","Moved Temporarily");
    delete outmsg.Payload(outpayload);
  }
  else {  
    //The http content should be <saml:EncryptedAssertion/> or <saml:Assertion/>
    //Decrypt the assertion (if it is encrypted) by using SP's private key (the key is the 
    //same as the one for the main message chain)
    //std::cout<<"saml assertion from peer side: "<<msg_content<<std::endl;
    Arc::XMLNode assertion_nd(msg_content);
    if(MatchXMLName(assertion_nd, "EncryptedAssertion")) {
      //Decrypt the encrypted saml assertion
      std::string saml_assertion;
      assertion_nd.GetXML(saml_assertion);
      logger.msg(Arc::DEBUG,"Encrypted saml assertion: %s", saml_assertion.c_str());

      Arc::XMLSecNode sec_assertion_nd(assertion_nd);
      Arc::XMLNode decrypted_assertion_nd;

      bool r = sec_assertion_nd.DecryptNode(privkey_file_, decrypted_assertion_nd);
      if(!r) { 
        logger.msg(Arc::ERROR,"Can not decrypt the EncryptedAssertion from saml response"); 
        return Arc::MCC_Status(); 
      }

      std::string decrypted_saml_assertion;
      decrypted_assertion_nd.GetXML(decrypted_saml_assertion);
      logger.msg(Arc::DEBUG,"Decrypted SAML Assertion: %s", decrypted_saml_assertion.c_str());
     
      //Decrypt the <saml:EncryptedID/> if it exists in the above saml assertion
      Arc::XMLNode nameid_nd = decrypted_assertion_nd["Subject"]["EncryptedID"];
    
      if((bool)nameid_nd) {
        std::string nameid;
        nameid_nd.GetXML(nameid);
        logger.msg(Arc::DEBUG,"Encrypted name id: %s", nameid.c_str());
     
        Arc::XMLSecNode sec_nameid_nd(nameid_nd);
        Arc::XMLNode decrypted_nameid_nd;
        r = sec_nameid_nd.DecryptNode(privkey_file_, decrypted_nameid_nd);
        if(!r) { logger.msg(Arc::ERROR,"Can not decrypt the EncryptedID from saml assertion"); return Arc::MCC_Status(); }

        std::string decrypted_nameid;
        decrypted_nameid_nd.GetXML(decrypted_nameid);
        logger.msg(Arc::DEBUG,"Decrypted SAML NameID: %s", decrypted_nameid.c_str());

        //Replace the <saml:EncryptedID/> with <saml:NameID/>
        nameid_nd.Replace(decrypted_nameid_nd);
      }

      //Check the <saml:Condition/> <saml:AuthnStatement/> 
      //and <saml:/Subject> part of saml assertion
      XMLNode subject   = decrypted_assertion_nd["Subject"];
      XMLNode conditions = decrypted_assertion_nd["Conditions"];
      XMLNode authnstatement = decrypted_assertion_nd["AuthnStatement"];

      std::string notbefore_str = (std::string)(conditions.Attribute("NotBefore"));
      Time notbefore = notbefore_str;
      std::string notonorafter_str = (std::string)(conditions.Attribute("NotOnOrAfter"));
      Time notonorafter = notonorafter_str;
      Time now = Time();
      if(!notbefore_str.empty() && notbefore >= now) {
        logger.msg(Arc::ERROR,"saml:Conditions, current time is before the start time"); 
        return Arc::MCC_Status(); 
      } else if (!notonorafter_str.empty() && notonorafter < now) {
        logger.msg(Arc::ERROR,"saml:Conditions, current time is after the end time"); 
        return Arc::MCC_Status(); 
      }     

      XMLNode subject_confirmation = subject["SubjectConfirmation"];
      std::string confirm_method = (std::string)(subject_confirmation.Attribute("Method"));
      if(confirm_method == "urn:oasis:names:tc:SAML:2.0:cm:bearer") {
        //TODO
      } else if (confirm_method == "urn:oasis:names:tc:SAML:2.0:cm:holder-of-key") {
        //TODO
      }
      notbefore_str = (std::string)(subject_confirmation.Attribute("NotBefore"));
      notbefore = notbefore_str;
      notonorafter_str = (std::string)(subject_confirmation.Attribute("NotOnOrAfter"));
      notonorafter = notonorafter_str;
      now = Time();
      if(!notbefore_str.empty() && notbefore >= now) {
        logger.msg(Arc::ERROR,"saml:Subject, current time is before the start time");
        return Arc::MCC_Status();
      } else if (!notonorafter_str.empty() && notonorafter < now) {
        logger.msg(Arc::ERROR,"saml:Subject, current time is after the end time");
        return Arc::MCC_Status();
      }
      //TODO: "InResponseTo", need to recorde the ID?

      
  
      //Record the saml assertion into message context, this information
      //will be checked by saml2sso_serviceprovider handler later to decide
      //whether authorize the incoming message which is from the same session
      //as this saml2sso process

      SAMLAssertionSecAttr* sattr = new SAMLAssertionSecAttr(decrypted_assertion_nd);
      inmsg.Auth()->set("SAMLAssertion", sattr);

      Arc::PayloadRaw* outpayload = NULL;
      outpayload = new Arc::PayloadRaw;
      //std::string authorization_info("SAML2SSO process succeeded");
      //outpayload->Insert(authorization_info.c_str(), 0, authorization_info.size());
      delete outmsg.Payload(outpayload);
    }
    else if(MatchXMLName(assertion_nd, "Assertion")) {

    }
    else {
      logger.msg(Arc::ERROR,"Can not get saml:Assertion or saml:EncryptedAssertion from IdP");
      Arc::MCC_Status();
    }
  }

  return Arc::MCC_Status(Arc::STATUS_OK);
}


bool Service_SP::RegistrationCollector(Arc::XMLNode &doc) {
  Arc::NS isis_ns; isis_ns["isis"] = "http://www.nordugrid.org/schemas/isis/2008/08";
  Arc::XMLNode regentry(isis_ns, "RegEntry");
  regentry.NewChild("SrcAdv").NewChild("Type") = "org.nordugrid.security.saml";
  regentry.New(doc);
  return true;
}


} // namespace SPService

