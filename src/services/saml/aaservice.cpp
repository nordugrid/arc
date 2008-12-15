#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <iostream>

#include <sys/types.h>
#include <pwd.h>

#include <arc/loader/Loader.h>
#include <arc/security/ClassLoader.h>
/* ##include <arc/loader/ServiceLoader.h> */
#include <arc/message/Plexer.h>
#include <arc/message/PayloadSOAP.h>
#include <arc/message/PayloadRaw.h>
#include <arc/message/PayloadStream.h>
#include <arc/message/SOAPEnvelope.h>
#include <arc/Thread.h>
#include <arc/DateTime.h>
#include <arc/GUID.h>


#include <xmlsec/base64.h>
#include <xmlsec/errors.h>
#include <xmlsec/xmltree.h>
#include <xmlsec/xmldsig.h>
#include <xmlsec/xmlenc.h>
#include <xmlsec/templates.h>
#include <xmlsec/crypto.h>
  
#include <xmlsec/openssl/app.h>
#include <openssl/bio.h>
    
#include <openssl/evp.h>
#include <openssl/sha.h>
#include <openssl/rand.h>
#ifdef CHARSET_EBCDIC
#include <openssl/ebcdic.h>
#endif

#include <arc/xmlsec/XmlSecUtils.h>
#include <arc/xmlsec/XMLSecNode.h>
#include "../../hed/libs/common/MysqlWrapper.h"

#include "aaservice.h"

