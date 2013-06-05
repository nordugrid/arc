#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/StringConv.h>

#include "JobStateBES.h"

namespace Arc {

  JobState::StateType JobStateBES::StateMap(const std::string& state) {
    /// \mapname BES BES
    std::string state_ = Arc::lower(state);
    /// \mapattr pending -> ACCEPTED
    if (state_ == "pending")
      return JobState::ACCEPTED;
    /// \mapattr running -> RUNNING
    else if (state_ == "running")
      return JobState::RUNNING;
    /// \mapattr finished -> FINISHED
    else if (state_ == "finished")
      return JobState::FINISHED;
    /// \mapattr cancelled -> KILLED
    else if (state_ == "cancelled")
      return JobState::KILLED;
    /// \mapattr failed -> FAILED
    else if (state_ == "failed")
      return JobState::FAILED;
    /// \mapattr Any other state -> UNDEFINED
    else
      return JobState::UNDEFINED;
  }

}
