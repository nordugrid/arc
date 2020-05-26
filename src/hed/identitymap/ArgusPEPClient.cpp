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
#include "ArgusPEPClient.h"

static const char XACML_DATATYPE_FQAN[]= "http://glite.org/xacml/datatype/fqan";

#define AREX_JOB_POLICY_OPERATION_URN "http://www.nordugrid.org/schemas/policy-arc/types/a-rex/joboperation"
#define AREX_JOB_POLICY_OPERATION_CREATE "Create"
#define AREX_JOB_POLICY_OPERATION_MODIFY "Modify"
#define AREX_JOB_POLICY_OPERATION_READ   "Read"
#define AREX_POLICY_OPERATION_URN "http://www.nordugrid.org/schemas/policy-arc/types/a-rex/operation"
#define AREX_POLICY_OPERATION_ADMIN "Admin"
#define AREX_POLICY_OPERATION_INFO  "Info"

#define EMIES_OPERATION_CREATION "http://www.eu-emi.eu/es/2010/12/creation"
#define EMIES_OPERATION_ACTIVITY "http://www.eu-emi.eu/es/2010/12/activity"
#define EMIES_OPERATION_ACTIVITYMANGEMENT "http://www.eu-emi.eu/es/2010/12/activitymanagement"
#define EMIES_OPERATION_RESOURCEINFO "http://www.eu-emi.eu/es/2010/12/resourceinfo"
#define EMIES_OPERATION_DELEGATION "http://www.gridsite.org/namespaces/delegation-21"
#define EMIES_OPERATION_ANY "http://dci-sec.org/xacml/action/ANY"

static Arc::Plugin* get_sechandler(Arc::PluginArgument* arg) {
    ArcSec::SecHandlerPluginArgument* shcarg = arg?dynamic_cast<ArcSec::SecHandlerPluginArgument*>(arg):NULL;
    if(!shcarg) return NULL;
    ArcSec::ArgusPEPClient* plugin = new ArcSec::ArgusPEPClient((Arc::Config*)(*shcarg),arg);
    if(!plugin) return NULL;
    if(!(*plugin)) { delete plugin; return NULL;};
    return plugin;
}

