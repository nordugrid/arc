#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "job_desc.h"

namespace Arc
{

std::string JobDescription::getJobName(void) { 
    return (std::string)(*this)["JobDefinition"]["JobDescription"]["JobIdentification"]["JobName"]; 
}

std::string JobDescription::getOS(void) { 

    return (std::string)(*this)["JobDefinition"]["Resources"]["OperatingSystem"]["OperatingSystemType"]["OperatingSystemName"]; 
}
 
 
std::string JobDescription::getArch(void) { 

    return (std::string)(*this)["JobDefinition"]["Resources"]["CPUArchitecture"]["CPUArchitectureName"]; 
}

JobDescription::JobDescription(Arc::XMLNode& d) {
    JobDescription::descr=d;
}

};

