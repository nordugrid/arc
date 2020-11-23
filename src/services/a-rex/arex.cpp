#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <iostream>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <arc/loader/Loader.h>
#include <arc/message/MCCLoader.h>
#include <arc/message/Plexer.h>
#include <arc/message/PayloadSOAP.h>
#include <arc/message/PayloadRaw.h>
#include <arc/message/PayloadStream.h>
#include <arc/message/SecAttr.h>
#include <arc/ws-addressing/WSA.h>
#include <arc/JobPerfLog.h>
#include <arc/Thread.h>
#include <arc/StringConv.h>
#include <arc/FileUtils.h>
#include <arc/Utils.h>
#include <arc/otokens/openid_metadata.h>
#include "grid-manager/log/JobLog.h"
#include "grid-manager/log/JobsMetrics.h"
#include "grid-manager/log/HeartBeatMetrics.h"
#include "grid-manager/log/SpaceMetrics.h"
#include "grid-manager/run/RunPlugin.h"
#include "grid-manager/jobs/ContinuationPlugins.h"
#include "grid-manager/files/ControlFileHandling.h"
#include "arex.h"

namespace ARex {

#define DEFAULT_INFOPROVIDER_WAKEUP_PERIOD (600)
#define DEFAULT_INFOSYS_MAX_CLIENTS (1)
#define DEFAULT_JOBCONTROL_MAX_CLIENTS (100)
#define DEFAULT_DATATRANSFER_MAX_CLIENTS (100)
 
static const std::string BES_ARC_NPREFIX("a-rex");
static const std::string BES_ARC_NAMESPACE("http://www.nordugrid.org/schemas/a-rex");

static const std::string DELEG_ARC_NPREFIX("arcdeleg");
static const std::string DELEG_ARC_NAMESPACE("http://www.nordugrid.org/schemas/delegation");

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

char const* ARexService::InfoPath = "*info";
char const* ARexService::LogsPath = "*logs";
char const* ARexService::NewPath = "*new";
char const* ARexService::DelegationPath = "*deleg";
char const* ARexService::CachePath = "cache";
char const* ARexService::RestPath = "rest";

#define AREX_POLICY_OPERATION_URN "http://www.nordugrid.org/schemas/policy-arc/types/a-rex/operation"
#define AREX_POLICY_OPERATION_ADMIN "Admin"
#define AREX_POLICY_OPERATION_INFO  "Info"

// Id: http://www.nordugrid.org/schemas/policy-arc/types/arex/joboperation
// Value: 
//        Create - creation of new job
//        Modify - modification of job paramaeters - change state, write data.
//        Read   - accessing job information - get status information, read data.
// Id: http://www.nordugrid.org/schemas/policy-arc/types/arex/operation
// Value: 
//        Admin  - administrator level operation
//        Info   - information about service

class ARexSecAttr: public Arc::SecAttr {
 public:
  ARexSecAttr(const std::string& action);
  ARexSecAttr(const Arc::XMLNode op);
  virtual ~ARexSecAttr(void);
  virtual operator bool(void) const;
  virtual bool Export(Arc::SecAttrFormat format,Arc::XMLNode &val) const;
  virtual std::string get(const std::string& id) const;
  void SetResource(const std::string& service, const std::string& job, const std::string& action);
 protected:
  std::string action_;
  std::string id_;
  std::string service_;
  std::string job_;
  std::string file_;
  virtual bool equal(const Arc::SecAttr &b) const;
};

ARexSecAttr::ARexSecAttr(const std::string& action) {
  id_=JOB_POLICY_OPERATION_URN;
  action_=action;
}

ARexSecAttr::ARexSecAttr(const Arc::XMLNode op) {
  if(MatchXMLNamespace(op,BES_ARC_NAMESPACE)) {
    if(MatchXMLName(op,"CacheCheck")) {
      id_=AREX_POLICY_OPERATION_URN;
      action_=AREX_POLICY_OPERATION_INFO;
    }
  } else if(MatchXMLNamespace(op,DELEG_ARC_NAMESPACE)) {
    if(MatchXMLName(op,"DelegateCredentialsInit")) {
      id_=JOB_POLICY_OPERATION_URN;
      action_=JOB_POLICY_OPERATION_CREATE;
    } else if(MatchXMLName(op,"UpdateCredentials")) {
      id_=JOB_POLICY_OPERATION_URN;
      action_=JOB_POLICY_OPERATION_MODIFY;
    }
  } else if(MatchXMLNamespace(op,ES_CREATE_NAMESPACE)) {
    if(MatchXMLName(op,"CreateActivity")) {
      id_=JOB_POLICY_OPERATION_URN;
      action_=JOB_POLICY_OPERATION_CREATE;
    }
  } else if(MatchXMLNamespace(op,ES_DELEG_NAMESPACE)) {
    if(MatchXMLName(op,"InitDelegation")) {
      id_=JOB_POLICY_OPERATION_URN;
      action_=JOB_POLICY_OPERATION_CREATE;
    } else if(MatchXMLName(op,"PutDelegation")) {
      id_=JOB_POLICY_OPERATION_URN;
      action_=JOB_POLICY_OPERATION_MODIFY;
    } else if(MatchXMLName(op,"GetDelegationInfo")) {
      id_=JOB_POLICY_OPERATION_URN;
      action_=JOB_POLICY_OPERATION_READ;
    }
  } else if(MatchXMLNamespace(op,ES_RINFO_NAMESPACE)) {
    if(MatchXMLName(op,"GetResourceInfo")) {
      id_=AREX_POLICY_OPERATION_URN;
      action_=AREX_POLICY_OPERATION_INFO;
    } else if(MatchXMLName(op,"QueryResourceInfo")) {
      id_=AREX_POLICY_OPERATION_URN;
      action_=AREX_POLICY_OPERATION_INFO;
    }
  } else if(MatchXMLNamespace(op,ES_MANAG_NAMESPACE)) {
    if(MatchXMLName(op,"PauseActivity")) {
      id_=JOB_POLICY_OPERATION_URN;
      action_=JOB_POLICY_OPERATION_MODIFY;
    } else if(MatchXMLName(op,"ResumeActivity")) {
      id_=JOB_POLICY_OPERATION_URN;
      action_=JOB_POLICY_OPERATION_MODIFY;
    } else if(MatchXMLName(op,"ResumeActivity")) {
      id_=JOB_POLICY_OPERATION_URN;
      action_=JOB_POLICY_OPERATION_MODIFY;
    } else if(MatchXMLName(op,"NotifyService")) {
      id_=JOB_POLICY_OPERATION_URN;
      action_=JOB_POLICY_OPERATION_MODIFY;
    } else if(MatchXMLName(op,"CancelActivity")) {
      id_=JOB_POLICY_OPERATION_URN;
      action_=JOB_POLICY_OPERATION_MODIFY;
    } else if(MatchXMLName(op,"WipeActivity")) {
      id_=JOB_POLICY_OPERATION_URN;
      action_=JOB_POLICY_OPERATION_MODIFY;
    } else if(MatchXMLName(op,"RestartActivity")) {
      id_=JOB_POLICY_OPERATION_URN;
      action_=JOB_POLICY_OPERATION_MODIFY;
    } else if(MatchXMLName(op,"GetActivityStatus")) {
      id_=JOB_POLICY_OPERATION_URN;
      action_=JOB_POLICY_OPERATION_READ;
    } else if(MatchXMLName(op,"GetActivityInfo")) {
      id_=JOB_POLICY_OPERATION_URN;
      action_=JOB_POLICY_OPERATION_READ;
    }
  } else if(MatchXMLNamespace(op,ES_AINFO_NAMESPACE)) {
    if(MatchXMLName(op,"ListActivities")) {
      id_=JOB_POLICY_OPERATION_URN;
      action_=JOB_POLICY_OPERATION_READ;
    } else if(MatchXMLName(op,"GetActivityStatus")) {
      id_=JOB_POLICY_OPERATION_URN;
      action_=JOB_POLICY_OPERATION_READ;
    } else if(MatchXMLName(op,"GetActivityInfo")) {
      id_=JOB_POLICY_OPERATION_URN;
      action_=JOB_POLICY_OPERATION_READ;
    }
  }
}

void ARexSecAttr::SetResource(const std::string& service, const std::string& job, const std::string& action) {
  service_ = service;
  job_ = job;
  action_ = action;
}

ARexSecAttr::~ARexSecAttr(void) {
}

ARexSecAttr::operator bool(void) const {
  return !action_.empty();
}

bool ARexSecAttr::equal(const SecAttr &b) const {
  try {
    const ARexSecAttr& a = (const ARexSecAttr&)b;
    return ((id_ == a.id_) && (action_ == a.action_));
  } catch(std::exception&) { };
  return false;
}

bool ARexSecAttr::Export(Arc::SecAttrFormat format,Arc::XMLNode &val) const {
  if(format == UNDEFINED) {
  } else if(format == ARCAuth) {
    Arc::NS ns;
    ns["ra"]="http://www.nordugrid.org/schemas/request-arc";
    val.Namespaces(ns); val.Name("ra:Request");
    Arc::XMLNode item = val.NewChild("ra:RequestItem");
    if(!action_.empty()) {
      Arc::XMLNode action = item.NewChild("ra:Action");
      action=action_;
      action.NewAttribute("Type")="string";
      action.NewAttribute("AttributeId")=id_;
    };
    // TODO: add resource part
    return true;
  } else {
  };
  return false;
}

std::string ARexSecAttr::get(const std::string& id) const {
  if(id == "ACTION") return action_;
  if(id == "NAMESPACE") return id_;
  if(id == "SERVICE") return service_;
  if(id == "JOB") return job_;
  if(id == "FILE") return file_;
  return "";
};

static bool match_lists(const std::list<std::pair<bool,std::string> >& list1, const std::list<std::string>& list2, std::string& matched) {
  for(std::list<std::pair<bool,std::string> >::const_iterator l1 = list1.begin(); l1 != list1.end(); ++l1) {
    for(std::list<std::string>::const_iterator l2 = list2.begin(); l2 != list2.end(); ++l2) {
      if((l1->second) == (*l2)) {
        matched = l1->second;
        return l1->first;
      };
    };
  };
  return false;
}

static bool match_groups(std::list<std::pair<bool,std::string> > const & groups, Arc::Message& inmsg) {
  std::string matched_group;
  if(!groups.empty()) {
    Arc::MessageAuth* auth = inmsg.Auth();
    if(auth) {
      Arc::SecAttr* sattr = auth->get("ARCLEGACY");
      if(sattr) {
        if(match_lists(groups, sattr->getAll("GROUP"), matched_group)) {
          return true;
        };
      };
    };
    auth = inmsg.AuthContext();
    if(auth) {
      Arc::SecAttr* sattr = auth->get("ARCLEGACY");
      if(sattr) {
        if(match_lists(groups, sattr->getAll("GROUP"), matched_group)) {
          return true;
        };
      };
    };
  };
  return false;
}

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

//static Arc::LogStream logcerr(std::cerr);

static Arc::Plugin* get_service(Arc::PluginArgument* arg) {
    Arc::ServicePluginArgument* srvarg =
            arg?dynamic_cast<Arc::ServicePluginArgument*>(arg):NULL;
    if(!srvarg) return NULL;
    ARexService* arex = new ARexService((Arc::Config*)(*srvarg),arg);
    if(!*arex) { delete arex; arex=NULL; };
    return arex;
}

void CountedResource::Acquire(void) {
  lock_.lock();
  while((limit_ >= 0) && (count_ >= limit_)) {
    cond_.wait(lock_);
  };
  ++count_;
  lock_.unlock();
}

void CountedResource::Release(void) {
  lock_.lock();
  --count_;
  cond_.signal();
  lock_.unlock();
}

void CountedResource::MaxConsumers(int maxconsumers) {
  limit_ = maxconsumers;
}

CountedResource::CountedResource(int maxconsumers):
                        limit_(maxconsumers),count_(0) {
}

CountedResource::~CountedResource(void) {
}

class CountedResourceLock {
 private:
  CountedResource& r_;
 public:
  CountedResourceLock(CountedResource& resource):r_(resource) {
    r_.Acquire();
  };
  ~CountedResourceLock(void) {
    r_.Release();
  };
};

static std::string GetPath(std::string url){
  std::string::size_type ds, ps;
  ds=url.find("//");
  if (ds==std::string::npos) {
    ps=url.find("/");
  } else {
    ps=url.find("/", ds+2);
  }
  if (ps==std::string::npos) return "";
  return url.substr(ps);
}


Arc::MCC_Status ARexService::make_soap_fault(Arc::Message& outmsg, const char* resp) {
  Arc::PayloadSOAP* outpayload = new Arc::PayloadSOAP(ns_,true);
  Arc::SOAPFault* fault = outpayload?outpayload->Fault():NULL;
  if(fault) {
    fault->Code(Arc::SOAPFault::Sender);
    if(!resp) {
      fault->Reason("Failed processing request");
    } else {
      fault->Reason(resp);
    };
  };
  delete outmsg.Payload(outpayload);
  return Arc::MCC_Status(Arc::STATUS_OK);
}

Arc::MCC_Status ARexService::extract_content(Arc::Message& inmsg,std::string& content,uint32_t size_limit) {
  // Identify payload
  Arc::MessagePayload* payload = inmsg.Payload();
  if(!payload) {
    return Arc::MCC_Status(Arc::GENERIC_ERROR,"","Missing payload");
  };
  Arc::PayloadStreamInterface* stream = dynamic_cast<Arc::PayloadStreamInterface*>(payload);
  Arc::PayloadRawInterface* buf = dynamic_cast<Arc::PayloadRawInterface*>(payload);
  if((!stream) && (!buf)) {
    return Arc::MCC_Status(Arc::GENERIC_ERROR,"","Error processing payload");
  }
  // Fetch content
  content.clear();
  if(stream) {
    std::string add_str;
    while(stream->Get(add_str)) {
      content.append(add_str);
      if((size_limit != 0) && (content.size() >= size_limit)) break;
    }
  } else {
    for(unsigned int n = 0;buf->Buffer(n);++n) {
      content.append(buf->Buffer(n),buf->BufferSize(n));
      if((size_limit != 0) && (content.size() >= size_limit)) break;
    };
  };
  return Arc::MCC_Status(Arc::STATUS_OK);
}


Arc::MCC_Status ARexService::make_http_fault(Arc::Message& outmsg,int code,const char* resp) {
  Arc::PayloadRaw* outpayload = new Arc::PayloadRaw();
  delete outmsg.Payload(outpayload);
  outmsg.Attributes()->set("HTTP:CODE",Arc::tostring(code));
  if(resp) outmsg.Attributes()->set("HTTP:REASON",resp);
  return Arc::MCC_Status(Arc::UNKNOWN_SERVICE_ERROR);
}

Arc::MCC_Status ARexService::make_fault(Arc::Message& /*outmsg*/) {
  // That will cause 500 Internal Error in HTTP
  return Arc::MCC_Status();
}

Arc::MCC_Status ARexService::make_empty_response(Arc::Message& outmsg) {
  Arc::PayloadRaw* outpayload = new Arc::PayloadRaw();
  delete outmsg.Payload(outpayload);
  return Arc::MCC_Status(Arc::STATUS_OK);
}

static std::string GetPath(Arc::Message &inmsg,std::string &base) {
  base = inmsg.Attributes()->get("HTTP:ENDPOINT");
  Arc::AttributeIterator iterator = inmsg.Attributes()->getAll("PLEXER:EXTENSION");
  std::string path;
  if(iterator.hasMore()) {
    // Service is behind plexer
    path = *iterator;
    if(base.length() > path.length()) base.resize(base.length()-path.length());
  } else {
    // Standalone service
    path=Arc::URL(base).Path();
    base.resize(0);
  };
  // Path is encoded in HTTP URLs
  path = Arc::uri_unencode(path);
  return path;
}

#define SOAP_NOT_SUPPORTED { \
  logger_.msg(Arc::ERROR, "SOAP operation is not supported: %s", op.Name()); \
  delete outpayload; \
  return make_soap_fault(outmsg,"Operation not supported"); \
}

