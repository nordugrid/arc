#ifndef __TASKSET_H__
#define __TASKSET_H__

#if HAVE_CONFIG_H
	#include <config.h>
#endif


#include <string>

#include <arc/Logger.h>



#include "Task.h"


namespace DREService
{


class TaskSet
{

	public:

        	TaskSet(int size);

	        virtual ~TaskSet(void);

		void flush();

		/**
		 * Checks wheter there is a task in the queue having that taskID
		 * in order to return it.
		 * If such a taskID is not available, themethod blocks until 
		 * such a taskID is available. The task will be removed from the stack in that case. 
		 */
		Task* removeTask(int);

		int addTask(Task* task);



	private:

		static Arc::Logger logger;

		enum TaskSetState     {
			INITIALIZED,			// Task exist
			FLUSH				// No task 
		};
		TaskSetState state;


		int size;

		Task**                   ppTask;

		int                      level;
		int                      start;  // FiFo
		int                      taskCounter;
		int                      semaphore;
	
		pthread_mutex_t          mutex;

		pthread_mutex_t  removed_mutex;
		pthread_cond_t   removed_condition;

		pthread_mutex_t  added_mutex;
		pthread_cond_t   added_condition;


};

}

#endif