#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "job_status.h"

namespace Paul 
{

JobStatus::JobStatus(void)
{
   str_map[ACCEPTING] = "ACCEPTING";
   str_map[ACCEPTED] = "ACCEPTED";
   str_map[PREPARING] = "PREPARING";
   str_map[PREPARED] = "PREPARED";
   str_map[SUBMITTING] = "SUBMITTING";
   str_map[EXECUTING] = "EXECUTING";
   str_map[KILLING] = "KILLING";
   str_map[EXECUTED] = "EXECUTED";
   str_map[FINISHING] = "FINISHING";
   str_map[FAILED] = "FAILED";
   str_map[HELD] = "HELD"; 
}
    
JobStatus::JobStatus(JobStatusLevel l)
{
    level = l;
}

void JobStatus::add_map(std::map<int, std::string> &m)
{
    str_map = m;
}

JobStatusLevel JobStatus::get(void)
{   
    return level;
}

JobStatus::operator std::string(void) 
{
    return str_map[level];
}

}; // namespace Paul
