#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <fstream>
#include <vector>
#include <arc/Logger.h>
#include <arc/StringConv.h>
#include <arc/URL.h>
#include <arc/message/MCCLoader.h>
#include <arc/message/PayloadSOAP.h>
#include <arc/message/SecAttr.h>

#include "ArgusPEP2.h"
#include "ArgusXACMLConstant.h"

static const char XACML_DATATYPE_FQAN[]= "http://glite.org/xacml/datatype/fqan";

static Arc::XMLNode xacml_create_request() {
    Arc::NS ns;
    ns["xacml-ctx"]="urn:oasis:names:tc:xacml:2.0:context:schema:os";
    Arc::XMLNode node(ns, "Request");
    Arc::XMLNode ret;
    node.New(ret);
    return ret;
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
    ArcSec::ArgusPEP2* plugin = new ArcSec::ArgusPEP2((Arc::Config*)(*shcarg),arg);
    if(!plugin) return NULL;
    if(!(*plugin)) { delete plugin; return NULL;};
    return plugin;
}

Arc::PluginDescriptor PLUGINS_TABLE_NAME[] = {
    { "arguspep2.map", "HED:SHC", NULL, 0, &get_sechandler},
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

Arc::Logger ArgusPEP2::logger(Arc::Logger::getRootLogger(), "SecHandler.Argus");

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
ArgusPEP2::ArgusPEP2(Arc::Config *cfg,Arc::PluginArgument* parg):ArcSec::SecHandler(cfg,parg) {  
    valid_ = false;
    accept_mapping = false;
    logger.setThreshold(Arc::DEBUG);
    pepdlocation = (std::string)(*cfg)["PEPD"];  
    if(pepdlocation.empty()) {
        logger.msg(Arc::ERROR, "PEPD location is missing");
        return;
    }
    logger.msg(Arc::DEBUG, "PEPD location: %s",pepdlocation);

    std::string conversion_str = (std::string)(*cfg)["Conversion"];
    if(conversion_str == "direct") {
        logger.msg(Arc::DEBUG, "Conversion mode is set to DIRECT");
        conversion = conversion_direct;
    } else if(conversion_str == "subject") {
        logger.msg(Arc::DEBUG, "Conversion mode is set to SUBJECT");
        conversion = conversion_subject;
    } else if(conversion_str == "cream") {
        logger.msg(Arc::DEBUG, "Conversion mode is set to CREAM");
        conversion = conversion_cream;
    } else if(conversion_str == "emi") {
        logger.msg(Arc::DEBUG, "Conversion mode is set to EMI");
        conversion = conversion_emi;
    } else {
        logger.msg(Arc::INFO, "Unknown conversion mode %s, using default", conversion_str);
        conversion = conversion_subject;
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

    valid_ = true;
}

ArgusPEP2::~ArgusPEP2(void) {
}

bool ArgusPEP2::Handle(Arc::Message* msg) const {
    int rc = 0;
    bool res = true;
    //PEP* pep_handle = NULL;
    //pep_error_t pep_rc = PEP_OK;
    //xacml_response_t * response = NULL;
    //xacml_request_t * request = NULL;
    //std::list<xacml_request_t*> requests;
    Arc::XMLNode request;
    Arc::XMLNode response;
    std::list<Arc::XMLNode> requests;

    std::string subject , resource , action;
    Arc::XMLNode secattr;   

/*
    try{

        // set up the communication with the pepd host
        pep_handle = pep_initialize();
        if (pep_handle == NULL) throw pep_ex(std::string("Failed to initialize PEP client:"));

        pep_rc = pep_setoption(pep_handle, PEP_OPTION_LOG_LEVEL, pep_log_level);
        if (pep_rc != PEP_OK) throw pep_ex("Failed to set PEP log level: '" + pepdlocation + "' "+ pep_strerror(pep_rc));

        pep_rc = pep_setoption(pep_handle, PEP_OPTION_LOG_HANDLER, &pep_log);
        if (pep_rc != PEP_OK) throw pep_ex("Failed to set PEP log handler: '" + pepdlocation + "' "+ pep_strerror(pep_rc));

        pep_rc = pep_setoption(pep_handle, PEP_OPTION_ENDPOINT_SSL_VALIDATION, 1);
        if (pep_rc != PEP_OK) throw pep_ex("Failed to set PEP validation: '" + pepdlocation + "' "+ pep_strerror(pep_rc));

        pep_rc = pep_setoption(pep_handle, PEP_OPTION_ENDPOINT_URL, pepdlocation.c_str());
        if (pep_rc != PEP_OK) throw pep_ex("Failed to set PEP URL: '" + pepdlocation + "' "+ pep_strerror(pep_rc));

        pep_rc = pep_setoption(pep_handle, PEP_OPTION_ENABLE_OBLIGATIONHANDLERS, 0);
        if (pep_rc != PEP_OK) throw pep_ex("Failed to set PEP obligation handling: '" + pepdlocation + "' "+ pep_strerror(pep_rc));

        if(!capath.empty()) {
            pep_rc = pep_setoption(pep_handle, PEP_OPTION_ENDPOINT_SERVER_CAPATH, capath.c_str());
            if (pep_rc != PEP_OK) throw pep_ex("Failed to set PEP CA path: '" + pepdlocation + "' "+ pep_strerror(pep_rc));
        }

        if(!keypath.empty()) {
            pep_rc = pep_setoption(pep_handle, PEP_OPTION_ENDPOINT_CLIENT_KEY, keypath.c_str());
            if (pep_rc != PEP_OK) throw pep_ex("Failed to set PEP key: '" + pepdlocation + "' "+ pep_strerror(pep_rc));
        }

        if(!certpath.empty()) {
            pep_rc = pep_setoption(pep_handle, PEP_OPTION_ENDPOINT_CLIENT_CERT, certpath.c_str());
            if (pep_rc != PEP_OK) throw pep_ex("Failed to set PEP certificate: '" + pepdlocation + "' "+ pep_strerror(pep_rc));
        }

        if(conversion == conversion_direct) {
            msg->Auth()->Export(Arc::SecAttr::ARCAuth, secattr);
            msg->AuthContext()->Export(Arc::SecAttr::ARCAuth, secattr);
            rc= create_xacml_request_direct(requests,secattr);
        } else if(conversion == conversion_subject) {
            //resource= (std::string) secattr["RequestItem"][0]["Resource"][0];
            //action=  (std::string) secattr["RequestItem"][0]["Action"][0];
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
            rc = create_xacml_request(&request,subject.c_str(),resource.c_str(),action.c_str());
            if(request != NULL) requests.push_back(request);
            request = NULL;
        } else if(conversion == conversion_cream) {
            std::list<Arc::MessageAuth*> auths;
            auths.push_back(msg->Auth());
            auths.push_back(msg->AuthContext());
            Arc::PayloadSOAP* payload = NULL;
            try {
                payload = dynamic_cast<Arc::PayloadSOAP*>(msg->Payload());
            } catch(std::exception& e) { };
            if(!payload) throw pep_ex("No SOAP in message");
            rc = create_xacml_request_cream(&request,auths,msg->Attributes(),payload->Child(0));
            if(request != NULL) requests.push_back(request);
            request = NULL;
        } else if(conversion == conversion_emi) {
            std::list<Arc::MessageAuth*> auths;
            auths.push_back(msg->Auth());
            auths.push_back(msg->AuthContext());
            Arc::PayloadSOAP* payload = NULL;
            try {
                payload = dynamic_cast<Arc::PayloadSOAP*>(msg->Payload());
            } catch(std::exception& e) { };
            if(!payload) throw pep_ex("No SOAP in message");
            rc = create_xacml_request_emi(&request,auths,msg->Attributes(),payload->Child(0));
            if(request != NULL) requests.push_back(request);
            request = NULL;
        } else {
            throw pep_ex("Unsupported conversion mode " + Arc::tostring(conversion));
        } 
        if (rc != 0) {
            throw pep_ex("Failed to create XACML request(s): " + Arc::tostring(rc));
        }
        std::string local_id; 
        xacml_decision_t decision = XACML_DECISION_INDETERMINATE; 
        // Simple combining algorithm. At least one deny means deny. If none, then at 
        // least one permit means permit. Otherwise deny. TODO: configurable.
        logger.msg(Arc::DEBUG, "Have %i requests to process", requests.size());
        while(requests.size() > 0) {
            request = requests.front();
            requests.pop_front();
            pep_rc = pep_authorize(pep_handle,&request,&response);
            if (pep_rc != PEP_OK) {
                throw pep_ex(std::string("Failed to process XACML request: ")+pep_strerror(pep_rc));
            }   
            if (response == NULL) {
                throw pep_ex("XACML response is empty");
            }
            // Extract the local user name from the response to be mapped to the GID
            size_t results_l = xacml_response_results_length(response);
            int i = 0;
            for(i = 0; i<results_l; i++) {
                xacml_result_t * result = xacml_response_getresult(response,i);        
                if(result == NULL) break;
                switch(xacml_result_getdecision(result)) {
                   case XACML_DECISION_DENY: decision = XACML_DECISION_DENY; break;
                   case XACML_DECISION_PERMIT: decision = XACML_DECISION_PERMIT; break;
                };
                if(decision == XACML_DECISION_DENY) break;
                std::size_t obligations_l = xacml_result_obligations_length(result);
                int j =0;
                for(j = 0; j<obligations_l; j++) {
                    xacml_obligation_t * obligation = xacml_result_getobligation(result,j);
                    if(obligation == NULL) break;
                    std::size_t attrs_l = xacml_obligation_attributeassignments_length(obligation);
                    int k= 0;
                    for (k= 0; k<attrs_l; k++) {
                        xacml_attributeassignment_t * attr = xacml_obligation_getattributeassignment(obligation,k);
                        if(attr == NULL) break;
                        const char * id = xacml_attributeassignment_getvalue(attr); 
                        local_id =  id?id:"";
                    } 
                }
            } 
            xacml_response_delete(response); response = NULL;
            xacml_request_delete(request); request = NULL;
            if(decision == XACML_DECISION_DENY) break;
        }
        if (decision != XACML_DECISION_PERMIT ){
            if(conversion == conversion_direct) {
                std::string xml;
                secattr.GetXML(xml);
                logger.msg(Arc::INFO,"Not authorized according to request:\n%s",xml);
            } else if(conversion == conversion_subject) {
                logger.msg(Arc::INFO,"%s is not authorized to do action %s in resource %s ",
                           subject, action, resource);
            } else {
                logger.msg(Arc::INFO,"Not authorized");
            }
            throw pep_ex("The reached decision is: " + xacml_decision_to_string(decision));
        }     
        if(accept_mapping && !local_id.empty()) {
            logger.msg(Arc::INFO,"Grid identity is mapped to local identity '%s'", local_id);
            msg->Attributes()->set("SEC:LOCALID", local_id);
        }
    } catch (pep_ex& e) {
        logger.msg(Arc::ERROR,"%s",e.desc);
        res = false;
    }
*/

    return res;
}

int ArgusPEP2::create_xacml_request(Arc::XMLNode& request,const char * subjectid, const char * resourceid, const char * actionid) const {
    request = xacml_create_request();

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

int ArgusPEP2::create_xacml_request_cream(Arc::XMLNode& request, std::list<Arc::MessageAuth*> auths,  
    Arc::MessageAttributes* attrs, Arc::XMLNode operation) const {
    logger.msg(Arc::DEBUG,"Doing CREAM request");
    class ierror {
     public:
      std::string desc;
      ierror(const std::string& err):desc(err) { };
    };
    try {
      request = xacml_create_request();

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
      xacml_element_add_attribute(subject, vos, XACML_DATATYPE_STRING, vo_attr_id, "");

      std::string fqan_attr_id = XACML_GLITE_ATTRIBUTE_FQAN; //"http://glite.org/xacml/attribute/fqan";
      std::string pfqan;
      std::list<std::string> fqans = get_sec_attrs(auths, "TLS", "VOMS");
      std::list<std::string> flatten_fqans;
      for(std::list<std::string>::iterator fqan = fqans.begin(); fqan!=fqans.end(); ++fqan) {
        std::string fqan_str = flatten_fqan(*fqan);
        if(fqan_str.empty()) continue;
        if(pfqan.empty()) pfqan = fqan_str;
        flatten_fqans.push_back(fqan_str);
        logger.msg(Arc::DEBUG,"Adding fqan value: %s",fqan_str);
      }
      xacml_element_add_attribute(subject, flatten_fqans, XACML_DATATYPE_FQAN, fqan_attr_id, "");

      // /voname=testers.eu-emi.eu/hostname=emitestbed07.cnaf.infn.it:15002/testers.eu-emi.eu/test3:test_ga=ciccio -> ?
      // /VO=testers.eu-emi.eu/Group=testers.eu-emi.eu/Group=test1 -> /testers.eu-emi.eu/test1

      if(!pfqan.empty()) {
        std::string pfqan_attr_id = XACML_GLITE_ATTRIBUTE_FQAN_PRIMARY; //"http://glite.org/xacml/attribute/fqan/primary";
        logger.msg(Arc::DEBUG,"Adding fqan/primary value: %s",pfqan);
        xacml_element_add_attribute(subject, pfqan, XACML_DATATYPE_FQAN, pfqan_attr_id, "");
        // TODO: convert to VOMS FQAN?
      }

      std::string cert_attr_id = XACML_SUBJECT_KEY_INFO; //urn:oasis:names:tc:xacml:1.0:subject:key-info
      std::string cert_attr_value = get_sec_attr(auths, "TLS", "CERTIFICATE");
      if(!cert_attr_value.empty()) throw ierror("Failed to create attribute key-info object");
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
      std::string act_attr_value = get_cream_action(operation,logger);
      if(act_attr_value.empty()) throw ierror("Failed to generate action name");
      logger.msg(Arc::DEBUG,"Adding action-id value: %s", act_attr_value);
      xacml_element_add_attribute(action, act_attr_value, XACML_DATATYPE_STRING, act_attr_id, "");

      return 0;

    } catch(ierror err) {
      logger.msg(Arc::DEBUG,"CREAM request generation failed: %s",err.desc);
      return 1;
    }
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

int ArgusPEP2::create_xacml_request_emi(Arc::XMLNode& request, std::list<Arc::MessageAuth*> auths, Arc::MessageAttributes* attrs, Arc::XMLNode operation) const {
    logger.msg(Arc::DEBUG,"Doing EMI request");

    class ierror {
     public:
      std::string desc;
      ierror(const std::string& err):desc(err) { };
    };

    try {

      request = xacml_create_request();

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
        logger.msg(Arc::DEBUG,"Adding virtual-organization value: %s",*vo);
      }
      xacml_element_add_attribute(subject, vos, XACML_DATATYPE_STRING, vo_attr_id, "");


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
        if(!split_voms(*fqan,vo,group,roles,attrs)) throw ierror("Failed to convert voms fqan");
        if(pgroup.empty()) pgroup = group;
        if(!group.empty()) groups.push_back(group);
      }
      xacml_element_add_attribute(subject, groups, XACML_DATATYPE_STRING, group_attr_id, "");

      if(!pgroup.empty()) {
        std::string pgroup_attr_id = XACML_DCISEC_ATTRIBUTE_GROUP_PRIMARY; //"http://dci-sec.org/xacml/attribute/group/primary"
        xacml_element_add_attribute(subject, pgroup, XACML_DATATYPE_STRING, pgroup_attr_id, "");
      }

      std::string prole;
      pgroup.resize(0);
      std::list<std::string> roles;
      for(std::list<std::string>::iterator fqan = fqans.begin(); fqan!=fqans.end(); ++fqan) {
        std::string vo;
        std::string group;
        std::list<std::string> roles;
        std::list<std::string> attrs;
        if(!split_voms(*fqan,vo,group,roles,attrs)) throw ierror("Failed to convert voms fqan");

        std::string role_attr_id = XACML_DCISEC_ATTRIBUTE_ROLE; //"http://dci-sec.org/xacml/attribute/role"


        // TODO: handle no roles
        for(std::list<std::string>::iterator role = roles.begin(); role!=roles.end(); ++role) {
            if(role->empty()) continue;
            if(prole.empty()) { prole = *role; pgroup = group; }
            roles.push_back(*role);
        }

        xacml_element_add_attribute(subject, roles, XACML_DATATYPE_STRING, role_attr_id, group);
      }
      if(!prole.empty()) {
        std::string prole_attr_id = XACML_DCISEC_ATTRIBUTE_ROLE_PRIMARY; //"http://dci-sec.org/xacml/attribute/role/primary"
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
      std::string act_attr_value = get_sec_attr(auths, "AREX", "NAMESPACE") + "/" + get_sec_attr(auths, "AREX", "ACTION");
      if(act_attr_value.empty()) throw ierror("Failed to generate action name");
      logger.msg(Arc::DEBUG,"Adding action-id value: %s", act_attr_value);
      xacml_element_add_attribute(action, act_attr_value, XACML_DATATYPE_STRING, act_attr_id, "");

      return 0;

    } catch(ierror err) {
      logger.msg(Arc::DEBUG,"EMI request generation failed: %s",err.desc);
      return 1;
    }
}

}  // namespace ArcSec

