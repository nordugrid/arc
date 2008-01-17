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
        Arc::XMLNode& getJobName(void) { return descr["JobDefinition"]["JobDescription"]["JobIdentification"]["JobName"]; };
        Arc::XMLNode& getOS(void) { return descr["JobDefinition"]["Resources"]["OperatingSystem"]["OperatingSystemType"]["OperatingSystemName"]; };
        Arc::XMLNode& getArch(void) { return descr["JobDefinition"]["Resources"]["CPUArchitecture"]["CPUArchitectureName"]; };
};

#endif // SCHED_JOB_DESCRIPTION
