#ifndef SCHED_JOB_QUEUE
#define SCHED_JOB_QUEUE

#include <string>
#include <map>
#include <iostream>

#include "job.h"

namespace GridScheduler
{

class JobQueue {
    private:
        std::map<std::string,Job> jobs;
        std::string type;
    public:
        JobQueue();
        virtual ~JobQueue();
        void addJob(Job &job);
        void removeJob(Job &job);
        void removeJob(std::string &job_id);
        Job getJob(std::string &job_id);
        bool CheckJobID(std::string &job_id);
};

}; // namespace Arc

#endif // SCHED_JOB_QUEUE

