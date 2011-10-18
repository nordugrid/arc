#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <fstream>
#include <vector>
#include <arc/Logger.h>
#include <arc/StringConv.h>
#include <arc/message/MCCLoader.h>
#include <arc/message/PayloadSOAP.h>
#include <arc/message/SecAttr.h>
#include "ArgusPEP.h"

static const char XACML_DATATYPE_FQAN[]= "http://glite.org/xacml/datatype/fqan";

static Arc::Plugin* get_sechandler(Arc::PluginArgument* arg) {
    ArcSec::SecHandlerPluginArgument* shcarg = arg?dynamic_cast<ArcSec::SecHandlerPluginArgument*>(arg):NULL;
    if(!shcarg) return NULL;
    ArcSec::ArgusPEP* plugin = new ArcSec::ArgusPEP((Arc::Config*)(*shcarg));
    if(!plugin) return NULL;
    if(!(*plugin)) { delete plugin; return NULL;};
    return plugin;
}

Arc::PluginDescriptor PLUGINS_TABLE_NAME[] = {
    { "arguspep.map", "HED:SHC", NULL, 0, &get_sechandler},
    { NULL, NULL, NULL, 0, NULL }
}; 

namespace ArcSec {

class pep_ex {
    public:
        std::string desc;
        pep_ex(const std::string& desc_):desc(desc_) {};
};

Arc::Logger ArgusPEP::logger(Arc::Logger::getRootLogger(), "SecHandler.Argus");

/* extract the elements from the configuration file */
ArgusPEP::ArgusPEP(Arc::Config *cfg):ArcSec::SecHandler(cfg),valid_(false) {  
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
        logger.msg(Arc::DEBUG, "Unknown conversion mode %s", conversion_str);
        conversion = conversion_subject;
    }

    Arc::XMLNode filter = (*cfg)["Filter"];
    if((bool)filter) {
        Arc::XMLNode select_attr = filter["Select"];
        Arc::XMLNode reject_attr = filter["Reject"];
        for(;(bool)select_attr;++select_attr) select_attrs.push_back((std::string)select_attr);
        for(;(bool)reject_attr;++reject_attr) reject_attrs.push_back((std::string)reject_attr);
    };

    /* set up the communication with the pepd host*/    
    valid_ = true;
    pep_handle= NULL;
    res = true;
    
    try {
        pep_handle = pep_initialize();
    
        if (pep_handle == NULL) {
            /* error handling */
            throw pep_ex(std::string("Failed to initialize PEP client:"));
        }
        pep_rc = pep_setoption(pep_handle, PEP_OPTION_ENDPOINT_URL, pepdlocation.c_str());
    
        if (pep_rc != PEP_OK) {
            throw pep_ex("Failed to set PEPd URL: '" + pepdlocation + "'"+ pep_strerror(pep_rc));
        }
    } catch (pep_ex& e) {
        logger.msg(Arc::ERROR, e.desc);
        res = false;
    }
}

ArgusPEP::~ArgusPEP(void) {
    if(pep_handle) pep_destroy(pep_handle);
}

bool ArgusPEP::Handle(Arc::Message* msg) const {
    int rc;             
    bool res = true;
    pep_error_t pep_rc;
    xacml_response_t * response = NULL;
    xacml_request_t * request = NULL;
    std::list<xacml_request_t*> requests;
    std::string subject , resource , action;
    Arc::XMLNode secattr;   
    try{
        if(conversion == conversion_direct) {
            msg->Auth()->Export(Arc::SecAttr::ARCAuth, secattr);
            msg->AuthContext()->Export(Arc::SecAttr::ARCAuth, secattr);
            rc= create_xacml_request(requests,secattr);
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
            rc = create_xacml_request_cream(&request,auths,msg->Attributes(),*payload);
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
            throw pep_ex("The reached decision is: " + Arc::tostring(decision));
        }     
        if(!local_id.empty()) {
            logger.msg(Arc::INFO,"Grid identity is mapped to local identity '%s'", local_id);
            msg->Attributes()->set("SEC:LOCALID", local_id);
        }
    } catch (pep_ex& e) {
        logger.msg(Arc::ERROR,e.desc);
        res = false;
    }
    if(response) xacml_response_delete(response);
    if(request) xacml_request_delete(request);
    while(requests.size() > 0) {
        xacml_request_delete(requests.front());
        requests.pop_front();
    }
    return res;
}

