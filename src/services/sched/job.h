#ifndef SCHED_JOB
#define SCHED_JOB

#include <string>
#include "job_desc.h"
#include "job_sched_meta.h"

namespace GridScheduler {

std::string make_uuid();

enum BESStatus { PENDING, RUNNING, CANCELLED, FAILED, FINISHED };

//static enum arex_status { ACCEPTING, ACCEPTED, PREPARING, PREPARED, SUBMITTING, EXECUTING, KILLING, EXECUTED, FINISHING, FAILED, HELD }

class Job {

    private:
        JobRequest descr;
        JobSchedMetaData sched_meta;
        std::string failure_;
        std::string id_;
        int status;
    public:
        Job(void) { };
        Job(JobRequest& d, JobSchedMetaData& m);
        Job(const std::string& job);
        Job(std::istream& job);
        virtual ~Job(void);
        void setJobRequest(const JobRequest &descr);
        JobRequest& getJobRequest(void) { return descr; };
        void setJobSchedMetaData(const JobSchedMetaData &sched_meta);
        JobSchedMetaData& getSchedMetaData(void) { return sched_meta; };
        std::string Failure(void) { std::string r=failure_; failure_=""; return r; };
        //void setID(std::string& id) { id_ = id;};
        std::string getID(void) { return id_;};
        void setStatus(int s) {  status=s;};
        int getStatus(void) { return status;};
};

}; // namespace Arc

#endif // SCHED_JOB




