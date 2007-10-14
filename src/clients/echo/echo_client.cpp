// echo_client.cpp

#include "echo_client.h"

namespace Arc {
  
  EchoClientError::EchoClientError(const std::string& what) :
    std::runtime_error(what)
  {
  }

  Arc::Logger EchoClient::logger(Arc::Logger::rootLogger, "ECHO-Client");

  EchoClient::EchoClient(std::string configFile)
    throw(EchoClientError)
  {
    logger.msg(Arc::INFO, "Creating an echo client.");

    if (configFile=="" && getenv("ARC_ECHO_CONFIG"))
      configFile = getenv("ARC_ECHO_CONFIG");
    if (configFile=="")
      configFile = "./echo_client.xml";

    client_config = new Arc::Config(configFile.c_str());
    if(!*client_config) {
      logger.msg(Arc::ERROR, "Failed to load client configuration.");
      throw EchoClientError("Failed to load client configuration.");
    }

    client_loader = new Arc::Loader(client_config);
    logger.msg(Arc::INFO, "Client side MCCs are loaded.");
    client_entry = (*client_loader)["soap"];
    if(!client_entry) {
      logger.msg(Arc::ERROR, "Client chain does not have entry point.");
      throw EchoClientError("Client chain does not have entry point.");
    }

    echo_ns["echo"]="urn:echo";
  }
  
  EchoClient::~EchoClient()
  {
    delete client_loader;
    delete client_config;
  }
  
  std::string EchoClient::echo(const std::string& text)
    throw(EchoClientError)
  {
    std::string answer;
    logger.msg(Arc::INFO, "Creating and sending request.");

    // Create job request
    Arc::PayloadSOAP req(echo_ns);
    req.NewChild("echo").NewChild("say")=text;

    // Send job request
    Arc::Message reqmsg;
    Arc::Message repmsg;
    Arc::MessageAttributes attributes;
    Arc::MessageContext context;
    reqmsg.Payload(&req);
    reqmsg.Attributes(&attributes);
    reqmsg.Context(&context);
    Arc::MCC_Status status = client_entry->process(reqmsg,repmsg);
    if(!status) {
      logger.msg(Arc::ERROR, "Echo request failed.");
      throw EchoClientError("Echo request failed.");
    }
    logger.msg(Arc::INFO, "Echo request succeed.");
    if(repmsg.Payload() == NULL) {
      logger.msg
	(Arc::ERROR, "There were no response to an echo request.");
      throw EchoClientError
	("There were no response to the echo request.");
    }
    Arc::PayloadSOAP* resp = NULL;
    try {
      resp = dynamic_cast<Arc::PayloadSOAP*>(repmsg.Payload());
    } catch(std::exception&) { };
    if(resp == NULL) {
      logger.msg(Arc::ERROR,
		 "A response to an echo request was not a SOAP message.");
      delete repmsg.Payload();
      throw EchoClientError
	("The response to the echo request was not a SOAP message.");
    };
    answer = (std::string)((*resp)["echoResponse"]["hear"]);
    delete repmsg.Payload();
    return answer;
  }  
  
}
