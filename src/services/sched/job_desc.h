#ifndef SCHED_JOB_DESCRIPTION
#define SCHED_JOB_DESCRIPTION

#include <string>
#include <list>

#include <arc/XMLNode.h>

namespace GridScheduler
{

class JobRequest: public Arc::XMLNode {

    private:
        Arc::XMLNode descr;
    public:
        JobRequest();
        JobRequest(Arc::XMLNode d);
        virtual ~JobRequest(void);
        std::string getJobName(void);
        std::string getOS(void);
        std::string getArch(void);
        Arc::XMLNode getJSDL(void) { return descr;};
        JobRequest& operator=(const JobRequest& j);
        JobRequest( const JobRequest& j );
};

};

#endif // SCHED_JOB_DESCRIPTION
