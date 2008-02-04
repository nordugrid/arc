#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "job.h"
#include <arc/StringConv.h>

namespace GridScheduler
{



std::string make_job_id(void) {
    std::string id_=Arc::tostring((unsigned int)getpid())+ Arc::tostring((unsigned int)time(NULL)) + Arc::tostring(rand(),1);
    return id_;
}


Job::Job(JobDescription &d, JobSchedMetaData &m)
{
    descr = d;
    sched_meta = m;
    id_ = make_job_id();

}

Job::Job(const std::string& job)
{
    Arc::XMLNode tmp_xml;
    (Arc::XMLNode (job)).New(tmp_xml);
    JobDescription job_desc(tmp_xml);
    setJobDescription(job_desc);

}

Job::Job(std::istream& job)
{

    std::string xml_document;
    std::string xml_line;
    Arc::XMLNode tmp_xml;

    while (getline(job, xml_line)) xml_document += xml_line;
    
    (Arc::XMLNode (xml_document)).New(tmp_xml);

    JobDescription job_desc(tmp_xml);
    setJobDescription(job_desc);
    
}

Job::~Job(void)
{
    // NOP
}

void Job::setJobDescription(const JobDescription &d)
{
    
    descr = d;

}

void Job::setJobSchedMetaData(const JobSchedMetaData &m)
{
    sched_meta = m;
}

};
