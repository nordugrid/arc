#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "JobStateGRIDFTPJOB.h"

namespace Arc {

  JobState::StateType JobStateGRIDFTPJOB::StateMap(const std::string& state) {
    std::string state_ = state;
/*
   Infosys states (mapped from GM states):
 ACCEPTING
 ACCEPTED
 PREPARED
 SUBMITTING
 INLRMS: *
 KILLING
 EXECUTED
 KILLED
 FAILED

   GM states (either not mapped or somehow obtained directly):
 ACCEPTED 
 PREPARING
 SUBMIT
 INLRMS
 CANCELING
 FINISHING
 FINISHED
 DELETED
 PENDING:*
*/
    /// \mapname GM Grid Manager
    /// \mapnote Prefix "PENDING:" and spaces are ignored when mapping states.
    if (state_.substr(0,8) == "PENDING:") state_.erase(0,8);
    // remove spaces because sometimes we may have 'INLRMS: *'
    std::string::size_type p = 0;
    while((p = state_.find(' ',p)) != std::string::npos)
      state_.erase(p,1);
    /// \mapattr ACCEPTED -> ACCEPTED
    /// \mapattr ACCEPTING -> ACCEPTED
    if ((state_ == "ACCEPTED") ||
        (state_ == "ACCEPTING"))
      return JobState::ACCEPTED;
    /// \mapattr PREPARING -> PREPARING
    /// \mapattr PREPARED -> PREPARING
    else if ((state_ == "PREPARING") ||
             (state_ == "PREPARED"))
      return JobState::PREPARING;
    /// \mapattr SUBMIT -> SUBMITTING
    /// \mapattr SUBMITTING -> SUBMITTING
    else if ((state_ == "SUBMIT") ||
             (state_ == "SUBMITTING"))
      return JobState::SUBMITTING;
    /// \mapattr INLRMS:Q -> QUEUING
    else if (state_ == "INLRMS:Q")
      return JobState::QUEUING;
    /// \mapattr INLRMS:R -> RUNNING
    else if (state_ == "INLRMS:R")
      return JobState::RUNNING;
    /// \mapattr INLRMS:H -> HOLD
    else if (state_ == "INLRMS:H")
      return JobState::HOLD;
    /// \mapattr INLRMS:S -> HOLD
    else if (state_ == "INLRMS:S")
      return JobState::HOLD;
    /// \mapattr INLRMS:E -> FINISHING
    else if (state_ == "INLRMS:E")
      return JobState::FINISHING;
    /// \mapattr INLRMS:O -> HOLD
    else if (state_ == "INLRMS:O")
      return JobState::HOLD;
    /// \mapattr INLRMS* -> QUEUING
    else if (state_.substr(0,6) == "INLRMS")
      return JobState::QUEUING; // expect worst ?
    /// \mapattr FINISHING -> FINISHING
    /// \mapattr KILLING -> FINISHING
    /// \mapattr CANCELING -> FINISHING
    /// \mapattr EXECUTED -> FINISHING
    else if ((state_ == "FINISHING") ||
             (state_ == "KILLING") ||
             (state_ == "CANCELING") ||
             (state_ == "EXECUTED"))
      return JobState::FINISHING;
    /// \mapattr FINISHED -> FINISHED
    else if (state_ == "FINISHED")
      return JobState::FINISHED;
    /// \mapattr KILLED -> KILLED
    else if (state_ == "KILLED")
      return JobState::KILLED;
    /// \mapattr FAILED -> FAILED
    else if (state_ == "FAILED")
      return JobState::FAILED;
    /// \mapattr DELETED -> DELETED
    else if (state_ == "DELETED")
      return JobState::DELETED;
    /// \mapattr "" -> UNDEFINED
    else if (state_ == "")
      return JobState::UNDEFINED;
    /// \mapattr Any other state -> OTHER
    else
      return JobState::OTHER;
  }

}
