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

	// Initiate the Logger and set it to the standard error stream
	Arc::Logger logger(Arc::Logger::getRootLogger(), "arcecho");
	Arc::LogStream logcerr(std::cerr);
	Arc::Logger::getRootLogger().addDestination(logcerr);
	Arc::Logger::rootLogger.setThreshold(Arc::WARNING);

	// Set the ARC installation directory
	Arc::ArcLocation::Init("/usr/lib/arc"); 
	setlocale(LC_ALL, "");  

	// URL to the endpoint
	std::string urlstring("http://localhost:60000/time");
	printf("URL: %s\n",urlstring.c_str());


	// Load user configuration (default ~/.arc/client.xml)
	std::string conffile; 
	Arc::UserConfig usercfg(conffile);
	if (!usercfg) {
		printf("Failed configuration initialization\n");
		return 1;
	}

	// Basic MCC configuration
	Arc::MCCConfig cfg;
	Arc::URL service(urlstring);

	// Creates a typical SOAP client based on the 
	// MCCConfig and the service URL
	Arc::ClientSOAP client(cfg, service);

	// Defining the namespace and create a request
	Arc::NS ns("time", "urn:time"); 
	Arc::PayloadSOAP request(ns);
  	request.NewChild("time").NewChild("timeRequest") = "";

	//std::string xml;
	//request.GetXML(xml, true);
	//printf("Request message:\n%s\n\n\n", xml.c_str());

	// Process request and get response
	Arc::PayloadSOAP *response = NULL;
	Arc::MCC_Status status = client.process(&request, &response);

	// Test for possible errors
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

	//response->GetXML(xml, true);
	//printf("Response message:\n%s\n\n\n", xml.c_str());

	// Extract answer out of the XML response and print it to the standard out
	std::string answer = (std::string)((*response)["time"]["timeResponse"]);
	std::cout << answer << std::endl;

	delete response;

	return 0;
}
