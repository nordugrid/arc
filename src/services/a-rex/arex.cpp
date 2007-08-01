#include <iostream>

#include <sys/types.h>
#include <pwd.h>

#include "loader/Loader.h"
#include "loader/ServiceLoader.h"
#include "message/PayloadSOAP.h"
#include "../../hed/libs/ws-addressing/WSA.h"
#include "job.h"

#include "arex.h"


static Arc::Service* get_service(Arc::Config *cfg,Arc::ChainContext *ctx) {
    return new ARex::ARexService(cfg);
}

service_descriptors ARC_SERVICE_LOADER = {
    { "a-rex", 0, &get_service },
    { NULL, 0, NULL }
};

using namespace ARex;
 

class ARexConfigContext:public Arc::MessageContextElement, public ARexGMConfig {
 public:
  ARexConfigContext(const std::string& config_file,const std::string& uname,const std::string& grid_name,const std::string& service_endpoint):ARexGMConfig(config_file,uname,grid_name,service_endpoint) { };
  virtual ~ARexConfigContext(void) { };
};


Arc::MCC_Status ARexService::CreateActivity(ARexGMConfig& config,Arc::XMLNode in,Arc::XMLNode out) {
  /*
  CreateActivity
    ActivityDocument
      jsdl:JobDefinition

  CreateActivityResponse
    ActivityIdentifier (wsa:EndpointReferenceType)
    ActivityDocument
      jsdl:JobDefinition

  InvalidRequestMessageFault
    InvalidElement
    Message
  */
  Arc::XMLNode jsdl = in["ActivityDocument"]["JobDefinition"];
  if(!jsdl) {
    // Wrongly formated request
    Arc::SOAPEnvelope fault(ns_,true);
    if(fault) {
      fault.Fault()->Code(Arc::SOAPFault::Sender);
      fault.Fault()->Reason("Can't find JobDefinition element in request");
      Arc::XMLNode f = fault.Fault()->Detail(true).NewChild("bes-factory:InvalidRequestMessageFault");
      f.NewChild("bes-factory:InvalidElement")="jsdl:JobDefinition";
      f.NewChild("bes-factory:Message")="Element is missing";
      out.Replace(fault.Child());
    };
    return Arc::MCC_Status();
  };
  ARexJob job(jsdl,config);
  if(!job) {
    // Failed to create new job (generic SOAP error)
    Arc::SOAPEnvelope fault(ns_,true);
    if(fault) {
      fault.Fault()->Code(Arc::SOAPFault::Receiver);
      fault.Fault()->Reason("Can't creat new activity");
      out.Replace(fault.Child());
    };
    return Arc::MCC_Status();
  };
  // Make SOAP response
  Arc::WSAEndpointReference identifier(out.NewChild("bes-factory:ActivityIdentifier"));
  // Make job's ID
  identifier.Address(config.Endpoint()); // address of service
  identifier.ReferenceParameters().NewChild("a-rex:JobID")=job.ID();
  identifier.ReferenceParameters().NewChild("a-rex:JobSessionDir")=config.Endpoint()+"/"+job.ID();
  out.NewChild(in["ActivityDocument"]);
  return Arc::MCC_Status(Arc::STATUS_OK);
}

Arc::MCC_Status ARexService::GetActivityStatuses(ARexGMConfig& config,Arc::XMLNode in,Arc::XMLNode out) {
  return Arc::MCC_Status();
}

Arc::MCC_Status ARexService::TerminateActivities(ARexGMConfig& config,Arc::XMLNode in,Arc::XMLNode out) {
  return Arc::MCC_Status();
}

Arc::MCC_Status ARexService::GetActivityDocuments(ARexGMConfig& config,Arc::XMLNode in,Arc::XMLNode out) {
  return Arc::MCC_Status();
}

Arc::MCC_Status ARexService::GetFactoryAttributesDocument(ARexGMConfig& config,Arc::XMLNode in,Arc::XMLNode out) {
  return Arc::MCC_Status();
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



ARexConfigContext* ARexService::get_configuration(void) {
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
    struct passwd pwbuf;
    char buf[4096];
    struct passwd* pw;
    if(getpwuid_r(getuid(),&pwbuf,buf,sizeof(buf),&pw) == 0) {
      if(pw && pw->pw_name) {
        std::string uname = pw->pw_name;
        std::string grid_name = inmsg.Attributes()->get("TLS:PEERDN");
        config=new ARexConfigContext("",uname,grid_name,endpoint);
        inmsg.Context()->Add("arex.gmconfig",config);
      };
    };
  };
  return config;
}


Arc::MCC_Status ARexService::process(Arc::Message& inmsg,Arc::Message& outmsg) {
  // Split request path into parts: service, job and file path. 
  std::string method = inmsg.Attributes()->get("HTTP:METHOD");
  std::string id = inmsg.Attributes()->get("PLEXER:EXTENSION");
  std::string endpoint = inmsg.Attributes()->get("HTTP:ENDPOINT");
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

  // Process grid-manager configuration if not done yet
  ARexConfigContext* config = get_configuration();
  if(!config) {
    // Service is not operational
    return Arc::MCC_Status();
  };

  // Identify which of served endpoints request is for.
  // Using simplified algorithm - POST for SOAP messages,
  // GET and PUT for data transfer
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
      Arc::PayloadSOAP* outpayload = new Arc::PayloadSOAP(ns_);
      Arc::PayloadSOAP& res = *outpayload;
      if(MatchXMLName(op,"CreateActivity")) {
        CreateActivity(*config,op,res.NewChild("bes-factory:CreateActivityResponse"));
      } else if(MatchXMLName(op,"GetActivityStatuses")) {
        GetActivityStatuses(*config,op,res.NewChild("bes-factory:GetActivityStatuses"));
      } else if(MatchXMLName(op,"TerminateActivities")) {
        TerminateActivities(*config,op,res.NewChild("bes-factory:TerminateActivities"));
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
      outmsg.Payload(outpayload);
    } else {
      // Listing operations for session directories
    };
  } else if(method == "GET") {
    
  } else if(method == "PUT") {
    if(id.empty() || subpath.empty()) {
      std::cerr << "A-Rex: input contains no proper path to file" << std::endl;
      return make_fault(outmsg);
    };
    Arc::PayloadRawInterface* inbufpayload = NULL;
    Arc::PayloadStreamInterface* instreampayload = NULL;
    try {
      inbufpayload = dynamic_cast<Arc::PayloadRawInterface*>(inmsg.Payload());
    } catch(std::exception& e) { };
    if(!inbufpayload) try {
      instreampayload = dynamic_cast<Arc::PayloadStreamInterface*>(inmsg.Payload());
    } catch(std::exception& e) { };
    if((!inbufpayload) && (!instreampayload)) {
      std::cerr << "A-Rex: input is neither stream nor buffer" << std::endl;
      return make_fault(outmsg);
    };
    if(inbufpayload) {






  } else if(method == "HEAD") {
  

  };
  return Arc::MCC_Status();
}
 
ARexService::ARexService(Arc::Config *cfg):Service(cfg) {
    ns_["a-rex"]="http://www.nordugrid.org/schemas/a-rex";
    ns_["bes-factory"]="http://schemas.ggf.org/bes/2006/08/bes-factory";
    ns_["wsa"]="http://www.w3.org/2005/08/addressing";
}

ARexService::~ARexService(void) {
}

