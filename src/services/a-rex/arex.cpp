#include <iostream>

#include <sys/types.h>
#include <pwd.h>

#include "loader/Loader.h"
#include "loader/ServiceLoader.h"
#include "loader/Plexer.h"
#include "message/PayloadSOAP.h"
#include "message/PayloadRaw.h"
#include "message/PayloadStream.h"
#include "ws-addressing/WSA.h"
#include "job.h"

#include "arex.h"

namespace ARex {


static Arc::LogStream logcerr(std::cerr);

static Arc::Service* get_service(Arc::Config *cfg,Arc::ChainContext *ctx) {
    return new ARexService(cfg);
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


Arc::MCC_Status ARexService::StopAcceptingNewActivities(ARexGMConfig& config,Arc::XMLNode in,Arc::XMLNode out) {
  return Arc::MCC_Status();
}

Arc::MCC_Status ARexService::StartAcceptingNewActivities(ARexGMConfig& config,Arc::XMLNode in,Arc::XMLNode out) {
  return Arc::MCC_Status();
}

Arc::MCC_Status ARexService::ChangeActivityStatus(ARexGMConfig& config,Arc::XMLNode in,Arc::XMLNode out) {
  return Arc::MCC_Status();
}

Arc::MCC_Status ARexService::make_fault(Arc::Message& outmsg) {
  Arc::PayloadSOAP* outpayload = new Arc::PayloadSOAP(ns_,true);
  Arc::SOAPFault* fault = outpayload->Fault();
  if(fault) {
    fault->Code(Arc::SOAPFault::Sender);
    fault->Reason("Failed processing request");
  };
  outmsg.Payload(outpayload);
  return Arc::MCC_Status();
}



ARexConfigContext* ARexService::get_configuration(Arc::Message& inmsg) {
  ARexConfigContext* config = NULL;
  Arc::MessageContextElement* mcontext = (*inmsg.Context())["arex.gmconfig"];
  if(mcontext) {
    try {
      config = dynamic_cast<ARexConfigContext*>(mcontext);
    } catch(std::exception& e) { };
  };
  if(!config) {
    // TODO: do configuration detection
    // TODO: do mapping to local unix name
    std::string uname = uname_;
    if(uname.empty()) {
      struct passwd pwbuf;
      char buf[4096];
      struct passwd* pw;
      if(getpwuid_r(getuid(),&pwbuf,buf,sizeof(buf),&pw) == 0) {
        if(pw && pw->pw_name) {
          uname = pw->pw_name;
        };
      };
    };
    if(!uname.empty()) {
      std::string grid_name = inmsg.Attributes()->get("TLS:PEERDN");
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
        };
      };
    };
  };
  return config;
}


