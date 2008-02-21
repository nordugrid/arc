// arex_client.cpp

#include "arex_client.h"

namespace Arc {

  AREXClientError::AREXClientError(const std::string& what) :
    std::runtime_error(what)
  {
  }

  Arc::Logger AREXClient::logger(Arc::Logger::rootLogger, "A-REX-Client");

  AREXClient::AREXClient(std::string configFile) throw(AREXClientError)
    :client_loader(NULL),client(NULL),client_entry(NULL)
  {
    logger.msg(Arc::INFO, "Creating an A-REX client.");

    if (configFile=="" && getenv("ARC_AREX_CONFIG"))
      configFile = getenv("ARC_AREX_CONFIG");
    if (configFile=="")
      configFile = "./arex_client.xml";

    client_config = new Arc::Config(configFile.c_str());
    if(!*client_config) {
      logger.msg(Arc::ERROR, "Failed to load client configuration.");
      throw AREXClientError("Failed to load client configuration.");
    }

    client_loader = new Arc::Loader(client_config);
    logger.msg(Arc::INFO, "Client side MCCs are loaded.");
    client_entry = (*client_loader)["soap"];
    if(!client_entry) {
      logger.msg(Arc::ERROR, "Client chain does not have entry point.");
      throw AREXClientError("Client chain does not have entry point.");
    }

    arex_ns["a-rex"]="http://www.nordugrid.org/schemas/a-rex";
    arex_ns["bes-factory"]="http://schemas.ggf.org/bes/2006/08/bes-factory";
    arex_ns["wsa"]="http://www.w3.org/2005/08/addressing";
    arex_ns["jsdl"]="http://schemas.ggf.org/jsdl/2005/11/jsdl";    
  }
  
  AREXClient::AREXClient(const Arc::URL& url,
                         const Arc::MCCConfig& cfg) throw(AREXClientError)
    :client_loader(NULL),client(NULL),client_entry(NULL) {

    logger.msg(Arc::INFO, "Creating an A-REX client.");
    bool tls;
    if(url.Protocol() == "http") { tls=false; }
    client = new Arc::ClientSOAP(cfg,url.Host(),url.Port(),tls,url.Path());
  }
  
  AREXClient::~AREXClient()
  {
    delete client_loader;
    delete client_config;
  }
  
  std::string AREXClient::submit(std::istream& jsdl_file)
    throw(AREXClientError)
  {
    std::string jobid, faultstring;
    logger.msg(Arc::INFO, "Creating and sending request.");

    // Create job request
    /*
      bes-factory:CreateActivity
        bes-factory:ActivityDocument
          jsdl:JobDefinition
    */
    Arc::PayloadSOAP req(arex_ns);
    Arc::XMLNode act_doc =
      req.NewChild("bes-factory:CreateActivity").
      NewChild("bes-factory:ActivityDocument");
    std::string jsdl_str; 
    std::getline<char>(jsdl_file,jsdl_str,0);
    act_doc.NewChild(Arc::XMLNode(jsdl_str));
    req.GetXML(jsdl_str);
    Arc::PayloadSOAP* resp = NULL;

    // Send job request
    if(client) {
      Arc::MCC_Status status = client->process(
         "http://schemas.ggf.org/bes/2006/08/bes-factory/BESFactoryPortType/CreateActivity",
         &req,&resp);
      if(!status) {
        logger.msg(Arc::ERROR, "Submission request failed.");
        throw AREXClientError("Submission request failed.");
      }
      if(resp == NULL) {
        logger.msg(Arc::ERROR,"There was no SOAP response.");
        throw AREXClientError("There was no SOAP response.");
      };
    } else if (client_entry) {
      Arc::Message reqmsg;
      Arc::Message repmsg;
      Arc::MessageAttributes attributes_req;
      attributes_req.set("SOAP:ACTION","http://schemas.ggf.org/bes/2006/08/bes-factory/BESFactoryPortType/CreateActivity");
      Arc::MessageAttributes attributes_rep;
      Arc::MessageContext context;
      reqmsg.Payload(&req);
      reqmsg.Attributes(&attributes_req);
      reqmsg.Context(&context);
      repmsg.Attributes(&attributes_rep);
      repmsg.Context(&context);
      Arc::MCC_Status status = client_entry->process(reqmsg,repmsg);
      if(!status) {
        logger.msg(Arc::ERROR, "Submission request failed.");
        throw AREXClientError("Submission request failed.");
      }
      logger.msg(Arc::INFO, "Submission request succeed.");
      if(repmsg.Payload() == NULL) {
        logger.msg(Arc::ERROR, "There were no response to a submission request.");
        throw AREXClientError("There were no response to the submission request.");
      }
      try {
        resp = dynamic_cast<Arc::PayloadSOAP*>(repmsg.Payload());
      } catch(std::exception&) { };
      if(resp == NULL) {
        logger.msg(Arc::ERROR,"A response to a submission request was not a SOAP message.");
        delete repmsg.Payload();
        throw AREXClientError("The response to the submission request was not a SOAP message.");
      };
    } else {
      throw AREXClientError("There is no connection chain configured.");
    };
    Arc::XMLNode id, fs;
    (*resp)["CreateActivityResponse"]["ActivityIdentifier"].New(id);
    (*resp)["Fault"]["faultstring"].New(fs);
    id.GetDoc(jobid);
    faultstring=(std::string)fs;
    delete resp;
    if (faultstring=="")
      return jobid;
    else
      throw AREXClientError(faultstring);
  }
  
