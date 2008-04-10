#ifndef SCHED_JOB_QUEUE
#define SCHED_JOB_QUEUE

#include <string>
#include <map>
#include <iostream>

#include "job.h"

namespace GridScheduler {

class JobQueue {
    private:
        std::map<std::string, Job> jobs;
        std::string db;
    public:
        JobQueue();
        virtual ~JobQueue();
        bool reload(const std::string &db_path);
        void addJob(Job &job);
        void removeJob(Job &job);
        void removeJob(const std::string &job_id);
        bool checkJob(const std::string &job_id);
        int size(void) { return jobs.size(); };
        std::map<const std::string, Job *> getJobsWithState(SchedStatusLevel s);
        std::map<const std::string, Job *> getAllJobs(void);
        Job &operator[](const std::string &job_id);
        // Job &getJob(const std::string &job_id);
        // bool setJobStatus(std::string job_id, SchedStatus status);
        // bool getJobStatus(std::string &job_id, SchedStatus &status);
        // bool setArexJobID(std::string job_id, std::string arex_job_id);
        // bool setArexID(std::string job_id, std::string arex_job_id);
        // std::map<std::string,Job> getJobs(void) {return jobs;};
        // bool CheckJobTimeout(std::string job_id);
        // bool setLastCheckTime(std::string job_id);
        // bool saveJobStatus(std::string job_id);
        
};

} // namespace Arc

#endif // SCHED_JOB_QUEUE

