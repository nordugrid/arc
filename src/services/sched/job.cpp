#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <sys/types.h>
#include <unistd.h>

#include "job.h"
#include <arc/StringConv.h>

namespace GridScheduler
{

Job::Job(JobRequest &d, JobSchedMetaData &m)
{
    descr = d;
    sched_meta = m;
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

void Job::setJobRequest(const JobRequest &d)
{
    
    descr = d;

}

void Job::setJobSchedMetaData(const JobSchedMetaData &m)
{
    sched_meta = m;
}

};