Arc::MCC_Status ARexService::process(Arc::Message& inmsg,Arc::Message& outmsg) {
  // Split request path into parts: service, job and file path. 
  // TODO: make it HTTP independent
  std::string method = inmsg.Attributes()->get("HTTP:METHOD");
  std::string id = inmsg.Attributes()->get("PLEXER:EXTENSION");
  std::string endpoint = inmsg.Attributes()->get("HTTP:ENDPOINT");
  if((inmsg.Attributes()->get("PLEXER:PATTERN").empty()) && id.empty()) id=endpoint;
  logger.msg(Arc::DEBUG, "process: method: %s", method.c_str());
  logger.msg(Arc::DEBUG, "process: endpoint: %s", endpoint.c_str());
  if(id.length() < endpoint.length()) {
    if(endpoint.substr(endpoint.length()-id.length()) == id) {
      endpoint.resize(endpoint.length()-id.length());
    };
  };
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
  logger.msg(Arc::DEBUG, "process: id: %s", id.c_str());
  logger.msg(Arc::DEBUG, "process: subpath: %s", subpath.c_str());

  // Process grid-manager configuration if not done yet
  ARexConfigContext* config = get_configuration(inmsg);
  if(!config) {
    logger.msg(Arc::ERROR, "Can't obtain configuration");
    // Service is not operational
    return Arc::MCC_Status();
  };

  // Identify which of served endpoints request is for.
  // Using simplified algorithm - POST for SOAP messages,
  // GET and PUT for data transfer
  if(method == "POST") {
    logger.msg(Arc::DEBUG, "process: POST");
    // Both input and output are supposed to be SOAP
    // Extracting payload
    Arc::PayloadSOAP* inpayload = NULL;
    try {
      inpayload = dynamic_cast<Arc::PayloadSOAP*>(inmsg.Payload());
    } catch(std::exception& e) { };
    if(!inpayload) {
      logger.msg(Arc::ERROR, "input is not SOAP");
      return make_fault(outmsg);
    };
    // Analyzing request
    Arc::XMLNode op = inpayload->Child(0);
    if(!op) {
      logger.msg(Arc::ERROR, "input does not define operation");
      return make_fault(outmsg);
    };
    logger.msg(Arc::DEBUG, "process: operation: %s",op.Name().c_str());
    // Check if request is for top of tree (BES factory) or particular 
    // job (listing activity)
    if(id.empty()) {
      // Factory operations
      logger.msg(Arc::DEBUG, "process: factory endpoint");
      Arc::PayloadSOAP* outpayload = new Arc::PayloadSOAP(ns_);
      Arc::PayloadSOAP& res = *outpayload;
      if(MatchXMLName(op,"CreateActivity")) {
        logger.msg(Arc::DEBUG, "process: CreateActivity");
        CreateActivity(*config,op,res.NewChild("bes-factory:CreateActivityResponse"));
      } else if(MatchXMLName(op,"GetActivityStatuses")) {
        GetActivityStatuses(*config,op,res.NewChild("bes-factory:GetActivityStatusesResponse"));
      } else if(MatchXMLName(op,"TerminateActivities")) {
        TerminateActivities(*config,op,res.NewChild("bes-factory:TerminateActivitiesResponse"));
      } else if(MatchXMLName(op,"GetActivityDocuments")) {
        GetActivityDocuments(*config,op,res.NewChild("bes-factory:GetActivityDocumentsResponse"));
      } else if(MatchXMLName(op,"GetFactoryAttributesDocument")) {
        GetFactoryAttributesDocument(*config,op,res.NewChild("bes-factory:GetFactoryAttributesDocumentResponse"));
      } else if(MatchXMLName(op,"StopAcceptingNewActivities")) {
        StopAcceptingNewActivities(*config,op,res.NewChild("bes-factory:StopAcceptingNewActivitiesResponse"));
      } else if(MatchXMLName(op,"StartAcceptingNewActivities")) {
        StartAcceptingNewActivities(*config,op,res.NewChild("bes-factory:StartAcceptingNewActivitiesResponse"));
      } else if(MatchXMLName(op,"ChangeActivityStatus")) {
        ChangeActivityStatus(*config,op,res.NewChild("bes-factory:ChangeActivityStatusResponse"));
      } else {
        std::cerr << "A-Rex: request is not supported - " << op.Name() << std::endl;
        return make_fault(outmsg);
      };
      {
        std::string str;
        outpayload->GetXML(str);
        logger.msg(Arc::DEBUG, "process: response=%s",str.c_str());
      };
      outmsg.Payload(outpayload);
    } else {
      // Listing operations for session directories
    };
  } else {
//    if(id.empty() || subpath.empty()) {
//std::cerr << "A-Rex: input contains no proper path to file" << std::endl;
//      return make_fault(outmsg);
//    };
    // HTTP plugin either provides buffer or stream
    Arc::PayloadRawInterface* inbufpayload = NULL;
    Arc::PayloadStreamInterface* instreampayload = NULL;
    try {
      inbufpayload = dynamic_cast<Arc::PayloadRawInterface*>(inmsg.Payload());
    } catch(std::exception& e) { };
    if(!inbufpayload) try {
      instreampayload = dynamic_cast<Arc::PayloadStreamInterface*>(inmsg.Payload());
    } catch(std::exception& e) { };
    if(method == "GET") {
      Arc::PayloadRaw* buf = new Arc::PayloadRaw;
      logger.msg(Arc::DEBUG, "process: GET");
      Arc::MCC_Status ret = Get(*config,id,subpath,*buf);
      if(!ret) { delete buf; return make_fault(outmsg); };
      outmsg.Payload(buf);
    } else if(method == "PUT") {
      logger.msg(Arc::DEBUG, "process: PUT");
      if(inbufpayload) {

      } else if(instreampayload) {
      } else {
        // Method PUT requres input
        std::cerr << "A-Rex: input is neither stream nor buffer" << std::endl;
        return make_fault(outmsg);
      };
    } else {
      logger.msg(Arc::DEBUG, "process: $s: not supported",method.c_str());
      return Arc::MCC_Status();
    };
  };
  return Arc::MCC_Status(Arc::STATUS_OK);
}
 
ARexService::ARexService(Arc::Config *cfg):Service(cfg),logger_(Arc::Logger::rootLogger, "A-REX") {
  logger_.addDestination(logcerr);
  ns_["a-rex"]="http://www.nordugrid.org/schemas/a-rex";
  ns_["bes-factory"]="http://schemas.ggf.org/bes/2006/08/bes-factory";
  ns_["wsa"]="http://www.w3.org/2005/08/addressing";
  ns_["jsdl"]="http://schemas.ggf.org/jsdl/2005/11/jsdl";
  endpoint_=(std::string)((*cfg)["endpoint"]);
  uname_=(std::string)((*cfg)["username"]);
  gmconfig_=(std::string)((*cfg)["gmconfig"]);
}

ARexService::~ARexService(void) {
}

}; // namespace ARex

service_descriptors ARC_SERVICE_LOADER = {
    { "a-rex", 0, &ARex::get_service },
    { NULL, 0, NULL }
};

