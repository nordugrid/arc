/**
 * @file janitorWebService.h
 * @brief Header file for the janitor Web Service
 *
 * @author Michael Glodek
 */

#ifndef __DRE_SERVICE_H__
#define __DRE_SERVICE_H__

#if HAVE_CONFIG_H
	#include <config.h>
#endif


#include <arc/message/Service.h>
#include <arc/Logger.h>


#include "PerlProcessor.h"


namespace DREService
{

class DREWebService: public Arc::Service
{
	/**
	 *  @class JanitorWebService
	 *  @brief This class realises the janitor Web Service interface.
	 *  The basic idea behind the Web Service is to integrate the programming language Perl, in which Janitor is written.
	 *  Due to the fact, that the Perl Interpreter is not very easy to handle, the structure has to 
	 *  be more complex than one would expect. In order to have a parallel execution AND a persitant Perl Interpreter,
	 *  threads needs to be introduced. Furthermore the Perl Interpreter is not able to have multiple instances 
	 *  (unless it is compiled with a special flag, which can't be checked) which leads to the usage of fork within the threads.
	 * 
	 *  The benefits of the current architecture enables a parallel FIFO execution with multiple persitent Perl Interpreters, such
	 *  that the start-up is minimized.
	 * 
	 *  The message passing between the janitorWebService and the Perl Interpreter is done within the classes TaskQueue (incoming) and 
	 *  TaskSet (outgoing). Within these both classes the mutal exclusion and thread safty concerning the messages are realised.
	 *
	 *  The PerlProcessor itself is able to generate an arbitary amount of threads waiting for Task delievered by the TaskQueue.
	 *  Once a Thread is able to get a Task, the message will be passed to a forked child, in which the Perl Interpreter is running.
	 *  Due to the Perl Interpreter of the child is using a sperate memory stack, it cannot be interferred a Perl Interpreter of an
	 *  other thread.
	 * 
	 *  Future plans: 
	 *  Within the current project state, it is planned that only the SOAP XML message is passed to the Perl script.
	 *  But one has to keep in mind that this procedure does not support the passing of certifcate entities or other configurations.
	 *  It may happen that the SOAP XML message will be wrapped by another XML message, or a second string is passed to the Perl script.
	 */

 	public:


		/**
		 * @brief Constructor of the Janitor Web Service
		 * Extracts information out of the HED configuration file and initializes the
		 * threaded architecture.
		 */
        	DREWebService(Arc::Config *cfg);

		/**
		 * @brief Destructor of the Janitor Web Service
		 * Deconstruct the thread architecture
		 */
	        virtual ~DREWebService(void);

		/**
		 * @brief Implementation of the virtual method defined in MCCInterface (to be found in MCC.h). 
		 * Due to the logic is exported to the PerlProcessor, the TaskQueue and the TaskSet, this
		 * class just creates a task, submits it into the queue and afterwards waits for the result.
		 * @param inmsg incoming message
		 * @param inmsg outgoing message
		 * @return Status of the result achieved
		 */
		virtual Arc::MCC_Status process(Arc::Message& inmsg,Arc::Message& outmsg);




	protected:

		/**
		 *  @brief Logger class of Arc. 
		 *  Generates output into the file specified in the arched configuration file used to 
		 *  invoke arched services.
		 */
		static Arc::Logger logger;

		/**
		 * @brief The object TaskQueue realises the outgoing message queue  into which message will be submitted.
		 */
		TaskQueue::TaskQueue*           pTaskqueue;

		/**
		 * @brief The object TaskSet realises the incoming message set from which message will return (is blocking).
		 */
		TaskSet::TaskSet*               pTaskset;

		/**
		 * @brief The object PerlProcessor is processing the messages using a set of threaded Perl Interpreters.
		 */
		PerlProcessor::PerlProcessor*   pPerlProcessor;
}; 

} //namespace ArcService

#endif 