static void GetIdFromPath(std::string& subpath, std::string& id) {
  std::string::size_type subpath_pos = Arc::get_token(id, subpath, 0, "/");
  subpath.erase(0,subpath_pos);
  while(subpath[0] == '/') subpath.erase(0,1);
}

Arc::MCC_Status ARexService::preProcessSecurity(Arc::Message& inmsg,Arc::Message& outmsg,Arc::SecAttr* sattr,bool is_soap,ARexConfigContext*& config) {
  config = NULL;
  if(sattr) inmsg.Auth()->set("AREX",sattr);

  {
    Arc::MCC_Status sret = ProcessSecHandlers(inmsg,"incoming");
    if(!sret) {
      logger_.msg(Arc::ERROR, "Security Handlers processing failed: %s", std::string(sret));
      std::string fault_str = "Not authorized: " + std::string(sret);
      return is_soap ?
        make_soap_fault(outmsg, fault_str.c_str()) :
        make_http_fault(outmsg, HTTP_ERR_FORBIDDEN, fault_str.c_str());
    };
  };

  // Process grid-manager configuration if not done yet
  config = ARexConfigContext::GetRutimeConfiguration(inmsg, config_, uname_, endpoint_);
  if(!config) {
    logger_.msg(Arc::ERROR, "Can't obtain configuration");
    // Service is not operational
    char const* fault = "User can't be assigned configuration";
    return is_soap ?
      make_soap_fault(outmsg, fault) :
      make_http_fault(outmsg, HTTP_ERR_FORBIDDEN, fault);
  };
  config->ClearAuths();
  config->AddAuth(inmsg.Auth());
  config->AddAuth(inmsg.AuthContext());

  return Arc::MCC_Status(Arc::STATUS_OK);
}

