#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/StringConv.h>

#include "JobStateEMIES.h"

namespace Arc {

  std::string JobStateEMIES::FormatSpecificState(const std::string& state) {
    EMIESJobState st_;
    st_ = XMLNode(state);
    
    // Return format: <state>[:<attribute1>[,<attribute2>[...]]]
    
    std::string attributes;
    if (!st_.attributes.empty()) {
      std::list<std::string>::const_iterator it = st_.attributes.begin();
      attributes = ":" + *it++;
      for (; it != st_.attributes.end(); ++it) {
        attributes += "," + *it;
      }
    }
    
    return st_.state + attributes;
  }
  
  JobState::StateType JobStateEMIES::StateMapS(const std::string& st) {
    EMIESJobState st_;
    st_ = st;
    return StateMapInt(st_);
  }

  JobState::StateType JobStateEMIES::StateMapX(const std::string& st) {
    EMIESJobState st_;
    st_ = XMLNode(st);
    return StateMapInt(st_);
  }

  JobState::StateType JobStateEMIES::StateMapInt(const EMIESJobState& st) {
    /// \mapname EMIES EMI ES
    /// \mapnote EMI ES states contains a state name and zero or more state attributes. For this mapping the notation:<br/><tt><name>:{*|<attribute>}</tt><br/> is used, where '*' applies to all attributes except those already specify for a particular state name.
    /// \mapattr accepted:* -> ACCEPTED
    if(st.state == EMIES_STATE_ACCEPTED_S) {
      return JobState::ACCEPTED;
    }
    /// \mapattr preprocessing:* -> ACCEPTED
    else if(st.state == EMIES_STATE_PREPROCESSING_S) {
      if(st.HasAttribute(EMIES_SATTR_CLIENT_STAGEIN_POSSIBLE_S)) return JobState::PREPARING;
      return JobState::ACCEPTED;
    }
    /// \mapattr processing:* -> QUEUING
    else if(st.state == EMIES_STATE_PROCESSING_S) {
      return JobState::QUEUING;
    }
    /// \mapattr processing-accepting:* -> SUBMITTING
    else if(st.state == EMIES_STATE_PROCESSING_ACCEPTING_S) {
      return JobState::SUBMITTING;
    }
    /// \mapattr processing-queued:* -> QUEUING
    else if(st.state == EMIES_STATE_PROCESSING_QUEUED_S) {
      return JobState::QUEUING;
    }
    /// \mapattr processing-running:* -> RUNNING
    else if(st.state == EMIES_STATE_PROCESSING_RUNNING_S) {
      return JobState::RUNNING;
    }
    /// \mapattr postprocessing:client-stageout-possible -> FINISHING
    /// \mapattr postprocessing:* -> OTHER
    else if(st.state == EMIES_STATE_POSTPROCESSING_S) {
      if(st.HasAttribute(EMIES_SATTR_CLIENT_STAGEOUT_POSSIBLE_S)) return JobState::FINISHING;
      return JobState::OTHER;
    }
    /// \mapattr terminal:preprocessing-cancel -> KILLED
    /// \mapattr terminal:processing-cancel -> KILLED
    /// \mapattr terminal:postprocessing-cancel -> KILLED
    /// \mapattr terminal:validation-failure -> FAILED
    /// \mapattr terminal:preprocessing-failure -> FAILED
    /// \mapattr terminal:processing-failure -> FAILED
    /// \mapattr terminal:postprocessing-failure -> FAILED
    /// \mapattr terminal:app-failure -> FAILED
    /// \mapattr terminal:expired -> DELETED
    /// \mapattr terminal:* -> FINISHED
    else if(st.state == EMIES_STATE_TERMINAL_S) {
      if(st.HasAttribute(EMIES_SATTR_PREPROCESSING_CANCEL_S)) return JobState::KILLED;
      if(st.HasAttribute(EMIES_SATTR_PROCESSING_CANCEL_S)) return JobState::KILLED;
      if(st.HasAttribute(EMIES_SATTR_POSTPROCESSING_CANCEL_S)) return JobState::KILLED;
      if(st.HasAttribute(EMIES_SATTR_VALIDATION_FAILURE_S)) return JobState::FAILED;
      if(st.HasAttribute(EMIES_SATTR_PREPROCESSING_FAILURE_S)) return JobState::FAILED;
      if(st.HasAttribute(EMIES_SATTR_PROCESSING_FAILURE_S)) return JobState::FAILED;
      if(st.HasAttribute(EMIES_SATTR_POSTPROCESSING_FAILURE_S)) return JobState::FAILED;
      if(st.HasAttribute(EMIES_SATTR_APP_FAILURE_S)) return JobState::FAILED;
      if(st.HasAttribute(EMIES_SATTR_EXPIRED_S)) return JobState::DELETED;
      return JobState::FINISHED;
    }
    /// \mapattr "":* -> UNDEFINED
    else if(st.state == "") {
      return JobState::UNDEFINED;
    }
    /// \mapattr Any other state -> OTHER
    return JobState::OTHER;
  }

}
