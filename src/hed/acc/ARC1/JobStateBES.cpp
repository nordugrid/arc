#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/StringConv.h>

#include "JobStateBES.h"

namespace Arc {

  JobState::StateType JobStateBES::StateMap(const std::string& state) {
    std::string state_ = Arc::lower(state);
    if (state_ == "pending")
      return JobState::ACCEPTED;
    else if (state_ == "running")
      return JobState::RUNNING;
    else if (state_ == "finished")
      return JobState::FINISHED;
    else if (state_ == "cancelled")
      return JobState::KILLED;
    else if (state_ == "failed")
      return JobState::FAILED;
    else
      return JobState::UNDEFINED;
  }

}
