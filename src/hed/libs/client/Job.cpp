#include <arc/ArcConfig.h>
#include <arc/client/Job.h>

namespace Arc {
  
  Job::Job() : requested_cpu_time(-1),
	       requested_wall_time(-1),
	       queue_rank(-1),
	       submission_time(-1),
	       completion_time(-1),
	       used_cpu_time(-1),
	       used_wall_time(-1),
	       erase_time(-1),
	       used_memory(-1),
	       exitcode(-1),
	       cpu_count(-1),
	       proxy_expire_time(-1),
	       mds_validfrom(-1),
	       mds_validto(-1) {}
  
  Job::~Job() {}
  
} // namespace Arc
