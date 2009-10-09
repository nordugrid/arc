#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "JobStateBES.h"

namespace Arc {

  JobState::StateType JobStateBES::StateMap(const std::string& state) {
    if (state == "Pending")
      return JobState::ACCEPTED;
    else if (state == "Running")
      return JobState::RUNNING;
    else if (state == "Finished")
      return JobState::FINISHED;
    else if (state == "Cancelled")
      return JobState::KILLED;
    else if (state == "Failed")
      return JobState::FAILED;
    else
      return JobState::UNDEFINED;
  }

}
