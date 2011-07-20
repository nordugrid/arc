#include "Task.h"

namespace DREService
{

// 	/*
// 	 * If taskID < 0, it is considered as unset
// 	 */
// 	Task::Task(int taskID, Arc::Message& request, Arc::Message& response ){
// 
// 	}


	Arc::Logger  Task::logger(Arc::Logger::rootLogger, "Task");


	Task::Task(int taskID, Arc::Message* pRequest, Arc::Message* pResponse): bla(0) {
		this->taskID    = taskID;
		this->pRequest  = pRequest;
		this->pResponse = pResponse;
	}

	Task::~Task(){


	}

	void Task::setTaskID(int taskID){
		this->taskID = taskID;
	}

	int Task::getTaskID(){
		return taskID;
	}


	Arc::Message* Task::getRequestMessage(){
		return pRequest;
	}

	Arc::Message* Task::getResponseMessage(){
		return pResponse;
	}

}
 









