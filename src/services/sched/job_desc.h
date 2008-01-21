#ifndef SCHED_JOB_DESCRIPTION
#define SCHED_JOB_DESCRIPTION

#include <string>
#include <list>

#include <arc/XMLNode.h>

namespace Arc
{

class JobDescription:XMLNode {

    private:
        XMLNode descr;
    public:
        JobDescription();
        virtual ~JobDescription(void);
        std::string getJobName(void);
        std::string getOS(void);
        std::string getArch(void);
};

};

#endif // SCHED_JOB_DESCRIPTION