  std::string AREXClient::stat(const std::string& jobid)
    throw(AREXClientError)
  {
    std::string state, substate, faultstring;
    logger.msg(Arc::INFO, "Creating and sending a status request.");
    
    Arc::PayloadSOAP req(arex_ns);
    Arc::XMLNode jobref =
      req.NewChild("bes-factory:GetActivityStatuses").
      NewChild(Arc::XMLNode(jobid));
    
    // Send status request
    Arc::PayloadSOAP* resp = NULL;

    if(client) {
      Arc::MCC_Status status = client->process(
          "http://schemas.ggf.org/bes/2006/08/bes-factory/BESFactoryPortType/GetActivityStatuses",
          &req,&resp);
      if(resp == NULL) {
        logger.msg(Arc::ERROR,"There was no SOAP response.");
        throw AREXClientError("There was no SOAP response.");
      }
    } else if(client_entry) {
      Arc::Message reqmsg;
      Arc::Message repmsg;
      Arc::MessageAttributes attributes_req;
      attributes_req.set("SOAP:ACTION","http://schemas.ggf.org/bes/2006/08/bes-factory/BESFactoryPortType/GetActivityStatuses");
      Arc::MessageAttributes attributes_rep;
      Arc::MessageContext context;
      reqmsg.Payload(&req);
      reqmsg.Attributes(&attributes_req);
      reqmsg.Context(&context);
      repmsg.Attributes(&attributes_rep);
      repmsg.Context(&context);
      Arc::MCC_Status status = client_entry->process(reqmsg,repmsg);
      if(!status) {
        logger.msg(Arc::ERROR, "A status request failed.");
        throw AREXClientError("The status request failed.");
      }
      logger.msg(Arc::INFO, "A status request succeed.");
      if(repmsg.Payload() == NULL) {
        logger.msg(Arc::ERROR, "There were no response to a status request.");
        throw AREXClientError("There were no response.");
      }
      try {
        resp = dynamic_cast<Arc::PayloadSOAP*>(repmsg.Payload());
      } catch(std::exception&) { };
      if(resp == NULL) {
        logger.msg(Arc::ERROR,
		 "The response of a status request was not a SOAP message.");
        delete repmsg.Payload();
        throw AREXClientError("The response is not a SOAP message.");
      }
    } else {
      throw AREXClientError("There is no connection chain configured.");
    };
    Arc::XMLNode st, fs;
    (*resp)["GetActivityStatusesResponse"]["Response"]
           ["ActivityStatus"].New(st);
    state = (std::string)st.Attribute("state");
    Arc::XMLNode sst;
    (*resp)["GetActivityStatusesResponse"]["Response"]
           ["ActivityStatus"]["state"].New(sst);
    substate = (std::string)sst;
    (*resp)["Fault"]["faultstring"].New(fs);
    faultstring=(std::string)fs;
    delete resp;
    if (faultstring!="")
      throw AREXClientError(faultstring);
    else if (state=="")
      throw AREXClientError("The job status could not be retrieved.");
    else
      return state+"/"+substate;
  }
  
