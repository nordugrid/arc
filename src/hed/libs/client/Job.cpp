#include <arc/ArcConfig.h>
#include <arc/client/Job.h>

namespace Arc {
  
  Job::Job() : ExitCode(-1),
	       WaitingPosition(-1),
	       RequestedWallTime(-1),
	       RequestedTotalCPUTime(-1),
	       RequestedMainMemory(-1),
	       RequestedSlots(-1),
	       UsedWallTime(-1),
	       UsedTotalCPUTime(-1),
	       UsedMainMemory(-1),
	       UsedSlots(-1),
	       SubmissionTime(-1),
	       ComputingManagerSubmissionTime(-1),
	       StartTime(-1),
	       ComputingManagerEndTime(-1),
	       EndTime(-1),
	       WorkingAreaEraseTime(-1),
	       ProxyExpirationTime(-1),
	       VirtualMachine(false) {}
  
  Job::~Job() {}
  
} // namespace Arc
