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
	Arc::ServicePluginArgument* mccarg = dynamic_cast<Arc::ServicePluginArgument*>(arg);
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
		"sececho",				/* Unique name of plugin in scope of its kind	*/
		"HED:SERVICE",			/* Type/kind of plugin							*/
		1,						/* Version of plugin (0 if not applicable)		*/
		&get_service 			/* Pointer to constructor function				*/
	},
	{ NULL, NULL, 0, NULL }		/* The array is terminated by element			*/
};								/* with all components set to NULL				*/



using namespace Arc;

namespace ArcService
{

	/**
	* Constructor. Calls the super constructor, initializing the logger and
	* setting the namespace of the payload. Furthermore the prefix and suffix will
	* extracted out of the Config object (Elements were declared inside the HED 
	* configuration file).
	*/
	SecEchoService::SecEchoService(Arc::Config *cfg) : Service(cfg),logger(Logger::rootLogger, "Echo") 
	{
		ns_["sececho"]="urn:sececho";
		prefix_=(std::string)((*cfg)["prefix"]);
		suffix_=(std::string)((*cfg)["suffix"]); 
	}

	/**
	* Deconstructor. Nothing to be done here.
	*/
	SecEchoService::~SecEchoService(void) 
	{
	}

	/**
	* Method which creates a fault payload 
	*/
	Arc::MCC_Status SecEchoService::makeFault(Arc::Message& outmsg, const std::string &reason) 
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

	/**
	* Processes the incoming message and generates an outgoing message.
	* Returns the incoming string ordinary or reverse - depending on the operation 
	* requested.
	* @param inmsg incoming message
	* @param inmsg outgoing message
	* @return Status of the result achieved
	*/
	Arc::MCC_Status SecEchoService::process(Arc::Message& inmsg, Arc::Message& outmsg) 
	{
		logger.msg(Arc::DEBUG, "Secure Echo Web Service has been started...");



		/** Processes the SecHandlers of the service *///(@*\label{lst_code:sececho_service_cpp_process_secH}*@)
		logger.msg(Arc::DEBUG, "Secure Echo Web Service: Current Client has the DN: \"%s\"",inmsg.Attributes()->get("TLS:PEERDN"));

		if(!ProcessSecHandlers(inmsg,"incoming")) {
			logger.msg(Arc::DEBUG, "Secure Echo Web Service: Didn't passes SecHandler!");
			return makeFault(outmsg, "Access to this service is not permitted!");
		}else{
			logger.msg(Arc::DEBUG, "Secure Echo Web Service: Passed SecHandler!");
		}
		/** */




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
			return makeFault(outmsg, "Received message was not a valid SOAP message.");
		};
		/** */

		/** Analyzing and execute request */
		Arc::XMLNode requestNode  = (*inpayload)["sececho:secechoRequest"];
		Arc::XMLNode sayNode      = requestNode["sececho:say"];
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
			return makeFault(outmsg, "Unknown operation. Please use \"ordinary\" or \"reverse\"");
		}
		/** */

		outpayload = new Arc::PayloadSOAP(ns_);
		outpayload->NewChild("sececho:secechoResponse").NewChild("sececho:hear")=hear;

		outmsg.Payload(outpayload);
		{
			std::string str;
			outpayload->GetDoc(str, true);
			logger.msg(Arc::DEBUG, "process: response=%s",str);
		}; 

		logger.msg(Arc::DEBUG, "Secure Echo Web Service  done...");
  		return Arc::MCC_Status(Arc::STATUS_OK);
	}


}//namespace



