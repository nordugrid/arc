/**      
 * Header file of Time Web Service
 *
 * The task of the service is alway to reply with the
 * current system time.
 */

#ifndef __ARCSERVICE_TIME_H__
#define __ARCSERVICE_TIME_H__

#include <arc/message/Service.h>
#include <arc/Logger.h>

namespace ArcService
{

class TimeService: public Arc::Service
{
	protected:

		/**
		*  Arc-intern logger. Redirects the log messages into the 
		*  file specified within the server configuration file.
		*/
		Arc::Logger logger;

		/**
		* XML namespace of the outgoing SOAP message.
		*/
		Arc::NS ns_;

	public:

		/**
		* Constructor of the Time Web Service.
		*/
		TimeService(Arc::Config *cfg);

		/**
		* Destructor of the Time Web Service.
		*/
		virtual ~TimeService(void);

		/**
		* Implementation of the virtual method defined in MCCInterface
		* (to be found in MCC.h). 
		* @param inmsg incoming message
		* @param inmsg outgoing message
		* @return Status of the achieved result 
		*/
		virtual Arc::MCC_Status process(Arc::Message& inmsg, Arc::Message& outmsg);
}; 

} //namespace ArcService

#endif 