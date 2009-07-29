#ifndef __TASKQUEUE_H__
#define __TASKQUEUE_H__

#if HAVE_CONFIG_H
	#include <config.h>
#endif

#include <string>


#include <arc/Logger.h>

#include "Task.h"


namespace DREService
{


class TaskQueue
{

	public:

		static const int TIME_TO_SLEEP_IF_QUEUE_IS_FULL = 1;





		/**
		* Constructor which is capable to extract prefix and suffix
		* for the echo service.
		*/
        	TaskQueue(int length);

		/**
		* Destructor.
		*/
	        virtual ~TaskQueue(void);

		void flush();

		/**
		 * Blocks, if taskqueue is full. If task is stored in the queue and had a taskID == -1 it gets a fresh taskID.
		 */
		int pushTask(Task* task);

		/**
		 * Shifts the first task from the queue (and removes it).
		 */
		Task* shiftTask();

	private:

		static Arc::Logger logger;

		enum TaskQueueState     {
			INITIALIZED,			// Task exist
			FLUSH				// No task 
		};
		TaskQueueState state;

		int length;

		Task**                   ppTask;

		int                      taskCounter;
		int                      level;
		int                      start;  // FiFo
		int                      semaphore;

		pthread_mutex_t          mutex;

		pthread_mutex_t  removed_mutex;
		pthread_cond_t   removed_condition;

		pthread_mutex_t  added_mutex;
		pthread_cond_t   added_condition;




};

}

#endif