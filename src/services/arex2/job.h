#ifndef __AREX2_JOB_H__
#define __AREX2_JOB_H__

#include <string>
#include "job_state.h"
#include "job_data_cache.h"
#include "job_control.h"
#include "job_descr.h"
#include "lrms.h"

/** Collect all information (status, lrms info, user) required to handle job */
class Job
{
    private:
        JobDescription descr_;
        JobUser user_;
        JobState state_;
        JobLRMSInfo lrms_info_;
        JobControl ctrl;
        std::string id_;
        // Create job ID
        bool make_job_id(void);
        // Delete job ID
        bool delete_job_id(void);
    
    public:
        /** Constructor: Creates empty job */
        Job(void);
        /** Constructor: load job information form files */
        Job(std::string path);
        /** Destuctior */
        ~Job(void);
        /** Helper logical operators */
        operator bool(void) { return !id_.empty() };
        bool operator!(void) { return id_empty() };
        /** Starts job. Most of the cases it means to submit to LRMS */
        bool Start(void);
        /** Cancel processing/execution of job */
        bool Cancel(void);
        /** Resume execution of job after error */
        bool Resume(void);
        /** Returns the string represnetation of job state */
        std::string GetState(void);
        /** Returns the session directory of the job */
        std::string GetSessionDir(void);
};

}; // namespace ARex2

#endif // __AREX2_JOB_H__
