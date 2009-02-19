#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/ArcLocation.h>
#include <arc/message/MCC.h>
#include <arc/client/ClientInterface.h>
#include <arc/client/UserConfig.h>

#include <iostream>
#include <string>

using namespace Arc;

int main(int argc, char** argv) {

  std::string urlstring("http://localhost:60000/echo");
  std::string message("She's crazy like a fool");

  // Initiate logger to get the output of ARC classes
  Arc::Logger logger(Arc::Logger::getRootLogger(), "arcecho");
  Arc::LogStream logcerr(std::cerr);
  Arc::Logger::getRootLogger().addDestination(logcerr);
  Arc::Logger::rootLogger.setThreshold(Arc::WARNING);


  // Set ARC installation directory for being able to 
  // loaded modules dynamically
  Arc::ArcLocation::Init("/usr/lib/arc");  

  // Creates the counterpart to the HED provided by the arched server.
  // A stack with several Message Chain Components (MCC) will be created
  // depending on the given configuration
  Arc::MCCConfig cfg;
  Arc::URL service(urlstring);
  Arc::ClientSOAP client(cfg, service);

	std::string xml;
	Arc::NS ns("echo", "urn:echo"); 
	Arc::PayloadSOAP request(ns);
	Arc::PayloadSOAP *response = NULL;
	request.NewChild("echo:echoRequest").NewChild("echo:say") = message;

	request.GetXML(xml, true);
	printf("Request:\n\n%s\n\n\n", xml.c_str());


	Arc::MCC_Status status = client.process(&request, &response);

	if (!status) {
		printf("Error %s",((std::string)status).c_str());
		if (response)
			delete response;
		return 1;
	}


  if (!response) {
    printf("No SOAP response");
    return 1;
  }

  response->GetXML(xml, true);
  printf("Response:\n\n%s\n\n\n", xml.c_str());
  std::string answer = (std::string)((*response)["echoResponse"]["hear"]);


  delete response;

  std::cout << answer << std::endl;

  return 0;
}
