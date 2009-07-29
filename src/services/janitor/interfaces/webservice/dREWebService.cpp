#include <iostream>
#include <string>
#include <fstream>
#include <stdio.h>


#include <arc/loader/Plugin.h>
#include <arc/message/PayloadSOAP.h>
#include <arc/ArcLocation.h>


#include "dREWebService.h"


/**
 * Method which is capable to process the argument PluginArgument.
 * Initializes the service and returns it.
 * It is needed to load the class as a plugin.
 */
static Arc::Plugin* get_service(Arc::PluginArgument* arg)
{
	// The dynamic cast can handle NULL
	Arc::ServicePluginArgument* mccarg = dynamic_cast<Arc::ServicePluginArgument*>(arg);
	// may be NULL in case dynamic cast fails
	if(!mccarg) return NULL;
	return new DREService::DREWebService((Arc::Config*)(*mccarg));
}

/**
 * This variable is defining basic entities of the implemented service.
 * It is needed to get the correct entry point into the plugin.
 * @see Plugin#get_instance()
 */
Arc::PluginDescriptor PLUGINS_TABLE_NAME[] = {
	{
		"dreservice",				/* Unique name of plugin in scope of its kind	*/
		"HED:SERVICE",			/* Type/kind of plugin							*/
		1,						/* Version of plugin (0 if not applicable)		*/
		&get_service 			/* Pointer to constructor function				*/
	},
	{ NULL, NULL, 0, NULL }		/* The array is terminated by element			*/
								/* with all components set to NULL				*/
};







using namespace Arc;

	

namespace DREService
{

	Arc::Logger               DREWebService::logger(Arc::Logger::rootLogger, "dREWebService");

	/**
	* Constructor. Calls the super constructor, initializing the logger and
	* setting the namespace of the payload. Furthermore the prefix and suffix will
	* extracted out of the Config object (Elements were declared inside the HED 
	* configuration file).
	*/
	DREWebService::DREWebService(Arc::Config *cfg) : Service(cfg)//,logger(Logger::rootLogger, "dRE") 
	{
		/** Get settings from configuration function*/
		// like TASK_QUEUE_LENGTH
		int taskQueueLength = 10;
		int taskSetLength   = 5;

 		pTaskqueue     = new TaskQueue::TaskQueue(taskQueueLength);
 		pTaskset       = new TaskSet::TaskSet(taskSetLength);
		pPerlProcessor = new PerlProcessor::PerlProcessor(5, pTaskqueue, pTaskset);

		/** Setting the namespace of the outgoing payload */
		ns_[""] = "urn:dynamicruntime";


	}

	/**
	* Deconstructor. Nothing to be done here.
	*/
	DREWebService::~DREWebService(void) 
	{
		logger.msg(Arc::VERBOSE, " Deconstructing Web Service");
		logger.msg(Arc::VERBOSE, " Flushing set and queue");
		pTaskqueue->flush();
		pTaskset->flush();

		logger.msg(Arc::VERBOSE, " Deconstructing is waiting for PerlProcessor");
		delete pPerlProcessor; // holds taskqueue and taskset


		logger.msg(Arc::VERBOSE, " Deconstructing is waiting for TaskQueue");
		delete pTaskqueue;
		logger.msg(Arc::VERBOSE, " Deconstructing is waiting for TaskSet");
		delete pTaskset;

		
		logger.msg(Arc::VERBOSE, " Deconstructing Web Service ... done");
	}

	/**
	* Method which creates a fault payload 
	*/
	Arc::MCC_Status DREWebService::makeFault(Arc::Message& outmsg, const std::string &reason) 
	{
		logger.msg(Arc::WARNING, "Creating fault! Reason: \"%s\"",reason);
		Arc::PayloadSOAP* outpayload = new Arc::PayloadSOAP(ns_,true);
		Arc::SOAPFault* fault = outpayload->Fault();
		if(fault) {
			fault->Code(Arc::SOAPFault::Sender);
			fault->Reason(reason);
		};
		outmsg.Payload(outpayload);
		return Arc::MCC_Status(Arc::STATUS_OK);
	}


