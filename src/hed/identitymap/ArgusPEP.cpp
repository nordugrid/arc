#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <fstream>
#include <vector>
#include <arc/Logger.h>
#include <arc/StringConv.h>
#include <arc/message/MCCLoader.h>
#include <arc/XMLNode.h>
#include <arc/message/SecAttr.h>
#include "ArgusPEP.h"


static Arc::Plugin* get_sechandler(Arc::PluginArgument* arg) {
  ArcSec::SecHandlerPluginArgument* shcarg =  arg?dynamic_cast<ArcSec::SecHandlerPluginArgument*>(arg):NULL;
  if(!shcarg)   return NULL;
  ArcSec::ArgusPEP* plugin = new ArcSec::ArgusPEP((Arc::Config*)(*shcarg));
  if(!plugin)    return NULL;
  if(!(*plugin)) {    delete plugin;    return NULL; };
  return plugin;
}


Arc::PluginDescriptor PLUGINS_TABLE_NAME[] = {
  { "arguspep.map", "HED:SHC", 0, &get_sechandler},
  { NULL, NULL, 0, NULL }
}; 

namespace ArcSec {

Arc::Logger ArgusPEP::logger(SecHandler::logger,"Argus");

ArgusPEP::ArgusPEP(Arc::Config *cfg):ArcSec::SecHandler(cfg),valid_(false) {  
    pepdlocation = (std::string)(*cfg)["PEPD"];  
    if(pepdlocation.empty()) {
      logger.msg(Arc::ERROR, "PEPD location is missing");
      return;
    }
    logger.msg(Arc::DEBUG, "PEPD location: %s",pepdlocation);
    if((std::string)(*cfg)["Conversion"] == "direct") {
      logger.msg(Arc::DEBUG, "Conversion mode is set to DIRECT");
      conversion = conversion_direct;
    } else {
      logger.msg(Arc::DEBUG, "Conversion mode is set to SUBJECT");
      conversion = conversion_subject;
    }
    Arc::XMLNode filter = (*cfg)["Filter"];
    if((bool)filter) {
      Arc::XMLNode select_attr = filter["Select"];
      Arc::XMLNode reject_attr = filter["Reject"];
      for(;(bool)select_attr;++select_attr) select_attrs.push_back((std::string)select_attr);
      for(;(bool)reject_attr;++reject_attr) reject_attrs.push_back((std::string)reject_attr);
    };

    valid_ = true;
}

ArgusPEP::~ArgusPEP(void) {
}

class pep_ex {
  public:
    std::string desc;
    pep_ex(const std::string& desc_):desc(desc_) {};
};

bool ArgusPEP::Handle(Arc::Message* msg) const {
  
    bool res = true;
    pep_error_t pep_rc; 
    int rc;             

    xacml_response_t * response = NULL;
    xacml_request_t * request = NULL;
    std::list<xacml_request_t*> requests;
    bool client_initialized = false;

    try {
  
    pep_rc= pep_initialize();
    if (pep_rc != PEP_OK) {
        throw pep_ex(std::string("Failed to initialize PEP client: ")+pep_strerror(pep_rc));
    }

    pep_rc= pep_setoption(PEP_OPTION_ENDPOINT_URL,pepdlocation.c_str());
    if (pep_rc != PEP_OK) {
        throw pep_ex("Failed to set PEPd URL: '"+pepdlocation+"'");
    }

    std::string subject , resource , action;
    Arc::XMLNode secattr;   

    if(conversion == conversion_direct) {
        msg->Auth()->Export(Arc::SecAttr::ARCAuth, secattr);
        msg->AuthContext()->Export(Arc::SecAttr::ARCAuth, secattr);
        rc= create_xacml_request(requests,secattr);
    } else {
        //resource= (std::string) secattr["RequestItem"][0]["Resource"][0];
        //action=  (std::string) secattr["RequestItem"][0]["Action"][0];

        /* Extract the user subject according to RFC2256 format */	
        std::string dn = msg->Attributes()->get("TLS:IDENTITYDN"); 
        do {
            std::string s = dn.substr(dn.rfind("/")+1,dn.length()) ;
            subject = subject+s + "," ;
            dn = dn.substr(0, dn.rfind("/")) ;
        } while (dn.rfind("/")!= std::string::npos) ; 
        subject = subject.substr(0, subject.length()-1);

        if(resource.empty()) resource = "ANY";
        if(action.empty()) action = "ANY";

        rc= create_xacml_request(&request,subject.c_str(),resource.c_str(),action.c_str());
        if(request != NULL) requests.push_back(request);
        request = NULL;
    }
    if (rc != 0) {
        throw pep_ex("Failed to create XACML request(s): "+Arc::tostring(rc));
    }

    bool authorized = false;
    std::string local_id; 
    xacml_decision_t decision = XACML_DECISION_INDETERMINATE; 
    // Simple combining algorithm. At least one deny means deny. If none, then at 
    // least one permit means permit. Otherwise deny. TODO: configurable.

    logger.msg(Arc::DEBUG, "Have %i requests to process",requests.size());
    while(requests.size() > 0) {
        request = requests.front();
        requests.pop_front();
        pep_rc= pep_authorize(&request,&response);
        if (pep_rc != PEP_OK) {
           throw pep_ex(std::string("Failed to process XACML request: ")+pep_strerror(pep_rc));
            pep_destroy();
        }   
        if (response== NULL) {
            throw pep_ex("XACML response is empty");
        }
        /* Extract the local user name from the response to be mapped to the GID */
        size_t results_l= xacml_response_results_length(response);
        int i= 0;
        for(i= 0; i<results_l; i++) {
            xacml_result_t * result= xacml_response_getresult(response,i);        
            if(result == NULL) break;
            switch(xacml_result_getdecision(result)) {
              case XACML_DECISION_DENY: decision = XACML_DECISION_DENY; break;
              case XACML_DECISION_PERMIT: decision = XACML_DECISION_PERMIT; break;
            };
            if(decision == XACML_DECISION_DENY) break;
 	    std::size_t obligations_l= xacml_result_obligations_length(result);
            int j=0;
            for(j= 0; j<obligations_l; j++) {
                xacml_obligation_t * obligation= xacml_result_getobligation(result,j);
                if(obligation == NULL) break;
                std::size_t attrs_l= xacml_obligation_attributeassignments_length(obligation);
                int k= 0;
                for (k= 0; k<attrs_l; k++) {
                    xacml_attributeassignment_t * attr= xacml_obligation_getattributeassignment(obligation,k);
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
            logger.msg(Arc::INFO,"Not authorized according to request:\n%s",
                       xml);
        } else {
            logger.msg(Arc::INFO,"%s is not authorized to do action %s in resource %s ",
                       subject, action, resource);
        }
        throw pep_ex("The reached decision is: " + Arc::tostring(decision));
    }
        
    if(!local_id.empty()) {
        logger.msg(Arc::INFO,"Grid identity is mapped to local identity '%s'",local_id);
        msg->Attributes()->set("SEC:LOCALID",local_id);
    }

    } catch (pep_ex& e) {
        logger.msg(Arc::ERROR,e.desc);
        res = false;
    }

   /*  clean up */
   if(client_initialized) {
      // Release the PEP client 
      pep_rc= pep_destroy();
      if (pep_rc != PEP_OK) {
          logger.msg(Arc::DEBUG,"Failed to release PEP client request: %s\n",pep_strerror(pep_rc));
      }
    }

    // Delete resquest and response
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
	logger.msg(Arc::DEBUG,"Can not create XACML SubjectAttribute: %s\n",XACML_SUBJECT_ID);
        xacml_subject_delete(subject);
        return 1;
    }
    xacml_attribute_addvalue(subject_attr_id,subjectid);
    xacml_attribute_setdatatype(subject_attr_id,XACML_DATATYPE_X500NAME);
    xacml_subject_addattribute(subject,subject_attr_id); 
    xacml_resource_t * resource= xacml_resource_create();
    if (resource == NULL) {
        logger.msg(Arc::DEBUG,"Can not create XACML Resource \n");
        xacml_subject_delete(subject);
        return 2;
    }
    xacml_attribute_t * resource_attr_id= xacml_attribute_create(XACML_RESOURCE_ID);
    if (resource_attr_id == NULL) {
        logger.msg(Arc::DEBUG,"Can not create XACML ResourceAttribute: %s\n",XACML_RESOURCE_ID);
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
        logger.msg(Arc::DEBUG,"Can not create XACML ActionAttribute: %s\n",XACML_ACTION_ID);
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

int ArgusPEP::create_xacml_request(std::list<xacml_request_t*>& requests,Arc::XMLNode arcreq) const {
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
            xacml_attribute_addvalue(attr,((std::string)arcattr).c_str());
            xacml_attribute_setdatatype(attr,XACML_DATATYPE_STRING);
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

} 

