#ifndef SCHED_JOB
#define SCHED_JOB

#include <string>
#include "job_desc.h"
#include "job_sched_meta.h"

namespace GridScheduler {

std::string make_uuid();

//enum BESStatus { PENDING, RUNNING, CANCELLED, FAILED, FINISHED };

//enum ArexStatus { ACCEPTING, ACCEPTED, PREPARING, PREPARED, SUBMITTING, EXECUTING, KILLING, EXECUTED, FINISHING, FAILED, HELD };

enum SchedStatus { NEW, STARTING, RUNNING, CANCELLED, FAILED, FINISHED, UNKNOWN, KILLED};


class Job {

    private:
        JobRequest descr;
        JobSchedMetaData sched_meta;
        std::string failure_;
        std::string id_;
        std::string arex_job_id;
        SchedStatus status;
    public:
        Job(void) { };
        Job(JobRequest d, JobSchedMetaData m);
        Job(const std::string& job);
        Job(std::istream& job);
        virtual ~Job(void);
        void setJobRequest(JobRequest &descr);
        JobRequest& getJobRequest(void) { return descr; };
        void setJobSchedMetaData(JobSchedMetaData &sched_meta);
        JobSchedMetaData& getSchedMetaData(void) { return sched_meta; };
        std::string Failure(void) { std::string r=failure_; failure_=""; return r; };
        //void setID(std::string& id) { id_ = id;};
        std::string getID(void) { return id_;};
        void setStatus(SchedStatus s) {  status=s;};
        SchedStatus getStatus(void) { return status;};
        operator bool(void) { return (id_.empty() ? false : true ); };
        bool operator!(void) { return (id_.empty() ? true : false ); };
        Arc::XMLNode getJSDL(void);
        void setArexJobID(std::string id);
        void setArexID(std::string id);
        std::string& getArexJobID(void);
        std::string& getArexID(void);
        void setArexID(void);
        std::string& getURL(void){ return sched_meta.getArexID();};
};

}; // namespace Arc

#endif // SCHED_JOB




