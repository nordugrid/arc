#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "JobStateARC0.h"

namespace Arc {

  JobState::StateType JobStateARC0::StateMap(const std::string& state) {
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
    if (state_.substr(0,8) == "PENDING:") state_.erase(0,8);
    // remove spaces because sometimes we may have 'INLRMS: *'
    std::string::size_type p = 0;
    while((p = state_.find(' ',p)) != std::string::npos)
      state_.erase(p,1);
    if ((state_ == "ACCEPTED") ||
        (state_ == "ACCEPTING"))
      return JobState::ACCEPTED;
    else if ((state_ == "PREPARING") ||
             (state_ == "PREPARED"))
      return JobState::PREPARING;
    else if ((state_ == "SUBMIT") ||
             (state_ == "SUBMITTING"))
      return JobState::SUBMITTING;
    else if (state_ == "INLRMS:Q")
      return JobState::QUEUING;
    else if (state_ == "INLRMS:R")
      return JobState::RUNNING;
    else if (state_ == "INLRMS:H")
      return JobState::HOLD;
    else if (state_.substr(0,6) == "INLRMS")
      return JobState::QUEUING; // expect worst ?
    else if ((state_ == "FINISHING") ||
             (state_ == "KILLING") ||
             (state_ == "CANCELING") ||
             (state_ == "EXECUTED"))
      return JobState::FINISHING;
    else if (state_ == "FINISHED")
      return JobState::FINISHED;
    else if (state_ == "KILLED")
      return JobState::KILLED;
    else if (state_ == "FAILED")
      return JobState::FAILED;
    else if (state_ == "DELETED")
      return JobState::DELETED;
    else if (state_ == "")
      return JobState::UNDEFINED;
    else
      return JobState::OTHER;
  }

}