Arc::MCC_Status ARexService::postProcessSecurity(Arc::Message& outmsg) {
  Arc::MCC_Status sret = ProcessSecHandlers(outmsg,"outgoing");
  if(!sret) {
    logger_.msg(Arc::ERROR, "Security Handlers processing failed: %s", std::string(sret));
    delete outmsg.Payload(NULL);
  };
  return sret;
}

Arc::MCC_Status ARexService::process(Arc::Message& inmsg,Arc::Message& outmsg) {
  // Split request path into parts: service, job and file path. 
  // TODO: make it HTTP independent
  std::string endpoint;
  std::string method = inmsg.Attributes()->get("HTTP:METHOD");
  std::string clientid = (inmsg.Attributes()->get("TCP:REMOTEHOST"))+":"+(inmsg.Attributes()->get("TCP:REMOTEPORT"));
  logger_.msg(Arc::INFO, "Connection from %s: %s", inmsg.Attributes()->get("TCP:REMOTEHOST"), inmsg.Attributes()->get("TLS:IDENTITYDN"));
  std::string subpath = GetPath(inmsg,endpoint);
  if((inmsg.Attributes()->get("PLEXER:PATTERN").empty()) && subpath.empty()) subpath=endpoint;
  logger_.msg(Arc::VERBOSE, "process: method: %s", method);
  logger_.msg(Arc::VERBOSE, "process: endpoint: %s", endpoint);
  std::string id;
  GetIdFromPath(subpath, id);
  // Make SubOpCache separate service
  enum { SubOpNone, SubOpInfo, SubOpLogs, SubOpNew, SubOpDelegation, SubOpCache, SubOpRest } sub_op = SubOpNone;
  // Sort out path
  if(id == InfoPath) {
    sub_op = SubOpInfo;
    id.erase();
  } else if(id == LogsPath) {
    sub_op = SubOpLogs;
    GetIdFromPath(subpath, id);
  } else if(id == NewPath) {
    sub_op = SubOpNew;
    id.erase();
  } else if(id == DelegationPath) {
    sub_op = SubOpDelegation;
    GetIdFromPath(subpath, id);
  } else if(id == CachePath) {
    sub_op = SubOpCache;
    id.erase();
  } else if(id == RestPath) {
    sub_op = SubOpRest;
    id.erase();
  };
  logger_.msg(Arc::VERBOSE, "process: id: %s", id);
  logger_.msg(Arc::VERBOSE, "process: subop: %s", (sub_op==SubOpNone)?"none":
                                                  ((sub_op==SubOpInfo)?InfoPath:
                                                  ((sub_op==SubOpLogs)?LogsPath:
                                                  ((sub_op==SubOpNew)?NewPath:
                                                  ((sub_op==SubOpDelegation)?DelegationPath:
                                                  ((sub_op==SubOpCache)?CachePath:
                                                  ((sub_op==SubOpRest)?RestPath:"unknown")))))));
  logger_.msg(Arc::VERBOSE, "process: subpath: %s", subpath);

  // Switch to REST ASAP
  if(sub_op == SubOpRest) {
    ARexSecAttr* sattr = new ARexSecAttr(std::string(JOB_POLICY_OPERATION_UNDEFINED));
    if(sattr) sattr->SetResource(endpoint,id,subpath);
    ARexConfigContext* config(NULL);
    Arc::MCC_Status sret = preProcessSecurity(inmsg,outmsg,sattr,false,config);
    if(!sret) return sret;

    sret = rest_.process(inmsg, outmsg);

    if(sret) {
      sret = postProcessSecurity(outmsg);
    }

    return sret;
  }

  // Sort out request and identify operation requested
  Arc::PayloadSOAP* inpayload = NULL;
  Arc::XMLNode op;
  ARexSecAttr* sattr = NULL;
  if(method == "POST") {
    logger_.msg(Arc::VERBOSE, "process: POST");
    // Both input and output are supposed to be SOAP
    // Extracting payload
    try {
      inpayload = dynamic_cast<Arc::PayloadSOAP*>(inmsg.Payload());
    } catch(std::exception& e) { };
    if(!inpayload) {
      logger_.msg(Arc::ERROR, "input is not SOAP");
      return make_soap_fault(outmsg);
    };
    if(logger_.getThreshold() <= Arc::VERBOSE) {
      std::string str;
      inpayload->GetDoc(str, true);
      logger_.msg(Arc::VERBOSE, "process: request=%s",str);
    };
    // Analyzing request
    op = inpayload->Child(0);
    if(!op) {
      logger_.msg(Arc::ERROR, "input does not define operation");
      return make_soap_fault(outmsg);
    };
    logger_.msg(Arc::INFO, "process: operation: %s",op.Name());
    // Adding A-REX attributes
    sattr = new ARexSecAttr(op);
  } else if(method == "GET") {
    sattr = new ARexSecAttr(std::string(JOB_POLICY_OPERATION_READ));
  } else if(method == "HEAD") {
    sattr = new ARexSecAttr(std::string(JOB_POLICY_OPERATION_READ));
  } else if(method == "PUT") {
    sattr = new ARexSecAttr(std::string(JOB_POLICY_OPERATION_MODIFY));
  } else if(method == "DELETE") {
    sattr = new ARexSecAttr(std::string(JOB_POLICY_OPERATION_MODIFY));
  }
  if(sattr) sattr->SetResource(endpoint,id,subpath);
  ARexConfigContext* config(NULL);
  Arc::MCC_Status sret = preProcessSecurity(inmsg,outmsg,sattr,method=="POST",config);
  if(!sret) return sret;

  // Identify which of served endpoints request is for.
  // Using simplified algorithm - POST for SOAP messages,
  // GET and PUT for data transfer
  if(method == "POST") {
    // Check if request is for top of tree (factory) or particular 
    // job (listing activity)
    // It must be base URL in request
    if(sub_op != SubOpNone) {
      logger_.msg(Arc::ERROR, "POST request on special path is not supported");
      return make_soap_fault(outmsg);
    };
    if(id.empty()) {
      // Factory operations
      logger_.msg(Arc::VERBOSE, "process: factory endpoint");
      Arc::PayloadSOAP* outpayload = new Arc::PayloadSOAP(ns_);
      Arc::PayloadSOAP& res = *outpayload;
      // Preparing known namespaces
      outpayload->Namespaces(ns_);
      if(config_.EMIESInterfaceEnabled() && MatchXMLNamespace(op,ES_CREATE_NAMESPACE)) {
        // Aplying known namespaces
        inpayload->Namespaces(ns_);
        if(MatchXMLName(op,"CreateActivity")) {
          CountedResourceLock cl_lock(beslimit_);
          ESCreateActivities(*config,op,ESCreateResponse(res,"CreateActivity"),clientid);
        } else {
          SOAP_NOT_SUPPORTED;
        }
      } else if(config_.EMIESInterfaceEnabled() && MatchXMLNamespace(op,ES_RINFO_NAMESPACE)) {
        // Aplying known namespaces
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
      } else if(config_.ARCInterfaceEnabled() && MatchXMLNamespace(op,BES_ARC_NAMESPACE)) {
        // Aplying known namespaces
        inpayload->Namespaces(ns_);
        if(MatchXMLName(op,"CacheCheck")) {
          CacheCheck(*config,*inpayload,*outpayload);
        } else {
          SOAP_NOT_SUPPORTED;
        }
      } else if(delegation_stores_.MatchNamespace(*inpayload)) {
        // Aplying known namespaces
        inpayload->Namespaces(ns_);
        CountedResourceLock cl_lock(beslimit_);
        std::string credentials;
        if(!delegation_stores_.Process(config->GmConfig().DelegationDir(),
                                *inpayload,*outpayload,config->GridName(),credentials)) {
          delete outpayload;
          return make_soap_fault(outmsg);
        };
        if(!credentials.empty()) {
          // Credentials obtained as outcome of operation
          if(MatchXMLNamespace(op,DELEG_ARC_NAMESPACE)) {
            // ARC delegation is done per job but stored under
            // own id. So storing must be done outside processing code.
            UpdateCredentials(*config,op,outpayload->Child(),credentials);
          } else if(MatchXMLNamespace(op,ES_DELEG_NAMESPACE)) {
            // ES has delegations assigned their own ids and are
            // already updated in delegation_stores_.Process()
#if 1
            // But for compatibility during intermediate period store delegations in
            // per-job proxy file too.
            if(op.Name() == "PutDelegation") {
              // PutDelegation
              //   DelegationID
              //   Credential
              std::string id = op["DelegationId"];
              if(!id.empty()) {
                DelegationStore& delegation_store(delegation_stores_[config->GmConfig().DelegationDir()]);
                std::list<std::string> job_ids;
                if(delegation_store.GetLocks(id,config->GridName(),job_ids)) {
                  for(std::list<std::string>::iterator job_id = job_ids.begin(); job_id != job_ids.end(); ++job_id) {
                    // check if that is main delegation for this job
                    std::string delegationid;
                    if(job_local_read_delegationid(*job_id,config->GmConfig(),delegationid)) {
                      if(id == delegationid) {
                        std::string credentials;
                        if(delegation_store.GetCred(id,config->GridName(),credentials)) {
                          if(!credentials.empty()) {
                            GMJob job(*job_id,Arc::User(config->User().get_uid()));
                            (void)job_proxy_write_file(job,config->GmConfig(),credentials);
                          };
                        };
                      };
                    };
                  }; 
                };
              };
            };
#endif
          };
        };
      } else {
        SOAP_NOT_SUPPORTED;
      };
      if(logger_.getThreshold() <= Arc::VERBOSE) {
        std::string str;
        outpayload->GetDoc(str, true);
        logger_.msg(Arc::VERBOSE, "process: response=%s",str);
      };
      outmsg.Payload(outpayload);
    } else {
      // Listing operations for session directories
      // TODO: proper failure like interface is not supported
      logger_.msg(Arc::ERROR, "Per-job POST/SOAP requests are not supported");
      return make_soap_fault(outmsg,"Operation not supported");
    };
    Arc::MCC_Status sret = postProcessSecurity(outmsg);
    if(!sret) return sret;
    return Arc::MCC_Status(Arc::STATUS_OK);
  } else if(method == "GET") {
    // HTTP plugin either provides buffer or stream
    logger_.msg(Arc::VERBOSE, "process: GET");
    logger_.msg(Arc::INFO, "GET: id %s path %s", id, subpath);
    Arc::MCC_Status ret;
    CountedResourceLock cl_lock(datalimit_);
    switch(sub_op) {
      case SubOpInfo:
        ret = GetInfo(inmsg,outmsg,*config,subpath);
        break;
      case SubOpNew:
        ret = GetNew(inmsg,outmsg,*config,subpath);
        break;
      case SubOpLogs:
        ret = GetLogs(inmsg,outmsg,*config,id,subpath);
        break;
      case SubOpDelegation:
        ret = GetDelegation(inmsg,outmsg,*config,id,subpath);
        break;
      case SubOpCache:
        ret = GetCache(inmsg,outmsg,*config,subpath);
        break;
      case SubOpNone:
      default:
        ret = GetJob(inmsg,outmsg,*config,id,subpath);
        break;
    };
    if(ret) {
      Arc::MCC_Status sret = postProcessSecurity(outmsg);
      if(!sret) return sret;
    };
    return ret;
  } else if(method == "HEAD") {
    logger_.msg(Arc::VERBOSE, "process: HEAD");
    logger_.msg(Arc::INFO, "HEAD: id %s path %s", id, subpath);
    Arc::MCC_Status ret;
    CountedResourceLock cl_lock(datalimit_);
    switch(sub_op) {
      case SubOpInfo:
        ret = HeadInfo(inmsg,outmsg,*config,subpath);
        break;
      case SubOpNew:
        ret = HeadNew(inmsg,outmsg,*config,subpath);
        break;
      case SubOpLogs:
        ret = HeadLogs(inmsg,outmsg,*config,id,subpath);
        break;
      case SubOpDelegation:
        ret = HeadDelegation(inmsg,outmsg,*config,id,subpath);
        break;
      case SubOpCache:
        ret = HeadCache(inmsg,outmsg,*config,subpath);
        break;
      case SubOpNone:
      default:
        ret = HeadJob(inmsg,outmsg,*config,id,subpath);
        break;
    };
    if(ret) {
      Arc::MCC_Status sret = postProcessSecurity(outmsg);
      if(!sret) return sret;
    };
    return ret;
  } else if(method == "PUT") {
    logger_.msg(Arc::VERBOSE, "process: PUT");
    Arc::MCC_Status ret;
    CountedResourceLock cl_lock(datalimit_);
    switch(sub_op) {
      case SubOpInfo:
        ret = PutInfo(inmsg,outmsg,*config,subpath);
        break;
      case SubOpNew:
        ret = PutNew(inmsg,outmsg,*config,subpath);
        break;
      case SubOpLogs:
        ret = PutLogs(inmsg,outmsg,*config,id,subpath);
        break;
      case SubOpDelegation:
        ret = PutDelegation(inmsg,outmsg,*config,id,subpath);
        break;
      case SubOpCache:
        ret = PutCache(inmsg,outmsg,*config,subpath);
        break;
      case SubOpNone:
      default:
        ret = PutJob(inmsg,outmsg,*config,id,subpath);
        break;
    };
    if(ret) {
      Arc::MCC_Status sret = postProcessSecurity(outmsg);
      if(!sret) return sret;
    };
    return ret;
  } else if(method == "DELETE") {
    logger_.msg(Arc::VERBOSE, "process: DELETE");
    Arc::MCC_Status ret;
    CountedResourceLock cl_lock(datalimit_);
    switch(sub_op) {
      case SubOpInfo:
        ret = DeleteInfo(inmsg,outmsg,*config,subpath);
        break;
      case SubOpNew:
        ret = DeleteNew(inmsg,outmsg,*config,subpath);
        break;
      case SubOpLogs:
        ret = DeleteLogs(inmsg,outmsg,*config,id,subpath);
        break;
      case SubOpDelegation:
        ret = DeleteDelegation(inmsg,outmsg,*config,id,subpath);
        break;
      case SubOpCache:
        ret = DeleteCache(inmsg,outmsg,*config,subpath);
        break;
      case SubOpNone:
      default:
        ret = DeleteJob(inmsg,outmsg,*config,id,subpath);
        break;
    };
    if(ret) {
      Arc::MCC_Status sret = postProcessSecurity(outmsg);
      if(!sret) return sret;
    };
    return ret;
  } else if(!method.empty()) {
    logger_.msg(Arc::VERBOSE, "process: method %s is not supported",method);
    return make_http_fault(outmsg,501,"Not Implemented");
  } else {
    logger_.msg(Arc::VERBOSE, "process: method is not defined");
    return Arc::MCC_Status();
  };
  return Arc::MCC_Status();
}

