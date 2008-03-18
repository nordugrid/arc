#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string>
#include <iostream>
#include <fstream>
#include <signal.h>

#include <arc/ArcConfig.h>
#include <arc/Logger.h>
#include <arc/XMLNode.h>
#include <arc/loader/Loader.h>
#include <arc/message/SOAPEnvelope.h>
#include <arc/message/PayloadSOAP.h>
#include <arc/StringConv.h>
#include <arc/XMLNode.h>

///A example about how to compose a Request and call the Service_PDP service
int main(void) {
  signal(SIGTTOU,SIG_IGN);
  signal(SIGTTIN,SIG_IGN);
  signal(SIGPIPE,SIG_IGN);
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
  
  // -------------------------------------------------------
  //    Compose request and send to pdp service
  // -------------------------------------------------------
  //Compose request
  Arc::NS ns;
  ns["ra"] = "http://www.nordugrid.org/ws/schemas/request-arc";
  Arc::PayloadSOAP reqdoc(ns);
  
  Arc::XMLNode request = reqdoc.NewChild("ra:Request");
  request.Namespaces(ns);
  Arc::XMLNode requestitem = request.NewChild("ra:RequestItem");

  Arc::XMLNode sub = requestitem.NewChild("ra:Subject");
  Arc::XMLNode subattr1 = sub.NewChild("ra:Attribute");
  //Fill in a fake value
  std::string remotehost = "127.0.0.1";
  subattr1 = remotehost;
  Arc::XMLNode subattr1Id = subattr1.NewAttribute("ra:AttributeId");
  subattr1Id = "123";
  Arc::XMLNode subattr1Type = subattr1.NewAttribute("ra:Type");
  subattr1Type = "string";

  Arc::XMLNode subattr2 = sub.NewChild("ra:Attribute");
  //Fill in a fake value
  std::string subject = "/O=Grid/O=Test/CN=CA";
  subattr2 = subject;
  Arc::XMLNode subattr2Id = subattr2.NewAttribute("ra:AttributeId");
  subattr2Id = "xyz";
  Arc::XMLNode subattr2Type = subattr2.NewAttribute("ra:Type");
  subattr2Type = "string";

  Arc::XMLNode act = requestitem.NewChild("ra:Action");
  //Fill in a fake value
  std::string action = "POST";
  act=action;
  Arc::XMLNode actionId = act.NewAttribute("ra:AttributeId");
  actionId = "ijk";
  Arc::XMLNode actionType = act.NewAttribute("ra:Type");
  actionType = "string";

  std::string req_str;
  reqdoc.GetXML(req_str);
  logger.msg(Arc::INFO, "Request: %s", req_str); 

  // Send request
  Arc::MessageContext context;
  Arc::Message reqmsg;
  Arc::Message repmsg;
  Arc::MessageAttributes attributes_in;
  Arc::MessageAttributes attributes_out;
  reqmsg.Payload(&reqdoc);
  reqmsg.Attributes(&attributes_in);
  reqmsg.Context(&context);
  repmsg.Attributes(&attributes_out);
  repmsg.Context(&context);

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
  
  std::string str;
  resp->GetXML(str);
  logger.msg(Arc::INFO, "Response: %s", str);
  
  delete repmsg.Payload();

  return 0;
}
