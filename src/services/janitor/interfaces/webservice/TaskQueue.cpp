
#include "TaskQueue.h"

#define max(a, b) ( (a)>(b) ? (a) : (b) )

namespace DREService
{

	Arc::Logger  TaskQueue::logger(Arc::Logger::rootLogger, "TaskQueue");

	TaskQueue::TaskQueue(int length){
		this->length = length;

		level       = 0;
		start       = 0;
 		taskCounter = 0;
		semaphore   = 0;
		ppTask      = new Task*[length];

		pthread_mutex_init(&mutex, NULL);

		pthread_mutex_init(&removed_mutex, NULL);
		pthread_cond_init (&removed_condition, NULL);

		pthread_mutex_init(&added_mutex, NULL);
		pthread_cond_init (&added_condition, NULL);
	
		state = TaskQueue::INITIALIZED;
	}



	TaskQueue::~TaskQueue(void){


		// wait until semaphore is set to zero
		do{
			pthread_mutex_lock(&mutex);
			if(semaphore == 0){
				break;
			}
			pthread_mutex_unlock(&mutex);
			logger.msg(Arc::VERBOSE, "TaskSet is waiting for objects still using the set.");
			sleep(1);
		}while(1);

		delete [] ppTask;

		pthread_mutex_destroy(&mutex);
		pthread_mutex_destroy(&added_mutex);
		pthread_mutex_destroy(&removed_mutex);

	}

	void TaskQueue::flush(){

		pthread_mutex_lock(&mutex);

			state = TaskQueue::FLUSH;
			// Flush signals
			pthread_cond_broadcast(&removed_condition);
			pthread_cond_broadcast(&added_condition);

		pthread_mutex_unlock(&mutex);

	}


	int TaskQueue::pushTask(Task::Task* pTask){

		pthread_mutex_lock(&mutex);
		semaphore++;
		pthread_mutex_unlock(&mutex);
	
		int found = 0;
		do{
			/** Critical region */
			pthread_mutex_lock(&mutex);

				if(state == TaskQueue::FLUSH){
					pthread_mutex_unlock(&mutex);
					break;
				}	


				if(level < length){
					int taskID = pTask->getTaskID();


					if(taskID < 0){
						int isUnique = 0;
						do{
							isUnique = 1;
							taskCounter++;
							taskID = taskCounter;
							for (int i=0; i < level; i++){
								int taskAdr = (start + i) % length;
								if(ppTask[taskAdr]->getTaskID() == taskID){
									isUnique = 0;
								}
							}
						}while(!isUnique);
						pTask->setTaskID(taskID);
					}
	
		
					int taskAddress = (start + level)%length;
	
					ppTask[taskAddress]   = pTask;
					level = level+1;
					found = 1;
					logger.msg(Arc::VERBOSE, "Pushed Task %d into the queue. ",taskID);
// 					logger.msg(Arc::VERBOSE, "Buffer statistic: Level: %d   Start: %d",level,start);
				}
			pthread_mutex_unlock(&mutex);
			/** */

			if(found){
				break;
			}


			pthread_mutex_lock(&removed_mutex);
				pthread_cond_wait(&removed_condition, &removed_mutex);
			pthread_mutex_unlock(&removed_mutex);


		}while(!found && state != TaskQueue::FLUSH);

		pthread_mutex_lock(&mutex);
		semaphore--;
		pthread_mutex_unlock(&mutex);

		pthread_cond_signal(&added_condition);



		return pTask->getTaskID();

	}


	Task::Task* TaskQueue::shiftTask(){

		pthread_mutex_lock(&mutex);
		semaphore++;
		pthread_mutex_unlock(&mutex);

		Task* pTask = 0;  // we will return that one
		int found = 0;
		do{  

			pthread_mutex_lock(&mutex);
			if(state == TaskQueue::FLUSH){
				pthread_mutex_unlock(&mutex);
				break;
			}
			if(level > 0){
				pTask = ppTask[start];
				start = (start + 1) % length;
				level--;
				found = 1;
				logger.msg(Arc::VERBOSE, "Shifted Task %d out of to the queue. ",pTask->getTaskID());
			}
			pthread_mutex_unlock(&mutex);

			if(found){
				break;
			}

			pthread_mutex_lock(&added_mutex);
				pthread_cond_wait(&added_condition, &added_mutex);
			pthread_mutex_unlock(&added_mutex);

			
		}while(!found && state != TaskQueue::FLUSH);

		/** Signal, that space is available*/
		pthread_cond_signal(&removed_condition);

		pthread_mutex_lock(&mutex);
		semaphore--;
		pthread_mutex_unlock(&mutex);

		return pTask;
	}



}


