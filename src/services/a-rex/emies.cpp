#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/Thread.h>
#include <arc/Utils.h>
#include <arc/message/PayloadSOAP.h>
#include "arex.h"

namespace ARex {

#define SOAP_NOT_SUPPORTED { \
  logger_.msg(Arc::ERROR, "SOAP operation is not supported: %s", op.Name()); \
  delete outpayload; \
  return make_soap_fault(outmsg,"Operation not supported"); \
}

static const std::string ES_TYPES_NPREFIX("estypes");
static const std::string ES_TYPES_NAMESPACE("http://www.eu-emi.eu/es/2010/12/types");

static const std::string ES_CREATE_NPREFIX("escreate");
static const std::string ES_CREATE_NAMESPACE("http://www.eu-emi.eu/es/2010/12/creation/types");

static const std::string ES_DELEG_NPREFIX("esdeleg");
static const std::string ES_DELEG_NAMESPACE("http://www.eu-emi.eu/es/2010/12/delegation/types");

static const std::string ES_RINFO_NPREFIX("esrinfo");
static const std::string ES_RINFO_NAMESPACE("http://www.eu-emi.eu/es/2010/12/resourceinfo/types");

static const std::string ES_MANAG_NPREFIX("esmanag");
static const std::string ES_MANAG_NAMESPACE("http://www.eu-emi.eu/es/2010/12/activitymanagement/types");

static const std::string ES_AINFO_NPREFIX("esainfo");
static const std::string ES_AINFO_NAMESPACE("http://www.eu-emi.eu/es/2010/12/activity/types");

static Arc::XMLNode ESCreateResponse(Arc::PayloadSOAP& res,const char* opname) {
  Arc::XMLNode response = res.NewChild(ES_CREATE_NPREFIX + ":" + opname + "Response");
  return response;
}

/*
static Arc::XMLNode ESDelegResponse(Arc::PayloadSOAP& res,const char* opname) {
  Arc::XMLNode response = res.NewChild(ES_DELEG_NPREFIX + ":" + opname + "Response");
  return response;
}
*/

static Arc::XMLNode ESRInfoResponse(Arc::PayloadSOAP& res,const char* opname) {
  Arc::XMLNode response = res.NewChild(ES_RINFO_NPREFIX + ":" + opname + "Response");
  return response;
}

static Arc::XMLNode ESManagResponse(Arc::PayloadSOAP& res,const char* opname) {
  Arc::XMLNode response = res.NewChild(ES_MANAG_NPREFIX + ":" + opname + "Response");
  return response;
}

static Arc::XMLNode ESAInfoResponse(Arc::PayloadSOAP& res,const char* opname) {
  Arc::XMLNode response = res.NewChild(ES_AINFO_NPREFIX + ":" + opname + "Response");
  return response;
}

