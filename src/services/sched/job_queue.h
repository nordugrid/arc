/**
 */
#ifndef SCHED_JOB_QUEUE
#define SCHED_JOB_QUEUE

#include <string>
#include <list>
#include <iostream>

#include "job_desc.h"


class JobQueue {
    private:
        std::list<Job> jobs;

    public:
        JobQueue();
        virtual JobQueue& operator=(const JobQueue& j);
        virtual ~JobQueue();

};

#endif // SCHED_JOB_QUEUE

