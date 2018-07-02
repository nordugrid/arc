#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/StringConv.h>

#include "JobStateINTERNAL.h"



/*
Maps/translates a INTERNAL state - which is a state corresponding to the ARexJob state, hence
GM-job state, to an ARC:JobState 
*/


namespace ARexINTERNAL {

  Arc::JobState::StateType JobStateINTERNAL::StateMap(const std::string& state) {
    std::string state_ = Arc::lower(state);
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
    if (state_.substr(0,8) == "pending:") state_.erase(0,8);
    // remove spaces because sometimes we may have 'INLRMS: *'
    std::string::size_type p = 0;
    while((p = state_.find(' ',p)) != std::string::npos)
      state_.erase(p,1);


    /// \mapattr ACCEPTED -> ACCEPTED
    /// \mapattr ACCEPTING -> ACCEPTED
    if ((state_ == "accepted") ||
        (state_ == "accepting"))
      return JobState::ACCEPTED;
    /// \mapattr PREPARING -> PREPARING
    /// \mapattr PREPARED -> PREPARING
    else if ((state_ == "preparing") ||
             (state_ == "prepared"))
      return JobState::PREPARING;
    /// \mapattr SUBMIT -> SUBMITTING
    /// \mapattr SUBMITTING -> SUBMITTING
    else if ((state_ == "submit") ||
             (state_ == "submitting"))
      return JobState::SUBMITTING;
    /// \mapattr INLRMS:Q -> QUEUING
    else if (state_ == "inlrms:q")
      return JobState::QUEUING;
    /// \mapattr INLRMS:R -> RUNNING
    else if (state_ == "inlrms:r")
      return JobState::RUNNING;
    /// \mapattr INLRMS:H -> HOLD
    else if (state_ == "inlrms:h")
      return JobState::HOLD;
    /// \mapattr INLRMS:S -> HOLD
    else if (state_ == "inlrms:s")
      return JobState::HOLD;
    /// \mapattr INLRMS:E -> FINISHING
    else if (state_ == "inlrms:e")
      return JobState::FINISHING;
    /// \mapattr INLRMS:O -> HOLD
    else if (state_ == "inlrms:o")
      return JobState::HOLD;
    /// \mapattr INLRMS* -> QUEUING
    else if (state_.substr(0,6) == "inlrms")
      return JobState::QUEUING; // expect worst ?
    /// \mapattr FINISHING -> FINISHING
    /// \mapattr KILLING -> FINISHING
    /// \mapattr CANCELING -> FINISHING
    /// \mapattr EXECUTED -> FINISHING
    else if ((state_ == "finishing") ||
             (state_ == "killing") ||
	     (state_ == "canceling") ||
             (state_ == "executed"))
      return JobState::FINISHING;
    /// \mapattr FINISHED -> FINISHED
    else if (state_ == "finished")
      return JobState::FINISHED;
    /// \mapattr KILLED -> KILLED
    else if (state_ == "killed")
      return JobState::KILLED;
    /// \mapattr FAILED -> FAILED
    else if (state_ == "failed")
      return JobState::FAILED;
    /// \mapattr DELETED -> DELETED
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

