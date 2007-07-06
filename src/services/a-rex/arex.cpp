#include <iostream>

#include <sys/types.h>
#include <pwd.h>

#include "../../hed/libs/loader/Loader.h"
#include "../../hed/libs/loader/ServiceLoader.h"
#include "../../hed/libs/message/PayloadSOAP.h"
#include "job.h"

#include "arex.h"


static Arc::Service* get_service(Arc::Config *cfg,Arc::ChainContext *ctx) {
    return new ARex::ARexService(cfg);
}

service_descriptor __arc_service_modules__[] = {
    { "a-rex", 0, &get_service },
    { NULL, 0, NULL }
};

using namespace ARex;
 

class ARexConfigContext:public Arc::MessageContextElement, public ARexGMConfig {
 public:
  ARexConfigContext(const std::string& config_file,const std::string& uname,const std::string& grid_name):ARexGMConfig(config_file,uname,grid_name) { };
  virtual ~ARexConfigContext(void) { };
};


Arc::MCC_Status ARexService::CreateActivity(ARexGMConfig& config,Arc::XMLNode& in,Arc::XMLNode& out) {
  /*
  CreateActivity
    ActivityDocument
      jsdl:JobDefinition

  CreateActivityResponse
    ActivityIdentifier (wsa:EndpointReferenceType)
    ActivityDocument
      jsdl:JobDefinition
  */
  Arc::XMLNode jsdl = in["ActivityDocument"]["JobDefinition"];
  if(!jsdl) {
    // Wrongly formated request
    return Arc::MCC_Status();
  };
  ARexJob* job = new ARexJob(jsdl,config);
  if((!job) || (!(*job))) {
    // Failed to create new job
    if(job) delete job;
  };
  // Make job's ID


  return Arc::MCC_Status();
}

Arc::MCC_Status ARexService::GetActivityStatuses(ARexGMConfig& config,Arc::XMLNode& in,Arc::XMLNode& out) {
  return Arc::MCC_Status();
}

Arc::MCC_Status ARexService::TerminateActivities(ARexGMConfig& config,Arc::XMLNode& in,Arc::XMLNode& out) {
  return Arc::MCC_Status();
}

Arc::MCC_Status ARexService::GetActivityDocuments(ARexGMConfig& config,Arc::XMLNode& in,Arc::XMLNode& out) {
  return Arc::MCC_Status();
}

Arc::MCC_Status ARexService::GetFactoryAttributesDocument(ARexGMConfig& config,Arc::XMLNode& in,Arc::XMLNode& out) {
  return Arc::MCC_Status();
}

Arc::MCC_Status ARexService::StopAcceptingNewActivities(ARexGMConfig& config,Arc::XMLNode& in,Arc::XMLNode& out) {
  return Arc::MCC_Status();
}

Arc::MCC_Status ARexService::StartAcceptingNewActivities(ARexGMConfig& config,Arc::XMLNode& in,Arc::XMLNode& out) {
  return Arc::MCC_Status();
}

Arc::MCC_Status ARexService::ChangeActivityStatus(ARexGMConfig& config,Arc::XMLNode& in,Arc::XMLNode& out) {
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


Arc::MCC_Status ARexService::process(Arc::Message& inmsg,Arc::Message& outmsg) {

  // Process grid-manager configuration if not done yet
  ARexConfigContext* config = NULL;
  {
    Arc::MessageContextElement* mcontext = (*inmsg.Context())["arex.gmconfig"];
    if(mcontext) {
      try {
        config = dynamic_cast<ARexConfigContext*>(mcontext);
      } catch(std::exception& e) { };
    };
  };
  if(!config) {
    // TODO: do configuration detection
    // TODO: do mapping to local unix name
    struct passwd pwbuf;
    char buf[4096];
    struct passwd* pw;
    if(getpwuid_r(getuid(),&pwbuf,buf,sizeof(buf),&pw) == 0) {
      if(pw && pw->pw_name) {
        std::string uname = pw->pw_name;
        std::string grid_name = inmsg.Attributes()->get("TLS:PEERDN");
        config=new ARexConfigContext("",uname,grid_name);
        inmsg.Context()->Add("arex.gmconfig",config);
      };
    };
  };
  if(!config) {
    // Service is not operational
    return Arc::MCC_Status();
  };

  // Identify which of served endpoints request is for
  // Using simplified algorithm - POST for SOAP messages,
  // GET and PUT for data transfer
  std::string method = inmsg.Attributes()->get("HTTP:METHOD");
  std::string id = inmsg.Attributes()->get("PLEXER:EXTENSION");
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
  if(method == "POST") {
    // Both input and output are supposed to be SOAP
    // Extracting payload
    Arc::PayloadSOAP* inpayload = NULL;
    try {
      inpayload = dynamic_cast<Arc::PayloadSOAP*>(inmsg.Payload());
    } catch(std::exception& e) { };
    if(!inpayload) {
      std::cerr << "A-Rex: input is not SOAP" << std::endl;
      return make_fault(outmsg);
    };
    // Analyzing request
    Arc::XMLNode op = inpayload->Child(0);
    if(!op) {
      std::cerr << "A-Rex: input does not define operation" << std::endl;
      return make_fault(outmsg);
    };
    // Check if request is for top of tree (BES factory) or particular 
    // job (listing activity)
    if(id.empty()) {
      // Factory operations
      Arc::XMLNode res;
      if(MatchXMLName(op,"CreateActivity")) {
        CreateActivity(*config,op,res);
      } else if(MatchXMLName(op,"GetActivityStatuses")) {
        GetActivityStatuses(*config,op,res);
      } else if(MatchXMLName(op,"TerminateActivities")) {
        TerminateActivities(*config,op,res);
      } else if(MatchXMLName(op,"GetActivityDocuments")) {
        GetActivityDocuments(*config,op,res);
      } else if(MatchXMLName(op,"GetFactoryAttributesDocument")) {
        GetFactoryAttributesDocument(*config,op,res);
      } else if(MatchXMLName(op,"StopAcceptingNewActivities")) {
        StopAcceptingNewActivities(*config,op,res);
      } else if(MatchXMLName(op,"StartAcceptingNewActivities")) {
        StartAcceptingNewActivities(*config,op,res);
      } else if(MatchXMLName(op,"ChangeActivityStatus")) {
        ChangeActivityStatus(*config,op,res);
      } else {
        std::cerr << "A-Rex: request is not supported - " << op.Name() << std::endl;
        return make_fault(outmsg);
      };
      Arc::PayloadSOAP* outpayload = new Arc::PayloadSOAP(ns_);
      outpayload->NewChild(res);
      outmsg.Payload(outpayload);
    } else {
      // Listing operations for session directories
    };
  } else if(method == "GET") {
  } else if(method == "PUT") {
  } else if(method == "HEAD") {
  };
  return Arc::MCC_Status();
}
 
ARexService::ARexService(Arc::Config *cfg):Service(cfg) {
    ns_["a-rex"]="http://www.nordugrid.org/schemas/a-rex";
    ns_["bes-factory"]="http://schemas.ggf.org/bes/2006/08/bes-factory";
}

ARexService::~ARexService(void) {
}

