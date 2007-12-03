/**
 */
#ifndef ARCLIB_JOB_JSDL
#define ARCLIB_JOB_JSDL

#include <string>
#include <iostream>

#include <arc/XMLNode.h>

#include "job.h"

//struct soap;
//class jsdl__JobDefinition_USCOREType;
//class jsdl__JobDescription_USCOREType;

/**
 * Class to represent the request for computational job.
 */
class JobRequestJSDL: public JobRequest {
	protected:
		bool set(std::istream& s) throw(JobRequestError);
                //bool set(jsdl__JobDescription_USCOREType* job_description_) throw(JobRequestError);
		//bool set_jsdl(Arc::XMLNode jobDescription);
                bool set(Arc::XMLNode jsdl_description_) throw(JobRequestError);
                double get_limit(Arc::XMLNode range);
                virtual bool print(std::string& s) throw(JobRequestError);

	public:
		JobRequestJSDL(const JobRequest& j) throw(JobRequestError);
		JobRequestJSDL(const char* s) throw(JobRequestError);
		JobRequestJSDL(const std::string& s) throw(JobRequestError);
		JobRequestJSDL(std::istream& i) throw(JobRequestError);
		virtual ~JobRequestJSDL(void);
	        virtual JobRequest& operator=(const JobRequest& j) throw (JobRequestError);

	private:
		Arc::XMLNode jobDefinition;

                Arc::NS jsdl_namespaces;
		Arc::XMLNode jsdl_document;

};


#endif // ARCLIB_JOB_JSDL

