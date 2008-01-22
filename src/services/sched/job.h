#ifndef SCHED_JOB
#define SCHED_JOB

#include <string>
#include "job_desc.h"
#include "job_sched_meta.h"

namespace Arc {

class Job {

    private:
        JobDescription descr;
        JobSchedMetaData sched_meta;
    public:
        Job(void) { };
        Job(const JobDescription& d, const JobSchedMetaData& m);
        Job(const std::string& job);
        Job(std::istream& job);
        virtual ~Job(void);
        void setJobDescription(const JobDescription &descr);
        JobDescription& getJobDescription(void) { return descr; };
        void setJobSchedMetaData(const JobSchedMetaData &sched_meta);
        JobSchedMetaData& getSchedMetaData(void) { return sched_meta; };
};

}; // namespace Arc

#endif // SCHED_JOB
