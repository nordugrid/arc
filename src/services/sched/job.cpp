#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "job.h"

namespace Arc
{

Job::Job(const JobDescription &d, const JobSchedMetaData &m)
{
    descr = d;
    sched_meta = m;
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