Arc::MCC_Status ARexService::HeadDelegation(Arc::Message& inmsg,Arc::Message& outmsg,ARexGMConfig& config,std::string const& id,std::string const& subpath) {
  return make_http_fault(outmsg,405,"No probing on delegation interface");
}

Arc::MCC_Status ARexService::GetDelegation(Arc::Message& inmsg,Arc::Message& outmsg,ARexGMConfig& config,std::string const& id,std::string const& subpath) {
  if(!&config) {
    return make_http_fault(outmsg, HTTP_ERR_FORBIDDEN, "User is not identified");
  };
  if(!subpath.empty()) {
    return make_http_fault(outmsg,500,"No additional path expected");
  };
  std::string deleg_id = id; // Update request in case of non-empty id
  std::string deleg_request;
  if(!delegation_stores_.GetRequest(config.GmConfig().DelegationDir(),
                                    deleg_id,config.GridName(),deleg_request)) {
    return make_http_fault(outmsg,500,"Failed generating delegation request");
  };
  // Create positive HTTP response
  Arc::PayloadRaw* buf = new Arc::PayloadRaw;
  if(buf) buf->Insert(deleg_request.c_str(),0,deleg_request.length());
  outmsg.Payload(buf);
  outmsg.Attributes()->set("HTTP:content-type","application/x-pem-file"); // ??
  outmsg.Attributes()->set("HTTP:CODE",Arc::tostring(200));
  outmsg.Attributes()->set("HTTP:REASON",deleg_id.c_str());
  return Arc::MCC_Status(Arc::STATUS_OK);
}