	Arc::MCC_Status DREWebService::process(Arc::Message& inmsg, Arc::Message& outmsg) 
	{




		Task::Task *pTask = new Task::Task(-1,&inmsg, &outmsg);
		pTask->bla = 666;

		logger.msg(Arc::VERBOSE, "DREWEBSERVICE 1  %d",pTask->bla);
		int taskID = pTaskqueue->pushTask(pTask);
		pTaskset->removeTask(taskID);  // we don't want the return pointer. 
		                               // the result should be also in our original one
		                               // if not, something went wrong
		
		// We don't have to extract the outmsg either
		// because the object is already manipulated
		logger.msg(Arc::VERBOSE, "DREWEBSERVICE 2  %d",pTask->bla);
	

// 		PerlInterpreter *my_perl = NULL;
// 
// 		logger.msg(Arc::WARNING, "Allocating memory for perl");
// 
// 		if((my_perl = perl_alloc()) == NULL) {
// 
// 			logger.msg(Arc::INFO, "Unable to allocate memory for perl.");
// 
// 		}else{
// 
// 			perl_construct(my_perl);
// 
//  			char *embedding[] = { (char*)"", (char*)"perl/search.pl" };
// 
// 			if(perl_parse(my_perl, xs_init, 2, embedding, NULL)) {
// 
// 				logger.msg(Arc::INFO, "Error while initializing Perl.");
// 
// 			}else{
// 			
// 				// Run main function to initialize global variables
// 				perl_run(my_perl);
// 	
// 				logger.msg(Arc::INFO, "Analyzing XML message.");
// 
// 
// 				Arc::PayloadSOAP* inpayload  = NULL;
// 				Arc::PayloadSOAP* outpayload = NULL;
// 
// 				/**  Extracting incoming payload */
// 				try {//(@*\label{lst_code:echo_cpp_extracting}*@)
// 					inpayload = dynamic_cast<Arc::PayloadSOAP*>(inmsg.Payload());
// 				} catch(std::exception& e) { };
// 		
// 				if(!inpayload) {
// 					logger.msg(Arc::ERROR, "Input is not SOAP");
// 					return makeFault(outmsg, "Received message was not a valid SOAP message.");
// 				};
// 				/** */
// 
// 				std::string xml;
// 				inpayload->GetXML(xml, true); 
// 				printf("Request message:\n\%s\n\n\n", xml.c_str());
// 
// 				/** Analyze incoming message */
// 				std::string prefix = (*inpayload).NamespacePrefix("urn:dynamicruntime");
// 				prefix += ":";
// 
// 				std::string nodename = prefix;nodename += "request";
// 				printf("%s\n",nodename.c_str());
// 				Arc::XMLNode requestNode            = (*inpayload)[nodename];
// 				nodename = prefix;nodename += "initiator";
// 				Arc::XMLNode initiatorNode          = requestNode[nodename];
// 				nodename = prefix;nodename += "runtimeenvironment";
// 				Arc::XMLNode runtimeenvironmentNode = requestNode[nodename];
// 
// 				
// 				requestNode.GetXML(xml, true); 
// 				printf("requestNode:\n\%s\n\n\n", xml.c_str());
// 
// 				std::string action = (std::string) requestNode.Attribute("action");
// 				std::string jobid  = (std::string) initiatorNode.Attribute("jobid");
// 
// 
// 				std::string runtimeEnvironments = "";
// 				std::string type   = (std::string) runtimeenvironmentNode.Attribute("type");
// 				if(type.compare("dynamic") == 0){/* no namespace check*/
// 					int childrenSize = runtimeenvironmentNode.Size();
// 					for(int i = 0; i < childrenSize;i++){
// 						Arc::XMLNode packageNode = runtimeenvironmentNode.Child(i);
// 						std::string name         = (std::string) packageNode.Attribute("name");
// 						runtimeEnvironments.append(name);
// 						runtimeEnvironments.append("\r\n");
// 					}
// 				}
// 
// 				logger.msg(Arc::INFO, "Message:\n  JobID:   %s\n  Action:  %s\n  Runtime: %s",
// 					jobid.c_str(),action.c_str(),runtimeEnvironments.c_str());
// 				/** */
// 
// 				char* rstr = NULL;
// 				perl_call_va (my_perl,(char*) "triggerAction",
// 				"s", action.c_str(),
// 				"s", jobid.c_str(),
// 				"s", runtimeEnvironments.c_str(),
// 				"OUT",
// 				"s",  &rstr,         //Output parameter 
// 				NULL);               //Don't forget this! 
// 				std::string result(rstr);
// 				delete rstr;
// 	
// 				printf("%s\n",result.c_str());
// 	
// 			
// 				// check $@ 
// 				if(SvTRUE(ERRSV)){
// 					logger.msg(Arc::ERROR, "Error while function evaluation:\n  %s\n", SvPV(ERRSV,PL_na));	
// 				}
// 
// 			}
// 
// 			logger.msg(Arc::INFO, "Perl destruct level");
// 	
// 			PL_perl_destruct_level = 1; // "Makes everything squeaky clean"
// 
// 			logger.msg(Arc::INFO, "Perl destruct");
// 	
// 			perl_destruct(my_perl);
// 
// 			logger.msg(Arc::INFO, "Perl free");
// 	
// 			perl_free(my_perl);
// 
// 			logger.msg(Arc::INFO, "Perl ressources are freeed");
/*
		}*/


		return Arc::MCC_Status(Arc::STATUS_OK);
	}







}//namespace


















