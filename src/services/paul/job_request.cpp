#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "job_request.h"

namespace Paul {

JobRequest::JobRequest() 
{
    // NOP
}

JobRequest::~JobRequest() 
{
    // NOP
}

JobRequest::JobRequest(Arc::XMLNode &r) 
{
    r.New(request);
}

JobRequest::JobRequest(const JobRequest &j) 
{
    j.request.New(request);
}

const std::string JobRequest::getName(void) { 
    return (std::string)request["JobDefinition"]["JobDescription"]["JobIdentification"]["JobName"];  
}

const std::string JobRequest::getOS(void) { 
    return (std::string)request["JobDefinition"]["Resources"]["OperatingSystem"]["OperatingSystemType"]["OperatingSystemName"]; 
}
 
 
const std::string JobRequest::getArch(void) { 
    return (std::string)request["JobDefinition"]["Resources"]["CPUArchitecture"]["CPUArchitectureName"]; 
}

JobRequest &JobRequest::operator=(const JobRequest &j) 
{
   if (this != &j) {
     j.request.New(request);
   }

   return *this;
}

} //namespace
