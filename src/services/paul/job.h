#ifndef __ARC_PAUL_JOB_H__
#define __ARC_PAUL_JOB_H__

#include <string>
#include "job_status.h"
#include "job_request.h"

namespace Paul {

class Job
{
    private:
        JobRequest descr;
        JobStatus status;
        std::string failure_;
        std::string id;
        std::string lrms_id;
    public:
        Job(void) {} ;
        Job(JobRequest &r) { descr=r; } ;
        std::string getID(void) { return id; };
        std::string getName(void) { return descr.getJobName(); };
        void setStatus(JobStatus &s) { status = s; };
        JobStatus getStatus(void) { return status; };
        bool Cancel(void);


}; // class Job

}; // namespace Paul
#endif
