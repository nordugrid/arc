#ifndef SCHED_JOB_DESCRIPTION
#define SCHED_JOB_DESCRIPTION

#include <string>
#include <list>

#include <arc/XMLNode.h>


class JobDescription {

    private:
        XMLNode descr;
    public:
        JobDescription();
        JobDescription(const Arc::XMLNode& jsdl);
        virtual JobDescription& operator=(const JobDescription& j);
        virtual ~JobDescription(void);
        std::string getJobName(void);
        std::string getOS(void);
        std::string getArch(void);
};

#endif // SCHED_JOB_DESCRIPTION