Arc::MCC_Status ARexService::PutDelegation(Arc::Message& inmsg,Arc::Message& outmsg,ARexGMConfig& config,std::string const& id,std::string const& subpath) {
  if(!subpath.empty()) {
    return make_http_fault(outmsg,500,"No additional path expected");
  };
  if(id.empty()) {
    return make_http_fault(outmsg,500,"Delegation id expected");
  };
  // Fetch HTTP content
  std::string content;
  Arc::MCC_Status res = ARexService::extract_content(inmsg,content,1024*1024); // 1mb size limit is sane enough
  if(!res)
    return make_http_fault(outmsg,500,res.getExplanation().c_str());
  if(content.empty())
    return make_http_fault(outmsg,500,"Missing payload");
  if(!delegation_stores_.PutDeleg(config.GmConfig().DelegationDir(),
                                    id,config.GridName(),content)) {
    return make_http_fault(outmsg,500,"Failed accepting delegation");
  };
#if 1
  // In case of update for compatibility during intermediate period store delegations in
  // per-job proxy file too.
  DelegationStore& delegation_store(delegation_stores_[config.GmConfig().DelegationDir()]);
  std::list<std::string> job_ids;
  if(delegation_store.GetLocks(id,config.GridName(),job_ids)) {
    for(std::list<std::string>::iterator job_id = job_ids.begin(); job_id != job_ids.end(); ++job_id) {
      // check if that is main delegation for this job
      std::string delegationid;
      if(job_local_read_delegationid(*job_id,config.GmConfig(),delegationid)) {
        if(id == delegationid) {
          std::string credentials;
          if(delegation_store.GetCred(id,config.GridName(),credentials)) {
            if(!credentials.empty()) {
              GMJob job(*job_id,Arc::User(config.User().get_uid()));
              (void)job_proxy_write_file(job,config.GmConfig(),credentials);
            };
          };
        };
      };
    };
  };
#endif
  return make_empty_response(outmsg);
}

