#include <arc/loader/Plugin.h>
#include <arc/message/PayloadSOAP.h>
#include <stdio.h>


#include "echoservice.h"

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
	return new ArcService::EchoService((Arc::Config*)(*mccarg));
}

/**
 * This variable is defining basic entities of the implemented service.
 * It is needed to get the correct entry point into the plugin.
 * @see Plugin#get_instance()
 */
Arc::PluginDescriptor PLUGINS_TABLE_NAME[] = {
	{
		"echo",				/* Unique name of plugin in scope of its kind */
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

	EchoService::EchoService(Arc::Config *cfg) : Service(cfg),logger(Logger::rootLogger, "Echo") 
	{
		// Setting the namespace of the incoming and outgoing payloads
		ns_["echo"]="urn:echo";
		// Extract prefix and suffix out of the arched configuration file
		prefix_=(std::string)((*cfg)["prefix"]);
		suffix_=(std::string)((*cfg)["suffix"]); 
	}

	EchoService::~EchoService(void) 
	{
	}

	Arc::MCC_Status EchoService::make_fault(Arc::Message& outmsg) 
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

	Arc::MCC_Status EchoService::process(Arc::Message& inmsg, Arc::Message& outmsg) 
	{
		logger.msg(Arc::DEBUG, "Echoservice has been started...");


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

		/**Export the formated policy-decision request**/
		MessageAuth* mauth = inmsg.Auth();
		MessageAuth* cauth = inmsg.AuthContext();
		if((!mauth) && (!cauth)) {
			logger.msg(ERROR,"Missing security object in message");
			return Arc::MCC_Status();
		};
		NS ns;
		XMLNode requestxml(ns,"");
		if(mauth) {
			if(!mauth->Export(SecAttr::ARCAuth,requestxml)) {
				delete mauth;
				logger.msg(ERROR,"Failed to convert security information to ARC request");
				return Arc::MCC_Status();
			};
		};
		if(cauth) {
			if(!cauth->Export(SecAttr::ARCAuth,requestxml)) {
				delete mauth;
				logger.msg(ERROR,"Failed to convert security information to ARC request");
				return Arc::MCC_Status();
			};
		};
		/** */

		// Analyzing request 
		Arc::XMLNode echo_op = (*inpayload)["echoRequest"];
		if(!echo_op) {
			logger.msg(Arc::ERROR, "Request is not supported - \n%s", echo_op.Name());
			return make_fault(outmsg);
		};
		std::string say = echo_op["say"];
		std::string hear = prefix_+say+suffix_; // <----------------- PASSING DATA FROM THE ARCHED XML!!!!! COOOLLLLL!
		outpayload = new Arc::PayloadSOAP(ns_);
		outpayload->NewChild("echo:echoResponse").NewChild("echo:hear")=hear;

		outmsg.Payload(outpayload);
		{
			std::string str;
			outpayload->GetDoc(str, true);
			logger.msg(Arc::DEBUG, "process: response=%s",str);
		}; 

		logger.msg(Arc::DEBUG, "Echoservice done...");
  		return Arc::MCC_Status(Arc::STATUS_OK);
	}


}//namespace






	int main(int argc, char* argv[]) 
	{
		printf("huhu\n");
		return 0;
	}

