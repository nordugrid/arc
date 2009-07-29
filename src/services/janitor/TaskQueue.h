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

        	TaskQueue(int length);

	        virtual ~TaskQueue(void);

		void flush();

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

