#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "job_desc.h"


namespace GridScheduler
{

std::string JobRequest::getJobName(void) { 
    return (std::string)(*this)["JobDefinition"]["JobDescription"]["JobIdentification"]["JobName"]; 
}

std::string JobRequest::getOS(void) { 
    return (std::string)(*this)["JobDefinition"]["Resources"]["OperatingSystem"]["OperatingSystemType"]["OperatingSystemName"]; 
}
 
 
std::string JobRequest::getArch(void) { 
    return (std::string)(*this)["JobDefinition"]["Resources"]["CPUArchitecture"]["CPUArchitectureName"]; 
}




JobRequest::JobRequest(Arc::XMLNode d) {
    d.New(descr);
}


JobRequest::JobRequest() {

}

JobRequest::~JobRequest() {

}

JobRequest&  JobRequest::operator=( const  JobRequest& j )
{
   if ( this != &j )
   {
        j.descr.New(descr);
   }

   return *this;
}

JobRequest::JobRequest( const JobRequest& j )
{
    j.descr.New(descr);
}


}; //namespace
