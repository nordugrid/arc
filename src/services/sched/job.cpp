#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "job.h"

namespace GridScheduler
{

Job::Job(JobRequest d, JobSchedMetaData m, int t)
{
    descr = d;
    sched_meta = m;
    sched_meta.timeout = t;
    id_ = make_uuid();
}

Job::Job(const std::string& job)
{
    Arc::XMLNode tmp_xml;
    (Arc::XMLNode (job)).New(tmp_xml);
    JobRequest job_desc(tmp_xml);
    setJobRequest(job_desc);

}

Job::Job(std::istream& job)
{

    std::string xml_document;
    std::string xml_line;
    Arc::XMLNode tmp_xml;

    while (getline(job, xml_line)) xml_document += xml_line;
    
    (Arc::XMLNode (xml_document)).New(tmp_xml);

    JobRequest job_desc(tmp_xml);
    setJobRequest(job_desc);
}

Job::~Job(void)
{
    // NOP
}

void Job::setJobRequest(JobRequest &d)
{
    descr = d;
}

void Job::setJobSchedMetaData(JobSchedMetaData &m)
{
    sched_meta = m;
}

Arc::XMLNode Job::getJSDL(void)
{
    return descr.getJSDL();
}

void Job::setArexJobID(std::string id)
{
    arex_job_id = id;
}


std::string& Job::getArexJobID(void)
{
    return  arex_job_id;
}

void Job::setArexID(std::string id)
{
    sched_meta.setArexID(id);
}

std::string& Job::getArexID(void)
{
    return sched_meta.getArexID();
}

bool Job::CheckTimeout(void)
{
    unsigned i = (unsigned)time(NULL) - sched_meta.last_check_time;
    std::cout << "Checktime diff: " << i << std::endl;
    if ( i  < timeout)
        return true;
    else
        return false;
}


bool SchedStatetoString(SchedStatus s, std::string &state) {

    switch (NEW){
    case NEW:
        state = "New";
        break;
    case STARTING:
        state = "Starting";
        break;
    case RUNNING:
        state = "Running";
        break;
    case CANCELLED:
        state = "Running";
        break;
    case FAILED:
        state = "Failed";
        break;
    case FINISHED:
        state = "Finished";
        break;
    case UNKNOWN:
        state = "Unknown";
        break;
    case KILLED:
        state = "Killed";
        break;
    default:
        return false;
    }
    return true;
}


bool ArexStatetoSchedState(std::string &arex_state, SchedStatus sched_state) {

    if(arex_state == "Accepted") {
        sched_state = STARTING;
    } else if(arex_state == "Preparing") {
        sched_state = STARTING;
    } else if(arex_state == "Submiting") {
        sched_state = STARTING;
    } else if(arex_state == "Executing") {
        sched_state = RUNNING;
    } else if(arex_state == "Finishing") {
        sched_state = RUNNING;
    } else if(arex_state == "Finished") {
        sched_state = FINISHED;
    } else if(arex_state == "Deleted") {
        sched_state = FINISHED;
    } else if(arex_state == "Killing") {
        sched_state = RUNNING;
    };

}

bool Job::Cancel(void) { 
    status = KILLED;
    return true;
}

};
