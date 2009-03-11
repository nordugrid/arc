#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <iostream>

#include <sys/types.h>
#include <pwd.h>
#include <unistd.h>

#include <arc/loader/Loader.h>
#include <arc/message/MCCLoader.h>
#include <arc/message/Plexer.h>
#include <arc/message/PayloadSOAP.h>
#include <arc/message/PayloadRaw.h>
#include <arc/message/PayloadStream.h>
#include <arc/ws-addressing/WSA.h>
#include <arc/Thread.h>
#include "job.h"

#include "arex.h"

namespace ARex {

static const std::string BES_FACTORY_ACTIONS_BASE_URL("http://schemas.ggf.org/bes/2006/08/bes-factory/BESFactoryPortType/");
static const std::string BES_FACTORY_NPREFIX("bes-factory");
static const std::string BES_FACTORY_NAMESPACE("http://schemas.ggf.org/bes/2006/08/bes-factory");

static const std::string BES_MANAGEMENT_ACTIONS_BASE_URL("http://schemas.ggf.org/bes/2006/08/bes-management/BESManagementPortType/");
static const std::string BES_MANAGEMENT_NPREFIX("bes-management");
static const std::string BES_MANAGEMENT_NAMESPACE("http://schemas.ggf.org/bes/2006/08/bes-management");

static const std::string BES_ARC_NPREFIX("a-rex");
static const std::string BES_ARC_NAMESPACE("http://www.nordugrid.org/schemas/a-rex");

static const std::string BES_GLUE_NPREFIX("glue");
static const std::string BES_GLUE_NAMESPACE("http://schemas.ogf.org/glue/2008/05/spec_2.0_d41_r01");


static Arc::XMLNode BESFactoryResponse(Arc::PayloadSOAP& res,const char* opname) {
  Arc::XMLNode response = res.NewChild(BES_FACTORY_NPREFIX + ":" + opname + "Response");
  Arc::WSAHeader(res).Action(BES_FACTORY_ACTIONS_BASE_URL + opname + "Response");
  return response;
}

static Arc::XMLNode BESManagementResponse(Arc::PayloadSOAP& res,const char* opname) {
  Arc::XMLNode response = res.NewChild(BES_MANAGEMENT_NPREFIX + ":" + opname + "Response");
  Arc::WSAHeader(res).Action(BES_MANAGEMENT_ACTIONS_BASE_URL + opname + "Response");
  return response;
}

static Arc::XMLNode BESARCResponse(Arc::PayloadSOAP& res,const char* opname) {
  Arc::XMLNode response = res.NewChild(BES_ARC_NPREFIX + ":" + opname + "Response");
  return response;
}

//static Arc::LogStream logcerr(std::cerr);

static Arc::Plugin* get_service(Arc::PluginArgument* arg) {
    Arc::ServicePluginArgument* srvarg =
            arg?dynamic_cast<Arc::ServicePluginArgument*>(arg):NULL;
    if(!srvarg) return NULL;
    return new ARexService((Arc::Config*)(*srvarg));
}

class ARexConfigContext:public Arc::MessageContextElement, public ARexGMConfig {
 public:
  ARexConfigContext(const std::string& config_file,const std::string& uname,const std::string& grid_name,const std::string& service_endpoint):ARexGMConfig(config_file,uname,grid_name,service_endpoint) { };
  virtual ~ARexConfigContext(void) { };
};

static std::string GetPath(std::string url){
  std::string::size_type ds, ps;
  ds=url.find("//");
  if (ds==std::string::npos)
    ps=url.find("/");
  else
    ps=url.find("/", ds+2);
  if (ps==std::string::npos)
    return "";
  else
    return url.substr(ps);
}


Arc::MCC_Status ARexService::StopAcceptingNewActivities(ARexGMConfig& /*config*/,Arc::XMLNode /*in*/,Arc::XMLNode /*out*/) {
  return Arc::MCC_Status();
}

Arc::MCC_Status ARexService::StartAcceptingNewActivities(ARexGMConfig& /*config*/,Arc::XMLNode /*in*/,Arc::XMLNode /*out*/) {
  return Arc::MCC_Status();
}

Arc::MCC_Status ARexService::make_soap_fault(Arc::Message& outmsg) {
  Arc::PayloadSOAP* outpayload = new Arc::PayloadSOAP(ns_,true);
  Arc::SOAPFault* fault = outpayload?outpayload->Fault():NULL;
  if(fault) {
    fault->Code(Arc::SOAPFault::Sender);
    fault->Reason("Failed processing request");
  };
  outmsg.Payload(outpayload);
  return Arc::MCC_Status(Arc::STATUS_OK);
}

Arc::MCC_Status ARexService::make_fault(Arc::Message& /*outmsg*/) {
  return Arc::MCC_Status();
}

Arc::MCC_Status ARexService::make_response(Arc::Message& outmsg) {
  Arc::PayloadRaw* outpayload = new Arc::PayloadRaw();
  outmsg.Payload(outpayload);
  return Arc::MCC_Status(Arc::STATUS_OK);
}

ARexConfigContext* ARexService::get_configuration(Arc::Message& inmsg) {
  ARexConfigContext* config = NULL;
  Arc::MessageContextElement* mcontext = (*inmsg.Context())["arex.gmconfig"];
  if(mcontext) {
    try {
      config = dynamic_cast<ARexConfigContext*>(mcontext);
    } catch(std::exception& e) { };
  };
  if(config) return config;
  // TODO: do configuration detection
  // TODO: do mapping to local unix name
  std::string uname;
  uname=inmsg.Attributes()->get("SEC:LOCALID");
  if(uname.empty()) uname=uname_;
  if(uname.empty()) {
    if(getuid() == 0) {
      logger_.msg(Arc::ERROR, "Will not map to 'root' account by default");
      return NULL;
    };
    struct passwd pwbuf;
    char buf[4096];
    struct passwd* pw;
    if(getpwuid_r(getuid(),&pwbuf,buf,sizeof(buf),&pw) == 0) {
      if(pw && pw->pw_name) {
        uname = pw->pw_name;
      };
    };
  };
  if(uname.empty()) {
    logger_.msg(Arc::ERROR, "No local account name specified");
    return NULL;
  };
  logger_.msg(Arc::VERBOSE,"Using local account '%s'",uname);
  std::string grid_name = inmsg.Attributes()->get("TLS:IDENTITYDN");
  std::string endpoint = endpoint_;
  if(endpoint.empty()) {
    std::string http_endpoint = inmsg.Attributes()->get("HTTP:ENDPOINT");
    std::string tcp_endpoint = inmsg.Attributes()->get("TCP:ENDPOINT");
    bool https_proto = !grid_name.empty();
    endpoint = tcp_endpoint;
    if(https_proto) {
      endpoint="https"+endpoint;
    } else {
      endpoint="http"+endpoint;
    };
    endpoint+=GetPath(http_endpoint);
  };
  config=new ARexConfigContext(gmconfig_,uname,grid_name,endpoint);
  if(config) {
    if(*config) {
      inmsg.Context()->Add("arex.gmconfig",config);
    } else {
      delete config; config=NULL;
      logger_.msg(Arc::ERROR, "Failed to acquire grid-manager's configuration");
    };
  };
  return config;
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
  return path;
}

Arc::MCC_Status ARexService::process(Arc::Message& inmsg,Arc::Message& outmsg) {
  // Split request path into parts: service, job and file path. 
  // TODO: make it HTTP independent
  std::string endpoint;
  std::string method = inmsg.Attributes()->get("HTTP:METHOD");
  std::string id = GetPath(inmsg,endpoint);
  std::string clientid = (inmsg.Attributes()->get("TCP:REMOTEHOST"))+":"+(inmsg.Attributes()->get("TCP:REMOTEPORT"));
  if((inmsg.Attributes()->get("PLEXER:PATTERN").empty()) && id.empty()) id=endpoint;
  logger_.msg(Arc::DEBUG, "process: method: %s", method);
  logger_.msg(Arc::DEBUG, "process: endpoint: %s", endpoint);
  while(id[0] == '/') id=id.substr(1);
  std::string subpath;
  {
    std::string::size_type p = id.find('/');
    if(p != std::string::npos) {
      subpath = id.substr(p);
      id.resize(p);
      while(subpath[0] == '/') subpath=subpath.substr(1);
    };
  };
  logger_.msg(Arc::DEBUG, "process: id: %s", id);
  logger_.msg(Arc::DEBUG, "process: subpath: %s", subpath);

  // Process grid-manager configuration if not done yet
  ARexConfigContext* config = get_configuration(inmsg);
  if(!config) {
    logger_.msg(Arc::ERROR, "Can't obtain configuration");
    // Service is not operational
    return Arc::MCC_Status();
  };
  config->ClearAuths();
  config->AddAuth(inmsg.Auth());
  config->AddAuth(inmsg.AuthContext());

  // Collect any service specific Security Attributes here
  if(!ProcessSecHandlers(inmsg,"incoming")) {
    logger_.msg(Arc::ERROR, "Security Handlers processing failed");
    return Arc::MCC_Status();
  };

  // Identify which of served endpoints request is for.
  // Using simplified algorithm - POST for SOAP messages,
  // GET and PUT for data transfer
  if(method == "POST") {
    logger_.msg(Arc::DEBUG, "process: POST");
    // Both input and output are supposed to be SOAP
    // Extracting payload
    Arc::PayloadSOAP* inpayload = NULL;
    try {
      inpayload = dynamic_cast<Arc::PayloadSOAP*>(inmsg.Payload());
    } catch(std::exception& e) { };
    if(!inpayload) {
      logger_.msg(Arc::ERROR, "input is not SOAP");
      return make_soap_fault(outmsg);
    };
    // Aplying known namespaces
    inpayload->Namespaces(ns_);
    {
        std::string str;
        inpayload->GetDoc(str, true);
        logger_.msg(Arc::DEBUG, "process: request=%s",str);
    };
    // Analyzing request
    Arc::XMLNode op = inpayload->Child(0);
    if(!op) {
      logger_.msg(Arc::ERROR, "input does not define operation");
      return make_soap_fault(outmsg);
    };
    logger_.msg(Arc::DEBUG, "process: operation: %s",op.Name());
    // Check if request is for top of tree (BES factory) or particular 
    // job (listing activity)
    if(id.empty()) {
      // Factory operations
      logger_.msg(Arc::DEBUG, "process: factory endpoint");
      Arc::PayloadSOAP* outpayload = new Arc::PayloadSOAP(ns_);
      Arc::PayloadSOAP& res = *outpayload;
      // Preparing known namespaces
      outpayload->Namespaces(ns_);
      if(MatchXMLName(op,"CreateActivity")) {
        logger_.msg(Arc::DEBUG, "process: CreateActivity");
        CreateActivity(*config,op,BESFactoryResponse(res,"CreateActivity"),clientid);
      } else if(MatchXMLName(op,"GetActivityStatuses")) {
        GetActivityStatuses(*config,op,BESFactoryResponse(res,"GetActivityStatuses"));
      } else if(MatchXMLName(op,"TerminateActivities")) {
        TerminateActivities(*config,op,BESFactoryResponse(res,"TerminateActivities"));
      } else if(MatchXMLName(op,"GetActivityDocuments")) {
        GetActivityDocuments(*config,op,BESFactoryResponse(res,"GetActivityDocuments"));
      } else if(MatchXMLName(op,"GetFactoryAttributesDocument")) {
        GetFactoryAttributesDocument(*config,op,BESFactoryResponse(res,"GetFactoryAttributesDocument"));
      } else if(MatchXMLName(op,"StopAcceptingNewActivities")) {
        StopAcceptingNewActivities(*config,op,BESManagementResponse(res,"StopAcceptingNewActivities"));
      } else if(MatchXMLName(op,"StartAcceptingNewActivities")) {
        StartAcceptingNewActivities(*config,op,BESManagementResponse(res,"StartAcceptingNewActivities"));
      } else if(MatchXMLName(op,"ChangeActivityStatus")) {
        ChangeActivityStatus(*config,op,BESARCResponse(res,"ChangeActivityStatus"));
      } else if(MatchXMLName(op,"CacheCheck")) {
        CacheCheck(*config,*inpayload,*outpayload);
      } else if(MatchXMLName(op,"DelegateCredentialsInit")) {
        if(!delegations_.DelegateCredentialsInit(*inpayload,*outpayload)) {
          delete outpayload;
          return make_soap_fault(outmsg);
        };
      } else if(MatchXMLNamespace(op,"http://docs.oasis-open.org/wsrf/rp-2")) {
        // TODO: do not copy out_ to outpayload.
        Arc::SOAPEnvelope* out_ = infodoc_.Process(*inpayload);
        if(out_) {
          *outpayload=*out_;
          delete out_;
        } else {
          delete outpayload;
          return make_soap_fault(outmsg);
        };
      } else {
        logger_.msg(Arc::ERROR, "SOAP operation is not supported: %s", op.Name());
        delete outpayload;
        return make_soap_fault(outmsg);
      };
      {
        std::string str;
        outpayload->GetDoc(str, true);
        logger_.msg(Arc::DEBUG, "process: response=%s",str);
      };
      outmsg.Payload(outpayload);
    } else {
      // Listing operations for session directories
      // TODO: proper failure like interface is not supported
    };
    if(!ProcessSecHandlers(outmsg,"outgoing")) {
      logger_.msg(Arc::ERROR, "Security Handlers processing failed");
      delete outmsg.Payload(NULL);
      return Arc::MCC_Status();
    };
    return Arc::MCC_Status(Arc::STATUS_OK);
  } else if(method == "GET") {
    // HTTP plugin either provides buffer or stream
    logger_.msg(Arc::DEBUG, "process: GET");
    // TODO: in case of error generate some content
    Arc::MCC_Status ret = Get(inmsg,outmsg,*config,id,subpath);
    if(ret) {
      if(!ProcessSecHandlers(outmsg,"outgoing")) {
        logger_.msg(Arc::ERROR, "Security Handlers processing failed");
        delete outmsg.Payload(NULL);
        return Arc::MCC_Status();
      };
    };
    return ret;
  } else if(method == "PUT") {
    logger_.msg(Arc::DEBUG, "process: PUT");
    Arc::MCC_Status ret = Put(inmsg,outmsg,*config,id,subpath);
    if(!ret) return make_fault(outmsg);
    // Put() does not generate response yet
    ret=make_response(outmsg);
    if(ret) {
      if(!ProcessSecHandlers(outmsg,"outgoing")) {
        logger_.msg(Arc::ERROR, "Security Handlers processing failed");
        delete outmsg.Payload(NULL);
        return Arc::MCC_Status();
      };
    };
    return ret;
  } else {
    logger_.msg(Arc::DEBUG, "process: method %s is not supported",method);
    // TODO: make useful response
    return Arc::MCC_Status();
  };
  return Arc::MCC_Status();
}

static void thread_starter(void* arg) {
  if(!arg) return;
  ((ARexService*)arg)->InformationCollector();
}
 
ARexService::ARexService(Arc::Config *cfg):Service(cfg),logger_(Arc::Logger::rootLogger, "A-REX"),inforeg_(*cfg,this),gm_(NULL) {
  // logger_.addDestination(logcerr);
  // Define supported namespaces
  ns_[BES_ARC_NPREFIX]=BES_ARC_NAMESPACE;
  ns_[BES_GLUE_NPREFIX]=BES_GLUE_NAMESPACE;
  ns_[BES_FACTORY_NPREFIX]=BES_FACTORY_NAMESPACE;
  ns_[BES_MANAGEMENT_NPREFIX]=BES_MANAGEMENT_NAMESPACE;
  ns_["deleg"]="http://www.nordugrid.org/schemas/delegation";
  ns_["wsa"]="http://www.w3.org/2005/08/addressing";
  ns_["jsdl"]="http://schemas.ggf.org/jsdl/2005/11/jsdl";
  ns_["wsrf-bf"]="http://docs.oasis-open.org/wsrf/bf-2";
  ns_["wsrf-r"]="http://docs.oasis-open.org/wsrf/r-2";
  ns_["wsrf-rw"]="http://docs.oasis-open.org/wsrf/rw-2";
  // Obtain information from configuration
  endpoint_=(std::string)((*cfg)["endpoint"]);
  uname_=(std::string)((*cfg)["usermap"]["defaultLocalName"]);
  gmconfig_=(std::string)((*cfg)["gmconfig"]);
  std::string gmrun_ = (std::string)((*cfg)["gmrun"]);
  common_name_ = (std::string)((*cfg)["commonName"]);
  long_description_ = (std::string)((*cfg)["longDescription"]);
  lrms_name_ = (std::string)((*cfg)["LRMSName"]);
  os_name_ = (std::string)((*cfg)["OperatingSystem"]);
  CreateThreadFunction(&thread_starter,this);
  // Run grid-manager in thread
  /*
  Arc::NS ns;
  Arc::XMLNode gmargv(ns,"argv");
  for(Arc::XMLNode n = (*cfg)["gmarg"];(bool)n;n=n[1]) {
    gmargv.NewChild("arg")=(std::string)n;
  };
  if(!gmconfig_.empty()) {
    gmargv.NewChild("arg")="-c";
    gmargv.NewChild("arg")=gmconfig_;
  };
  */
  if((gmrun_.empty()) || (gmrun_ == "internal")) {
    //gm_=new GridManager(gmargv);
    gm_=new GridManager(gmconfig_.empty()?NULL:gmconfig_.c_str());
    if(!gm_) return;
    if(!(*gm_)) { delete gm_; gm_=NULL; return; };
  };
}

ARexService::~ARexService(void) {
  if(gm_) delete gm_;
}

} // namespace ARex

Arc::PluginDescriptor PLUGINS_TABLE_NAME[] = {
    { "a-rex", "HED:SERVICE", 0, &ARex::get_service },
    { NULL, NULL, 0, NULL }
};

