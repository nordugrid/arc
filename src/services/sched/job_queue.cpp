#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "job_queue.h"
#include "grid_sched.h"
#include <iostream>
#include <fstream>
#include <ios>

namespace GridScheduler
{

JobQueue::JobQueue(){

}

bool JobQueue::reload(std::string &db_path)
{
  db = db_path;
  char buf[1000];
  char id[100];

  std::string  fname = db + "/" + "/ids.sched";
  std::ifstream f(fname.c_str());

  if(f.is_open() ) {
  for(;!f.eof();) {
    f.getline(buf, 1000);
    if (sscanf (buf,"%s",id) != 1) continue;
    std::cout << "Job recovered:  " << id << std::endl;
    std::string job_id(id);
    Job j(job_id, db);
    j.load();
    jobs.insert( make_pair( j.getID(), j ) );
  }
  f.close();
  return true;
  }
  else
    return false;
}

JobQueue::~JobQueue(void)
{
    // nop
}

void JobQueue::addJob(Job &job)
{
    jobs.insert( make_pair( job.getID(), job ) );
    std::string  fname = db + "/" + "/ids.sched";
    std::ofstream f(fname.c_str(),std::ios_base::app);
    f << job.getID() << std::endl;
    f.close();
    job.save();
}

void JobQueue::removeJob(Job &job)
{
    std::string s = job.getID();
    removeJob(s);
}

void JobQueue::removeJob(std::string &job_id)
{
    jobs[job_id].remove();
    jobs.erase(job_id);
    std::string  fname = db + "/" + "/ids.sched";
    std::ofstream f(fname.c_str());

    std::map<std::string,Job>::iterator iter;
    for( iter = jobs.begin(); iter != jobs.end(); iter++ ) {
        f << (iter->second).getID() << std::endl;
    }
    f.close();
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

bool JobQueue::CheckJobTimeout(std::string job_id)
{
    std::map<std::string,Job>::iterator iter = jobs.find(job_id);
    if (jobs.find(job_id) == jobs.end() ) 
        return false;
    else {
        return jobs[job_id].CheckTimeout();
    }
}

bool JobQueue::setLastCheckTime(std::string job_id)
{
    std::map<std::string,Job>::iterator iter = jobs.find(job_id);
    if (jobs.find(job_id) == jobs.end() ) 
        return false;
    else {
        jobs[job_id].sched_meta.last_check_time = (unsigned)time(NULL);
    }
}

 
bool JobQueue::getJobStatus(std::string &job_id, SchedStatus &status) {
    std::map<std::string,Job>::iterator iter = jobs.find(job_id);
    if (jobs.find(job_id) == jobs.end() ) 
        return false;
    else {
        status = jobs[job_id].getStatus();
        return true;
    }

}

bool JobQueue::saveJobStatus(std::string job_id) {
    std::map<std::string,Job>::iterator iter = jobs.find(job_id);
    if (jobs.find(job_id) == jobs.end() ) 
        return false;
    else {
        return jobs[job_id].save();
    }

}

};
