/**
 */
#ifndef ARCLIB_JOB_JSDL
#define ARCLIB_JOB_JSDL

#include <string>
#include <iostream>

#include "job.h"

struct soap;
class jsdl__JobDefinition_USCOREType;
class jsdl__JobDescription_USCOREType;

/**
 * Class to represent the request for computational job.
 */
class JobRequestJSDL: public JobRequest {
	protected:
		bool set(std::istream& s) throw(JobRequestError);
        bool set(jsdl__JobDescription_USCOREType* job_description_) throw(JobRequestError);
		bool set_jsdl(jsdl__JobDescription_USCOREType* job_description_,struct soap* sp_);

        virtual bool print(std::string& s) throw(JobRequestError);

	public:
		JobRequestJSDL(const JobRequest& j) throw(JobRequestError);
		JobRequestJSDL(const char* s) throw(JobRequestError);
		JobRequestJSDL(const std::string& s) throw(JobRequestError);
		JobRequestJSDL(std::istream& i) throw(JobRequestError);
		virtual ~JobRequestJSDL(void);
        virtual JobRequest& operator=(const JobRequest& j) throw (JobRequestError);

	private:
		struct soap* sp_;
		jsdl__JobDefinition_USCOREType* job_;

};


#endif // ARCLIB_JOB_JSDL

