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

// DEFINES CLEVER MACROS LIKE    "Copy" "New" etc. 
// in order to be able to compile that code, these header 
// files have to be on the bottom
#include <EXTERN.h>               /* from the Perl distribution     */
#include <perl.h>                 /* from the Perl distribution     */

namespace DREService
{


class PerlProcessor// : public Consumer
{



	public:

		/**
		* Constructor which is capable to extract prefix and suffix
		* for the echo service.
		*/
		PerlProcessor(int threadNumber, TaskQueue* pTaskQueue, TaskSet* pTaskSet) ;


		/**
		* Destructor.
		*/
		virtual ~PerlProcessor(void);


	private:



		/**
		*  Arc-intern logger. Generates output into the file specified 
		*  in the arched configuration file used to invoke arched services.
		*/
		static Arc::Logger logger;

		static void *run(void* pVoid);

		enum State{PROCESSING, EXITING};
		struct ThreadInterface
		{
			State state;
			TaskQueue    *pTaskQueue;
			TaskSet      *pTaskSet;

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