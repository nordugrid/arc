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

	bool printhelp = false;

	std::string command;
	std::string jobid;
	std::string newstate;
	std::vector <std::string> rtes;

	if(argc > 1){

		/** Load the client configuration file into a string */
		std::string xmlstring;
		std::ifstream file(argv[1]);
		if (file.good())
		{
			printf("Reading XML configuration from %s\n",argv[1]);
			file.seekg(0,std::ios::end);
			std::ifstream::pos_type size = file.tellg();
			file.seekg(0,std::ios::beg);
			std::ifstream::char_type *buf = new std::ifstream::char_type[size];
			file.read(buf,size);
			xmlstring = buf;
			delete []buf; 
			file.close(); 
		}
		else
		{
			printf("File %s not found.\n\n",argv[1]);
			printhelp = true;
		}
		/** */

		command.assign(argv[2]);

		std::string commands[] = {	std::string("REGISTER"),
						std::string("DEPLOY"),
						std::string("REMOVE"),
						std::string("INFO"),
						std::string("LIST"),
						std::string("SETSTATE"),
						std::string("SEARCH") };
		unsigned n;
		for(n = 0; n < 6 && strcmp(commands[n].c_str(), command.c_str()); n++); 
			
// 		if(n == anzahlStrings) /* string wurde nicht gefunden */
// 			return -1;
		switch(n) {
			case 0: // REGISTER
				if(argc > 2){
					jobid.assign(argv[2]);
					for(int i = 3; i < argc; i++){
						rtes.push_back(std::string(argv[i]));
					}
				}else{
					printf("Jobid is missing.\n\n");
					printhelp = true;	
				}
			break;
			case 1: // DEPLOY
				if(argc > 2){
					jobid.assign(argv[2]);
				}else{
					printf("Jobid is missing.\n\n");
					printhelp = true;	
				}
			break;
			case 2: // REMOVE
				if(argc > 2){
					jobid.assign(argv[2]);
				}else{
					printf("Jobid is missing.\n\n");
					printhelp = true;	
				}
			break;
			case 3: // INFO
				if(argc > 2){
					jobid.assign(argv[2]);
				}else{
					printf("Jobid is missing.\n\n");
					printhelp = true;	
				}
			break;
			case 4: // LIST
			break;
			case 5: // SETSTATE
				if(argc > 2){
					newstate.assign(argv[2]);
					for(int i = 3; i < argc; i++){
						rtes.push_back(std::string(argv[i]));
					}
				}else{
					printf("New state is missing.\n\n");
					printhelp = true;	
				}
			break;
			case 6: // SEARCH
				for(int i = 2; i < argc; i++){
					rtes.push_back(std::string(argv[i]));
				}
			break;
			default:
				printf("Unknown janitor command.\n\n");
				printhelp = true;
		};

	}else{
		printhelp = true;
	}

	/** Arguments passed by the command line*/
	if(printhelp){
		printf("Usage:\n");
		printf("\n");
		printf("  janitor [config-file] [command] [arg0] [arg1] [arg2] ...\n");
		printf("\n");
		printf("config-file   - specifies the URL and the security attributes.\n");
		printf("                \n");
		printf("command       - REGISTER [jobid] [rte0] [rte1] ...\n");
		printf("                DEPLOY   [jobid]\n");
		printf("                REMOVE   [jobid]\n");
		printf("                INFO     [jobid]\n");
		printf("                LIST\n");
		printf("                SETSTATE [newstate] [rte0] [rte1] ...\n");
		printf("                SEARCH   [string] [string] ...\n");
		printf("\n");
		printf("Examples:\n");
		printf("\n");
		printf("  janitor REGISTER 1919 APPS/BIO/GRID ENV/JAVA/JRE1.5\n");
		printf("  janitor DEPLOY 1919\n");
		printf("  janitor INFO 1919\n");
		printf("  janitor REMOVE 1919\n");
		printf("  janitor SETSTATE REMOVAL_PENDING ENV/JAVA/JRE1.5\n");
		printf("\n");
		return -1;
	}
	/** */

	printf("Command:  %s\n",command.c_str());
	printf("jobid:    %s\n",jobid.c_str());
	printf("newstate  %s\n",newstate.c_str());

	for(int i=0;i < rtes.size(); i++)	{
		printf("RTE:      %s\n",rtes.at(i).c_str());
	}




	// Initiate the Logger and set it to the standard error stream
/*	Arc::Logger logger(Arc::Logger::getRootLogger(), "arcecho");
	Arc::LogStream logcerr(std::cerr);
	Arc::Logger::getRootLogger().addDestination(logcerr);
	Arc::Logger::rootLogger.setThreshold(Arc::WARNING);

	// Set the ARC installation directory
	std::string arclib("/usr/lib/arc");
	Arc::ArcLocation::Init(arclib);  
*/
	/** Load the client configuration */
/*	Arc::XMLNode clientXml(xmlstring);
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
*/
	/** */


	/** Create and process message */
/*	Arc::NS ns("sececho", "urn:sececho");
	Arc::PayloadSOAP  request(ns);
	Arc::PayloadSOAP* response = NULL;
	
	// Due to the fact that the class ClientSOAP isn't used anymore,
	// one has to prepare the messages which envelope the payload oneself.
	Arc::Message reqmsg;
	Arc::Message repmsg;
	Arc::MessageAttributes attributes_req;
	Arc::MessageAttributes attributes_rep;
	Arc::MessageContext context;
	repmsg.Attributes(&attributes_rep);
	reqmsg.Attributes(&attributes_req);
	reqmsg.Context(&context);
	repmsg.Context(&context);  

	XMLNode sayNode = request.NewChild("sececho:secechoRequest").NewChild("sececho:say") = message;
	sayNode.NewAttribute("operation") = type;
	reqmsg.Payload(&request);
*/
	//(@*\drain{std::string xml;  request.GetXML(xml, true); printf("Request message:\n\%s\n\n\n", xml.c_str());/*Remove backslashes and drain*/}*@)
/*	Arc::MCC_Status status = clientEntry->process(reqmsg,repmsg);
	response = dynamic_cast<Arc::PayloadSOAP*>(repmsg.Payload());
*/
	/** */
/*
	if (!response) {
		printf("No SOAP response");
		return 1;
	}else if(response->IsFault()){
		printf("A SOAP fault occured:\n");
        std::string hoff = (std::string) response->Fault()->Reason();
		printf("  Fault code:   %d\n", (response->Fault()->Code()));
		printf("  Fault string: \"%s\"\n", (response->Fault()->Reason()).c_str());
*/
		/** 0  undefined,             4  Sender,   // Client in SOAP 1.0 
            1  unknown,               5  Receiver, // Server in SOAP 1.0 
            2  VersionMismatch,       6  DataEncodingUnknown
            3  MustUnderstand,                                            **/

		//(@*\drain{std::string xmlFault;response->GetXML(xmlFault, true);  printf("\%s\n\n", xmlFault.c_str());}*@)
/*		std::string xmlFault;response->GetXML(xmlFault, true);  printf("\%s\n\n", xmlFault.c_str());
		return 1;
	}

	// Test for possible errors
	if (!status) {
		printf("Error %s",((std::string)status).c_str());
		if (response)
			delete response;
		return 1;
	}
*/
	//(@*\drain{response->GetXML(xml, true);  printf("Response message:\n\%s\n\n\n", xml.c_str());/*Remove backslashes and drain*/}*@)
/*
	std::string answer = (std::string)((*response)["sececho:secechoResponse"]["sececho:hear"]);
	std::cout << answer <<std::endl;

	delete response;
*/
}

