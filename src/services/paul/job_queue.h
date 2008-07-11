#ifndef SCHED_JOB_QUEUE
#define SCHED_JOB_QUEUE

#include <string>
#include <map>
#include <iostream>

#include "job.h"

namespace Paul {

class JobQueue {
    private:
        std::map<std::string, Job> jobs;
        std::string db;
    public:
        JobQueue();
        virtual ~JobQueue();
//        bool reload(const std::string &db_path, SchedStatusFactory &status_factory);
        void setDB(const std::string &_db) { db = _db; };
        void addJob(Job &job);
        void removeJob(Job &job);
        void removeJob(const std::string &job_id);
        bool checkJob(const std::string &job_id);
        int size(void) { return jobs.size(); };
        // std::map<const std::string, Job *> getJobsWithState(const SchedStatus &s);
        std::map<const std::string, Job *> getAllJobs(void);
        Job &operator[](const std::string &job_id);
        unsigned int getTotalJobs(void) { return 0; };
        unsigned int getRunningJobs(void) { return 0; };
        unsigned int getWaitingJobs(void) { return 0; };
        unsigned int getStagingJobs(void) { return 0; };
        unsigned int getLocalRunningJobs(void) { return 0; };
};

} // namespace Arc

#endif // SCHED_JOB_QUEUE