void *run1(void *arg)
{
	DREService::DREWebService *dummy = ((DREService::DREWebService*)arg);



	Arc::NS ns("b", "urn:dynamicruntime");
	Arc::PayloadSOAP  request(ns);
	Arc::PayloadSOAP* response = NULL;
	
	Arc::Message reqmsg;
	Arc::Message repmsg;
	Arc::MessageAttributes attributes_req;
	Arc::MessageAttributes attributes_rep;
	Arc::MessageContext context;
	reqmsg.Attributes(&attributes_req);
	repmsg.Attributes(&attributes_rep);
	reqmsg.Context(&context);
	repmsg.Context(&context);  

	//REGISTER
// 	XMLNode requestNode = request.NewChild("b:request");
// 	requestNode.NewAttribute("action") = "REGISTER";
// 	requestNode.NewChild("b:initiator").NewAttribute("jobid") = "4321";
// 	XMLNode runtimeNode = requestNode.NewChild("b:runtimeenvironment");
// 	runtimeNode.NewAttribute("type") = "dynamic";
// 	runtimeNode.NewChild("b:package").NewAttribute("name") = "APPS/BIO/WEKA-3.4.10";
// 	reqmsg.Payload(&request);

// 	//CHECK
// 	XMLNode requestNode = request.NewChild("b:request");
// 	requestNode.NewAttribute("action") = "CHECK";
// 	requestNode.NewChild("b:initiator").NewAttribute("jobid") = "4321";
// 	reqmsg.Payload(&request);

	//LIST
// 	XMLNode requestNode = request.NewChild("b:request");
// 	requestNode.NewAttribute("action") = "LIST";
// 	reqmsg.Payload(&request);

 	//DEPLOY
	XMLNode requestNode = request.NewChild("b:Request");
	requestNode.NewAttribute("action") = "DEPLOY";
	requestNode.NewChild("b:Initiator").NewAttribute("jobid") = "4321";
	reqmsg.Payload(&request);

 	//REMOVE
// 	XMLNode requestNode = request.NewChild("b:request");
// 	requestNode.NewAttribute("action") = "REMOVE";
// 	requestNode.NewChild("b:initiator").NewAttribute("jobid") = "4321";
// 	reqmsg.Payload(&request);

	dummy->process(reqmsg, repmsg);


	pthread_exit((void*) 0);
}




