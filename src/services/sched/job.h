#ifndef SCHED_JOB
#define SCHED_JOB

#include <string>
#include "job_desc.h"
#include "job_sched_meta.h"

namespace GridScheduler {

std::string make_uuid();

//enum BESStatus { PENDING, RUNNING, CANCELLED, FAILED, FINISHED };

//enum ArexStatus { ACCEPTING, ACCEPTED, PREPARING, PREPARED, SUBMITTING, EXECUTING, KILLING, EXECUTED, FINISHING, FAILED, HELD };

enum SchedStatus { NEW, STARTING, RUNNING, CANCELLED, FAILED, FINISHED, UNKNOWN, KILLED, KILLING};

bool ArexStatetoSchedState(std::string &arex_state, SchedStatus sched_state);

bool SchedStatetoString(SchedStatus s, std::string &state);

class Job {

    private:
        JobRequest descr;
        JobSchedMetaData sched_meta;
        std::string failure_;
        std::string id;
        std::string arex_job_id;
        SchedStatus status;
        int timeout;
        friend class JobQueue;
    public:
        Job(void);
        Job(JobRequest d, JobSchedMetaData m, int t);
        Job(const std::string& job);
        Job(std::istream& job);
        virtual ~Job(void);
        void setJobRequest(JobRequest &descr);
        JobRequest& getJobRequest(void) { return descr; };
        void setJobSchedMetaData(JobSchedMetaData &sched_meta);
        JobSchedMetaData& getSchedMetaData(void) { return sched_meta; };
        std::string Failure(void) { std::string r=failure_; failure_=""; return r; };
        void setID(std::string& id_) { id = id_;};
        std::string getID(void) { return id;};
        void setStatus(SchedStatus s) {  status=s;};
        SchedStatus getStatus(void) { return status;};
        operator bool(void) { return (id.empty() ? false : true ); };
        bool operator!(void) { return (id.empty() ? true : false ); };
        Arc::XMLNode getJSDL(void);
        void setArexJobID(std::string id);
        void setArexID(std::string id);
        std::string getArexJobID(void);
        std::string getArexID(void);
        std::string& getURL(void){ return sched_meta.getArexID();};
        bool CheckTimeout(void);
        bool Cancel(void);
        bool save(void);
        bool load(void);
        bool remove(void);
};

}; // namespace Arc

#endif // SCHED_JOB


