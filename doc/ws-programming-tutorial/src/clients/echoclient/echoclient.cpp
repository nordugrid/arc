#include <arc/ArcLocation.h>
#include <arc/message/MCC.h>
#include <arc/client/ClientInterface.h>
#include <arc/client/UserConfig.h>

#include <iostream>
#include <string>

using namespace Arc;

int main(int argc, char** argv)
{

	/** Get arguments passed by the command line*/
	if(argc != 4){
		printf("Usage:\n");
		printf("  echoclient url type message\n");
		printf("\n");
		printf("The first argument defines the URL of the service.\n");
		printf("The second argument the type of echo which shall be performed (ordinary or reverse).\n");
		printf("The third agrument holds the message to be transmitted.\n");
		printf("\n");
		printf("Example:\n");
		printf("  ./echoclient http://localhost:60000/echo ordinary text_to_be_transmitted\n");
		return -1;
	}
	
	std::string urlstring(argv[1]);
	std::string type(argv[2]);
	std::string message(argv[3]);
	/** */

	// Initiate the Logger and set it to the standard error stream
	Arc::Logger::rootLogger.setThreshold(Arc::WARNING);
	Arc::Logger logger(Arc::Logger::getRootLogger(), "echo");
	Arc::LogStream logcerr(std::cerr);
	Arc::Logger::getRootLogger().addDestination(logcerr);
	Arc::Logger::rootLogger.setThreshold(Arc::WARNING);


	// Set the ARC installation directory
	Arc::ArcLocation::Init("/usr/lib/arc");
	setlocale(LC_ALL, "");   

	// Load user configuration (default ~/.arc/client.xml)
	std::string conffile; 												//(@*\label{lst_code:time_client_cpp_config}*@)
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
	Arc::ClientSOAP client(cfg, service);								//(@*\label{lst_code:time_client_cpp_client}*@)


	// Defining the namespace and create a request
	Arc::NS ns("echo", "urn:echo"); 										//(@*\label{lst_code:time_client_cpp_message_reqeust}*@)
	Arc::PayloadSOAP request(ns);
	XMLNode sayNode = request.NewChild("echo:echoRequest").NewChild("echo:say") = message;
	sayNode.NewAttribute("operation") = type;
	//(@*\drain{std::string xml;  request.GetXML(xml, true); printf("Request message:\n\%s\n\n\n", xml.c_str());/*Remove backslashes and drain*/}*@)

	// Process request and get response
	Arc::PayloadSOAP *response = NULL;								//(@*\label{lst_code:time_client_cpp_message_response}*@)
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
	//(@*\drain{response->GetXML(xml, true);  printf("Response message:\n\%s\n\n\n", xml.c_str());/*Remove backslashes and drain*/}*@)

	std::string answer = (std::string)((*response)["echo:echoResponse"]["echo:hear"]);
	std::cout << answer << std::endl;

	delete response;

	return 0;
}