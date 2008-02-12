#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "job_queue.h"
#include "grid_sched.h"

namespace GridScheduler
{

JobQueue::JobQueue(void)
{
    // nop
}

JobQueue::~JobQueue(void)
{
    // nop
}

void JobQueue::addJob(Job &job)
{
    jobs.insert( make_pair( job.getID(), job ) );
}

void JobQueue::removeJob(Job &job)
{
    jobs.erase(job.getID());

}

void JobQueue::removeJob(std::string &job_id)
{
    jobs.erase(job_id);
}

bool JobQueue::CheckJobID(std::string &job_id)
{
    std::cout << "jobs.size() is " << (int) jobs.size() << std::endl;
    if (jobs.find(job_id) == jobs.end() ) {
        std::cout << std::endl << "This job is NOT IN the queue: " <<job_id.c_str() << std::endl;
        return false;
    }
    else {
        std::cout << std::endl << "This job is IN the queue: " <<job_id.c_str() << std::endl;
        return true;
    }
}

Job JobQueue::getJob(std::string &job_id)
{
    if (CheckJobID(job_id))
        return jobs[job_id];
    //TODO: else part
}

};
