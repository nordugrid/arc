#ifndef SCHED_JOB_QUEUE
#define SCHED_JOB_QUEUE

#include <string>
#include <map>
#include <iostream>

#include "job.h"

namespace Arc
{

class JobQueue {
    private:
        std::map<std::string,Arc::Job> jobs;
        std::string type;
    public:
        JobQueue();
        virtual ~JobQueue();
        void addJob(const Job &job);
        void removeJob(const Job &job);
        void removeJob(const std::string &job_id);
};

}; // namespace Arc

#endif // SCHED_JOB_QUEUE

