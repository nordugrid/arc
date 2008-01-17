#ifndef SCHED_JOB
#define SCHED_JOB

#include <string>
#include "job_desc.h"
#include "job_sched_meta.h"

class Job {

    private:
        JobDescription descr;
        JobSchedMetaData sched_meta;
    public:
        Job(const JobDescription& d, const JobSchedMeta& c);
        virtual Job& operator=(const Job& j);
        virtual ~Job(void);
        JobDescription& getJobDescription(void) { return descr; };
        JobSchedMetaData& getSchedMetaData(void) { return sched_meta; };
};

#endif // SCHED_JOB
