#ifndef SCHED_JOB_DESCRIPTION
#define SCHED_JOB_DESCRIPTION

#include <string>
#include <list>

#include <arc/XMLNode.h>

namespace GridScheduler
{

class JobDescription: public Arc::XMLNode {

    private:
        XMLNode descr;
        std::string id_;
    public:
        JobDescription();
        JobDescription(Arc::XMLNode& d);
        virtual ~JobDescription(void);
        std::string getJobName(void);
        std::string getOS(void);
        std::string getArch(void);
        operator bool(void) { return !id_.empty(); };
        bool operator!(void) { return id_.empty(); };
};

};

#endif // SCHED_JOB_DESCRIPTION
