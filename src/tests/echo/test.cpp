#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <iostream>
#include <signal.h>

#include <arc/ArcConfig.h>
#include <arc/Logger.h>
#include <arc/loader/Loader.h>
#include <arc/message/SOAPEnvelope.h>
#include <arc/message/PayloadSOAP.h>

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
  Arc::XMLNode client_doc("\
    <ArcConfig\
      xmlns=\"http://www.nordugrid.org/schemas/ArcConfig/2007\"\
      xmlns:tcp=\"http://www.nordugrid.org/schemas/ArcMCCTCP/2007\">\
     <ModuleManager>\
        <Path>.libs/</Path>\
        <Path>../../hed/mcc/http/.libs/</Path>\
        <Path>../../hed/mcc/soap/.libs/</Path>\
        <Path>../../hed/mcc/tls/.libs/</Path>\
        <Path>../../hed/mcc/tcp/.libs/</Path>\
     </ModuleManager>\
     <Plugins><Name>mcctcp</Name></Plugins>\
     <Plugins><Name>mcctls</Name></Plugins>\
     <Plugins><Name>mcchttp</Name></Plugins>\
     <Plugins><Name>mccsoap</Name></Plugins>\
     <Chain>\
      <Component name='tcp.client' id='tcp'><tcp:Connect><tcp:Host>127.0.0.1</tcp:Host><tcp:Port>60000</tcp:Port></tcp:Connect></Component>\
      <Component name='tls.client' id='tls'><next id='tcp'/></Component>\
      <Component name='http.client' id='http'><next id='tls'/><Method>POST</Method><Endpoint>/echo1</Endpoint></Component>\
      <Component name='soap.client' id='soap' entry='soap'><next id='http'/></Component>\
     </Chain>\
    </ArcConfig>");
  Arc::Config client_config(client_doc);
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
  // Create and send echo request
  logger.msg(Arc::INFO, "Creating and sending request");
  Arc::NS echo_ns; echo_ns["echo"]="urn:echo";
  Arc::PayloadSOAP req(echo_ns);
  req.NewChild("echo").NewChild("say")="HELLO";
  Arc::Message reqmsg;
  Arc::Message repmsg;
  reqmsg.Payload(&req);
  // It is a responsibility of code initiating first Message to 
  // provide Context and Attributes as well.
  Arc::MessageAttributes attributes;
  Arc::MessageContext context;
  reqmsg.Attributes(&attributes);
  reqmsg.Context(&context);
  Arc::MCC_Status status = client_entry->process(reqmsg,repmsg);
  if(!status) {
    logger.msg(Arc::ERROR, "Request failed");
    return -1;
  };

  logger.msg(Arc::INFO, "Request succeed!!!");

  Arc::PayloadSOAP* resp = NULL;
  if(repmsg.Payload() == NULL) {
    logger.msg(Arc::ERROR, "There is no response");
    return -1;
  };
  try {
    resp = dynamic_cast<Arc::PayloadSOAP*>(repmsg.Payload());
  } catch(std::exception&) { };
  if(resp == NULL) {
    logger.msg(Arc::ERROR, "Response is not SOAP");
    delete repmsg.Payload();
    return -1;
  };
  
  std::cout << "Response: " << (std::string)((*resp)["echoResponse"]["hear"])
	    << std::endl;
  delete repmsg.Payload();
  };
 
  return 0;
}
