#include <arc/loader/Plugin.h>
#include <arc/message/PayloadSOAP.h>

#include <stdio.h>

#include "echoservice.h"


/**
 * Initializes the echo service and returns it.
 */
static Arc::Plugin* get_service(Arc::PluginArgument* arg)
{
	Arc::ServicePluginArgument* mccarg = dynamic_cast<Arc::ServicePluginArgument*>(arg);
	if(!mccarg) return NULL;
	return new ArcService::EchoService((Arc::Config*)(*mccarg));
}

/**
 * This PLUGINS_TABLE_NAME is defining basic entities of the implemented .
 * service. It is used to get the correct entry point to the plugin.
 */
Arc::PluginDescriptor PLUGINS_TABLE_NAME[] = {//(@*\label{lst_code:time_cpp_ptn}*@)
	{
		"echo",					/* Unique name of plugin in scope of its kind */
		"HED:SERVICE",			/* Type/kind of plugin */
		1,						/* Version of plugin (0 if not applicable) */
		&get_service 			/* Pointer to constructor function */
	},
	{ NULL, NULL, 0, NULL }		/* The array is terminated by element */
								/* with all components set to NULL*/
};


using namespace Arc;

namespace ArcService
{

	/**
	* Constructor. Calls the super constructor, initializing the logger and
	* setting the namespace of the payload. Furthermore the prefix and suffix will
	* extracted out of the Config object (Elements were declared inside the HED 
	* configuration file).
	*/
	EchoService::EchoService(Arc::Config *cfg) : Service(cfg),logger(Logger::rootLogger, "Echo") 
	{
		// Setting the namespace of the outgoing payload
		ns_["echo"]="urn:echo";

		// Extract prefix and suffix out of the arched configuration file
		prefix_=(std::string)((*cfg)["prefix"]);
		suffix_=(std::string)((*cfg)["suffix"]); 
	}

	/**
	* Deconstructor. Nothing to be done here.
	*/
	EchoService::~EchoService(void) 
	{
	}

	/**
	* Method which creates a fault payload 
	*/
	Arc::MCC_Status EchoService::makeFault(Arc::Message& outmsg) 
	{

		// The boolean true indicates that inside of PayloadSOAP, 
		// an object SOAPFault will be created inside.
		Arc::PayloadSOAP* outpayload = new Arc::PayloadSOAP(ns_,true);
		Arc::SOAPFault* fault = outpayload->Fault();
		if(fault) {
			fault->Code(Arc::SOAPFault::Sender);
			fault->Reason("Failed processing request");
		};
		outmsg.Payload(outpayload);
		return Arc::MCC_Status(Arc::GENERIC_ERROR);
	}

	/**
	* Processes the incoming message and generates an outgoing message.
	* Returns the incoming string ordinary or reverse - depending on the operation 
	* requested.
	* @param inmsg incoming message
	* @param inmsg outgoing message
	* @return Status of the result achieved
	*/
	Arc::MCC_Status EchoService::process(Arc::Message& inmsg, Arc::Message& outmsg) 
	{
		logger.msg(Arc::DEBUG, "Echoservice started...");

		/** Both input and output are supposed to be SOAP */
		Arc::PayloadSOAP* inpayload  = NULL;
		Arc::PayloadSOAP* outpayload = NULL;
		/** */

		/**  Extracting incoming payload */
		try {
			inpayload = dynamic_cast<Arc::PayloadSOAP*>(inmsg.Payload());
		} catch(std::exception& e) { };
		if(!inpayload) {
			logger.msg(Arc::ERROR, "Input is not SOAP");
			return makeFault(outmsg);
		};
		/** */

		/** Analyzing and execute request */
		Arc::XMLNode requestNode  = (*inpayload)["echo:echoRequest"];
		Arc::XMLNode sayNode      = requestNode["echo:say"];
		std::string operation = (std::string) sayNode.Attribute("operation");
		std::string say       = (std::string) sayNode;
		std::string hear      = "";
		logger.msg(Arc::DEBUG, "Say: \"%s\"  Operation: \"%s\"",say,operation);

		//Compare strings with constants defined in the header file
		if(operation.compare(ECHO_TYPE_ORDINARY) == 0)
		{
			hear = prefix_+say+suffix_;
		}
		else if(operation.compare(ECHO_TYPE_REVERSE) == 0)
		{
			int len = say.length ();
			int n;
			std::string reverse = say;
			for (n = 0;n < len; n++)
			{
				reverse[len - n - 1] = say[n];
			}
			hear = prefix_+ reverse +suffix_;
		}
		else
		{
			hear = "Unknown operation. Please use \"ordinary\" or \"reverse\"";
		}
		/** */

		/** Create response */
		outpayload = new Arc::PayloadSOAP(ns_);
		outpayload->NewChild("echo:echoResponse").NewChild("echo:hear")=hear;
		outmsg.Payload(outpayload);
		//(@*\drain{std::string str;outpayload->GetDoc(str, true);logger.msg(Arc::DEBUG, "XML = \%s",str);//strip backslashes}*@)
		/** */

		logger.msg(Arc::DEBUG, "Echoservice done...");
		return Arc::MCC_Status(Arc::STATUS_OK);
	}

}//namespace