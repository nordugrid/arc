#include "job_desc.h"

std::string JobDescription::getJobName(void) { 
    return (std::string)descr["JobDefinition"]["JobDescription"]["JobIdentification"]["JobName"]; 
}

std::string JobDescription::getOS(void) { 

    return (std::string)descr["JobDefinition"]["Resources"]["OperatingSystem"]["OperatingSystemType"]["OperatingSystemName"]; 
}
 
 
std::string JobDescription::XMLNode& getArch(void) { 

    return (std::string)descr["JobDefinition"]["Resources"]["CPUArchitecture"]["CPUArchitectureName"]; 
}