Arc::MCC_Status ARexService::ESOperations(ARexConfigContext* config, std::string const & clientid,
                                          Arc::XMLNode op, Arc::PayloadSOAP* inpayload,
                                          Arc::Message& outmsg, bool& processed, bool& passed) {
      processed = true;
      passed = false;
      Arc::PayloadSOAP* outpayload = new Arc::PayloadSOAP(ns_);
      // Preparing known namespaces
      outpayload->Namespaces(ns_);
      Arc::PayloadSOAP& res = *outpayload;
      if(config_.EMIESInterfaceEnabled() && MatchXMLNamespace(op,ES_CREATE_NAMESPACE)) {
        if(!config) {
          delete outpayload;
          return make_soap_fault(outmsg, "User can't be assigned configuration");
        }
        // Applying known namespaces
        inpayload->Namespaces(ns_);
        if(MatchXMLName(op,"CreateActivity")) {
          CountedResourceLock cl_lock(beslimit_);
          ESCreateActivities(*config,op,ESCreateResponse(res,"CreateActivity"),clientid);
        } else {
          SOAP_NOT_SUPPORTED;
        }
      } else if(config_.EMIESInterfaceEnabled() && MatchXMLNamespace(op,ES_RINFO_NAMESPACE)) {
        // Aplying known namespaces
        // Resource information is available for anonymous requests - null config is handled properly
        inpayload->Namespaces(ns_);
        if(MatchXMLName(op,"GetResourceInfo")) {
          CountedResourceLock cl_lock(infolimit_);
          ESGetResourceInfo(*config,op,ESRInfoResponse(res,"GetResourceInfo"));
        } else if(MatchXMLName(op,"QueryResourceInfo")) {
          CountedResourceLock cl_lock(infolimit_);
          ESQueryResourceInfo(*config,op,ESRInfoResponse(res,"QueryResourceInfo"));
        } else {
          SOAP_NOT_SUPPORTED;
        }
      } else if(config_.EMIESInterfaceEnabled() && MatchXMLNamespace(op,ES_MANAG_NAMESPACE)) {
        if(!config) {
          delete outpayload;
          return make_soap_fault(outmsg, "User can't be assigned configuration");
        }
        // Aplying known namespaces
        inpayload->Namespaces(ns_);
        if(MatchXMLName(op,"PauseActivity")) {
          CountedResourceLock cl_lock(beslimit_);
          ESPauseActivity(*config,op,ESManagResponse(res,"PauseActivity"));
        } else if(MatchXMLName(op,"ResumeActivity")) {
          CountedResourceLock cl_lock(beslimit_);
          ESResumeActivity(*config,op,ESManagResponse(res,"ResumeActivity"));
        } else if(MatchXMLName(op,"NotifyService")) {
          CountedResourceLock cl_lock(beslimit_);
          ESNotifyService(*config,op,ESManagResponse(res,"NotifyService"));
        } else if(MatchXMLName(op,"CancelActivity")) {
          CountedResourceLock cl_lock(beslimit_);
          ESCancelActivity(*config,op,ESManagResponse(res,"CancelActivity"));
        } else if(MatchXMLName(op,"WipeActivity")) {
          CountedResourceLock cl_lock(beslimit_);
          ESWipeActivity(*config,op,ESManagResponse(res,"WipeActivity"));
        } else if(MatchXMLName(op,"RestartActivity")) {
          CountedResourceLock cl_lock(beslimit_);
          ESRestartActivity(*config,op,ESManagResponse(res,"RestartActivity"));
        } else if(MatchXMLName(op,"GetActivityStatus")) {
          CountedResourceLock cl_lock(beslimit_);
          ESGetActivityStatus(*config,op,ESManagResponse(res,"GetActivityStatus"));
        } else if(MatchXMLName(op,"GetActivityInfo")) {
          CountedResourceLock cl_lock(beslimit_);
          ESGetActivityInfo(*config,op,ESManagResponse(res,"GetActivityInfo"));
        } else {
          SOAP_NOT_SUPPORTED;
        }
      } else if(config_.EMIESInterfaceEnabled() && MatchXMLNamespace(op,ES_AINFO_NAMESPACE)) {
        if(!config) {
          delete outpayload;
          return make_soap_fault(outmsg, "User can't be assigned configuration");
        }
        // Aplying known namespaces
        inpayload->Namespaces(ns_);
        if(MatchXMLName(op,"ListActivities")) {
          CountedResourceLock cl_lock(beslimit_);
          ESListActivities(*config,op,ESAInfoResponse(res,"ListActivities"));
        } else if(MatchXMLName(op,"GetActivityStatus")) {
          CountedResourceLock cl_lock(beslimit_);
          ESGetActivityStatus(*config,op,ESAInfoResponse(res,"GetActivityStatus"));
        } else if(MatchXMLName(op,"GetActivityInfo")) {
          CountedResourceLock cl_lock(beslimit_);
          ESGetActivityInfo(*config,op,ESAInfoResponse(res,"GetActivityInfo"));
        } else {
          SOAP_NOT_SUPPORTED;
        }
      } else {
        processed = false;
      };

  if(processed) {
    outmsg.Payload(outpayload);
  } else {
    delete outpayload;
  };

  passed = true;

  return Arc::MCC_Status(Arc::STATUS_OK);
}

} // namespace ARex 
