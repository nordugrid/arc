#ifndef SCHED_JOB
#define SCHED_JOB

#include <string>
#include "job_request.h"
#include "job_sched_meta.h"
#include "job_status.h"

namespace GridScheduler {


class Job {

    private:
        JobRequest request;
        JobSchedMetaData sched_meta;
        std::string failure;
        std::string id;
        std::string db;
        SchedStatus status;
        int timeout;
        int check;
    public:
        Job(void);
        Job(JobRequest &d, JobSchedMetaData &m, int t, const std::string &db_path);
        Job(const std::string& job, const std::string &db_path);
        Job(std::istream &job, const std::string &db_path);
        virtual ~Job(void);
        void setJobRequest(JobRequest &r) { request = r; };
        JobRequest &getJobRequest(void) { return request; };
        void setJobSchedMetaData(JobSchedMetaData &sched_meta);
        JobSchedMetaData &getSchedMetaData(void) { return sched_meta; };
        const std::string getFailure(void);
        void setID(const std::string& id_) { id = id_; };
        const std::string &getID(void) { return id; };
        void setStatus(const SchedStatus &s) { status = s; };
        const SchedStatus &getStatus(void) { return status; };
        operator bool(void) { return (id.empty() ? false : true ); };
        bool operator!(void) { return (id.empty() ? true : false ); };
        Arc::XMLNode &getJSDL(void) { return request.getJSDL(); };
        void setResourceJobID(const std::string &id) { sched_meta.setResourceJobID(id); };
        const std::string &getResourceJobID(void) { return sched_meta.getResourceJobID(); };
        void setResourceID(const std::string &id) { sched_meta.setResourceID(id); };
        const std::string &getResourceID(void) { return sched_meta.getResourceID(); };
        bool CheckTimeout(void);
        bool Cancel(const SchedStatus &killed_state);
        bool save(void);
        bool load(SchedStatusFactory &status_factory);
        bool remove(void);
};

} // namespace Arc

#endif // SCHED_JOB