Arc::MCC_Status ARexService::DeleteDelegation(Arc::Message& inmsg,Arc::Message& outmsg,ARexGMConfig& config,std::string const& id,std::string const& subpath) {
  return make_http_fault(outmsg,501,"Not Implemented");
}

static void information_collector_starter(void* arg) {
  if(!arg) return;
  ((ARexService*)arg)->InformationCollector();
}

void ARexService::gm_threads_starter(void* arg) {
  if(!arg) return;
  ARexService* arex = (ARexService*)arg;
  arex->gm_threads_starter();
}

void ARexService::gm_threads_starter() {
  if (!endpoint_.empty()) { // no need to do this if no WS interface
    // Remove the WS log from the log destinations.
    // Here we assume the order is gm, ws, [stderr (arched -f)]
    std::list<Arc::LogDestination*> dests = Arc::Logger::getRootLogger().getDestinations();
    if (dests.size() > 1) {
      std::list<Arc::LogDestination*>::iterator i = dests.begin();
      ++i;
      dests.erase(i);

      Arc::Logger::getRootLogger().setThreadContext();
      Arc::Logger::getRootLogger().removeDestinations();
      Arc::Logger::getRootLogger().addDestinations(dests);
    }
  }
  // Run grid-manager in thread
  if ((gmrun_.empty()) || (gmrun_ == "internal")) {
    gm_ = new GridManager(config_);
    if (!(*gm_)) {
      logger_.msg(Arc::ERROR, "Failed to run Grid Manager thread");
      delete gm_; gm_=NULL; return;
    }
  }
  // Start info collector thread
  CreateThreadFunction(&information_collector_starter, this);
}

