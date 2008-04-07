#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <iostream>
#include <fstream>
#include <ios>
#include "job_queue.h"

namespace Paul
{

JobQueue::JobQueue()
{
    // NOP
}

#if 0
bool JobQueue::reload(const std::string &db_path) 
{
    // XXX make it more c++ 
    db = db_path;
    char buf[1000];
    char id[100];

    std::string  fname = db + "/" + "/ids.sched";
    std::ifstream f(fname.c_str());

    if (!f.is_open()) {
        return false;
    }
    for (;!f.eof();) {
        f.getline(buf, 1000);
        if (sscanf (buf, "%s", id) != 1) {
            continue;
        }
        std::cout << "Job recovered:  " << id << std::endl;
        std::string job_id(id);
        Job j(job_id, db);
        j.load();
        jobs.insert (make_pair(j.getID(), j));
    }
    f.close();
    return true;
}
#endif

JobQueue::~JobQueue(void) 
{
    // NOP
}

void JobQueue::addJob(Job &job) 
{
    // Insert job to the map
    jobs.insert(make_pair(job.getID(), job));
#if 0
    // create id file
    std::string fname = db + "/" + "/ids.sched";
    std::ofstream f(fname.c_str(), std::ios_base::app);
    f << job.getID() << std::endl;
    f.close();
#endif
    // save job
    // job.save();
}

void JobQueue::removeJob(Job &job) 
{
    std::string s = job.getID();
    removeJob(s);
}

void JobQueue::removeJob(const std::string &job_id) 
{
    // jobs[job_id].remove();
    jobs.erase(job_id);
#if 0
    std::string fname = db + "/" + "/ids.sched";
    std::ofstream f(fname.c_str());

    std::map<std::string,Job>::iterator iter;
    for (iter = jobs.begin(); iter != jobs.end(); iter++) {
        f << (iter->second).getID() << std::endl;
    }
    f.close();
#endif
}

bool JobQueue::checkJob(const std::string &job_id) 
{
    std::cout << "jobs.size() is " << (int) jobs.size() << std::endl;
    if (jobs.find(job_id) == jobs.end()) {
        std::cout << std::endl << "This job is NOT IN the queue: " <<job_id.c_str() << std::endl;
        return false;
    } else {
        std::cout << std::endl << "This job is IN the queue: " <<job_id.c_str() << std::endl;
        return true;
    }
}

#if 0
std::map<const std::string, Job *> JobQueue::getJobsWithState(const SchedStatus &s) 
{   
    std::map<const std::string, Job *> job_list;
    std::map<std::string, Job>::iterator iter;
    for (iter = jobs.begin(); iter != jobs.end(); iter++ ) {
        const std::string &key = iter->first;
        Job *j = &(iter->second);
        if (s == j->getStatus()) {
            job_list[key] = j;
        }
    }
    return job_list;
}
#endif

std::map<const std::string, Job *> JobQueue::getAllJobs(void)
{
    std::map<const std::string, Job *> job_list;
    std::map<std::string, Job>::iterator iter;
    for (iter = jobs.begin(); iter != jobs.end(); iter++ ) {
        const std::string &key = iter->first;
        Job *j = &(iter->second);
        job_list[key] = j;
    }
    return job_list;
}

Job &JobQueue::operator[](const std::string &job_id)
{
    // lets the runtime environment to make excpetion 
    // if the job_id does not exsists
    return jobs[job_id];
}

}
