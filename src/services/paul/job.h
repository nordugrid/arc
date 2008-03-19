#ifndef __ARC_PAUL_JOB_H__
#define __ARC_PAUL_JOB_H__

#include <string>
#include "job_status.h"
#include "job_request.h"

namespace Paul {

class Job
{
    private:
        JobRequest request;
        JobStatus status;
        std::string failure_;
        std::string id;
        std::string lrms_id;
    public:
        Job(void) { };
        Job(JobRequest &r) { request = r; };
        void setID(const std::string &id_) { id = id_; };
        std::string getID(void) { return id; };
        std::string getName(void) { return request.getName(); };
        void setStatus(const JobStatus &s) { status = s; };
        JobStatus getStatus(void) { return status; };
        bool Cancel(void);

}; // class Job

} // namespace Paul

#endif
