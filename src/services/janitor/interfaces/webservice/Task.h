#ifndef __DRE_TASK_H__
#define __DRE_TASK_H__

#if HAVE_CONFIG_H
	#include <config.h>
#endif

// #undef New


#include <arc/message/PayloadSOAP.h>
#include <arc/Logger.h>

#include <arc/message/Message.h>
// #include <arc/message/PayloadSOAP.h>
//#include <arc/message/Service.h>


// class PayloadSOAP;
// class XMLNode;


namespace DREService
{


class Task
{

	private:

		static Arc::Logger logger;

		int taskID;
 		Arc::Message* pRequest;
 		Arc::Message* pResponse;

	public:

		int bla;
		/**
		* Constructor which is capable to extract prefix and suffix
		* for the echo service.
		*/
		Task(int taskID, Arc::Message* request, Arc::Message* response);


		/**
		* Destructor.
		*/
		virtual ~Task(void);

		void setTaskID(int taskID);

		int getTaskID();

		Arc::Message* getRequestMessage();

		Arc::Message* getResponseMessage();

};

}








#endif
