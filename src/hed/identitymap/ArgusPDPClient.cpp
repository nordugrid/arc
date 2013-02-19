#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <fstream>
#include <vector>
#include <arc/Logger.h>
#include <arc/StringConv.h>
#include <arc/URL.h>
#include <arc/GUID.h>
#include <arc/message/MCCLoader.h>
#include <arc/message/PayloadSOAP.h>
#include <arc/message/SecAttr.h>
#include <arc/communication/ClientInterface.h>
#include <arc/credential/Credential.h>

#include "ArgusPDPClient.h"
#include "ArgusXACMLConstant.h"

static const char XACML_DATATYPE_FQAN[]= "http://glite.org/xacml/datatype/fqan";

#define SAML_NAMESPACE "urn:oasis:names:tc:SAML:2.0:assertion"
#define SAMLP_NAMESPACE "urn:oasis:names:tc:SAML:2.0:protocol"
#define XACML_SAMLP_NAMESPACE "urn:oasis:names:tc:xacml:2.0:profile:saml2.0:v2:schema:protocol"
//#define XACML_SAMLP_NAMESPACE "urn:oasis:xacml:2.0:saml:protocol:schema:os"


static void xacml_create_request(Arc::XMLNode& request) {
    Arc::NS ns;
    ns["xacml-ctx"]="urn:oasis:names:tc:xacml:2.0:context:schema:os";
    Arc::XMLNode node(ns, "xacml-ctx:Request");
    node.New(request);
    return;
}

static Arc::XMLNode xacml_request_add_element(Arc::XMLNode& request_node, const std::string& element_name) {
    std::string elm_name = "xacml-ctx:";
    elm_name.append(element_name);
    Arc::XMLNode element = request_node.NewChild(elm_name);
    return element;
}

static Arc::XMLNode xacml_element_add_attribute(Arc::XMLNode& element_node, const std::string& attribute,
    const std::string& data_type, const std::string& id, const std::string& issuer) {
   Arc::XMLNode attr = element_node.NewChild("xacml-ctx:Attribute");
   attr.NewAttribute("DataType") = data_type;
   attr.NewAttribute("AttributeId") = id;
   if(!issuer.empty()) attr.NewAttribute("Issuer") = issuer;

   attr.NewChild("xacml-ctx:AttributeValue") = attribute;

   return attr;
}

static Arc::XMLNode xacml_element_add_attribute(Arc::XMLNode& element_node, const std::list<std::string>& attributes,
    const std::string& data_type, const std::string& id, const std::string& issuer) {
   Arc::XMLNode attr = element_node.NewChild("xacml-ctx:Attribute");
   attr.NewAttribute("DataType") = data_type;
   attr.NewAttribute("AttributeId") = id;
   if(!issuer.empty()) attr.NewAttribute("Issuer") = issuer;

   for(std::list<std::string>::const_iterator it = attributes.begin(); it!=attributes.end(); ++it) {
     attr.NewChild("xacml-ctx:AttributeValue") = *it;
   }

   return attr;
}



static Arc::Plugin* get_sechandler(Arc::PluginArgument* arg) {
    ArcSec::SecHandlerPluginArgument* shcarg = arg?dynamic_cast<ArcSec::SecHandlerPluginArgument*>(arg):NULL;
    if(!shcarg) return NULL;
    ArcSec::ArgusPDPClient* plugin = new ArcSec::ArgusPDPClient((Arc::Config*)(*shcarg),arg);
    if(!plugin) return NULL;
    if(!(*plugin)) { delete plugin; return NULL;};
    return plugin;
}

Arc::PluginDescriptor ARC_PLUGINS_TABLE_NAME[] = {
    { "arguspdpclient.map", "HED:SHC", NULL, 0, &get_sechandler},
    { NULL, NULL, NULL, 0, NULL }
}; 