class ArexServiceNamespaces: public Arc::NS {
 public:
  ArexServiceNamespaces() {
    // Define supported namespaces
    Arc::NS& ns_(*this);
    ns_[BES_ARC_NPREFIX]=BES_ARC_NAMESPACE;
    ns_[DELEG_ARC_NPREFIX]=DELEG_ARC_NAMESPACE;
    ns_[ES_TYPES_NPREFIX]=ES_TYPES_NAMESPACE;
    ns_[ES_CREATE_NPREFIX]=ES_CREATE_NAMESPACE;
    ns_[ES_DELEG_NPREFIX]=ES_DELEG_NAMESPACE;
    ns_[ES_RINFO_NPREFIX]=ES_RINFO_NAMESPACE;
    ns_[ES_MANAG_NPREFIX]=ES_MANAG_NAMESPACE;
    ns_[ES_AINFO_NPREFIX]=ES_AINFO_NAMESPACE;
    ns_["wsa"]="http://www.w3.org/2005/08/addressing";
    ns_["jsdl"]="http://schemas.ggf.org/jsdl/2005/11/jsdl";
    ns_["wsrf-bf"]="http://docs.oasis-open.org/wsrf/bf-2";
    ns_["wsrf-r"]="http://docs.oasis-open.org/wsrf/r-2";
    ns_["wsrf-rw"]="http://docs.oasis-open.org/wsrf/rw-2";
  };
};

Arc::NS ARexService::ns_ = ArexServiceNamespaces();

