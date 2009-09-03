#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "JobStateCREAM.h"

namespace Arc {

  JobState::StateType JobStateCREAM::StateMap(const std::string& state) {
    if (state == "REGISTERED")
      return JobState::ACCEPTED;
    else if (state == "PENDING")
      return JobState::ACCEPTED;
    else if (state == "RUNNING")
      return JobState::RUNNING;
    else if (state == "REALLY-RUNNING")
      return JobState::RUNNING;
    else if (state == "HELD")
      return JobState::HOLD;
    else if (state == "DONE-FAILED")
      return JobState::FAILED;
    else if (state == "DONE-OK")
      return JobState::FINISHED;
    else if (state == "ABORTED")
      return JobState::FAILED;
    else if (state == "CANCELLED")
      return JobState::KILLED;
    else if (state == "IDLE")
      return JobState::QUEUING;
    else
      return JobState::UNDEFINED;
  }

}
/*
   A    CREAM JOB STATES
   Here below is provided a brief description of the meaning of each possible state a CREAM job can enter:
   • REGISTERED: the job has been registered but it has not been started yet.
   • PENDING the job has been started, but it has still to be submitted to the LRMS abstraction layer
      module (i.e. BLAH).
   • IDLE: the job is idle in the Local Resource Management System (LRMS).
   • RUNNING: the job wrapper, which “encompasses” the user job, is running in the LRMS.
   • REALLY-RUNNING: the actual user job (the one speciﬁed as Executable in the job JDL) is running
      in the LRMS.
   • HELD: the job is held (suspended) in the LRMS.
   • CANCELLED: the job has been cancelled.
   • DONE-OK: the job has successfully been executed.
   • DONE-FAILED: the job has been executed, but some errors occurred.
   • ABORTED: errors occurred during the “management” of the job, e.g. the submission to the LRMS
      abstraction layer software (BLAH) failed.
   • UNKNOWN: the job is an unknown status.
 */