namespace ArcSec {

class pep_ex {
    public:
        std::string desc;
        pep_ex(const std::string& desc_):desc(desc_) {};
};

static std::string path_to_x500(const std::string& path) {
    class url_ex: public Arc::URL {
      public:
        static std::string Path2BaseDN(const std::string& path) {
            return Arc::URL::Path2BaseDN(path);
        };
    };
    return url_ex::Path2BaseDN(path);
}


std::string flatten_fqan(const std::string& wfqan) {
  const std::string vo_tag("/VO=");
  const std::string group_tag("/Group=");
  std::string fqan;
  std::string::size_type pos1 = 0;
  std::string::size_type pos2 = 0;
  if(wfqan.substr(0,vo_tag.length()) != vo_tag) return fqan;
  for(;;) {
    pos1 = wfqan.find(group_tag,pos2);
    if(pos1 == std::string::npos) break;
    pos2 = wfqan.find("/",pos1+1);
    if(pos2 == std::string::npos) {
      fqan += "/" + wfqan.substr(pos1+group_tag.length());
      break;
    };
    fqan += "/" + wfqan.substr(pos1+group_tag.length(),pos2-pos1-group_tag.length());
  };
  return fqan;
}

Arc::Logger ArgusPDPClient::logger(Arc::Logger::getRootLogger(), "SecHandler.Argus");

std::string xacml_decision_to_string(xacml_decision_t decision) {
    switch(decision) {
        case XACML_DECISION_DENY: return "DENY";
        case XACML_DECISION_PERMIT: return "PERMIT";
        case XACML_DECISION_INDETERMINATE: return "INDETERMINATE";
        case XACML_DECISION_NOT_APPLICABLE: return "NOT APPLICABLE";
    };
    return "UNKNOWN";
}

/* extract the elements from the configuration file */
ArgusPDPClient::ArgusPDPClient(Arc::Config *cfg,Arc::PluginArgument* parg):ArcSec::SecHandler(cfg,parg), conversion(conversion_emi) {  
    valid_ = false;
    accept_mapping = false;
    accept_notapplicable = false;
    logger.setThreshold(Arc::DEBUG);
    pdpdlocation = (std::string)(*cfg)["PDPD"];  
    if(pdpdlocation.empty()) {
        logger.msg(Arc::ERROR, "PDPD location is missing");
        return;
    }
    logger.msg(Arc::DEBUG, "PDPD location: %s",pdpdlocation);

    std::string conversion_str = (std::string)(*cfg)["Conversion"];

    if(conversion_str == "subject") {
        logger.msg(Arc::DEBUG, "Conversion mode is set to SUBJECT");
        conversion = conversion_subject;
    } else if(conversion_str == "cream") {
        logger.msg(Arc::DEBUG, "Conversion mode is set to CREAM");
        conversion = conversion_cream;
    } else if(conversion_str == "emi") {
        logger.msg(Arc::DEBUG, "Conversion mode is set to EMI");
        conversion = conversion_emi;
    } else if(!conversion_str.empty()) {
        logger.msg(Arc::INFO, "Unknown conversion mode %s, using default", conversion_str);
    }

    Arc::XMLNode filter = (*cfg)["Filter"];
    if((bool)filter) {
        Arc::XMLNode select_attr = filter["Select"];
        Arc::XMLNode reject_attr = filter["Reject"];
        for(;(bool)select_attr;++select_attr) select_attrs.push_back((std::string)select_attr);
        for(;(bool)reject_attr;++reject_attr) reject_attrs.push_back((std::string)reject_attr);
    };

    capath = (std::string)(*cfg)["CACertificatesDir"];
    keypath = (std::string)(*cfg)["KeyPath"];
    certpath = (std::string)(*cfg)["CertificatePath"];
    std::string proxypath = (std::string)(*cfg)["ProxyPath"];
    if(!proxypath.empty()) {
        keypath = proxypath;
        certpath = proxypath;
    };

    std::string mapping_str = (std::string)(*cfg)["AcceptMapping"];
    if((mapping_str == "1") || (mapping_str == "true")) accept_mapping = true;

    std::string notapplicable_str = (std::string)(*cfg)["AcceptNotApplicable"];
    if((notapplicable_str == "1") || (notapplicable_str == "true")) accept_notapplicable = true;

    valid_ = true;
}

ArgusPDPClient::~ArgusPDPClient(void) {
}


static bool contact_pdp(Arc::ClientSOAP* client, const std::string& pdpdlocation, const std::string& certpath, 
    Arc::Logger& logger, Arc::XMLNode& request, Arc::XMLNode& response) {

    bool ret = false;

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

    Arc::Credential cred(certpath, "", "", "");
    std::string local_dn_str = cred.GetDN();
    std::string local_dn = Arc::convert_to_rdn(local_dn_str);
    std::string issuer_name = local_dn;
    authz_query.NewChild("saml:Issuer") = issuer_name;
    authz_query.NewAttribute("InputContextOnly") = std::string("false");
    authz_query.NewAttribute("ReturnContext") = std::string("true");
    authz_query.NewChild(request);

    Arc::NS req_ns;
    Arc::SOAPEnvelope req_env(req_ns);
    req_env.NewChild(authz_query);

    Arc::PayloadSOAP req(req_env);
    Arc::PayloadSOAP* resp = NULL;
    Arc::MCC_Status status = client->process(&req, &resp);
    if(!status) {
      logger.msg(Arc::ERROR, "Failed to contact PDP server: %s", pdpdlocation);
    }
    if(resp == NULL) {
      logger.msg(Arc::ERROR,"There was no SOAP response return from PDP server: %s", pdpdlocation);
    }
    else {
      std::string str;
      resp->GetXML(str);
      logger.msg(Arc::DEBUG, "SOAP response: %s", str);

      //The authorization query response from argus pdp server is like the following
/*
      <saml2p:Response xmlns:saml2p="urn:oasis:names:tc:SAML:2.0:protocol" ID="_2d1186642b32e202cb99bc94eb9b319c" InResponseTo="9b79bc42-555a-4029-a96a-ee4cb46be0e5" Version="2.0">
         <saml2p:Status>
            <saml2p:StatusCode Value="urn:oasis:names:tc:SAML:2.0:status:Success"/>
         </saml2p:Status>
         <saml2:Assertion xmlns:saml2="urn:oasis:names:tc:SAML:2.0:assertion" ID="_4ecf88ff3971e5d7de195e600529d632" Version="2.0">
            <saml2:Issuer Format="urn:oasis:names:tc:SAML:2.0:nameid-format:entity">http://localhost.localdomain/pdp</saml2:Issuer>
            <saml2:Statement xmlns:xacml-saml="urn:oasis:names:tc:xacml:2.0:profile:saml2.0:v2:schema:assertion" xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xsi:type="xacml-saml:XACMLAuthzDecisionStatementType">
               <xacml-ctx:Request xmlns:xacml-ctx="urn:oasis:names:tc:xacml:2.0:context:schema:os">
                 ...duplication of the request...
               </xacml-ctx:Request>
               <xacml-context:Response xmlns:xacml-context="urn:oasis:names:tc:xacml:2.0:context:schema:os">
                  <xacml-context:Result ResourceId="https://127.0.0.1:60000/echo">
                     <xacml-context:Decision>NotApplicable</xacml-context:Decision>
                     <xacml-context:Status>
                        <xacml-context:StatusCode Value="urn:oasis:names:tc:xacml:1.0:status:ok"/>
                     </xacml-context:Status>
                  </xacml-context:Result>
               </xacml-context:Response>
            </saml2:Statement>
         </saml2:Assertion>
      </saml2p:Response>
*/

      Arc::XMLNode respxml = (*resp)["saml2p:Response"]["saml2:Assertion"]["saml2:Statement"]["xacml-context:Response"];
      if((bool)respxml) respxml.New(response);

      delete resp;
      ret = true;
    }
    return ret;
}


SecHandlerStatus ArgusPDPClient::Handle(Arc::Message* msg) const {
    int rc = 0;
    bool res = true;
    Arc::XMLNode request;
    Arc::XMLNode response;
    std::list<Arc::XMLNode> requests;

    std::string subject , resource , action;
    Arc::XMLNode secattr;   

    try{

      // Create xacml
      if(conversion == conversion_subject) {
        // Extract the user subject according to RFC2256 format
        std::string dn = msg->Attributes()->get("TLS:IDENTITYDN"); 
        while (dn.rfind("/") != std::string::npos) {
          std::string s = dn.substr(dn.rfind("/")+1,dn.length()) ;
          subject = subject + s + ",";
          dn = dn.substr(0, dn.rfind("/")) ;
        }; 
        subject = subject.substr(0, subject.length()-1);
        if(resource.empty()) resource = "ANY";
        if(action.empty()) action = "ANY";
        rc = create_xacml_request(request,subject.c_str(),resource.c_str(),action.c_str());
        if((bool)request) requests.push_back(request);
      }
      else if(conversion == conversion_cream) {
        std::list<Arc::MessageAuth*> auths;
        auths.push_back(msg->Auth());
        auths.push_back(msg->AuthContext());
        Arc::PayloadSOAP* payload = NULL;
        try {
          payload = dynamic_cast<Arc::PayloadSOAP*>(msg->Payload());
        } catch(std::exception& e) { };
        //if(!payload) throw pep_ex("No SOAP in message");
        if(payload) {
          rc = create_xacml_request_cream(request,auths,msg->Attributes(),payload->Child(0));
        } else {
          // For HTTP operations
          rc = create_xacml_request_cream(request,auths,msg->Attributes(),Arc::XMLNode());
        }
        if((bool)request) requests.push_back(request);
      }
      else if(conversion == conversion_emi) {
        std::list<Arc::MessageAuth*> auths;
        auths.push_back(msg->Auth());
        auths.push_back(msg->AuthContext());
        Arc::PayloadSOAP* payload = NULL;
        try {
          payload = dynamic_cast<Arc::PayloadSOAP*>(msg->Payload());
        } catch(std::exception& e) { };
        //if(!payload) throw pep_ex("No SOAP in message");
        if(payload) {
          rc = create_xacml_request_emi(request,auths,msg->Attributes(),payload->Child(0));
        } else {
          // For HTTP operations
          rc = create_xacml_request_emi(request,auths,msg->Attributes(),Arc::XMLNode());
        }
        if((bool)request) requests.push_back(request);
      } 
      else {
        throw pep_ex("Unsupported conversion mode " + Arc::tostring(conversion));
      }
      if (rc != 0) {
        throw pep_ex("Failed to create XACML request(s): " + Arc::tostring(rc));
      }

      // Contact PDP server
      std::string local_id; 
      xacml_decision_t decision = XACML_DECISION_INDETERMINATE; 
      // Simple combining algorithm. At least one deny means deny. If none, then at 
      // least one permit means permit. Otherwise deny. TODO: configurable.
      logger.msg(Arc::DEBUG, "Have %i requests to process", requests.size());

      logger.msg(Arc::INFO, "Creating a client to Argus PDP service");
      Arc::URL pdp_url(pdpdlocation);
      Arc::MCCConfig mcc_cfg;
      mcc_cfg.AddPrivateKey(keypath);
      mcc_cfg.AddCertificate(certpath);
      mcc_cfg.AddCADir(capath);
      Arc::ClientSOAP client(mcc_cfg,pdp_url,60);

      for(std::list<Arc::XMLNode>::iterator it = requests.begin(); it != requests.end(); it++) {
        Arc::XMLNode req = *it;

        std::string str;
        req.GetXML(str);
        logger.msg(Arc::DEBUG, "XACML authorisation request: %s", str);

        bool res = contact_pdp(&client, pdpdlocation, certpath, logger, request, response);
        if (!res) {
          throw pep_ex(std::string("Failed to process XACML request"));
        }   
        if (!response) {
          throw pep_ex("XACML response is empty");
        }

        response.GetXML(str);
        logger.msg(Arc::DEBUG, "XACML authorisation response: %s", str);

        // Extract the local user name from the response to be mapped to the GID
        for (int cn = 0;; ++cn) {
          Arc::XMLNode cnode = response.Child(cn);
          if (!cnode) break;
          std::string authz_res = (std::string)(cnode["xacml-context:Decision"]);
          if(authz_res.empty()) break;
          if(authz_res == "Permit") decision =  XACML_DECISION_PERMIT;
          else if(authz_res == "Deny") decision = XACML_DECISION_DENY;
          else if(authz_res == "NotApplicable") decision = XACML_DECISION_NOT_APPLICABLE;
          if(decision == XACML_DECISION_DENY) break;

/*
<xacml:Obligation ObligationId="http://www.example.com/" FulfillOn="Permit">
   <xacml:AttributeAssignment DataType="http://www.example.com/" AttributeId="http://www.example.com/">
      <!--any element-->
   </xacml:AttributeAssignment>
</xacml:Obligation>
*/
          for(int n = 0;; ++n) {
            Arc::XMLNode scn = cnode.Child(n);
            if(!scn) break;
            if(!MatchXMLName(scn, "Obligations")) continue;
            for(int m = 0;; ++m) {
              Arc::XMLNode sscn = scn.Child(m);
              if(!sscn) break;
              std::string id = (std::string)sscn;
              local_id = id.empty() ? "":id;
            }
          }

        }
        if(decision == XACML_DECISION_DENY) break;
      }


      if ((decision != XACML_DECISION_PERMIT) && 
          (decision != XACML_DECISION_NOT_APPLICABLE)) {
        if(conversion == conversion_subject) {
          logger.msg(Arc::INFO,"%s is not authorized to do action %s in resource %s ",
                     subject, action, resource);
        } else {
          logger.msg(Arc::INFO,"Not authorized");
        }
        throw pep_ex("The reached decision is: " + xacml_decision_to_string(decision));
      }
      else if((decision == XACML_DECISION_NOT_APPLICABLE) && (accept_notapplicable == false)) {
        logger.msg(Arc::INFO,"Not authorized");
        throw pep_ex("The reached decision is: " + xacml_decision_to_string(decision) + ". But this service will treat NotApplicable decision as reason to deny request");
      }
     
      if(accept_mapping && !local_id.empty()) {
        logger.msg(Arc::INFO,"Grid identity is mapped to local identity '%s'", local_id);
        msg->Attributes()->set("SEC:LOCALID", local_id);
      }

    } catch (pep_ex& e) {
      logger.msg(Arc::ERROR,"%s",e.desc);
      res = false;
    }

    return res;
}

int ArgusPDPClient::create_xacml_request(Arc::XMLNode& request,const char * subjectid, const char * resourceid, const char * actionid) const {
    xacml_create_request(request);

    Arc::XMLNode subject = xacml_request_add_element(request, "Subject");
    std::string subject_attribute = path_to_x500(subjectid);
    Arc::XMLNode subject_attr = xacml_element_add_attribute(subject, subject_attribute, 
        XACML_DATATYPE_X500NAME, XACML_SUBJECT_ID, "");

    Arc::XMLNode resource = xacml_request_add_element(request, "Resource");
    Arc::XMLNode resource_attr = xacml_element_add_attribute(resource, resourceid,      
        XACML_DATATYPE_STRING, XACML_RESOURCE_ID, "");

    Arc::XMLNode action = xacml_request_add_element(request, "Action");
    Arc::XMLNode action_attr = xacml_element_add_attribute(action, actionid,
        XACML_DATATYPE_STRING, XACML_ACTION_ID, "");

    return 0;  
}


// This part is ARC vs CREAM specific.
// In a future such mapping should be pluggable or configurable or done by service
static const std::string BES_FACTORY_NAMESPACE("http://schemas.ggf.org/bes/2006/08/bes-factory");
static const std::string BES_MANAGEMENT_NAMESPACE("http://schemas.ggf.org/bes/2006/08/bes-management");
static const std::string BES_ARC_NAMESPACE("http://www.nordugrid.org/schemas/a-rex");
static const std::string DELEG_ARC_NAMESPACE("http://www.nordugrid.org/schemas/delegation");
static const std::string WSRF_NAMESPACE("http://docs.oasis-open.org/wsrf/rp-2");

static std::string get_cream_action(Arc::XMLNode op, Arc::Logger& logger) {
  logger.msg(Arc::DEBUG,"Converting to CREAM action - namespace: %s, operation: %s",op.Namespace(),op.Name());
    if(MatchXMLNamespace(op,BES_FACTORY_NAMESPACE)) {
        if(MatchXMLName(op,"CreateActivity"))
            return "http://glite.org/xacml/action/ce/job/submit";
        if(MatchXMLName(op,"GetActivityStatuses"))
            return "http://glite.org/xacml/action/ce/job/get-info";
        if(MatchXMLName(op,"TerminateActivities"))
            return "http://glite.org/xacml/action/ce/job/terminate";
        if(MatchXMLName(op,"GetActivityDocuments"))
            return "http://glite.org/xacml/action/ce/job/get-info";
        if(MatchXMLName(op,"GetFactoryAttributesDocument"))
            return "http://glite.org/xacml/action/ce/get-info";
        return "";
    } else if(MatchXMLNamespace(op,BES_MANAGEMENT_NAMESPACE)) {
        if(MatchXMLName(op,"StopAcceptingNewActivities"))
            return "";
        if(MatchXMLName(op,"StartAcceptingNewActivities"))
            return "";
    } else if(MatchXMLNamespace(op,BES_ARC_NAMESPACE)) {
        if(MatchXMLName(op,"ChangeActivityStatus"))
            return "http://glite.org/xacml/action/ce/job/manage";
        if(MatchXMLName(op,"MigrateActivity"))
            return "http://glite.org/xacml/action/ce/job/manage";
        if(MatchXMLName(op,"CacheCheck"))
            return "http://glite.org/xacml/action/ce/get-info";
        return "";
    } else if(MatchXMLNamespace(op,DELEG_ARC_NAMESPACE)) {
        if(MatchXMLName(op,"DelegateCredentialsInit"))
            return "http://glite.org/xacml/action/ce/delegation/manage";
        if(MatchXMLName(op,"UpdateCredentials"))
            return "http://glite.org/xacml/action/ce/delegation/manage";
        return "";
    } else if(MatchXMLNamespace(op,WSRF_NAMESPACE)) {
        return "http://glite.org/xacml/action/ce/get-info";
    }
    // http://glite.org/xacml/action/ce/job/submit
    // *http://glite.org/xacml/action/ce/job/terminate
    // *http://glite.org/xacml/action/ce/job/get-info
    // *http://glite.org/xacml/action/ce/job/manage
    // http://glite.org/xacml/action/ce/lease/get-info
    // http://glite.org/xacml/action/ce/lease/manage
    // *http://glite.org/xacml/action/ce/get-info
    // http://glite.org/xacml/action/ce/delegation/get-info
    // *http://glite.org/xacml/action/ce/delegation/manage
    // http://glite.org/xacml/action/ce/subscription/get-info
    // http://glite.org/xacml/action/ce/subscription/manage
    return "";
}

static std::string get_cream_action_http(const std::string& method, Arc::Logger& logger) {
    if(method == "GET") {
        return "http://glite.org/xacml/action/ce/job/get-info";
    } else if(method == "PUT") {
        return "http://glite.org/xacml/action/ce/job/submit";
    }
    return "";
}

static std::string get_sec_attr(std::list<Arc::MessageAuth*> auths, const std::string& sid, const std::string& aid) {
    for(std::list<Arc::MessageAuth*>::iterator a = auths.begin(); a != auths.end(); ++a) {
        Arc::SecAttr* sa = (*a)->get(sid);
        if(!sa) continue;
        std::string str = sa->get(aid);
        if(!str.empty()) return str;
    }
    return "";
}

static std::list<std::string> get_sec_attrs(std::list<Arc::MessageAuth*> auths, const std::string& sid, const std::string& aid) {
    for(std::list<Arc::MessageAuth*>::iterator a = auths.begin(); a != auths.end(); ++a) {
        Arc::SecAttr* sa = (*a)->get(sid);
        if(!sa) continue;
        std::list<std::string> strs = sa->getAll(aid);
        if(!strs.empty()) return strs;
    }
    return std::list<std::string>();
}

int ArgusPDPClient::create_xacml_request_cream(Arc::XMLNode& request, std::list<Arc::MessageAuth*> auths,  
    Arc::MessageAttributes* attrs, Arc::XMLNode operation) const {
    logger.msg(Arc::DEBUG,"Doing CREAM request");
    class ierror {
     public:
      std::string desc;
      ierror(const std::string& err):desc(err) { };
    };
    try {
      xacml_create_request(request);

      // Environment
      Arc::XMLNode environment = xacml_request_add_element(request, "Environment");

      std::string env_attr_id = XACML_GLITE_ATTRIBUTE_PROFILE_ID; //"http://glite.org/xacml/attribute/profile-id";
      std::string env_attr_value = "http://glite.org/xacml/profile/grid-ce/1.0";
      logger.msg(Arc::DEBUG,"Adding profile-id value: %s", env_attr_value);
      xacml_element_add_attribute(environment, env_attr_value,
          XACML_DATATYPE_ANYURI, env_attr_id, "");


      // Subject
      Arc::XMLNode subject = xacml_request_add_element(request, "Subject");

      std::string sub_attr_id = XACML_SUBJECT_ID; //"urn:oasis:names:tc:xacml:1.0:subject:subject-id";
      std::string sub_attr_value = get_sec_attr(auths, "TLS", "IDENTITY");
      if(sub_attr_value.empty()) throw ierror("Failed to extract TLS:IDENTITY");
      sub_attr_value = path_to_x500(sub_attr_value);
      logger.msg(Arc::DEBUG,"Adding subject-id value: %s", sub_attr_value);
      xacml_element_add_attribute(subject, sub_attr_value,
          XACML_DATATYPE_X500NAME, sub_attr_id, "");

      std::string iss_attr_id = XACML_GLITE_ATTRIBUTE_SUBJECT_ISSUER; //"http://glite.org/xacml/attribute/subject-issuer";
      std::string iss_attr_value = get_sec_attr(auths, "TLS", "CA");
      if(iss_attr_value.empty()) throw ierror("Failed to extract TLS:CA");
      iss_attr_value = path_to_x500(iss_attr_value);
      logger.msg(Arc::DEBUG,"Adding subject-issuer value: %s", iss_attr_value);
      xacml_element_add_attribute(subject, iss_attr_value,
          XACML_DATATYPE_X500NAME, iss_attr_id, "");

      std::string vo_attr_id = XACML_GLITE_ATTRIBUTE_VIRTUAL_ORGANIZATION; // "http://glite.org/xacml/attribute/virtual-organization";
      std::list<std::string> vos = get_sec_attrs(auths, "TLS", "VO");
      for(std::list<std::string>::iterator vo = vos.begin(); vo!=vos.end(); ++vo) {
        logger.msg(Arc::DEBUG,"Adding virtual-organization value: %s",*vo);
      }
      if(vos.size()>0) xacml_element_add_attribute(subject, vos, XACML_DATATYPE_STRING, vo_attr_id, "");

      std::string fqan_attr_id = XACML_GLITE_ATTRIBUTE_FQAN; //"http://glite.org/xacml/attribute/fqan";
      std::string pfqan;
      std::list<std::string> fqans = get_sec_attrs(auths, "TLS", "VOMS");
      std::list<std::string> flatten_fqans;
      for(std::list<std::string>::iterator fqan = fqans.begin(); fqan!=fqans.end(); ++fqan) {
        std::string fqan_str = flatten_fqan(*fqan);
        if(fqan_str.empty()) continue;
        if(pfqan.empty()) pfqan = fqan_str;
        flatten_fqans.push_back(fqan_str);
        logger.msg(Arc::DEBUG,"Adding FQAN value: %s",fqan_str);
      }
      if(flatten_fqans.size()>0)xacml_element_add_attribute(subject, flatten_fqans, XACML_DATATYPE_FQAN, fqan_attr_id, "");

      // /voname=testers.eu-emi.eu/hostname=emitestbed07.cnaf.infn.it:15002/testers.eu-emi.eu/test3:test_ga=ciccio -> ?
      // /VO=testers.eu-emi.eu/Group=testers.eu-emi.eu/Group=test1 -> /testers.eu-emi.eu/test1

      if(!pfqan.empty()) {
        std::string pfqan_attr_id = XACML_GLITE_ATTRIBUTE_FQAN_PRIMARY; //"http://glite.org/xacml/attribute/fqan/primary";
        logger.msg(Arc::DEBUG,"Adding FQAN/primary value: %s",pfqan);
        xacml_element_add_attribute(subject, pfqan, XACML_DATATYPE_FQAN, pfqan_attr_id, "");
        // TODO: convert to VOMS FQAN?
      }

      std::string cert_attr_id = XACML_SUBJECT_KEY_INFO; //urn:oasis:names:tc:xacml:1.0:subject:key-info
      std::string cert_attr_value = get_sec_attr(auths, "TLS", "CERTIFICATE");
      if(cert_attr_value.empty()) throw ierror("Failed to create attribute key-info object");
      std::string chain_attr_value = get_sec_attr(auths, "TLS", "CERTIFICATECHAIN");
      chain_attr_value = cert_attr_value + "\n" + chain_attr_value;
      logger.msg(Arc::DEBUG,"Adding cert chain value: %s", chain_attr_value);
      xacml_element_add_attribute(subject, chain_attr_value, XACML_DATATYPE_STRING, cert_attr_id, "");


      // Resource
      Arc::XMLNode resource = xacml_request_add_element(request, "Resource");
      std::string res_attr_id = XACML_RESOURCE_ID; //"urn:oasis:names:tc:xacml:1.0:resource:resource-id";
      std::string res_attr_value = attrs->get("ENDPOINT");
      if(res_attr_value.empty()) throw ierror("Failed to extract ENDPOINT");
      logger.msg(Arc::DEBUG,"Adding resource-id value: %s", res_attr_value);
      xacml_element_add_attribute(resource, res_attr_value, XACML_DATATYPE_STRING, res_attr_id, "");


      // Action
      Arc::XMLNode action = xacml_request_add_element(request, "Action");
      std::string act_attr_id = XACML_ACTION_ID; //"urn:oasis:names:tc:xacml:1.0:action:action-id";
      std::string act_attr_value;
      if((bool)operation) {
        act_attr_value = get_cream_action(operation,logger);
      } else if(attrs) {
        act_attr_value = get_cream_action_http(attrs->get("HTTP:METHOD"),logger);
      }
      if(act_attr_value.empty()) act_attr_value = "http://glite.org/xacml/action/ANY"; // throw ierror("Failed to generate action name");
      logger.msg(Arc::DEBUG,"Adding action-id value: %s", act_attr_value);
      xacml_element_add_attribute(action, act_attr_value, XACML_DATATYPE_STRING, act_attr_id, "");

    } catch(ierror& err) {
      logger.msg(Arc::DEBUG,"CREAM request generation failed: %s",err.desc);
      return 1;
    }
    return 0;
}

static bool split_voms(const std::string& voms_attr, std::string& vo, std::string& group, 
  std::list<std::string>& roles, std::list<std::string>& attrs) {
    vo.resize(0);
    group.resize(0);
    roles.clear();
    attrs.clear();
    std::list<std::string> elements;
    Arc::tokenize(voms_attr,elements,"/");
    std::list<std::string>::iterator element = elements.begin();
    for(;element!=elements.end();++element) {
        std::string::size_type p = element->find('=');
        if(p == std::string::npos) {
            attrs.push_back(*element);
        } else {
            std::string key = element->substr(0,p);
            if(key == "VO") {
                vo = element->substr(p+1);
            } else if(key == "Group") {
                group += "/"+element->substr(p+1);
            } else if(key == "Role") {
                roles.push_back(element->substr(p+1));
            } else {
                attrs.push_back(*element);
            }
        }
    }
    return true;
}

int ArgusPDPClient::create_xacml_request_emi(Arc::XMLNode& request, std::list<Arc::MessageAuth*> auths, Arc::MessageAttributes* attrs, Arc::XMLNode operation) const {
    logger.msg(Arc::DEBUG,"Doing EMI request");

    class ierror {
     public:
      std::string desc;
      ierror(const std::string& err):desc(err) { };
    };

    try {

      xacml_create_request(request);

      // Environment
      Arc::XMLNode environment = xacml_request_add_element(request, "Environment");
      std::string env_attr_id = XACML_DCISEC_ATTRIBUTE_PROFILE_ID; //"http://dci-sec.org/xacml/attribute/profile-id";
      std::string env_attr_value = "http://dci-sec.org/xacml/profile/common-ce/1.0"; //? not defined ?
      logger.msg(Arc::DEBUG,"Adding profile-id value: %s", env_attr_value);
      xacml_element_add_attribute(environment, env_attr_value,
          XACML_DATATYPE_ANYURI, env_attr_id, "");

      // Subject
      Arc::XMLNode subject = xacml_request_add_element(request, "Subject");

      std::string sub_attr_id = XACML_SUBJECT_ID; //"urn:oasis:names:tc:xacml:1.0:subject:subject-id";
      std::string sub_attr_value = get_sec_attr(auths, "TLS", "IDENTITY");
      if(sub_attr_value.empty()) throw ierror("Failed to extract TLS:IDENTITY");
      sub_attr_value = path_to_x500(sub_attr_value);
      logger.msg(Arc::DEBUG,"Adding subject-id value: %s", sub_attr_value);
      xacml_element_add_attribute(subject, sub_attr_value,
          XACML_DATATYPE_X500NAME, sub_attr_id, "");

      std::string iss_attr_id = XACML_DCISEC_ATTRIBUTE_SUBJECT_ISSUER; //"http://dci-sec.org/xacml/attribute/subject-issuer";
      std::string iss_attr_value = get_sec_attr(auths, "TLS", "CA");
      if(iss_attr_value.empty()) throw ierror("Failed to extract TLS:CA");
      iss_attr_value = path_to_x500(iss_attr_value);
      logger.msg(Arc::DEBUG,"Adding subject-issuer value: %s", iss_attr_value);
      xacml_element_add_attribute(subject, iss_attr_value,
          XACML_DATATYPE_X500NAME, iss_attr_id, "");

      std::string vo_attr_id = XACML_DCISEC_ATTRIBUTE_VIRTUAL_ORGANIZATION; // "http://dci-sec.org/xacml/attribute/virtual-organization";
      std::list<std::string> vos = get_sec_attrs(auths, "TLS", "VO");
      for(std::list<std::string>::iterator vo = vos.begin(); vo!=vos.end(); ++vo) {
        logger.msg(Arc::DEBUG,"Adding Virtual Organization value: %s",*vo);
      }
      if(vos.size()>0) xacml_element_add_attribute(subject, vos, XACML_DATATYPE_STRING, vo_attr_id, "");


      std::string group_attr_id = XACML_DCISEC_ATTRIBUTE_GROUP; //"http://dci-sec.org/xacml/attribute/group";
      std::list<std::string> fqans = get_sec_attrs(auths, "TLS", "VOMS");
      // TODO: handle no fqans
      std::list<std::string> groups;
      std::string pgroup;
      for(std::list<std::string>::iterator fqan = fqans.begin(); fqan!=fqans.end(); ++fqan) {
        std::string vo;
        std::string group;
        std::list<std::string> roles;
        std::list<std::string> attrs;
        if(!split_voms(*fqan,vo,group,roles,attrs)) throw ierror("Failed to convert VOMS FQAN");
        if(pgroup.empty()) pgroup = group;
        if(!group.empty()) {
          groups.push_back(group);
        }
      }
      groups.unique();
      for(std::list<std::string>::iterator g = groups.begin(); g!=groups.end(); ++g) {
        logger.msg(Arc::DEBUG,"Adding VOMS group value: %s", *g);
      }
      if(groups.size()>0) xacml_element_add_attribute(subject, groups, XACML_DATATYPE_STRING, group_attr_id, "");

      if(!pgroup.empty()) {
        std::string pgroup_attr_id = XACML_DCISEC_ATTRIBUTE_GROUP_PRIMARY; //"http://dci-sec.org/xacml/attribute/group/primary"
        logger.msg(Arc::DEBUG,"Adding VOMS primary group value: %s", pgroup);
        xacml_element_add_attribute(subject, pgroup, XACML_DATATYPE_STRING, pgroup_attr_id, "");
      }

      std::string prole;
      pgroup.resize(0);
      for(std::list<std::string>::iterator fqan = fqans.begin(); fqan!=fqans.end(); ++fqan) {
        std::string vo;
        std::string group;
        std::list<std::string> roles;
        std::list<std::string> attrs;
        if(!split_voms(*fqan,vo,group,roles,attrs)) throw ierror("Failed to convert VOMS FQAN");

        std::string role_attr_id = XACML_DCISEC_ATTRIBUTE_ROLE; //"http://dci-sec.org/xacml/attribute/role"

        // TODO: handle no roles
        for(std::list<std::string>::iterator role = roles.begin(); role!=roles.end(); ++role) {
          if(role->empty()) { roles.erase(role); continue; }
          if(prole.empty()) { prole = *role; pgroup = group; }
          logger.msg(Arc::DEBUG,"Adding VOMS role value: %s", *role);
        }
        if(roles.size()>0) xacml_element_add_attribute(subject, roles, XACML_DATATYPE_STRING, role_attr_id, group);
      }
      if(!prole.empty()) {
        std::string prole_attr_id = XACML_DCISEC_ATTRIBUTE_ROLE_PRIMARY; //"http://dci-sec.org/xacml/attribute/role/primary"
        logger.msg(Arc::DEBUG,"Adding VOMS primary role value: %s", prole);
        xacml_element_add_attribute(subject, prole, XACML_DATATYPE_STRING, prole_attr_id, pgroup);
      }


      // Resource
      Arc::XMLNode resource = xacml_request_add_element(request, "Resource");

      std::string res_attr_id = XACML_RESOURCE_ID; //"urn:oasis:names:tc:xacml:1.0:resource:resource-id"
      std::string res_attr_value = attrs->get("ENDPOINT");
      if(res_attr_value.empty()) throw ierror("Failed to extract ENDPOINT");
      logger.msg(Arc::DEBUG,"Adding resource-id value: %s", res_attr_value);
      xacml_element_add_attribute(resource, res_attr_value, XACML_DATATYPE_STRING, res_attr_id, "");

      std::string res_own_attr_id = XACML_DCISEC_ATTRIBUTE_RESOURCE_OWNER; //"http://dci-sec.org/xacml/attribute/resource-owner"
      std::string res_own_attr_value = get_sec_attr(auths, "TLS", "LOCALSUBJECT");
      if(res_own_attr_value.empty()) throw ierror("Failed to extract LOCALSUBJECT");
      logger.msg(Arc::DEBUG,"Adding resource-owner value: %s", res_own_attr_value);
      xacml_element_add_attribute(resource, path_to_x500(res_own_attr_value), XACML_DATATYPE_X500NAME, res_own_attr_id, "");


      // Action
      // In a future action names should be synchronized among services
      Arc::XMLNode action = xacml_request_add_element(request, "Action");
      std::string act_attr_id = XACML_ACTION_ID; //"urn:oasis:names:tc:xacml:1.0:action:action-id";
      //"http://dci-sec.org/xacml/action/arc/arex/"+operation.Name
      std::string arex_ns = get_sec_attr(auths, "AREX", "NAMESPACE");
      std::string arex_action = get_sec_attr(auths, "AREX", "ACTION");
      std::string act_attr_value;
      if(!arex_ns.empty()) act_attr_value = arex_ns + "/" + arex_action;

      if(act_attr_value.empty()) act_attr_value = "http://dci-sec.org/xacml/action/ANY"; //throw ierror("Failed to generate action name");
      logger.msg(Arc::DEBUG,"Adding action-id value: %s", act_attr_value);
      xacml_element_add_attribute(action, act_attr_value, XACML_DATATYPE_STRING, act_attr_id, "");

    } catch(ierror& err) {
      logger.msg(Arc::DEBUG,"EMI request generation failed: %s",err.desc);
      return 1;
    }
    return 0;
}

}  // namespace ArcSec

