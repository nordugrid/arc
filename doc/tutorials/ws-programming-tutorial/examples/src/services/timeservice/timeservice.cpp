#include <arc/loader/Plugin.h>
#include <arc/message/PayloadSOAP.h>

#include <stdio.h>
#include <time.h>

#include "timeservice.h"

/**
 * Within the following variable PLUGINS_TABLE_NAME a pointer to a
 * method with a certain signature has to be stored.
 *
 * This method has to be capable to process the argument PluginArgument 
 * and must return a pointer to a plugin, which is actually the timeservice.
 * 
 * Therefore the method initializes the time service and returns it.
 */
static Arc::Plugin* get_service(Arc::PluginArgument* arg) //(@*\label{lst_code:time_cpp_get_service}*@)
{
	// The dynamic cast can handle NULL
	Arc::ServicePluginArgument* mccarg = dynamic_cast<Arc::ServicePluginArgument*>(arg);
	if(!mccarg) return NULL;

	return new ArcService::TimeService((Arc::Config*)(*mccarg));
}


/**
 * This PLUGINS_TABLE_NAME is defining basic entities of the implemented .
 * service. It is used to get the correct entry point to the plugin.
 */
Arc::PluginDescriptor PLUGINS_TABLE_NAME[] = {//(@*\label{lst_code:time_cpp_ptn}*@)
	{
		"time",					/* Unique name of plugin in scope of its kind */
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
	* setting the namespace of the payload.
	*/
	TimeService::TimeService(Arc::Config *cfg) //(@*\label{lst_code:time_cpp_constructor}*@) 
			: Service(cfg),logger(Logger::rootLogger, "Time")
	{ 
		// Setting the null-namespace for the outgoing payload
		ns_[""]="urn:time";
	}

	/**
	* Deconstructor. Nothing to be done here.
	*/
	TimeService::~TimeService(void)//(@*\label{lst_code:time_cpp_deconstructor}*@)
	{
	}

	/**
	* Implementation of the virtual method defined in MCCInterface.h (to be found in MCC.h). 
	*
	* This method processes the incoming message and generates an outgoing message.
	* For this is a very simple service, the time always being returned without examining
	* the the incoming message.
	*
	* @param inmsg incoming message
	* @param inmsg outgoing message
	* @return Status of the result achieved
	*/
	Arc::MCC_Status TimeService::process(Arc::Message& inmsg,Arc::Message& outmsg)//(@*\label{lst_code:time_cpp_process}*@)
	{
		logger.msg(Arc::DEBUG, "Timeservice has been started...");

		// Get time and create an string out of it
		time_t timer;
		timer=time(NULL);
		std::string loctime = asctime(localtime(&timer));
		loctime = loctime.substr(0,loctime.length()-1); 

		// Create an output message with a tag time containing another
		// tag timeResponse which again shall contain the time string
		Arc::PayloadSOAP* outpayload = NULL; //(@*\label{lst_code:time_cpp_process_payloadSOAP}*@)
		outpayload = new Arc::PayloadSOAP(ns_);
		outpayload->NewChild("time").NewChild("timeResponse")=loctime;
		
		outmsg.Payload(outpayload);//(@*\label{lst_code:time_cpp_process_message_payload}*@)

		// Create well formated, userfriendly (second argument) XML string
		std::string xmlstring;
		outpayload->GetDoc(xmlstring, true);
		logger.msg(Arc::DEBUG, "Response message:\n\n%s",xmlstring.c_str());

		logger.msg(Arc::DEBUG, "Timeservice done...");

		return Arc::MCC_Status(Arc::STATUS_OK);//(@*\label{lst_code:time_cpp_return}*@)
	}

}//namespace