extern Arc::PluginDescriptor const ARC_PLUGINS_TABLE_NAME[] = {
    { "arguspepclient.map", "HED:SHC", NULL, 0, &get_sechandler},
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

Arc::Logger ArgusPEPClient::logger(Arc::Logger::getRootLogger(), "SecHandler.Argus");

int ArgusPEPClient::pep_log(int level, const char *fmt, va_list args) {
    char buf[1024];
    vsnprintf(buf,sizeof(buf)-1,fmt,args);
    buf[sizeof(buf)-1] = 0;
    Arc::LogLevel l = Arc::INFO;
    switch(logger.getThreshold()) {
        case PEP_LOGLEVEL_DEBUG: l = Arc::DEBUG; break;
        case PEP_LOGLEVEL_INFO:  l = Arc::INFO; break;
        case PEP_LOGLEVEL_WARN:  l = Arc::WARNING; break;
        case PEP_LOGLEVEL_ERROR: l = Arc::ERROR; break;
    }
    logger.msg(l,"%s",buf);
    return 0;
}

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
ArgusPEPClient::ArgusPEPClient(Arc::Config *cfg,Arc::PluginArgument* parg):ArcSec::SecHandler(cfg,parg),conversion(conversion_emi) {  
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

    pep_log_level = PEP_LOGLEVEL_NONE;
    switch(logger.getThreshold()) {
        case Arc::DEBUG:   pep_log_level = PEP_LOGLEVEL_DEBUG; break;
        case Arc::VERBOSE: pep_log_level = PEP_LOGLEVEL_INFO;  break;
        case Arc::INFO:    pep_log_level = PEP_LOGLEVEL_INFO;  break;
        case Arc::WARNING: pep_log_level = PEP_LOGLEVEL_WARN;  break;
        case Arc::ERROR:   pep_log_level = PEP_LOGLEVEL_ERROR; break;
        case Arc::FATAL:   pep_log_level = PEP_LOGLEVEL_ERROR; break;
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

ArgusPEPClient::~ArgusPEPClient(void) {
}

SecHandlerStatus ArgusPEPClient::Handle(Arc::Message* msg) const {
    int rc = 0;
    bool res = true;
    PEP* pep_handle = NULL;
    pep_error_t pep_rc = PEP_OK;
    xacml_response_t * response = NULL;
    xacml_request_t * request = NULL;
    std::list<xacml_request_t*> requests;
    std::string subject , resource , action;
    Arc::XMLNode secattr;   
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
            //if(!payload) throw pep_ex("No SOAP in message");
            if(payload) {
              rc = create_xacml_request_cream(&request,auths,msg->Attributes(),payload->Child(0));
            } else {
              // For HTTP operations
              rc = create_xacml_request_cream(&request,auths,msg->Attributes(),Arc::XMLNode());
            }
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
            //if(!payload) throw pep_ex("No SOAP in message");
            if(payload) {
              rc = create_xacml_request_emi(&request,auths,msg->Attributes(),payload->Child(0));
            } else {
              rc = create_xacml_request_emi(&request,auths,msg->Attributes(),Arc::XMLNode());
            }
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
                logger.msg(Arc::INFO,"%s is not authorized to do action %s in resource %s",
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
    if(response) xacml_response_delete(response);
    if(request) xacml_request_delete(request);
    while(requests.size() > 0) {
        xacml_request_delete(requests.front());
        requests.pop_front();
    }
    if(pep_handle) pep_destroy(pep_handle);
    return res;
}

int ArgusPEPClient::create_xacml_request(xacml_request_t ** request,const char * subjectid, const char * resourceid, const char * actionid) const {
    xacml_subject_t * subject= xacml_subject_create();
    if (subject == NULL) {
        logger.msg(Arc::DEBUG, "Subject of request is null");
        return 1;
    }
    xacml_attribute_t * subject_attr_id= xacml_attribute_create(XACML_SUBJECT_ID);
    if (subject_attr_id == NULL) {
    logger.msg(Arc::DEBUG,"Can not create XACML SubjectAttribute: %s", XACML_SUBJECT_ID);
        xacml_subject_delete(subject);
        return 1;
    }
    xacml_attribute_addvalue(subject_attr_id, path_to_x500(subjectid).c_str());
    xacml_attribute_setdatatype(subject_attr_id,XACML_DATATYPE_X500NAME);
    xacml_subject_addattribute(subject,subject_attr_id); 
    xacml_resource_t * resource = xacml_resource_create();
    if (resource == NULL) {
        logger.msg(Arc::DEBUG, "Can not create XACML Resource");
        xacml_subject_delete(subject);
        return 2;
    }
    xacml_attribute_t * resource_attr_id = xacml_attribute_create(XACML_RESOURCE_ID);
    if (resource_attr_id == NULL) {
        logger.msg(Arc::DEBUG,"Can not create XACML ResourceAttribute: %s", XACML_RESOURCE_ID);
        xacml_subject_delete(subject);
        xacml_resource_delete(resource);
        return 2;
    }
    xacml_attribute_addvalue(resource_attr_id,resourceid);
    xacml_resource_addattribute(resource,resource_attr_id);
    xacml_action_t * action= xacml_action_create();
    if (action == NULL) {
        logger.msg(Arc::DEBUG,"Can not create XACML Action");
        xacml_subject_delete(subject);
        xacml_resource_delete(resource);
        return 3;
    }
    xacml_attribute_t * action_attr_id= xacml_attribute_create(XACML_ACTION_ID);
    if (action_attr_id == NULL) {
        logger.msg(Arc::DEBUG,"Can not create XACML ActionAttribute: %s", XACML_ACTION_ID);
        xacml_subject_delete(subject);
        xacml_resource_delete(resource);
        xacml_action_delete(action);
        return 3;
    }
    xacml_attribute_addvalue(action_attr_id,actionid);
    xacml_action_addattribute(action,action_attr_id);
    *request= xacml_request_create();
    if (*request == NULL) {
    logger.msg(Arc::DEBUG,"Can not create XACML request");
    xacml_subject_delete(subject);
        xacml_resource_delete(resource);
        xacml_action_delete(action);
        return 4;
    }
    xacml_request_addsubject(*request,subject);
    xacml_request_addresource(*request,resource);
    xacml_request_setaction(*request,action);
    return 0;  
}

int ArgusPEPClient::create_xacml_request_direct(std::list<xacml_request_t*>& requests, Arc::XMLNode arcreq) const {
    //  -- XACML --
    // Request
    //  Subject *
    //    Attribute *
    //      AttributeValue *
    //      AttributeId
    //      DataType
    //      Issuer
    //  Resource *
    //    Attribute *
    //  Action .
    //    Attribute *
    //  Environment .
    //    Attribute *
    //
    //  -- ARC --
    // Request (1)
    //   RequestItem (1-)
    //     Subject (1-)
    //       SubjectAttribute (1-)
    //     Resource (0-)
    //     Action (0-)
    //     Context (0-)
    //       ContextAttribute (1-)
    Arc::XMLNode arcreqitem = arcreq["RequestItem"];
    int r = 0;
    for(;(bool)arcreqitem;++arcreqitem) {
        xacml_request_t* request = xacml_request_create();
        if(!request) { r=1; break; }

        Arc::XMLNode arcsubject = arcreqitem["Subject"];
        for(;(bool)arcsubject;++arcsubject) {
            Arc::XMLNode arcattr = arcsubject["SubjectAttribute"];
            if((bool)arcattr) {
                xacml_subject_t * subject = xacml_subject_create();
                if(!subject) { r=1; break; }
                for(;(bool)arcattr;++arcattr) {
                    std::string id = arcattr.Attribute("AttributeId");
                    xacml_attribute_t * attr = xacml_attribute_create(id.c_str());
                    if(!attr) { r=1; break; }
                    xacml_attribute_addvalue(attr,((std::string)arcattr).c_str());
                    xacml_attribute_setdatatype(attr,XACML_DATATYPE_STRING);
                    xacml_subject_addattribute(subject,attr);
                }
                xacml_request_addsubject(request,subject);
            }
        }
           
   
        Arc::XMLNode arcresource = arcreqitem["Resource"];
        if((bool)arcresource) {
            xacml_resource_t * resource = xacml_resource_create();
            if(resource) {
                std::string id = arcresource.Attribute("AttributeId");
                xacml_attribute_t * attr = xacml_attribute_create(id.c_str());
                if(attr) {
                    xacml_attribute_addvalue(attr,((std::string)arcresource).c_str());
                    xacml_attribute_setdatatype(attr,XACML_DATATYPE_STRING);
                    xacml_resource_addattribute(resource,attr);
                } else { r=1; }
                xacml_request_addresource(request,resource);
            } else { r=1; }
        }

        Arc::XMLNode arcaction = arcreqitem["Action"];
        if((bool)arcaction) {
            xacml_action_t * action = xacml_action_create();
            if(action) {
                std::string id = arcaction.Attribute("AttributeId");
                xacml_attribute_t * attr = xacml_attribute_create(id.c_str());
                if(attr) {
                    xacml_attribute_addvalue(attr,((std::string)arcaction).c_str());
                    xacml_attribute_setdatatype(attr,XACML_DATATYPE_STRING);
                    xacml_action_addattribute(action,attr);
                } else { r=1; }
                xacml_request_setaction(request,action);
            } else { r=1; }
        }

        Arc::XMLNode arccontext = arcreqitem["Context"];
        if((bool)arccontext) {
            Arc::XMLNode arcattr = arccontext["ContextAttribute"];
            if((bool)arcattr) {
                xacml_environment_t * environment = xacml_environment_create();
                if(environment) {
                    for(;(bool)arcattr;++arcattr) {
                        std::string id = arcattr.Attribute("AttributeId");
                        xacml_attribute_t * attr = xacml_attribute_create(id.c_str());
                        if(!attr) { r=1; break; }
                        xacml_attribute_addvalue(attr, ((std::string)arcattr).c_str());
                        xacml_attribute_setdatatype(attr, XACML_DATATYPE_STRING);
                        xacml_environment_addattribute(environment,attr);
                    }
                    xacml_request_setenvironment(request,environment);
                } else { r=1; }
            }
        }
    
        requests.push_back(request);
        if(r != 0) break; 

    }
    if(r != 0) {
        while(requests.size() > 0) {
            xacml_request_delete(requests.front());
            requests.pop_front();
        }
    }
    return r;  
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

static std::string get_emi_action_http(const std::string& method) { 
    if(method == "GET") { 
        return EMIES_OPERATION_CREATION; 
    } else if(method == "PUT") { 
        return EMIES_OPERATION_ACTIVITYMANGEMENT; 
    } 
    return ""; 
} 
 
static std::string get_emi_action(Arc::XMLNode op) { 
  if(MatchXMLNamespace(op,EMIES_OPERATION_CREATION)) return EMIES_OPERATION_CREATION; 
  if(MatchXMLNamespace(op,EMIES_OPERATION_ACTIVITY)) return EMIES_OPERATION_ACTIVITY; 
  if(MatchXMLNamespace(op,EMIES_OPERATION_ACTIVITYMANGEMENT)) return EMIES_OPERATION_ACTIVITYMANGEMENT; 
  if(MatchXMLNamespace(op,EMIES_OPERATION_RESOURCEINFO)) return EMIES_OPERATION_RESOURCEINFO; 
  if(MatchXMLNamespace(op,EMIES_OPERATION_DELEGATION)) return EMIES_OPERATION_DELEGATION; 
  return ""; 
} 
 
static std::string get_emi_action_arex(const std::string& ns, const std::string& action) { 
  if(ns == AREX_JOB_POLICY_OPERATION_URN) { 
    if(action == AREX_JOB_POLICY_OPERATION_CREATE) return EMIES_OPERATION_CREATION; 
    if(action == AREX_JOB_POLICY_OPERATION_MODIFY) return EMIES_OPERATION_ACTIVITYMANGEMENT; 
    if(action == AREX_JOB_POLICY_OPERATION_READ) return EMIES_OPERATION_ACTIVITY; 
  } else if(ns == AREX_POLICY_OPERATION_URN) { 
    if(action == AREX_POLICY_OPERATION_INFO) return EMIES_OPERATION_RESOURCEINFO; 
    if(action == AREX_POLICY_OPERATION_ADMIN) return ""; 
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

static std::string get_resource(std::list<Arc::MessageAuth*>& auths, Arc::MessageAttributes* attrs) { 
    std::string resource = get_sec_attr(auths, "AREX", "SERVICE"); 
    if(!resource.empty()) return resource; 
    if(attrs) resource = attrs->get("ENDPOINT"); 
    return resource; 
} 
 
int ArgusPEPClient::create_xacml_request_cream(xacml_request_t** request, std::list<Arc::MessageAuth*> auths,  Arc::MessageAttributes* attrs, Arc::XMLNode operation) const {
logger.msg(Arc::DEBUG,"Doing CREAM request");
    xacml_attribute_t* attr = NULL;
    xacml_environment_t* environment = NULL;
    xacml_subject_t* subject = NULL;
    xacml_resource_t* resource = NULL;
    xacml_action_t* action = NULL;
    class ierror {
     public:
      std::string desc;
      ierror(const std::string& err):desc(err) { };
    };
    try {
    *request = xacml_request_create();
    if(!*request) throw ierror("Failed to create request object");
    environment = xacml_environment_create();
    if(!environment) throw ierror("Failed to create environment object");
    subject = xacml_subject_create();
    if(!subject) throw ierror("Failed to create subject object");
    resource = xacml_resource_create();
    if(!resource) throw ierror("Failed to create resource object");
    action = xacml_action_create();
    if(!action) throw ierror("Failed to create action object");

    // Environment
    std::string profile_id = "http://glite.org/xacml/profile/grid-ce/1.0";
    attr = xacml_attribute_create("http://glite.org/xacml/attribute/profile-id");
    if(!attr) throw ierror("Failed to create attribute profile-id object");
    xacml_attribute_addvalue(attr, profile_id.c_str());
    logger.msg(Arc::DEBUG,"Adding profile-id value: %s",profile_id);
    xacml_attribute_setdatatype(attr, XACML_DATATYPE_ANYURI);
    xacml_environment_addattribute(environment,attr); attr = NULL;

    // Subject
    attr = xacml_attribute_create("urn:oasis:names:tc:xacml:1.0:subject:subject-id");
    if(!attr) throw ierror("Failed to create attribute subject-id object");
    std::string subject_str = get_sec_attr(auths, "TLS", "IDENTITY");
    if(subject_str.empty()) throw ierror("Failed to extract TLS:IDENTITY");
    subject_str = path_to_x500(subject_str);
    xacml_attribute_addvalue(attr, subject_str.c_str());
    logger.msg(Arc::DEBUG,"Adding subject-id value: %s",subject_str);
    xacml_attribute_setdatatype(attr, XACML_DATATYPE_X500NAME);
    xacml_subject_addattribute(subject,attr); attr = NULL;

    attr = xacml_attribute_create("http://glite.org/xacml/attribute/subject-issuer");
    if(!attr) throw ierror("Failed to create attribute subject-issuer object");
    std::string ca_str = get_sec_attr(auths, "TLS", "CA");
    if(ca_str.empty()) throw ierror("Failed to extract TLS:CA");
    ca_str = path_to_x500(ca_str);
    xacml_attribute_addvalue(attr, ca_str.c_str());
    logger.msg(Arc::DEBUG,"Adding subject-issuer value: %s",ca_str);
    xacml_attribute_setdatatype(attr, XACML_DATATYPE_X500NAME);
    xacml_subject_addattribute(subject,attr); attr = NULL;

    attr = xacml_attribute_create("http://glite.org/xacml/attribute/virtual-organization");
    if(!attr) throw ierror("Failed to create attribute virtual-organization object");
    std::list<std::string> vos = get_sec_attrs(auths, "TLS", "VO");
    for(std::list<std::string>::iterator vo = vos.begin(); vo!=vos.end(); ++vo) {
        if(!vo->empty()) xacml_attribute_addvalue(attr, vo->c_str());
        logger.msg(Arc::DEBUG,"Adding virtual-organization value: %s",*vo);
    }
    xacml_attribute_setdatatype(attr, XACML_DATATYPE_STRING);
    xacml_subject_addattribute(subject,attr); attr = NULL;

    attr = xacml_attribute_create("http://glite.org/xacml/attribute/fqan");
    if(!attr) throw ierror("Failed to create attribute fqan object");
    std::string pfqan;
    std::list<std::string> fqans = get_sec_attrs(auths, "TLS", "VOMS");
    // /voname=testers.eu-emi.eu/hostname=emitestbed07.cnaf.infn.it:15002/testers.eu-emi.eu/test3:test_ga=ciccio -> ?
    // /VO=testers.eu-emi.eu/Group=testers.eu-emi.eu/Group=test1 -> /testers.eu-emi.eu/test1

    for(std::list<std::string>::iterator fqan = fqans.begin(); fqan!=fqans.end(); ++fqan) {
        std::string fqan_str = flatten_fqan(*fqan);
        if(fqan_str.empty()) continue;
        if(pfqan.empty()) pfqan = fqan_str;
        if(!fqan->empty()) xacml_attribute_addvalue(attr, fqan_str.c_str());
        logger.msg(Arc::DEBUG,"Adding FQAN value: %s",fqan_str);
    }
    xacml_attribute_setdatatype(attr, XACML_DATATYPE_FQAN);
    xacml_subject_addattribute(subject,attr); attr = NULL;
    if(!pfqan.empty()) {
        attr = xacml_attribute_create("http://glite.org/xacml/attribute/fqan/primary");
        if(!attr) throw ierror("Failed to create attribute FQAN/primary object");
        // TODO: convert to VOMS FQAN?
        xacml_attribute_addvalue(attr, pfqan.c_str());
        logger.msg(Arc::DEBUG,"Adding FQAN/primary value: %s",pfqan);
        xacml_attribute_setdatatype(attr, XACML_DATATYPE_FQAN);
        xacml_subject_addattribute(subject,attr); attr = NULL;
    }

    attr = xacml_attribute_create("urn:oasis:names:tc:xacml:1.0:subject:key-info");
    if(!attr) throw ierror("Failed to create attribute key-info object");
    std::string certstr = get_sec_attr(auths, "TLS", "CERTIFICATE");
    std::string chainstr = get_sec_attr(auths, "TLS", "CERTIFICATECHAIN");
    chainstr = certstr + "\n" + chainstr;
    xacml_attribute_addvalue(attr, chainstr.c_str());
    logger.msg(Arc::DEBUG,"Adding cert chain value: %s",chainstr);
    xacml_attribute_setdatatype(attr, XACML_DATATYPE_STRING);
    xacml_subject_addattribute(subject,attr); attr = NULL;

    // Resource
    attr = xacml_attribute_create("urn:oasis:names:tc:xacml:1.0:resource:resource-id");
    if(!attr) throw ierror("Failed to create attribute resource-id object");
    std::string endpoint = get_resource(auths,attrs);
    if(endpoint.empty()) throw ierror("Failed to extract resource identifier");
    xacml_attribute_addvalue(attr, endpoint.c_str());
    logger.msg(Arc::DEBUG,"Adding resource-id value: %s",endpoint);
    xacml_attribute_setdatatype(attr, XACML_DATATYPE_STRING);
    xacml_resource_addattribute(resource,attr); attr = NULL;

    // Action
    attr = xacml_attribute_create("urn:oasis:names:tc:xacml:1.0:action:action-id");
    if(!attr) throw ierror("Failed to create attribute action-id object");
    std::string act;
    if((bool)operation) {
      act = get_cream_action(operation,logger);
    } else if(attrs) {
      act = get_cream_action_http(attrs->get("HTTP:METHOD"),logger);
    }
    if(act.empty()) act = "http://glite.org/xacml/action/ANY"; // throw ierror("Failed to generate action name");
    xacml_attribute_addvalue(attr, act.c_str());
    logger.msg(Arc::DEBUG,"Adding action-id value: %s",act);
    xacml_attribute_setdatatype(attr, XACML_DATATYPE_STRING);
    xacml_action_addattribute(action,attr); attr = NULL;

    // Add everything into request
    xacml_request_setenvironment(*request,environment); environment = NULL;
    xacml_request_addsubject(*request,subject); subject = NULL;
    xacml_request_addresource(*request,resource); resource = NULL;
    xacml_request_setaction(*request,action); action = NULL;

    } catch(ierror& err) {
    logger.msg(Arc::DEBUG,"CREAM request generation failed: %s",err.desc);
    if(attr) xacml_attribute_delete(attr);
    if(environment) xacml_environment_delete(environment);
    if(subject) xacml_subject_delete(subject);
    if(resource) xacml_resource_delete(resource);
    if(*request) xacml_request_delete(*request);
    *request = NULL;
    return 1;
    }
    return 0;
}

static bool split_voms(const std::string& voms_attr, std::string& vo, std::string& group, std::list<
std::string>& roles, std::list<std::string>& attrs) {
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

int ArgusPEPClient::create_xacml_request_emi(xacml_request_t** request, std::list<Arc::MessageAuth*> auths,  Arc::MessageAttributes* attrs, Arc::XMLNode operation) const {
    xacml_attribute_t* attr = NULL;
    xacml_environment_t* environment = NULL;
    xacml_subject_t* subject = NULL;
    xacml_resource_t* resource = NULL;
    xacml_action_t* action = NULL;
    class ierror { };
    try {
    *request = xacml_request_create();
    if(!*request) throw ierror();
    environment = xacml_environment_create();
    if(!environment) throw ierror();
    subject = xacml_subject_create();
    if(!subject) throw ierror();
    resource = xacml_resource_create();
    if(!resource) throw ierror();
    action = xacml_action_create();
    if(!action) throw ierror();

    // Environment
    attr = xacml_attribute_create("http://dci-sec.org/xacml/attribute/profile-id");
    if(!attr) throw ierror();
    xacml_attribute_addvalue(attr, "http://dci-sec.org/xacml/profile/common-ce/1.0");
    xacml_attribute_setdatatype(attr, XACML_DATATYPE_ANYURI);
    xacml_environment_addattribute(environment,attr); attr = NULL;

    // Subject
    attr = xacml_attribute_create("urn:oasis:names:tc:xacml:1.0:subject:subject-id");
    if(!attr) throw ierror();
    std::string subject_str = get_sec_attr(auths, "TLS", "IDENTITY");
    if(subject_str.empty()) throw ierror();
    xacml_attribute_addvalue(attr, path_to_x500(subject_str).c_str());
    xacml_attribute_setdatatype(attr, XACML_DATATYPE_X500NAME);
    xacml_subject_addattribute(subject,attr); attr = NULL;

    attr = xacml_attribute_create("http://dci-sec.org/xacml/attribute/subject-issuer");
    if(!attr) throw ierror();
    std::string ca_str = get_sec_attr(auths, "TLS", "CA");
    if(ca_str.empty()) throw ierror();
    xacml_attribute_addvalue(attr, path_to_x500(ca_str).c_str());
    xacml_attribute_setdatatype(attr, XACML_DATATYPE_X500NAME);
    xacml_subject_addattribute(subject,attr); attr = NULL;

    attr = xacml_attribute_create("http://dci-sec.org/xacml/attribute/virtual-organization");
    if(!attr) throw ierror();
    std::list<std::string> vos = get_sec_attrs(auths, "TLS", "VO");
    // TODO: handle no vos
    for(std::list<std::string>::iterator vo = vos.begin(); vo!=vos.end(); ++vo) {
        if(!vo->empty()) xacml_attribute_addvalue(attr, vo->c_str());
    }
    xacml_attribute_setdatatype(attr, XACML_DATATYPE_STRING);
    xacml_subject_addattribute(subject,attr); attr = NULL;

    attr = xacml_attribute_create("http://dci-sec.org/xacml/attribute/group");
    if(!attr) throw ierror();
    std::string pgroup;
    std::list<std::string> fqans = get_sec_attrs(auths, "TLS", "VOMS");
    // TODO: handle no fqans
    for(std::list<std::string>::iterator fqan = fqans.begin(); fqan!=fqans.end(); ++fqan) {
        std::string vo;
        std::string group;
        std::list<std::string> roles;
        std::list<std::string> attrs;
        if(!split_voms(*fqan,vo,group,roles,attrs)) throw ierror();
        if(pgroup.empty()) pgroup = group;
        if(!group.empty()) xacml_attribute_addvalue(attr, group.c_str());
    }
    xacml_attribute_setdatatype(attr, XACML_DATATYPE_STRING);
    xacml_subject_addattribute(subject,attr); attr = NULL;
    if(!pgroup.empty()) {
        attr = xacml_attribute_create("http://dci-sec.org/xacml/attribute/group/primary");
        if(!attr) throw ierror();
        xacml_attribute_addvalue(attr, pgroup.c_str());
        xacml_attribute_setdatatype(attr, XACML_DATATYPE_STRING);
        xacml_subject_addattribute(subject,attr); attr = NULL;
    }
    std::string prole;
    pgroup.resize(0);
    for(std::list<std::string>::iterator fqan = fqans.begin(); fqan!=fqans.end(); ++fqan) {
        std::string vo;
        std::string group;
        std::list<std::string> roles;
        std::list<std::string> attrs;
        if(!split_voms(*fqan,vo,group,roles,attrs)) throw ierror();
        attr = xacml_attribute_create("http://dci-sec.org/xacml/attribute/role");
        if(!attr) throw ierror();
        if(!group.empty()) xacml_attribute_setissuer(attr, group.c_str());
        // TODO: handle no roles
        for(std::list<std::string>::iterator role = roles.begin(); role!=roles.end(); ++role) {
            if(role->empty()) continue;
            if(prole.empty()) { prole = *role; pgroup = group; }
            xacml_attribute_addvalue(attr, role->c_str());
        }
        xacml_attribute_setdatatype(attr, XACML_DATATYPE_STRING);
        xacml_subject_addattribute(subject,attr); attr = NULL;
    }
    if(!prole.empty()) {
        attr = xacml_attribute_create("http://dci-sec.org/xacml/attribute/role/primary");
        if(!attr) throw ierror();
        if(!pgroup.empty()) xacml_attribute_setissuer(attr, pgroup.c_str());
        xacml_attribute_addvalue(attr, prole.c_str());
        xacml_attribute_setdatatype(attr, XACML_DATATYPE_STRING);
        xacml_subject_addattribute(subject,attr); attr = NULL;
    }

    // Resource
    attr = xacml_attribute_create("urn:oasis:names:tc:xacml:1.0:resource:resource-id");
    if(!attr) throw ierror();
    std::string endpoint = get_resource(auths,attrs);
    if(endpoint.empty()) throw ierror();
    xacml_attribute_addvalue(attr, endpoint.c_str());
    xacml_attribute_setdatatype(attr, XACML_DATATYPE_STRING);
    xacml_resource_addattribute(resource,attr); attr = NULL;

    attr = xacml_attribute_create("http://dci-sec.org/xacml/attribute/resource-owner");
    std::string owner_str = get_sec_attr(auths, "TLS", "LOCALSUBJECT");
    if(owner_str.empty()) throw ierror();
    xacml_attribute_addvalue(attr, path_to_x500(owner_str).c_str());
    xacml_attribute_setdatatype(attr, XACML_DATATYPE_X500NAME);
    xacml_attribute_setdatatype(attr, XACML_DATATYPE_X500NAME);
    xacml_resource_addattribute(resource,attr); attr = NULL;

    // Action
    // In a future action names should be synchronized among services
    attr = xacml_attribute_create("urn:oasis:names:tc:xacml:1.0:action:action-id");
    if(!attr) throw ierror();
    std::string arex_ns = get_sec_attr(auths, "AREX", "NAMESPACE"); 
    std::string arex_action = get_sec_attr(auths, "AREX", "ACTION"); 
    std::string act = get_emi_action(operation); 
    if(act.empty()) act = get_emi_action_arex(arex_ns, arex_action); 
    if(act.empty()) act = get_emi_action_http(attrs->get("HTTP:METHOD")); 
    //if(act.empty() && !arex_ns.empty()) act = arex_ns + "/" + arex_action; 
    if(act.empty()) act = EMIES_OPERATION_ANY; //throw ierror("Failed to generate action name"); 
    xacml_attribute_addvalue(attr, act.c_str());
    xacml_attribute_setdatatype(attr, XACML_DATATYPE_STRING);
    xacml_action_addattribute(action,attr); attr = NULL;

    // Add everything into request
    xacml_request_setenvironment(*request,environment); environment = NULL;
    xacml_request_addsubject(*request,subject); subject = NULL;
    xacml_request_addresource(*request,resource); resource = NULL;
    xacml_request_setaction(*request,action); action = NULL;

    } catch(ierror& err) {
    if(attr) xacml_attribute_delete(attr);
    if(environment) xacml_environment_delete(environment);
    if(subject) xacml_subject_delete(subject);
    if(resource) xacml_resource_delete(resource);
    if(*request) xacml_request_delete(*request);
    return 1;
    }
    return 0;
}

}  // namespace ArcSec

