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

std::map<std::string,Job> JobQueue::getJobsWithThisState(SchedStatus s)
{   
    std::map<std::string,Job> job_list;
    std::map<std::string,Job>::iterator iter;
    for( iter = jobs.begin(); iter != jobs.end(); iter++ ) {
        if ((iter -> second).getStatus() == s){
            job_list.insert( make_pair( (iter -> second).getID(), iter -> second ) );
        }
    }
    return job_list;
}

bool JobQueue::setJobStatus(std::string job_id, SchedStatus status)
{
    std::map<std::string,Job>::iterator iter = jobs.find(job_id);
    if (jobs.find(job_id) == jobs.end() ) 
        return false;
    else {
        jobs[job_id].setStatus(status);
        return true;
    }
}

bool JobQueue::setArexJobID(std::string job_id, std::string arex_job_id)
{
    std::map<std::string,Job>::iterator iter = jobs.find(job_id);
    if (jobs.find(job_id) == jobs.end() ) 
        return false;
    else {
        jobs[job_id].setArexJobID(arex_job_id);
        return true;
    }
}

bool JobQueue::setArexID(std::string job_id, std::string arex_id)
{
    std::map<std::string,Job>::iterator iter = jobs.find(job_id);
    if (jobs.find(job_id) == jobs.end() ) 
        return false;
    else {
        jobs[job_id].setArexID(arex_id);
        return true;
    }
}

};