  std::string AREXClient::sstat(void)
    throw(AREXClientError)
  {
    std::string state, faultstring;
    logger.msg(Arc::INFO, "Creating and sending a service status request.");
    
    Arc::PayloadSOAP req(arex_ns);
    Arc::XMLNode jobref =
      req.NewChild("bes-factory:GetFactoryAttributesDocument");
    
    // Send status request
    Arc::PayloadSOAP* resp = NULL;
    if(client) {
      Arc::MCC_Status status = client->process(
         "http://schemas.ggf.org/bes/2006/08/bes-factory/BESFactoryPortType/GetFactoryAttributesDocument",
         &req,&resp);
      if(resp == NULL) {
        logger.msg(Arc::ERROR,"There was no SOAP response.");
        throw AREXClientError("There was no SOAP response.");
      }
    } else if(client_entry) {
      Arc::Message reqmsg;
      Arc::Message repmsg;
      Arc::MessageAttributes attributes_req;
      attributes_req.set("SOAP:ACTION","http://schemas.ggf.org/bes/2006/08/bes-factory/BESFactoryPortType/GetFactoryAttributesDocument");
      Arc::MessageAttributes attributes_rep;
      Arc::MessageContext context;
      reqmsg.Payload(&req);
      reqmsg.Attributes(&attributes_req);
      reqmsg.Context(&context);
      repmsg.Attributes(&attributes_rep);
      repmsg.Context(&context);
      Arc::MCC_Status status = client_entry->process(reqmsg,repmsg);
      if(!status) {
        logger.msg(Arc::ERROR, "A service status request failed.");
        throw AREXClientError("The service status request failed.");
      }
      logger.msg(Arc::INFO, "A service status request succeed.");
      if(repmsg.Payload() == NULL) {
        logger.msg(Arc::ERROR, "There were no response to a service status request.");
        throw AREXClientError("There were no response.");
      }
      try {
        resp = dynamic_cast<Arc::PayloadSOAP*>(repmsg.Payload());
      } catch(std::exception&) { };
      if(resp == NULL) {
        logger.msg(Arc::ERROR,
		 "The response of a servicee status request was not a SOAP message.");
        delete repmsg.Payload();
        throw AREXClientError("The response is not a SOAP message.");
      }
    } else {
      throw AREXClientError("There is no connection chain configured.");
    };
    Arc::XMLNode st;
    (*resp)["GetFactoryAttributesDocumentResponse"]
           ["FactoryResourceAttributesDocument"].New(st);
    st.GetDoc(state);
    delete resp;
    if (state=="")
      throw AREXClientError("The service status could not be retrieved.");
    else
      return state;
  }

  void AREXClient::kill(const std::string& jobid)
    throw(AREXClientError)
  {
    std::string result, faultstring;
    logger.msg(Arc::INFO, "Creating and sending request to terminate a job.");
    
    Arc::PayloadSOAP req(arex_ns);
    Arc::XMLNode jobref =
      req.NewChild("bes-factory:TerminateActivities").
      NewChild(Arc::XMLNode(jobid));
    
    // Send kill request
    Arc::PayloadSOAP* resp = NULL;
    if(client) {
      Arc::MCC_Status status = client->process(
         "http://schemas.ggf.org/bes/2006/08/bes-factory/BESFactoryPortType/TerminateActivities",
         &req,&resp);
      if(resp == NULL) {
        logger.msg(Arc::ERROR,"There was no SOAP response.");
        throw AREXClientError("There was no SOAP response.");
      }
    } else if(client_entry) {
      Arc::Message reqmsg;
      Arc::Message repmsg;
      Arc::MessageAttributes attributes_req;
      attributes_req.set("SOAP:ACTION","http://schemas.ggf.org/bes/2006/08/bes-factory/BESFactoryPortType/TerminateActivities");
      Arc::MessageAttributes attributes_rep;
      Arc::MessageContext context;
      reqmsg.Payload(&req);
      reqmsg.Attributes(&attributes_req);
      reqmsg.Context(&context);
      repmsg.Attributes(&attributes_rep);
      repmsg.Context(&context);
      Arc::MCC_Status status = client_entry->process(reqmsg,repmsg);
      if(!status) {
        logger.msg(Arc::ERROR, "A job termination request failed.");
        throw AREXClientError("The job termination request failed.");
      }
      logger.msg(Arc::INFO, "A job termination request succeed.");
      if(repmsg.Payload() == NULL) {
        logger.msg(Arc::ERROR,
		 "There was no response to a job termination request.");
        throw AREXClientError
	("There was no response to the job termination request.");
      }
      try {
        resp = dynamic_cast<Arc::PayloadSOAP*>(repmsg.Payload());
      } catch(std::exception&) { };
      if(resp == NULL) {
        logger.msg(Arc::ERROR,
	"The response of a job termination request was not a SOAP message");
        delete repmsg.Payload();
        throw AREXClientError("The response is not a SOAP message.");
      }
    } else {
      throw AREXClientError("There is no connection chain configured.");
    };

    Arc::XMLNode cancelled, fs;
    (*resp)["TerminateActivitiesResponse"]
           ["Response"]["Cancelled"].New(cancelled);
    result = (std::string)cancelled;
    (*resp)["Fault"]["faultstring"].New(fs);
    faultstring=(std::string)fs;
    delete resp;
    if (faultstring!="")
      throw AREXClientError(faultstring);
    if (result!="true")
      throw AREXClientError("Job termination failed.");
  }
  
}
