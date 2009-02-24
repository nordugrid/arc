#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/ArcConfig.h>
#include <arc/ArcLocation.h>
#include <arc/client/ClientInterface.h>
#include <arc/client/UserConfig.h>
#include <arc/message/MCCLoader.h>
#include <arc/message/MCC.h>


#include <iostream>
#include <string>
#include <fstream>

using namespace Arc;

int main(int argc, char** argv) {


	/** Get arguments passed by the command line*/
	if(argc != 4){
		printf("Usage:\n");
		printf("  tlsechoclient configuration type message\n");
		printf("\n");
		printf("The first argument specifies the HED configuration file to be loaded\n");
		printf("The second argument the type of echo which shall be performed (ordinary or reverse).\n");
		printf("The third agrument holds the message to be transmitted.\n");
		printf("\n");
		printf("Example:\n");
		printf("  ./tlsechoclient clientHED.xml ordinary text_to_be_transmitted\n");
		return -1;
	}
	
	std::string type(argv[2]);
	std::string message(argv[3]);
	/** */

	std::string xmlstring;
	std::ifstream file(argv[1]);
	if (file.good())
	{
		printf("Reading XML schema from %s\n",argv[1]);
		file.seekg(0,std::ios::end);
		std::ifstream::pos_type size = file.tellg();
		file.seekg(0,std::ios::beg);
		std::ifstream::char_type *buf = new std::ifstream::char_type[size];
		file.read(buf,size);
		xmlstring = buf;
		//f("%s",xmlstring.c_str());
		delete []buf; 
		file.close(); 
	}
	else
	{
		std::cout << "File " << argv[1] << " not found." << std::endl;
		return -1;
	}



	// Initiate the Logger and set it to the standard error stream
	Arc::Logger logger(Arc::Logger::getRootLogger(), "arcecho");
	Arc::LogStream logcerr(std::cerr);
	Arc::Logger::getRootLogger().addDestination(logcerr);
	Arc::Logger::rootLogger.setThreshold(Arc::WARNING);

	// Set the ARC installation directory
	std::string arclib("/usr/lib/arc");
	Arc::ArcLocation::Init(arclib);  

	// Load configuration 
	Arc::XMLNode clientXml(xmlstring);
	Arc::Config clientConfig(clientXml);

	if(!clientConfig) {
	  logger.msg(Arc::ERROR, "Failed to load client configuration");
	  return -1;
	};

	Arc::MCCLoader loader(clientConfig);
	logger.msg(Arc::INFO, "Client side MCCs are loaded");
	Arc::MCC* clientEntry = loader["soap"];
	if(!clientEntry) {
	  logger.msg(Arc::ERROR, "Client chain does not have entry point");
	  return -1;
	};

	//Prepare the payload
	Arc::NS ns("tlsecho", "urn:tlsecho");
	Arc::PayloadSOAP  request(ns);
	Arc::PayloadSOAP* response = NULL;

	//Prepare the messages which will envelope the payload.
	//(*@\drain{It is a responsibility of code initiating first Message to provide Context and Attributes as well.}@*)
	Arc::Message reqmsg;
	Arc::Message repmsg;
	Arc::MessageAttributes attributes_req;
	Arc::MessageAttributes attributes_rep;
	Arc::MessageContext context;
	repmsg.Attributes(&attributes_rep);
	reqmsg.Attributes(&attributes_req);
	reqmsg.Context(&context);
	repmsg.Context(&context);  

	XMLNode sayNode = request.NewChild("tlsecho:tlsechoRequest").NewChild("tlsecho:say") = message;
	sayNode.NewAttribute("operation") = type;

	reqmsg.Payload(&request);
	//(@*\drain{std::string xml;  request.GetXML(xml, true); printf("Request message:\n\%s\n\n\n", xml.c_str());/*Remove backslashes and drain*/}*@)
std::string xml;  request.GetXML(xml, true); printf("Request message:\n%s\n\n\n", xml.c_str());

	Arc::MCC_Status status = clientEntry->process(reqmsg,repmsg);

	if(!status) {
		logger.msg(Arc::ERROR, "Request failed");
		std::cerr << "Status: " << std::string(status) << std::endl;
		return -1;
	};

	if(repmsg.Payload() == NULL) {
		logger.msg(Arc::ERROR, "There is no response");
		return -1;
	};

	try {
		response = dynamic_cast<Arc::PayloadSOAP*>(repmsg.Payload());
	} catch(std::exception&) { };

	if(response == NULL) {
		logger.msg(Arc::ERROR, "Response is not SOAP");
		return -1;
	};
	//(@*\drain{response->GetXML(xml, true);  printf("Response message:\n\%s\n\n\n", xml.c_str());/*Remove backslashes and drain*/}*@)
(*response).GetXML(xml, true); printf("response message:\n%s\n\n\n", xml.c_str());
	std::string answer = (std::string)((*response)["tlsecho:tlsechoResponse"]["tlsecho:hear"]);//
	std::cout << answer << std::endl;

	delete response;

	return 0;
}
