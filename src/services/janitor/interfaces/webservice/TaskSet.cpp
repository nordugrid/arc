

#include "TaskSet.h"





namespace DREService
{
//TODO: MAYBE BROADCAST
	Arc::Logger               TaskSet::logger(Arc::Logger::rootLogger, "TaskSet");

	TaskSet::TaskSet(int size){
		this->size = size;
		
		ppTask  = new Task*[size];
		for (int i=0; i<size; i++){
			ppTask[i] = 0;
		}

		semaphore   = 0;

		pthread_mutex_init(&mutex, NULL);

		pthread_mutex_init(&removed_mutex, NULL);
		pthread_cond_init (&removed_condition, NULL);

		pthread_mutex_init(&added_mutex, NULL);
		pthread_cond_init (&added_condition, NULL);

		state = TaskSet::INITIALIZED;
	}



	TaskSet::~TaskSet(){

		// wait until semaphore is set to zero
		do{
			pthread_mutex_lock(&mutex);
			if(semaphore == 0){
				break;
			}
			logger.msg(Arc::VERBOSE, "TaskSet is waiting for objects (%d) still using the set.",semaphore);
			pthread_mutex_unlock(&mutex);
			
			sleep(1);
		}while(1);

		delete [] ppTask;

		pthread_mutex_destroy(&mutex);
		pthread_mutex_destroy(&added_mutex);
		pthread_mutex_destroy(&removed_mutex);

	}


	void TaskSet::flush(){
		pthread_mutex_lock(&mutex);

			state = TaskSet::FLUSH;
			// Flush signals
			pthread_cond_broadcast(&removed_condition);
			pthread_cond_broadcast(&added_condition);

		pthread_mutex_unlock(&mutex);
	}


	// No new fresh taskID will be created. Instead the set will wait, until number is unique within the set
	int TaskSet::addTask(Task::Task* pTask){


		pthread_mutex_lock(&mutex);
		semaphore++;
		pthread_mutex_unlock(&mutex);


		int found = 0;
		do{
			pthread_mutex_lock(&mutex);
			if(state == TaskSet::FLUSH){
				pthread_mutex_unlock(&mutex);
				break;
			}

			// check if task would be unique
			int maxvalue = 1;
			for (int i=0; i<size; i++){
				if(ppTask[i] != 0 && ppTask[i]->getTaskID() == pTask->getTaskID()){ 
					found = 1;
					break;
				}
			}
			if(!found){
				for (int i=0; i<size; i++){
					if(ppTask[i] == 0){  // bingo
						ppTask[i] = pTask;
						logger.msg(Arc::VERBOSE, "Added Task %d to the set. ",pTask->getTaskID());
						found = 1;
						break;
					}
				}
			}else{
				found = 0;
			}

	
			pthread_mutex_unlock(&mutex);

			if(found){
				break;
			}
			pthread_mutex_lock(&removed_mutex);
				pthread_cond_wait(&removed_condition, &removed_mutex);
			pthread_mutex_unlock(&removed_mutex);

		}while(!found && state != TaskSet::FLUSH);

		pthread_mutex_lock(&mutex);
		semaphore--;
		pthread_mutex_unlock(&mutex);

		/** Signal, that space is available*/
		pthread_cond_signal(&added_condition);
		return pTask->getTaskID();
	}



	Task::Task* TaskSet::removeTask(int taskID){

		Task* pTask = 0;  // we will return that one

		pthread_mutex_lock(&mutex);
		semaphore++;
		pthread_mutex_unlock(&mutex);

		int found = 0;
		do{  

			pthread_mutex_lock(&mutex);
			if(state == TaskSet::FLUSH){
				pthread_mutex_unlock(&mutex);
				break;
			}
			for (int i=0; i<size; i++){
				if(ppTask[i] != 0 && ppTask[i]->getTaskID() == taskID){  // bingo
					pTask = ppTask[i];
					ppTask[i] = 0;
					found = 1;
					logger.msg(Arc::VERBOSE, "Removed Task %d out of to the set. ",pTask->getTaskID());
					break;
				}
			}
			pthread_mutex_unlock(&mutex);

			if(found){
				break;
			}
			pthread_mutex_lock(&added_mutex);
				pthread_cond_wait(&added_condition, &added_mutex);
			pthread_mutex_unlock(&added_mutex);

			
		}while(!found && state != TaskSet::FLUSH);

		pthread_mutex_lock(&mutex);
		semaphore--;
		pthread_mutex_unlock(&mutex);

		/** Signal, that space is available*/
		pthread_cond_signal(&removed_condition);


		return pTask;
	}

}
