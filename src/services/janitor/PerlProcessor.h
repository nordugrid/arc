/**
 * @file PerlProcessor.h
 * @brief Header file for the Perl Processor, which implements a procedure to process the incoming tasks.
 *
 * @author Michael Glodek
 */
#ifndef __PERLPROCESSOR_H__
#define __PERLPROCESSOR_H__

#if HAVE_CONFIG_H
	#include <config.h>
#endif

#include <string>
#include <time.h>

#include <arc/Logger.h>

#include "TaskQueue.h"
#include "TaskSet.h"



// UAHHH!!! THIS HEADER FILE DEFINES CLEVER MACROS LIKE    "Copy" "New" etc. 
// In order being able to compile my code, I had to put these header here
#include <EXTERN.h>               /* from the Perl distribution     */
#include <perl.h>                 /* from the Perl distribution     */

namespace DREService
{


class PerlProcessor
{
	/**
	 *  @class PerlProcessor
	 *  @brief This class realises the processing of incoming messages using a set of perl interpreters.
         * 
	 */


	public:

		/**
		 * @brief Constructor of the PerlProcessor
		 */
		PerlProcessor(int threadNumber, TaskQueue* pTaskQueue, TaskSet* pTaskSet, const std::string perlfile) ;


		/**
		* Destructor.
		*/
		virtual ~PerlProcessor(void);


	private:


		static Arc::Logger logger;

		static void *run(void* pVoid);

		

		enum State{PROCESSING, EXITING};
		struct ThreadInterface
		{
			State state;
			TaskQueue         *pTaskQueue;
			TaskSet           *pTaskSet;
			char              *pPerlfile;

			pthread_mutex_t    naming_mutex;  // just for thread naming purpose...
			int                naming_counter;
		};
		ThreadInterface* pThreadinterface;


		int threadNumber;
		pthread_t* pThreads;

		static int perl_call_va(PerlInterpreter* my_perl, char *subname, ...);



};

}

#endif