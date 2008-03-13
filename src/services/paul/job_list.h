#ifndef __ARC_LOB_LIST_H__
#define __ARC_JOB_LIST_H__

#include <list>
#include "job.h"

namespace Paul
{

class JobList: public std::list<Job>
{
    public:
        unsigned int getTotalJobs(void) { return 0; };
        unsigned int getRunningJobs(void) { return 0; };
        unsigned int getLocalRunningJobs(void) { return 0; };
        unsigned int getWaitingJobs(void) { return 0; };
};

}; // namespace paul

#endif
