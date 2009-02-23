#include <arc/loader/Plugin.h>
#include <arc/message/PayloadSOAP.h>
#include <stdio.h>


#include "secechoservice.h"

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
	return new ArcService::SecEchoService((Arc::Config*)(*mccarg));
}

/**
 * This variable is defining basic entities of the implemented service.
 * It is needed to get the correct entry point into the plugin.
 * @see Plugin#get_instance()
 */
Arc::PluginDescriptor PLUGINS_TABLE_NAME[] = {
	{
		"sececho",				/* Unique name of plugin in scope of its kind */
		"HED:SERVICE",			/* Type/kind of plugin */
		1,				/* Version of plugin (0 if not applicable) */
		&get_service 			/* Pointer to constructor function */
	},
	{ NULL, NULL, 0, NULL }			/* The array is terminated by element */
						/* with all components set to NULL*/
};


using namespace Arc;

namespace ArcService
{

	SecEchoService::SecEchoService(Arc::Config *cfg) : Service(cfg),logger(Logger::rootLogger, "Echo") 
	{
		// Setting the namespace of the incoming and outgoing payloads
		ns_["sececho"]="urn:sececho";
		// Extract prefix and suffix out of the arched configuration file
		prefix_=(std::string)((*cfg)["prefix"]);
		suffix_=(std::string)((*cfg)["suffix"]); 
	}

	SecEchoService::~SecEchoService(void) 
	{
	}

	Arc::MCC_Status SecEchoService::make_fault(Arc::Message& outmsg) 
	{

		
		// The boolean value in the constructur indicates a failure.
		Arc::PayloadSOAP* outpayload = new Arc::PayloadSOAP(ns_,true);
		Arc::SOAPFault* fault = outpayload->Fault();

		if(fault) {
			fault->Code(Arc::SOAPFault::Sender);
			fault->Reason("Failed processing request");
		};
		outmsg.Payload(outpayload);
		return Arc::MCC_Status(Arc::GENERIC_ERROR);
	}

	Arc::MCC_Status SecEchoService::process(Arc::Message& inmsg, Arc::Message& outmsg) 
	{
		logger.msg(Arc::DEBUG, "Echoservice has been started...");

		std::cout<<"DN of the client: =>"<<inmsg.Attributes()->get("TLS:PEERDN")<<"<="<<std::endl;


		/* Processes the SecHandlers of the service */
		if(!ProcessSecHandlers(inmsg,"incoming")) {
			logger.msg(Arc::DEBUG, "Echo: Didn't passes SecHandler!");
			return Arc::MCC_Status(Arc::GENERIC_ERROR);
		}else{
			logger.msg(Arc::DEBUG, "Echo: Passed SecHandler!");
		}





		// Both input and output are supposed to be SOAP 
		Arc::PayloadSOAP* inpayload  = NULL;
		Arc::PayloadSOAP* outpayload = NULL;

		// Extracting incoming payload
		try {
			inpayload = dynamic_cast<Arc::PayloadSOAP*>(inmsg.Payload());
		} catch(std::exception& e) { };
		if(!inpayload) {
			logger.msg(Arc::ERROR, "Input is not SOAP");
			return make_fault(outmsg);
		};
		{
			std::string str;
			inpayload->GetDoc(str, true);
			logger.msg(Arc::DEBUG, "process: request=\n%s",str);
		}; 


		// Analyzing request 
		Arc::XMLNode requestNode  = (*inpayload)["secechoRequest"];
		Arc::XMLNode sayNode      = requestNode["say"];
		std::string operation = (std::string) sayNode.Attribute("operation");
		std::string say       = (std::string) sayNode;
		std::string hear      = "";

		logger.msg(Arc::DEBUG, "Say: \"%s\"  Operation: \"%s\"",say,operation);

		if(!sayNode) {
			logger.msg(Arc::ERROR, "Request is not supported - \n%s", sayNode.Name());
			return make_fault(outmsg);
		};


	
		if(operation.compare(ECHO_TYPE_ORDINARY) == 0){

			hear = prefix_+say+suffix_;

		}else if(operation.compare(ECHO_TYPE_REVERSE) == 0){

			int len = say.length ();
			int n;
			std::string reverse = say;
			for (n = 0;n < len; n++)
			{
				reverse[len - n - 1] = say[n];
			}
			hear = prefix_+ reverse +suffix_;

		}else{
			hear = "Operation was not specified";
		}

		outpayload = new Arc::PayloadSOAP(ns_);
		outpayload->NewChild("secechoResponse").NewChild("hear")=hear;
		outmsg.Payload(outpayload);


		std::string str;
		outpayload->GetDoc(str, true);
		logger.msg(Arc::DEBUG, "process: response=%s",str);


		logger.msg(Arc::DEBUG, "Echoservice done...");
  		return Arc::MCC_Status(Arc::STATUS_OK);
	}


}//namespace