namespace ArcSec {

#define SAML_NAMESPACE "urn:oasis:names:tc:SAML:2.0:assertion"
#define SAMLP_NAMESPACE "urn:oasis:names:tc:SAML:2.0:protocol"

#define XENC_NAMESPACE   "http://www.w3.org/2001/04/xmlenc#"
#define DSIG_NAMESPACE   "http://www.w3.org/2000/09/xmldsig#"

static Arc::LogStream logcerr(std::cerr);

static Arc::Service* get_service(Arc::Config *cfg,Arc::ChainContext*) {
    return new Service_AA(cfg);
}

Arc::MCC_Status Service_AA::make_soap_fault(Arc::Message& outmsg) {
  Arc::PayloadSOAP* outpayload = new Arc::PayloadSOAP(ns_,true);
  Arc::SOAPFault* fault = outpayload?outpayload->Fault():NULL;
  if(fault) {
    fault->Code(Arc::SOAPFault::Sender);
    fault->Reason("Failed processing request");
  };
  outmsg.Payload(outpayload);
  return Arc::MCC_Status(Arc::STATUS_OK);
}

Arc::MCC_Status Service_AA::process(Arc::Message& inmsg,Arc::Message& outmsg) {

  //The following xml structure is according to the definition in lasso.
  //The RemoteProviderID here is meaningless. The useful information is <RemoteNameIdentifier>, which
  //is got from transport level authentication
  std::string identity_aa("<Identity xmlns=\"http://www.entrouvert.org/namespaces/lasso/0.0\" Version=\"2\">\
    <Federation xmlns:saml=\"urn:oasis:names:tc:SAML:2.0:assertion\"\
       RemoteProviderID=\"https://sp.com/SAML\"\
       FederationDumpVersion=\"2\">\
      <RemoteNameIdentifier>\
        <saml:NameID Format=\"urn:oasis:names:tc:SAML:1.1:nameid-format:X509SubjectName\"></saml:NameID>\
      </RemoteNameIdentifier>\
    </Federation>\
   </Identity>");
  Arc::XMLNode id_node(identity_aa);

  //Set the <saml:NameID> inside <RemoteNameIdentifier> by using the DN of peer certificate, 
  //which is the authentication result during tls;
  //<saml:NameID> could also be set by using the authentication result in message level
  //authentication, such as saml token profile in WS-Security.
  //Note: since the <saml:NameID> is obtained from transport level authentication, the request 
  //(with the peer certificate) is exactly the principal inside the <saml:NameID>, which means
  //the request can not be like ServiceProvider in WebSSO scenario which gets the <AuthnQuery>
  //result from IdP, and uses the principal (not the principal of ServiceProvider) in <AuthnQuery> 
  //result to initiate a <AttributeQuery> to AA.
  //Here the scenario is "SAML Attribute Self-Query Deployment Profile for X.509 Subjects" inside 
  //document "SAML V2.0 Deployment Profiles for X.509 Subjects"

  std::string peer_dn = inmsg.Attributes()->get("TLS:PEERDN");
  id_node["Federation"]["RemoteNameIdentifier"]["saml:NameID"] = peer_dn;
  id_node.GetXML(identity_aa);


  // Extracting payload
  Arc::PayloadSOAP* inpayload = NULL;
  try {
    inpayload = dynamic_cast<Arc::PayloadSOAP*>(inmsg.Payload());
  } catch(std::exception& e) { };
  if(!inpayload) {
    logger.msg(Arc::ERROR, "input is not SOAP");
    return make_soap_fault(outmsg);
  }

  Arc::XMLNode attrqry;
  attrqry = (*inpayload).Body().Child(0);

  std::string query_idname = "ID";
  std::string cafile = "./ca.pem";
  std::string capath = "";
  Arc::XMLSecNode attrqry_secnode(attrqry);
  if(attrqry_secnode.VerifyNode(query_idname, cafile, capath)) {
    logger.msg(Arc::INFO, "Succeeded to verify the signature under <AttributeQuery/>");
  }
  else {     
    logger.msg(Arc::ERROR, "Failed to verify the signature under <AttributeQuery/>");
    return Arc::MCC_Status();
  }

  Arc::NS ns;
  ns["saml"] = SAML_NAMESPACE;
  ns["samlp"] = SAMLP_NAMESPACE;
  //reset the namespaces of <AttributeQuery/> in case the prefix is not the same as "saml" and "samlp"
  attrqry.Namespaces(ns);

  Arc::XMLNode issuer = attrqry["saml:Issuer"];

  //Compare the <saml:NameID> inside the <AttributeQuery> message with the <saml:NameID>
  //which has been got from the former authentication
  //More complicated processing should be considered, according to 3.3.4 in SAML core specification
  Arc::XMLNode subject = attrqry["saml:Subject"];
  std::string name_id = (std::string)(subject["saml:NameID"]);
  if(name_id == peer_dn) {
    logger.msg(Arc::INFO, "The NameID inside request is the same as the NameID from the tls authentication: %s", peer_dn.c_str());
  }
  else {
    logger.msg(Arc::INFO, "The NameID inside request is: %s; not the same as the NameID from the tls authentication: %s",\
      name_id.c_str(), peer_dn.c_str());
    return Arc::MCC_Status();
  }

  //Get the <Attribute>s from <AttributeQuery> message, which is required by request; 
  //AA will only return those <Attribute> which is required by request
  std::vector<Arc::XMLNode> attributes;
  for(int i=0;; i++) {
    Arc::XMLNode cn =  attrqry["saml:Attribute"][i];
    if(!cn) break;
    attributes.push_back(cn);
  }

  //TODO: Need to check whether the remote provider ID is in the identity federation set.

  //AA: Generate a response 
  //AA will try to query local attribute database, intersect the attribute result from
  //database and the attribute requirement from the request
  //Then, insert those <Attribute> into response

  //TODO: access the local attribute database, use the <NameID> as searching key
  
  std::vector<std::string> attribute_name_list;
  for(int i=0; i<attributes.size(); i++) {
    Arc::XMLNode nd = attributes[i];
    std::string str = (std::string)(nd.Attribute("Name"));
    if(!str.empty()) { 
      attribute_name_list.push_back(str); 
    }
    else {
      logger.msg(Arc::ERROR, "There should be Name attribute in request's <Attribute> node");
      return Arc::MCC_Status();
    }
  }

  //TODO: Compare the attribute name from database result and the attribute_name_list,
  //Only use the intersect as the response
  
  //Compose <saml:Response/>
  Arc::XMLNode attr_response(ns, "samlp:Response");
  std::string aa_name("https://idp.com/SAML"); //TODO
  std::string response_id = Arc::UUID();
  attr_response.NewAttribute("ID") = response_id;
  std::string responseto_id = (std::string)(attrqry.Attribute("ID"));
  attr_response.NewAttribute("InResponseTo") = responseto_id;
  Arc::Time t;
  std::string current_time = t.str(Arc::UTCTime);
  attr_response.NewAttribute("IssueInstant") = current_time;
  attr_response.NewAttribute("Version") = std::string("2.0");

  attr_response.NewChild("saml:Issuer") = aa_name;
 
  //<samlp:Status/> 
  Arc::XMLNode status = attr_response.NewChild("samlp:Status");
  Arc::XMLNode statuscode = status.NewChild("samlp:StatusCode");
  std::string statuscode_value = "urn:oasis:names:tc:SAML:2.0:status:Success"; //TODO
  statuscode.NewAttribute("Value") = statuscode_value;

  //<saml:Assertion/>
  Arc::XMLNode assertion = attr_response.NewChild("saml:Assertion");
  assertion.NewAttribute("Version") = std::string("2.0");
  std::string assertion_id = Arc::UUID();
  assertion.NewAttribute("ID") = assertion_id;
  Arc::Time t1;
  std::string current_time1 = t1.str(Arc::UTCTime);
  assertion.NewAttribute("IssueInstant") = current_time1;

  //<saml:Issuer/>
  assertion.NewChild("saml:Issuer") = aa_name;

  //<saml:Subject/>
  //<saml:Subject/> is the same as the one in request
  assertion.NewChild(subject);
 
  //<saml:Conditions>
  Arc::XMLNode conditions = assertion.NewChild("saml:Conditions"); 
  Arc::XMLNode audience_restriction = conditions.NewChild("saml:AudienceRestriction");
  std::string client_name("https://sp.com/SAML"); //TODO
  audience_restriction.NewChild("saml:Audience") = client_name; 
 
  //<saml:AttributeStatement/> 
  Arc::XMLNode attr_statement = assertion.NewChild("saml:AttributeStatement");

  //<saml:Attribute/>
  //The following is just one <Attribute> result for test. The real <Attribute> 
  //should be compose according to the database searching result
  Arc::XMLNode attribute = attr_statement.NewChild("saml:Attribute");
  Arc::XMLNode attr_value = attribute.NewChild("saml:AttributeValue");
  attribute.NewAttribute("Name") = std::string("urn:oid:1.3.6.1.4.1.5923.1.1.1.6");
  attribute.NewAttribute("NameFormat")= std::string("urn:oasis:names:tc:SAML:2.0:attrname-format:uri");
  attribute.NewAttribute("FriendlyName") = std::string("eduPersonPrincipalName");
  attr_value = std::string("RoleA");
  //Add one or more <AttributeValue> into <Attribute>
  //Add one or more <Attribute> into <Assertion>

  Arc::XMLSecNode assertion_secnd(assertion);
  std::string assertion_idname("ID");
  std::string inclusive_namespaces = "saml ds";
  assertion_secnd.AddSignatureTemplate(assertion_idname, Arc::XMLSecNode::RSA_SHA1, inclusive_namespaces);
  std::string privkey("key.pem");
  std::string cert("cert.pem");
  if(assertion_secnd.SignNode(privkey,cert)) {
    std::cout<<"Succeed to sign the signature under <saml:Assertion/>"<<std::endl;
    std::string str;
    assertion_secnd.GetXML(str);
    std::cout<<"Signed node: "<<std::endl<<str<<std::endl;
  }

  Arc::XMLSecNode attr_response_secnd(attr_response);
  std::string attr_response_idname("ID");
  attr_response_secnd.AddSignatureTemplate(attr_response_idname, Arc::XMLSecNode::RSA_SHA1);
  if(attr_response_secnd.SignNode(privkey,cert)) {
    std::cout<<"Succeed to sign the signature under <samlp:Response/>"<<std::endl;
    std::string str;
    attr_response_secnd.GetXML(str);
    std::cout<<"Signed node: "<<std::endl<<str<<std::endl;
  }

  //Put the <samlp:Response/> into soap body.
  Arc::NS soap_ns;
  Arc::SOAPEnvelope envelope(soap_ns);
  envelope.NewChild(attr_response);

  Arc::PayloadSOAP *outpayload = new Arc::PayloadSOAP(envelope);

  std::string tmp;
  outpayload->GetXML(tmp);
  std::cout<<"Output payload: ----"<<tmp<<std::endl;

  outmsg.Payload(outpayload);
  return Arc::MCC_Status(Arc::STATUS_OK);
}

Service_AA::Service_AA(Arc::Config *cfg):Service(cfg), logger_(Arc::Logger::rootLogger, "AA_Service") {
  logger_.addDestination(logcerr);
  Arc::init_xmlsec();
}

Service_AA::~Service_AA(void) {
  Arc::final_xmlsec();
}

bool Service_AA::get_roles(std::vector<std::string>& fqans, std::string& userid, std::string& role, Arc::XMLNode& config) {
  Arc::QueryArrayResult attributes;
  std::vector<std::string> sqlargs;
  sqlargs.push_back(role);
  sqlargs.push_back(userid);
  std::string idofsqlset("Role");
  bool res;
  res = get_attributes(attributes, idofsqlset, sqlargs, config);
  if(!res) return res;
  for(int i = 0; i< attributes.size(); i++) {
    std::vector<std::string> item = attributes[i];
    int num = item.size();
    std::string fqan;
    if(num == 2) {
      fqan = item[0] + "/Role=" + item[1];
    }
    else if(num == 4) {
      fqan = item[2] + "::" + item[0] + "=" + item[1];
    }
    fqans.push_back(fqan);
  }
  return true;
}

bool Service_AA::get_attributes(Arc::QueryArrayResult& attributes, std::string& idofsqlset, std::vector<std::string>& sqlargs, Arc::XMLNode& config) {
  Arc::XMLNode nd;
  nd = config["aa:Database"];
  std::string server, dbname, user, password, portstr;
  int port;
  server = (std::string)(nd.Attribute("ip"));
  dbname = (std::string)(nd.Attribute("dbname"));
  user = (std::string)(nd.Attribute("user"));
  password = (std::string)(nd.Attribute("password"));
  portstr = (std::string)(nd.Attribute("port"));
  port = atoi((portstr.c_str()));
  
  Arc::MySQLDatabase mydb(server, port);
  bool res = false;
  res = mydb.connect(dbname,user,password);
  if(res == false) {std::cerr<<"Can't establish connection to mysql database"<<std::endl; return false;}

  Arc::MySQLQuery myquery(&mydb);
  std::cout<<"Is connected? "<<mydb.isconnected()<<std::endl;

  std::string querystr;
  for(int i = 0;; i++) {
    Arc::XMLNode cn = nd["aa:SQLSet"][i];
    if(!cn) break;
    if(((std::string)(cn.Attribute("name"))) == idofsqlset) {
      for(int k = 0;; k++) {
        Arc::XMLNode scn = cn["aa:SQL"][k];
        if(!scn) break;
        myquery.get_array(querystr, attributes, sqlargs);
      }
    }
  }
  std::cout<<"Get an result array with "<<attributes.size()<<" rows"<<std::endl;
  return true;
}

} // namespace ArcSec

service_descriptors ARC_SERVICE_LOADER = {
    { "aa.service", 0, &ArcSec::get_service },
    { NULL, 0, NULL }
};

