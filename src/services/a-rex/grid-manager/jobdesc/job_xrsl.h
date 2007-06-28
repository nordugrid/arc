/**
 */
#ifndef ARCLIB_JOB_XRSL
#define ARCLIB_JOB_XRSL

#include <string>

//@ #include "xrsl.h"
#include "job.h"

/**
 * Class to represent the request for computational job.
 */
class JobRequestXRSL: public JobRequest {
    protected:
        //@ bool set(const char* s) throw(JobRequestError);
        //@ bool set(Xrsl& xrsl) throw(JobRequestError);
        //@ bool set_xrsl(Xrsl& xrsl) throw (JobRequestError);
        virtual bool print(std::string& s) throw(JobRequestError);

    public:
		typedef enum {
			UserFriendly,
			NoUnits,
		} Type;
        JobRequestXRSL(const JobRequest& j,Type type = UserFriendly) throw(JobRequestError);
        JobRequestXRSL(const char* s,Type type = UserFriendly) throw(JobRequestError);
        JobRequestXRSL(const std::string& s,Type type = UserFriendly) throw(JobRequestError);
        JobRequestXRSL(std::istream& i,Type type = UserFriendly) throw(JobRequestError);
        virtual ~JobRequestXRSL(void);
        virtual JobRequest& operator=(const JobRequest& j) throw (JobRequestError);

    private:
        //@ Xrsl* xrsl_;
        //@ Type type_;

};


#endif // ARCLIB_JOB_XRSL