void *run2(void *arg)
{
	DREService::DREWebService *dummy = ((DREService::DREWebService*)arg);



	Arc::NS ns("b", "urn:dynamicruntime");

	Arc::PayloadSOAP  request2(ns);
	Arc::PayloadSOAP* response = NULL;
	
	// Due to the fact that the class ClientSOAP isn't used anymore,
	// one has to prepare the messages which envelope the payload oneself.
	Arc::Message req2msg;
	Arc::Message repmsg;
	Arc::MessageAttributes attributes_req2;
	Arc::MessageAttributes attributes_rep;
	Arc::MessageContext context;
	req2msg.Attributes(&attributes_req2);
	repmsg.Attributes(&attributes_rep);
	req2msg.Context(&context);
	repmsg.Context(&context);  


// 	//CHECK
	XMLNode request2Node = request2.NewChild("b:Request");
	request2Node.NewAttribute("action") = "CHECK";
	request2Node.NewChild("b:Initiator").NewAttribute("jobid") = "4321";
	req2msg.Payload(&request2);

	//LIST
// 	XMLNode request2Node = request2.NewChild("b:request");
// 	request2Node.NewAttribute("action") = "LIST";
// 	reqmsg.Payload(&reques2t);

 	//DEPLOY
// 	XMLNode request2Node = request2.NewChild("b:request");
// 	request2Node.NewAttribute("action") = "DEPLOY";
// 	request2Node.NewChild("b:initiator").NewAttribute("jobid") = "4321";
// 	req2msg.Payload(&request2);

 	//REMOVE
// 	XMLNode request2Node = request2.NewChild("b:request");
// 	request2Node.NewAttribute("action") = "REMOVE";
// 	request2Node.NewChild("b:initiator").NewAttribute("jobid") = "4321";
// 	req2msg.Payload(&request2);

	dummy->process(req2msg, repmsg);
	pthread_exit((void*) 0);
}







	int main(int argc, char** argv) {
		std::string conffile    = "dre-service_HED.xml";
		std::string arclib("/usr/lib/arc");


	// Initiate the Logger and set it to the standard error stream
	Arc::LogLevel threshold = Arc::VERBOSE;
	Arc::Logger logger(Arc::Logger::getRootLogger(), "dREMain");
	Arc::LogStream logcerr(std::cerr);
	Arc::Logger::getRootLogger().addDestination(logcerr);
	Arc::Logger::rootLogger.setThreshold(threshold);


		std::string xmlstring;
		std::ifstream file(conffile.c_str());
		if (file.good())
		{
			printf("Reading configuration file \"%s\"\n",conffile.c_str());
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
			printf("Client configuration file %s not found\n",argv[1]);
			return -1;
		}
		/** */
		// Set the ARC installation directory
		Arc::ArcLocation::Init(arclib);  
	
		/** Load the client configuration *///(@*\label{lst_code:tls_echo_client_cpp_loadconfiguration}*@)
		printf("Loading client configuration.\n");
		Arc::XMLNode clientXml(xmlstring);
		Arc::Config *clientConfig = new Arc::Config(clientXml);
		if(!clientConfig) {
		  	printf("Failed to load client configuration\n");
		  	return -1;
		};


		printf("Create Web Service\n");
		DREService::DREWebService *dummy = new DREService::DREWebService (clientConfig);




	
		pthread_t pThread;
		pthread_t pThread2;
		pthread_t pThread3;
		pthread_t pThread4;
		pthread_t pThread5;
		pthread_t pThread6;
		pthread_t pThread7;

		pthread_create(&pThread, NULL, run1,(void*) dummy);
		pthread_create(&pThread2, NULL, run2,(void*) dummy);	
		pthread_create(&pThread3, NULL, run1,(void*) dummy);
		pthread_create(&pThread4, NULL, run2,(void*) dummy);	
		pthread_create(&pThread5, NULL, run1,(void*) dummy);
		pthread_create(&pThread6, NULL, run2,(void*) dummy);	
		pthread_create(&pThread7, NULL, run1,(void*) dummy);



while (std::cin.get() != '\n');

// 		sleep(8);

		printf("Initate Web Service shutdown\n");	
		delete dummy;
		delete clientConfig;
		printf("Web Service is down\n");	
	}













