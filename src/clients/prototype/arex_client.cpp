// arex_client.cpp

#include "arex_client.h"

namespace Arc {
  
  AREXClientError::AREXClientError(const std::string& what) :
    std::runtime_error(what)
  {
  }

  Arc::Logger AREXClient::logger(Arc::Logger::rootLogger, "A-REX-Client");

  AREXClient::AREXClient(std::string configFile)
    throw(AREXClientError)
  {
    if (configFile=="")
      configFile = getenv("ARC_AREX_CONFIG");
    if (configFile=="")
      configFile = "./arex_client.xml";

    Arc::Config client_config(configFile.c_str());
    if(!client_config) {
      logger.msg(Arc::ERROR, "Failed to load client configuration.");
      throw AREXClientError("Failed to load client configuration.");
    }

    Arc::Loader client_loader(&client_config);
    logger.msg(Arc::INFO, "Client side MCCs are loaded");
    Arc::MCC* client_entry = client_loader["soap"];
    if(!client_entry) {
      logger.msg(Arc::ERROR, "Client chain does not have entry point.");
      throw AREXClientError("Client chain does not have entry point.");
    }
    
    arex_ns["a-rex"]="http://www.nordugrid.org/schemas/a-rex";
    arex_ns["bes-factory"]="http://schemas.ggf.org/bes/2006/08/bes-factory";
    arex_ns["wsa"]="http://www.w3.org/2005/08/addressing";
    arex_ns["jsdl"]="http://schemas.ggf.org/jsdl/2005/11/jsdl";    
  }
  
  AREXClient::~AREXClient()
  {
  }
  
  std::string AREXClient::submit(std::istream& jsdl_file)
    throw(AREXClientError)
  {
    std::string jobid;
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

    // Send job request
    Arc::Message reqmsg;
    Arc::Message repmsg;
    reqmsg.Payload(&req);
    Arc::MCC_Status status = client_entry->process(reqmsg,repmsg);
    if(!status) {
      logger.msg(Arc::ERROR, "Submission request failed.");
      throw AREXClientError("Submission request failed.");
    }
    logger.msg(Arc::INFO, "Submission request succeed.");
    if(repmsg.Payload() == NULL) {
      logger.msg
	(Arc::ERROR, "There were no response to a submission request.");
      throw AREXClientError
	("There were no response to the submission request.");
    }
    Arc::PayloadSOAP* resp = NULL;
    try {
      resp = dynamic_cast<Arc::PayloadSOAP*>(repmsg.Payload());
    } catch(std::exception&) { };
    if(resp == NULL) {
      logger.msg(Arc::ERROR,
		 "A response to a submission request was not a SOAP message.");
      delete repmsg.Payload();
      throw AREXClientError
	("The response to the submission request was not a SOAP message.");
    };
    Arc::NS ns;
    Arc::XMLNode id(ns);
    id.NewChild((*resp)["CreateActivityResponse"]["ActivityIdentifier"]);
    id.GetXML(jobid);
    delete repmsg.Payload();    
    return jobid;
  }
  
  std::string AREXClient::stat(const std::string& jobid)
    throw(AREXClientError)
  {
    std::string result;
    logger.msg(Arc::INFO, "Creating and sending a status request.");
    
    Arc::PayloadSOAP req(arex_ns);
    Arc::XMLNode jobref =
      req.NewChild("bes-factory:GetActivityStatuses").NewChild(jobid);
    
    // Send job request
    Arc::Message reqmsg;
    Arc::Message repmsg;
    reqmsg.Payload(&req);
    
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
    Arc::PayloadSOAP* resp = NULL;
    try {
      resp = dynamic_cast<Arc::PayloadSOAP*>(repmsg.Payload());
    } catch(std::exception&) { };
    if(resp == NULL) {
      logger.msg(Arc::ERROR,
		 "The response of a status request was not a SOAP message.");
      delete repmsg.Payload();
      throw AREXClientError("The response is not a SOAP message.");
    }
    resp->GetXML(result);
    return result;
  }

  void AREXClient::kill(const std::string& jobid)
    throw(AREXClientError)
  {
    logger.msg(Arc::INFO, "Creating and sending request to terminate a job.");
    
    Arc::PayloadSOAP req(arex_ns);
    Arc::XMLNode jobref =
      req.NewChild("bes-factory:TerminateActivities").NewChild(jobid);
    
    // Send job request
    Arc::Message reqmsg;
    Arc::Message repmsg;
    reqmsg.Payload(&req);
    
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
    Arc::PayloadSOAP* resp = NULL;
    try {
      resp = dynamic_cast<Arc::PayloadSOAP*>(repmsg.Payload());
    } catch(std::exception&) { };
    if(resp == NULL) {
      logger.msg(Arc::ERROR,
	"The response of a job termination request was not a SOAP message");
      delete repmsg.Payload();
      throw AREXClientError("The response is not a SOAP message.");
    }
    //resp->GetXML(str);
    //std::cout << "Response: " << str << std::endl;
  }
  
}
