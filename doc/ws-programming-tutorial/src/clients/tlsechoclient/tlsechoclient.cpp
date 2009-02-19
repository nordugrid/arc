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

	if(argc != 3){
		printf("Usage:\n");
		printf("  tlsechoclient clientHED.xml message\n");
		printf("\n");
		printf("The first argument (clientHED.xml) specifies the HED of the client.\n");
		printf("The argument message contains the text to be transmitted.\n");
		printf("\n");
		printf("Example:\n");
		printf("  ./tlsechoclient clientHED.xml text_to_be_transmitted\n");
		return -1;
	}

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

	std::string message(argv[2]);


	// Initiate logger to get the output of ARC classes
	Arc::Logger logger(Arc::Logger::getRootLogger(), "arcecho");
	Arc::LogStream logcerr(std::cerr);
	Arc::Logger::getRootLogger().addDestination(logcerr);
	Arc::Logger::rootLogger.setThreshold(Arc::WARNING);

	// Set ARC installation directory for being able to 
	// loaded modules dynamically
	std::string arclib("/usr/lib/arc");
	Arc::ArcLocation::Init(arclib);  


	// Loading client.xml file which is the counterpart of the arc 
	// configuration file
	Arc::XMLNode clientXml(xmlstring);

	// Wrap the xml for hed client configuration
	Arc::Config clientConfig(clientXml);
	if(!clientConfig) {
	  logger.msg(Arc::ERROR, "Failed to load client configuration");
	  return -1;
	};


	// Take XML configuration and perform configuration part
	Arc::MCCLoader loader(clientConfig);
	logger.msg(Arc::INFO, "Client side MCCs are loaded");
	Arc::MCC* clientEntry = loader["soap"];
	if(!clientEntry) {
	  logger.msg(Arc::ERROR, "Client chain does not have entry point");
	  return -1;
	};

	//std::string xml;
	Arc::NS ns("tlsecho", "urn:tlsecho");
 
	Arc::PayloadSOAP  request(ns);
	Arc::PayloadSOAP* response = NULL;

	// It is a responsibility of code initiating first Message to
	// provide Context and Attributes as well.
	Arc::Message reqmsg;
	Arc::Message repmsg;
	Arc::MessageAttributes attributes_req;
	Arc::MessageAttributes attributes_rep;
	Arc::MessageContext context;
	repmsg.Attributes(&attributes_rep);
	reqmsg.Attributes(&attributes_req);
	reqmsg.Context(&context);
	repmsg.Context(&context);  

	request.NewChild("tlsecho:tlsechoRequest").NewChild("tlsecho:say") = message;
	reqmsg.Payload(&request);

	//request.GetXML(xml, true);
	//printf("Request:\n\n%s\n\n\n", xml.c_str());

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

	//response->GetXML(xml);
	//printf("Response:\n\n%s\n\n\n", xml.c_str());

	std::string answer = (std::string)((*response)["tlsechoResponse"]["hear"]);//
	std::cout << answer << std::endl;

	delete response;

	return 0;
}
