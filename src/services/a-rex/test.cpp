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
#include <arc/delegation/DelegationInterface.h>
#include <arc/infosys/InformationInterface.h>
#include <arc/StringConv.h>

int main(void) {
  signal(SIGTTOU,SIG_IGN);
  signal(SIGTTIN,SIG_IGN);
  signal(SIGPIPE,SIG_IGN);


/*
     Arc::NS ns;
     Arc::Config c(ns);
     //Arc::XMLNode c(ns);
     c.NewChild("ArcConfig");
     c["ArcConfig"].NewChild("ModuleManager");
     c["ArcConfig"]["ModuleManager"].NewChild("Path");
     c["ArcConfig"]["ModuleManager"].NewChild("Path");
     c["ArcConfig"]["ModuleManager"]["Path"][0] = "../../hed/mcc/tcp/.libs";
     c["ArcConfig"]["ModuleManager"]["Path"][1] = "../../hed/mcc/http/.libs";
     c["ArcConfig"].NewChild("Plugins");
     c["ArcConfig"].NewChild("Plugins");
     c["ArcConfig"]["Plugins"][0].NewChild("Name");
     c["ArcConfig"]["Plugins"][0]["Name"] = "mcctcp";
     c["ArcConfig"]["Plugins"][1].NewChild("Name");
     c["ArcConfig"]["Plugins"][1]["Name"] = "mcchttp";
     c["ArcConfig"].NewChild("Chain");
     c["ArcConfig"]["Chain"].NewChild("Component");
     c["ArcConfig"]["Chain"].NewChild("Component");
     c["ArcConfig"]["Chain"]["Component"][0].NewAttribute("name");
     c["ArcConfig"]["Chain"]["Component"][0].Attribute("name") = "tcp.client";
     c["ArcConfig"]["Chain"]["Component"][0].NewAttribute("id");
     c["ArcConfig"]["Chain"]["Component"][0].Attribute("id") = "tcp";
     c["ArcConfig"]["Chain"]["Component"][0].NewChild("Connect");
     c["ArcConfig"]["Chain"]["Component"][0]["Connect"].NewChild("Host");
     c["ArcConfig"]["Chain"]["Component"][0]["Connect"]["Host"] = "http://somehost";
     c["ArcConfig"]["Chain"]["Component"][0]["Connect"].NewChild("Port");
     c["ArcConfig"]["Chain"]["Component"][0]["Connect"]["Port"] = "80";
     c["ArcConfig"]["Chain"]["Component"][1].NewAttribute("name");
     c["ArcConfig"]["Chain"]["Component"][1].Attribute("name") = "http.client";
     c["ArcConfig"]["Chain"]["Component"][1].NewAttribute("id");
     c["ArcConfig"]["Chain"]["Component"][1].Attribute("id") = "http";
     c["ArcConfig"]["Chain"]["Component"][1].NewAttribute("entry");
     c["ArcConfig"]["Chain"]["Component"][1].Attribute("entry") = "http";
     c["ArcConfig"]["Chain"]["Component"][1].NewChild("next");
     c["ArcConfig"]["Chain"]["Component"][1]["next"].NewAttribute("id");
     c["ArcConfig"]["Chain"]["Component"][1]["next"].Attribute("id") = "tcp";
     c["ArcConfig"]["Chain"]["Component"][1].NewChild("Method");
     c["ArcConfig"]["Chain"]["Component"][1]["Method"] = "GET";
     c["ArcConfig"]["Chain"]["Component"][1].NewChild("Endpoint");
     c["ArcConfig"]["Chain"]["Component"][1]["Endpoint"] = "http://somehost/somefile";
     std::string str;
     c.GetXML(str);
     std::cerr<<str<<std::endl;
     return 0;
*/



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

  Arc::MessageAttributes attributes;
  Arc::MessageContext context;

  // -------------------------------------------------------
  //    Preparing delegation
  // -------------------------------------------------------
  std::string credentials;
  {
    std::ifstream ic("cert.pem");
    for(;!ic.eof();) {
      char buf[256];
      ic.get(buf,sizeof(buf),0);
      if(ic.gcount() <= 0) break;
      credentials.append(buf,ic.gcount());
    };
  };
  {
    std::ifstream ic("key.pem");
    for(;!ic.eof();) {
      char buf[256];
      ic.get(buf,sizeof(buf),0);
      if(ic.gcount() <= 0) break;
      credentials.append(buf,ic.gcount());
    };
  };
  Arc::DelegationProviderSOAP deleg(credentials);
  if(!credentials.empty()) {
    logger.msg(Arc::INFO, "Initiating delegation procedure");
    if(!deleg.DelegateCredentialsInit(*client_entry,&attributes,&context)) {
      logger.msg(Arc::ERROR, "Failed to initiate delegation");
      return -1;
    };
  };


  // -------------------------------------------------------
  //    Requesting information about service
  // -------------------------------------------------------
  {
    Arc::NS ns;
    Arc::InformationRequest inforeq;
    Arc::PayloadSOAP req(*(inforeq.SOAP()));
    Arc::Message reqmsg;
    Arc::Message repmsg;
    reqmsg.Payload(&req);
    reqmsg.Attributes(&attributes);
    reqmsg.Context(&context);
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
    Arc::InformationResponse inforesp(*resp);
    if(!inforesp) {
      logger.msg(Arc::ERROR, "Response is not expected WS-RP");
      delete repmsg.Payload();
      return -1;
    };
    std::list<Arc::XMLNode> results = inforesp.Result();
    int n = 0;
    for(std::list<Arc::XMLNode>::iterator i = results.begin();i!=results.end();++i) {
      std::string str;
      i->GetXML(str);
      std::cout << "Response("<<n<<"): " << str << std::endl;
    };
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
    Arc::XMLNode op = req.NewChild("bes-factory:CreateActivity");
    Arc::XMLNode act_doc = op.NewChild("bes-factory:ActivityDocument");
    std::ifstream jsdl_file("jsdl.xml");
    std::string jsdl_str; 
    std::getline<char>(jsdl_file,jsdl_str,0);
    act_doc.NewChild(Arc::XMLNode(jsdl_str));
    deleg.DelegatedToken(op);
    req.GetXML(jsdl_str);

    // Send job request
    Arc::Message reqmsg;
    Arc::Message repmsg;
    reqmsg.Payload(&req);
    reqmsg.Attributes(&attributes);
    reqmsg.Context(&context);
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
      Arc::MessageAttributes attributes;
      Arc::MessageContext context;
      reqmsg.Payload(&req);
      reqmsg.Attributes(&attributes);
      reqmsg.Context(&context);

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
      delete resp;
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
      Arc::MessageAttributes attributes;
      Arc::MessageContext context;
      reqmsg.Payload(&req);
      reqmsg.Attributes(&attributes);
      reqmsg.Context(&context);

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
      delete resp;
    };

    // -------------------------------------------------------
    //    Requesting job's termination
    // -------------------------------------------------------
    {
      std::string str;
      logger.msg(Arc::INFO, "Creating and sending request");

      Arc::PayloadSOAP req(arex_ns);
      Arc::XMLNode jobref = req.NewChild("bes-factory:TerminateActivities").NewChild(id);

      // Send job request
      Arc::Message reqmsg;
      Arc::Message repmsg;
      Arc::MessageAttributes attributes;
      Arc::MessageContext context;
      reqmsg.Payload(&req);
      reqmsg.Attributes(&attributes);
      reqmsg.Context(&context);

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
      delete resp;
    };

    // -------------------------------------------------------
    //    Requesting service's attributes
    // -------------------------------------------------------
    {
      std::string str;
      logger.msg(Arc::INFO, "Creating and sending request");

      Arc::PayloadSOAP req(arex_ns);
      req.NewChild("bes-factory:GetFactoryAttributesDocument");

      // Send job request
      Arc::Message reqmsg;
      Arc::Message repmsg;
      Arc::MessageAttributes attributes;
      Arc::MessageContext context;
      reqmsg.Payload(&req);
      reqmsg.Attributes(&attributes);
      reqmsg.Context(&context);

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
      delete resp;
    };

  };

  for(;;) sleep(10);

  return 0;
}