ARexService::ARexService(Arc::Config *cfg,Arc::PluginArgument *parg):Arc::Service(cfg,parg),
              logger_(Arc::Logger::rootLogger, "A-REX"),
              delegation_stores_(),
              infodoc_(true),
              infoprovider_wakeup_period_(0),
              all_jobs_count_(0),
              gm_(NULL),
              rest_(cfg, parg, config_, delegation_stores_, all_jobs_count_) {
  valid = false;
  config_.SetJobLog(new JobLog());
  config_.SetJobsMetrics(new JobsMetrics());
  config_.SetHeartBeatMetrics(new HeartBeatMetrics());
  config_.SetSpaceMetrics(new SpaceMetrics());
  config_.SetJobPerfLog(new Arc::JobPerfLog());
  config_.SetContPlugins(new ContinuationPlugins());
  // logger_.addDestination(logcerr);
  // Obtain information from configuration

  endpoint_=(std::string)((*cfg)["endpoint"]);
  uname_=(std::string)((*cfg)["usermap"]["defaultLocalName"]);
  std::string gmconfig=(std::string)((*cfg)["gmconfig"]);
  if (Arc::lower((std::string)((*cfg)["publishStaticInfo"])) == "yes") {
    publishstaticinfo_=true;
  } else {
    publishstaticinfo_=false;
  }
  config_.SetDelegations(&delegation_stores_);
	config_.SetConfigFile(gmconfig);
  if (!config_.Load()) {
    logger_.msg(Arc::ERROR, "Failed to process configuration in %s", gmconfig);
    return;
  }
  // Check for mandatory commands in configuration
  if (config_.ControlDir().empty()) {
    logger.msg(Arc::ERROR, "No control directory set in configuration");
    return;
  }
  if (config_.SessionRoots().empty()) {
    logger.msg(Arc::ERROR, "No session directory set in configuration");
    return;
  }
  if (config_.DefaultLRMS().empty()) {
    logger.msg(Arc::ERROR, "No LRMS set in configuration");
    return;
  }
  // create control directory if not yet done
  if(!config_.CreateControlDirectory()) {
    logger_.msg(Arc::ERROR, "Failed to create control directory %s", config_.ControlDir());
    return;
  }

  // Pass information about delegation db type
  {
  DelegationStore::DbType deleg_db_type = DelegationStore::DbBerkeley;
    switch(config_.DelegationDBType()) {
     case GMConfig::deleg_db_bdb:
      deleg_db_type = DelegationStore::DbBerkeley;
      break;
     case GMConfig::deleg_db_sqlite:
      deleg_db_type = DelegationStore::DbSQLite;
      break;
    };
    delegation_stores_.SetDbType(deleg_db_type);
  };

  // Set default queue if none given
  if(config_.DefaultQueue().empty() && (config_.Queues().size() == 1)) {
    config_.SetDefaultQueue(config_.Queues().front());
  }
  gmrun_ = (std::string)((*cfg)["gmrun"]);
  common_name_ = (std::string)((*cfg)["commonName"]);
  long_description_ = (std::string)((*cfg)["longDescription"]);
  // TODO: check for enumeration values
  os_name_ = (std::string)((*cfg)["OperatingSystem"]);
  std::string debugLevel = (std::string)((*cfg)["debugLevel"]);
  if(!debugLevel.empty()) {
    logger_.setThreshold(Arc::istring_to_level(debugLevel));
  };
  int valuei;
  if ((!(*cfg)["InfoproviderWakeupPeriod"]) ||
      (!Arc::stringto((std::string)((*cfg)["InfoproviderWakeupPeriod"]),infoprovider_wakeup_period_))) {
    infoprovider_wakeup_period_ = DEFAULT_INFOPROVIDER_WAKEUP_PERIOD;
  };
  if ((!(*cfg)["InfosysInterfaceMaxClients"]) ||
      (!Arc::stringto((std::string)((*cfg)["InfosysInterfaceMaxClients"]),valuei))) {
    valuei = DEFAULT_INFOSYS_MAX_CLIENTS;
  };
  infolimit_.MaxConsumers(valuei);
  if ((!(*cfg)["JobControlInterfaceMaxClients"]) ||
      (!Arc::stringto((std::string)((*cfg)["JobControlInterfaceMaxClients"]),valuei))) {
    valuei = DEFAULT_JOBCONTROL_MAX_CLIENTS;
  };
  beslimit_.MaxConsumers(valuei);
  if ((!(*cfg)["DataTransferInterfaceMaxClients"]) ||
      (!Arc::stringto((std::string)((*cfg)["DataTransferInterfaceMaxClients"]),valuei))) {
    valuei = DEFAULT_DATATRANSFER_MAX_CLIENTS;
  };
  datalimit_.MaxConsumers(valuei);

  // If WS interface is enabled and multiple log files are configured then here
  // the log splits between WS interface operations and GM job processing.
  // Start separate thread to start GM and info collector threads so they can
  // log to GM log after we remove it in this thread
  Arc::SimpleCounter counter;
  if (!CreateThreadFunction(&gm_threads_starter, this, &counter)) return;
  counter.wait();
  if ((gmrun_.empty() || gmrun_ == "internal") && !gm_) return; // GM didn't start
  // If WS is used then remove gm log destination from this thread
  if (!endpoint_.empty()) {
    // Assume that gm log is first in list - potentially dangerous
    std::list<Arc::LogDestination*> dests = logger.getRootLogger().getDestinations();
    if (dests.size() > 1) {
      dests.pop_front();
      logger.getRootLogger().removeDestinations();
      logger.getRootLogger().addDestinations(dests);
    }
  }
  valid=true;
}

ARexService::~ARexService(void) {
  thread_count_.RequestCancel();
  delete gm_; // This should stop all GM-related threads too
  thread_count_.WaitForExit(); // Here A-REX threads are waited for
  // There should be no more threads using resources - can proceed
  if(config_.ConfigIsTemp()) unlink(config_.ConfigFile().c_str());
  delete config_.GetContPlugins();
  delete config_.GetJobLog();
  delete config_.GetJobPerfLog();
  delete config_.GetJobsMetrics();
  delete config_.GetHeartBeatMetrics();
  delete config_.GetSpaceMetrics();
}

} // namespace ARex

extern Arc::PluginDescriptor const ARC_PLUGINS_TABLE_NAME[] = {
    { "a-rex", "HED:SERVICE", NULL, 0, &ARex::get_service },
    { NULL, NULL, NULL, 0, NULL }
};
