#include <string>
#include <iostream>
#include <fstream>
#include <signal.h>

#include "../../libs/common/ArcConfig.h"
#include "../../libs/common/Logger.h"
#include "../../libs/common/XMLNode.h"
#include "../../hed/libs/loader/Loader.h"
#include "../../hed/libs/message/SOAPEnvelope.h"
#include "../../hed/libs/message/PayloadSOAP.h"

int main(void) {
  signal(SIGTTOU,SIG_IGN);
  signal(SIGTTIN,SIG_IGN);
  Arc::Logger logger(Arc::Logger::rootLogger, "Test");
  Arc::LogStream logcerr(std::cerr);
  Arc::Logger::rootLogger.addDestination(logcerr);

  // Load service chain
  logger.msg(Arc::INFO, "Creating service side chain");
  Arc::Config service_config("service.xml");
  if(!service_config) {
    logger.msg(Arc::ERROR, "Failed to load service configuration");
    return -1;
  };
  Arc::Loader service_loader(&service_config);
  logger.msg(Arc::INFO, "Service side MCCs are loaded");
  logger.msg(Arc::INFO, "Creating client side chain");

  // Create client chain
  Arc::Config client_config("client.xml");
  if(!client_config) {
    logger.msg(Arc::ERROR, "Failed to load client configuration");
    return -1;
  };
  Arc::Loader client_loader(&client_config);
  logger.msg(Arc::INFO, "Client side MCCs are loaded");
  Arc::MCC* client_entry = client_loader["soap"];
  if(!client_entry) {
    logger.msg(Arc::ERROR, "Client chain does not have entry point");
    return -1;
  };

  for(int n = 0;n<1;n++) {


    // -------------------------------------------------------
    //    Sending job request to service
    // -------------------------------------------------------
    logger.msg(Arc::INFO, "Creating and sending request");

    // Create job request
    /*
      bes-factory:CreateActivity
        bes-factory:ActivityDocument
          jsdl:JobDefinition
    */
    Arc::NS arex_ns;
    arex_ns["a-rex"]="http://www.nordugrid.org/schemas/a-rex";
    arex_ns["bes-factory"]="http://schemas.ggf.org/bes/2006/08/bes-factory";
    arex_ns["wsa"]="http://www.w3.org/2005/08/addressing";
    arex_ns["jsdl"]="http://schemas.ggf.org/jsdl/2005/11/jsdl";
    Arc::PayloadSOAP req(arex_ns);
    Arc::XMLNode act_doc = req.NewChild("bes-factory:CreateActivity").NewChild("bes-factory:ActivityDocument");
    std::ifstream jsdl_file("jsdl.xml");
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
      logger.msg(Arc::ERROR, "Request failed");
      return -1;
    };
    logger.msg(Arc::INFO, "Request succeed!!!");
    if(repmsg.Payload() == NULL) {
      logger.msg(Arc::ERROR, "There is no response");
      return -1;
    };
    Arc::PayloadSOAP* resp = NULL;
    try {
      resp = dynamic_cast<Arc::PayloadSOAP*>(repmsg.Payload());
    } catch(std::exception&) { };
    if(resp == NULL) {
      logger.msg(Arc::ERROR, "Response is not SOAP");
      delete repmsg.Payload();
      return -1;
    };
    {
      std::string str;
      resp->GetXML(str);
      std::cout << "Response: " << str << std::endl;
    };
    Arc::NS ns;
    Arc::XMLNode id(ns);
    id.NewChild((*resp)["CreateActivityResponse"]["ActivityIdentifier"]);
    {
      std::string str;
      id.GetXML(str);
      std::cout << "Job ID: " << std::endl << str << std::endl;
    };
    delete repmsg.Payload();

    // -------------------------------------------------------
    //    Requesting job's JSDL from service
    // -------------------------------------------------------
    {
      std::string str;
      logger.msg(Arc::INFO, "Creating and sending request");

      Arc::PayloadSOAP req(arex_ns);
      Arc::XMLNode jobref = req.NewChild("bes-factory:GetActivityDocuments").NewChild(id);

      // Send job request
      Arc::Message reqmsg;
      Arc::Message repmsg;
      reqmsg.Payload(&req);
    
      req.GetXML(str);
      std::cout << "REQUEST: " << str << std::endl;
      Arc::MCC_Status status = client_entry->process(reqmsg,repmsg);
      if(!status) {
        logger.msg(Arc::ERROR, "Request failed");
        return -1;
      };
      logger.msg(Arc::INFO, "Request succeed!!!");
      if(repmsg.Payload() == NULL) {
        logger.msg(Arc::ERROR, "There is no response");
        return -1;
      };
      Arc::PayloadSOAP* resp = NULL;
      try {
        resp = dynamic_cast<Arc::PayloadSOAP*>(repmsg.Payload());
      } catch(std::exception&) { };
      if(resp == NULL) {
        logger.msg(Arc::ERROR, "Response is not SOAP");
        delete repmsg.Payload();
        return -1;
      };
      resp->GetXML(str);
      std::cout << "Response: " << str << std::endl;
    };

    // -------------------------------------------------------
    //    Requesting job's status from service
    // -------------------------------------------------------
    {
      std::string str;
      logger.msg(Arc::INFO, "Creating and sending request");

      Arc::PayloadSOAP req(arex_ns);
      Arc::XMLNode jobref = req.NewChild("bes-factory:GetActivityStatuses").NewChild(id);

      // Send job request
      Arc::Message reqmsg;
      Arc::Message repmsg;
      reqmsg.Payload(&req);
    
      req.GetXML(str);
      std::cout << "REQUEST: " << str << std::endl;
      Arc::MCC_Status status = client_entry->process(reqmsg,repmsg);
      if(!status) {
        logger.msg(Arc::ERROR, "Request failed");
        return -1;
      };
      logger.msg(Arc::INFO, "Request succeed!!!");
      if(repmsg.Payload() == NULL) {
        logger.msg(Arc::ERROR, "There is no response");
        return -1;
      };
      Arc::PayloadSOAP* resp = NULL;
      try {
        resp = dynamic_cast<Arc::PayloadSOAP*>(repmsg.Payload());
      } catch(std::exception&) { };
      if(resp == NULL) {
        logger.msg(Arc::ERROR, "Response is not SOAP");
        delete repmsg.Payload();
        return -1;
      };
      resp->GetXML(str);
      std::cout << "Response: " << str << std::endl;
    };




  };
/*


  
  };
*/ 
  return 0;
}
