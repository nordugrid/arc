#ifndef __ARC_LOB_LIST_H__
#define __ARC_JOB_LIST_H__

#include <vector>
#include "job.h"

namespace Paul
{

class JobList
{
    private:
        std::vector<Job> jl;
    public:
        JobList(void) {};
        void add(const Job &j) { jl.push_back(j); };
        Job &operator[](int i) { return jl[i]; };
        unsigned int getTotalJobs(void) { return jl.size(); };
        unsigned int getRunningJobs(void);
        unsigned int getLocalRunningJobs(void) { return 0; };
        unsigned int getWaitingJobs(void);
};

}; // namespace paul

#endif
