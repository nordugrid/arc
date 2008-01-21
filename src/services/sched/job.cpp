#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "job.h"

namespace Arc
{

Job::Job(const JobDescription &descr, const JobSchedMetaData &sched_meta)
{
}

Job::Job(const std::string& job)
{

}

Job::Job(std::istream& job)
{

}

Job::~Job(void)
{
    // NOP
}

void Job::setJobDescription(const JobDescription &descr)
{
}

void Job::setJobSchedMetaData(const JobSchedMetaData &sched_meta)
{
}

};
