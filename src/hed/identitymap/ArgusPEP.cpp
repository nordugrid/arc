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

ArgusPEP::ArgusPEP(Arc::Config *cfg):ArcSec::SecHandler(cfg),valid_(false){  
    pepdlocation = (std::string)(*cfg)["PEPD"];  
    logger.msg(Arc::DEBUG, "PEPD location: %s",pepdlocation);
    valid_ = true;
}

ArgusPEP::~ArgusPEP(void) {
}

bool ArgusPEP::Handle(Arc::Message* msg) const{
  
   pep_error_t pep_rc; 
   int rc;             

   xacml_request_t * request;
   xacml_response_t * response;
  
   std::string dn = msg->Attributes()->get("TLS:IDENTITYDN"); 
 
   /* Not in use for the prototype version */	
   Arc::XMLNode secattr;   
   msg->Auth()->Export(Arc::SecAttr::ARCAuth, secattr);
   msg->AuthContext()->Export(Arc::SecAttr::ARCAuth, secattr);
   std::string resource , action;
   resource= (std::string) secattr["RequestItem"][0]["Resource"][0];
   action=  (std::string) secattr["RequestItem"][0]["Action"][0];


    /* Extract the user subject according to RFC2256 format */	
   std::string subject ;
   do {
        std::string s = dn.substr(dn.rfind("/")+1,dn.length()) ;
        subject = subject+s + "," ;
        dn = dn.substr(0, dn.rfind("/")) ;
   } while (dn.rfind("/")!= std::string::npos) ; 
   subject = subject.substr(0, subject.length()-1);
   char * subjectid= new char [subject.length()]; 
   strcpy(subjectid, subject.c_str());  

   const char * resourceid= "ANY";
   const char * actionid= "ANY";
    

    pep_rc= pep_initialize();
    if (pep_rc != PEP_OK) {
       logger.msg(Arc::DEBUG,"Failed to initialize PEP client: %s\n", pep_strerror(pep_rc));
       return false;
    }

    char * pep_url= new char [pepdlocation.length()];
    strcpy(pep_url,pepdlocation.c_str());
    pep_rc= pep_setoption(PEP_OPTION_ENDPOINT_URL,pep_url);
    if (pep_rc != PEP_OK) {
       logger.msg(Arc::DEBUG,"Failed to set PEPd URL: '%s'",pep_url );
       return false;
    }

    rc= create_xacml_request(&request,subjectid,resourceid,actionid);
 
    if (rc != 0) {
       logger.msg(Arc::DEBUG,"Failed to create XACML request: %d", rc);

	return false;
    }
    pep_rc= pep_authorize(&request,&response);
    if (pep_rc != PEP_OK) {
      logger.msg(Arc::DEBUG,"Failed to authorize XACML request: %s\n",pep_strerror(pep_rc));
       return false;
    }   

    /* Extrac the local user name from the response to be mapped to the GID */
    std::string local_id; 
    size_t results_l= xacml_response_results_length(response);
    int i= 0;

    xacml_decision_t decision; 
    if (response== NULL) {
            logger.msg (Arc::DEBUG, "Response is null");
           return false;
      }

    for(i= 0; i<results_l; i++) {
        xacml_result_t * result= xacml_response_getresult(response,i);        
        decision = xacml_result_getdecision(result);        
 	std::size_t obligations_l= xacml_result_obligations_length(result);
        int j=0;
        for(j= 0; j<obligations_l; j++) {
            xacml_obligation_t * obligation= xacml_result_getobligation(result,j);
            std::size_t attrs_l= xacml_obligation_attributeassignments_length(obligation);
            int k= 0;
            for (k= 0; k<attrs_l; k++) {
                xacml_attributeassignment_t * attr= xacml_obligation_getattributeassignment(obligation,k);
 		local_id =  xacml_attributeassignment_getvalue(attr);             
            }
        }
    }

   if (decision != XACML_DECISION_PERMIT ){
     logger.msg(Arc::INFO,"%s is not authorized to do action %s in resource %s ", subjectid, actionid, resourceid);
     logger.msg(Arc::DEBUG,"The reached decision is: %s ", decision);
      return false;   
   }	 
    // Delete resquest and response
    xacml_request_delete(request);
    xacml_response_delete(response);
        
    // Release the PEP client 
    pep_rc= pep_destroy();
    if (pep_rc != PEP_OK) {
        logger.msg(Arc::DEBUG,"Failed to release PEP client request: %s\n",pep_strerror(pep_rc));   
        return false;
    }
    logger.msg(Arc::INFO,"Grid identity is mapped to local identity '%s'",local_id);
    msg->Attributes()->set("SEC:LOCALID",local_id);

     return true;
 }


int ArgusPEP::create_xacml_request(xacml_request_t ** request,const char * subjectid, const char * resourceid, const char * actionid) const {
    xacml_subject_t * subject= xacml_subject_create();
    if (subject == NULL) {
        logger.msg(Arc::DEBUG, "Subject of request is null \n");
        return 1;
    }
    xacml_attribute_t * subject_attr_id= xacml_attribute_create(XACML_SUBJECT_ID);
    if (subject_attr_id == NULL) {
	logger.msg(Arc::DEBUG,"Can not create XACML SubjectAttribute:%s\n",XACML_SUBJECT_ID);
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
        logger.msg(Arc::DEBUG,"Can not create XACML ResourceAttribute:%s\n",XACML_RESOURCE_ID);
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
} 
