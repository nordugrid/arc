/**      
 * Implementation of a simple time service
 *
 * The time service will always reply with a message which contains
 * the current time of the service.
 *
 */

#ifndef __ARCSERVICE_TIME_H__
#define __ARCSERVICE_TIME_H__

#if HAVE_CONFIG_H
	#include <config.h>
#endif

#include <arc/message/Service.h>
#include <arc/Logger.h>

namespace ArcService
{

class TimeService: public Arc::Service
{

	protected:

		/**
		*  Arc-intern logger. Generates output into the file specified 
		*  in the arched configuration file used to invoke arched services.
		*/
		Arc::Logger logger;

		/**
		* Class which specifies a XML namespace i.e. "time".
		* Needed to extract the content out of the incoming message
		*/
		Arc::NS ns_;

	public:

		/**
		* Constructor getting the Arc-configuration.
		*/
		TimeService(Arc::Config *cfg);

		/**
		* Destructor.
		*/
		virtual ~TimeService(void);

		/**
		* Implementation of the virtual method defined in MCCInterface
		* (to be found in MCC.h). 
		* @param inmsg incoming message
		* @param inmsg outgoing message
		* @return Status of the result achieved
		*/
		virtual Arc::MCC_Status process(Arc::Message& inmsg, Arc::Message& outmsg);

}; 

} //namespace ArcService

#endif 