int ArgusPEP::create_xacml_request(xacml_request_t ** request,const char * subjectid, const char * resourceid, const char * actionid) const {
    xacml_subject_t * subject= xacml_subject_create();
    if (subject == NULL) {
        logger.msg(Arc::DEBUG, "Subject of request is null \n");
        return 1;
    }
    xacml_attribute_t * subject_attr_id= xacml_attribute_create(XACML_SUBJECT_ID);
    if (subject_attr_id == NULL) {
    logger.msg(Arc::DEBUG,"Can not create XACML SubjectAttribute: %s\n", XACML_SUBJECT_ID);
        xacml_subject_delete(subject);
        return 1;
    }
    xacml_attribute_addvalue(subject_attr_id,subjectid);
    xacml_attribute_setdatatype(subject_attr_id,XACML_DATATYPE_X500NAME);
    xacml_subject_addattribute(subject,subject_attr_id); 
    xacml_resource_t * resource = xacml_resource_create();
    if (resource == NULL) {
        logger.msg(Arc::DEBUG, "Can not create XACML Resource \n");
        xacml_subject_delete(subject);
        return 2;
    }
    xacml_attribute_t * resource_attr_id = xacml_attribute_create(XACML_RESOURCE_ID);
    if (resource_attr_id == NULL) {
        logger.msg(Arc::DEBUG,"Can not create XACML ResourceAttribute: %s\n", XACML_RESOURCE_ID);
        xacml_subject_delete(subject);
        xacml_resource_delete(resource);
        return 2;
    }
    xacml_attribute_addvalue(resource_attr_id,resourceid);
    xacml_resource_addattribute(resource,resource_attr_id);
    xacml_action_t * action= xacml_action_create();
    if (action == NULL) {
        logger.msg(Arc::DEBUG,"Can not create XACML Action\n");
        xacml_subject_delete(subject);
        xacml_resource_delete(resource);
        return 3;
    }
    xacml_attribute_t * action_attr_id= xacml_attribute_create(XACML_ACTION_ID);
    if (action_attr_id == NULL) {
        logger.msg(Arc::DEBUG,"Can not create XACML ActionAttribute: %s\n", XACML_ACTION_ID);
        xacml_subject_delete(subject);
        xacml_resource_delete(resource);
        xacml_action_delete(action);
        return 3;
    }
    xacml_attribute_addvalue(action_attr_id,actionid);
    xacml_action_addattribute(action,action_attr_id);
    *request= xacml_request_create();
    if (*request == NULL) {
    logger.msg(Arc::DEBUG,"Can not create XACML request\n");
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

int ArgusPEP::create_xacml_request(std::list<xacml_request_t*>& requests, Arc::XMLNode arcreq) const {
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
static std::string get_cream_acion(Arc::XMLNode op) {
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
        std::string str = sa->get(aid);
        if(!str.empty()) return str;
    }
    return "";
}

static std::list<std::string> get_sec_attrs(std::list<Arc::MessageAuth*> auths, const std::string& sid, const std::string& aid) {
    for(std::list<Arc::MessageAuth*>::iterator a = auths.begin(); a != auths.end(); ++a) {
        Arc::SecAttr* sa = (*a)->get(sid);
        std::list<std::string> strs = sa->getAll(aid);
        if(!strs.empty()) return strs;
    }
    return std::list<std::string>();
}

int ArgusPEP::create_xacml_request_cream(xacml_request_t** request, std::list<Arc::MessageAuth*> auths,  Arc::MessageAttributes* attrs, Arc::XMLNode operation) const {
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
    attr = xacml_attribute_create("http://glite.org/xacml/attribute/profile-id");
    if(!attr) throw ierror();
    xacml_attribute_addvalue(attr, "http://glite.org/xacml/profile/grid-ce/1.0");
    xacml_attribute_setdatatype(attr, XACML_DATATYPE_ANYURI);
    xacml_environment_addattribute(environment,attr); attr = NULL;

    // Subject
    attr = xacml_attribute_create("urn:oasis:names:tc:xacml:1.0:subject:subject-id");
    if(!attr) throw ierror();
    std::string subject_str = get_sec_attr(auths, "TLS", "IDENTITY");
    if(subject_str.empty()) throw ierror();
    xacml_attribute_addvalue(attr, subject_str.c_str());
    xacml_attribute_setdatatype(attr, XACML_DATATYPE_STRING);
    xacml_subject_addattribute(subject,attr); attr = NULL;

    attr = xacml_attribute_create("http://glite.org/xacml/attribute/subject-issuer");
    if(!attr) throw ierror();
    std::string ca_str = get_sec_attr(auths, "TLS", "CA");
    if(ca_str.empty()) throw ierror();
    xacml_attribute_addvalue(attr, ca_str.c_str());
    xacml_attribute_setdatatype(attr, XACML_DATATYPE_STRING);
    xacml_subject_addattribute(subject,attr); attr = NULL;

    attr = xacml_attribute_create("http://glite.org/xacml/attribute/virtual-organization");
    if(!attr) throw ierror();
    std::list<std::string> vos = get_sec_attrs(auths, "TLS", "VO");
    for(std::list<std::string>::iterator vo = vos.begin(); vo!=vos.end(); ++vo) {
        if(!vo->empty()) xacml_attribute_addvalue(attr, vo->c_str());
    }
    xacml_attribute_setdatatype(attr, XACML_DATATYPE_STRING);
    xacml_subject_addattribute(subject,attr); attr = NULL;

    attr = xacml_attribute_create("http://glite.org/xacml/attribute/fqan");
    if(!attr) throw ierror();
    std::string pfqan;
    std::list<std::string> fqans = get_sec_attrs(auths, "TLS", "VOMS");
    for(std::list<std::string>::iterator fqan = fqans.begin(); fqan!=fqans.end(); ++fqan) {
        if(pfqan.empty()) pfqan = *fqan;
        if(!fqan->empty()) xacml_attribute_addvalue(attr, fqan->c_str());
    }
    xacml_attribute_setdatatype(attr, XACML_DATATYPE_FQAN);
    xacml_subject_addattribute(subject,attr); attr = NULL;
    if(!pfqan.empty()) {
        attr = xacml_attribute_create("http://glite.org/xacml/attribute/fqan/primary");
        if(!attr) throw ierror();
        xacml_attribute_addvalue(attr, pfqan.c_str());
        xacml_attribute_setdatatype(attr, XACML_DATATYPE_FQAN);
        xacml_subject_addattribute(subject,attr); attr = NULL;
    }

    // Resource
    attr = xacml_attribute_create("urn:oasis:names:tc:xacml:1.0:resource:resource-id");
    if(!attr) throw ierror();
    std::string endpoint = attrs->get("ENDPOINT");
    if(endpoint.empty()) throw ierror();
    xacml_attribute_addvalue(attr, endpoint.c_str());
    xacml_attribute_setdatatype(attr, XACML_DATATYPE_STRING);
    xacml_resource_addattribute(resource,attr); attr = NULL;

    // Action
    attr = xacml_attribute_create("urn:oasis:names:tc:xacml:1.0:action:action-id");
    if(!attr) throw ierror();
    std::string act = get_cream_acion(operation);
    if(act.empty()) throw ierror();
    xacml_attribute_addvalue(attr, act.c_str());
    xacml_attribute_setdatatype(attr, XACML_DATATYPE_STRING);
    xacml_action_addattribute(action,attr); attr = NULL;

    // Add everything into request
    xacml_request_setenvironment(*request,environment); environment = NULL;
    xacml_request_addsubject(*request,subject); subject = NULL;
    xacml_request_addresource(*request,resource); resource = NULL;
    xacml_request_setaction(*request,action); action = NULL;

    return 0;

    } catch(ierror err) {
    if(attr) xacml_attribute_delete(attr);
    if(environment) xacml_environment_delete(environment);
    if(subject) xacml_subject_delete(subject);
    if(resource) xacml_resource_delete(resource);
    if(*request) xacml_request_delete(*request);
    return 1;
    }
}


}  // namespace ArcSec

