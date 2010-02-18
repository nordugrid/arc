#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/StringConv.h>

#include "JobStateARC1.h"

namespace Arc {

  JobState::StateType JobStateARC1::StateMap(const std::string& state) {
    std::string state_ = Arc::lower(state);
    std::string::size_type p = state_.find("pending:");
    if(p != std::string::npos) {
      state_.erase(p,8);
    }
    if (state_ == "accepted")
      return JobState::ACCEPTED;
    else if (state_ == "preparing" ||
             state_ == "prepared")    // obtained through BES
      return JobState::PREPARING;
    else if (state_ == "submit" ||
             state_ == "submitting")  // obtained through BES
      return JobState::SUBMITTING;
    else if (state_ == "inlrms:q")
      return JobState::QUEUING;
    else if (state_ == "inlrms:r" ||
             state_ == "inlrms:executed" ||
             state_ == "inlrms:s" ||
             state_ == "inlrms:e" ||
             state_ == "executing" || // obtained through BES
             state_ == "executed" ||  // obtained through BES
             state_ == "killing")     // obtained through BES
      return JobState::RUNNING;
    else if (state_ == "finishing")
      return JobState::FINISHING;
    else if (state_ == "finished")
      return JobState::FINISHED;
    else if (state_ == "killed")
      return JobState::KILLED;
    else if (state_ == "failed")
      return JobState::FAILED;
    else if (state_ == "deleted")
      return JobState::DELETED;
    else if (state_ == "")
      return JobState::UNDEFINED;
    else
      return JobState::OTHER;
  }

}
