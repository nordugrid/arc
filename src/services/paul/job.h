#ifndef SCHED_JOB
#define SCHED_JOB

#include <string>
#include "job_request.h"
#include "job_sched_meta.h"
#include "job_status.h"

namespace Paul {


class Job {

    private:
        JobRequest request;
        JobSchedMetaData sched_meta;
        std::string failure;
        std::string id;
        std::string db;
        SchedStatusLevel status;
        int timeout;
        int check;
        bool finished_reported;
    public:
        Job(void);
        Job(const Job &j);
        Job(JobRequest &r);
        Job(JobRequest &r, JobSchedMetaData &m, int t, const std::string &db_path);
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
        void setStatus(SchedStatusLevel s) { status = s; };
        SchedStatusLevel getStatus(void) { return status; };
        operator bool(void) { return (id.empty() ? false : true ); };
        bool operator!(void) { return (id.empty() ? true : false ); };
        Arc::XMLNode &getJSDL(void) { return request.getJSDL(); };
        void setResourceJobID(const std::string &id) { sched_meta.setResourceJobID(id); };
        const std::string &getResourceJobID(void) { return sched_meta.getResourceJobID(); };
        void setResourceID(const std::string &id) { sched_meta.setResourceID(id); };
        const std::string &getResourceID(void) { return sched_meta.getResourceID(); };
        bool CheckTimeout(void);
        bool Cancel(void);
        bool save(void);
        bool load(void);
        bool remove(void);
        void finishedReported(void) { finished_reported = true; };
        bool isFinishedReported(void) { return finished_reported; };
        void clean(const std::string &jobroot);
};

} // namespace Arc

#endif // SCHED_JOB
