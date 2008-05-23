#ifndef __ARC_JOB_H__
#define __ARC_JOB_H__

#include <arc/client/ACC.h>
#include <arc/DateTime.h>
#include <arc/URL.h>
#include <string>

namespace Arc {

  class Job{
   
  public:
    
    Job();
    ~Job();
    
    std::string id;
    std::string owner;
    std::string cluster;
    std::string queue;
    std::string sstdout;
    std::string sstderr;
    std::string sstdin;
    std::string rerunable;
    Period requested_cpu_time;
    Period requested_wall_time;
    std::string status;
    int queue_rank;
    std::string comment;
    std::string submission_ui;
    Time submission_time;
    Time completion_time;
    Period used_cpu_time;
    Period used_wall_time;
    Time erase_time;
    int used_memory;
    std::string errors;
    int exitcode;
    std::string job_name;
    //    std::list<RuntimeEnvironment> runtime_environments;
    int cpu_count;
    //    std::list<std::string> execution_nodes;
    std::string gmlog;
    std::string client_software;
    Time proxy_expire_time;
    Time mds_validfrom;
    Time mds_validto;

    URL InfoEndpoint;
    
  };
  
} // namespace Arc

#endif // __ARC_JOB_H__
