#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/StringConv.h>

#include "JobStateARC1.h"

namespace Arc {

  JobState::StateType JobStateARC1::StateMap(const std::string& state) {
    /// \mapname ARCBES ARC extended BES
    /// \mapnote The mapping is case-insensitive, and prefix "pending:" are ignored when performing the mapping.
    std::string state_ = Arc::lower(state);
    std::string::size_type p = state_.find("pending:");
    if(p != std::string::npos) {
      state_.erase(p,8);
    }
    /// \mapattr accepted -> ACCEPTED
    if (state_ == "accepted")
      return JobState::ACCEPTED;
    /// \mapattr preparing -> PREPARING
    /// \mapattr prepared -> PREPARING
    else if (state_ == "preparing" ||
             state_ == "prepared")    // obtained through BES
      return JobState::PREPARING;
    /// \mapattr submit -> SUBMITTING
    /// \mapattr submitting -> SUBMITTING
    else if (state_ == "submit" ||
             state_ == "submitting")  // obtained through BES
      return JobState::SUBMITTING;
    /// \mapattr inlrms:q -> QUEUING
    else if (state_ == "inlrms:q")
      return JobState::QUEUING;
    /// \mapattr inlrms:r -> RUNNING
    /// \mapattr inlrms:executed -> RUNNING
    /// \mapattr inlrms:s -> RUNNING
    /// \mapattr inlrms:e -> RUNNING
    /// \mapattr executing -> RUNNING
    /// \mapattr executed -> RUNNING
    /// \mapattr killing -> RUNNING
    else if (state_ == "inlrms:r" ||
             state_ == "inlrms:executed" ||
             state_ == "inlrms:s" ||
             state_ == "inlrms:e" ||
             state_ == "executing" || // obtained through BES
             state_ == "executed" ||  // obtained through BES
             state_ == "killing")     // obtained through BES
      return JobState::RUNNING;
    /// \mapattr finishing -> FINISHING
    else if (state_ == "finishing")
      return JobState::FINISHING;
    /// \mapattr finished -> FINISHED
    else if (state_ == "finished")
      return JobState::FINISHED;
    /// \mapattr killed -> KILLED
    else if (state_ == "killed")
      return JobState::KILLED;
    /// \mapattr failed -> FAILED
    else if (state_ == "failed")
      return JobState::FAILED;
    /// \mapattr deleted -> DELETED
    else if (state_ == "deleted")
      return JobState::DELETED;
    /// \mapattr "" -> UNDEFINED
    else if (state_ == "")
      return JobState::UNDEFINED;
    /// \mapattr Any other state -> OTHER
    else
      return JobState::OTHER;
  }

}
