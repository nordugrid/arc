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

};
