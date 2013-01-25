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
    if(st.state == EMIES_STATE_ACCEPTED_S) {
      return JobState::ACCEPTED;
    } else if(st.state == EMIES_STATE_PREPROCESSING_S) {
      if(st.HasAttribute(EMIES_SATTR_CLIENT_STAGEIN_POSSIBLE_S)) return JobState::PREPARING;
      return JobState::ACCEPTED;
    } else if(st.state == EMIES_STATE_PROCESSING_S) {
      return JobState::QUEUING;
    } else if(st.state == EMIES_STATE_PROCESSING_ACCEPTING_S) {
      return JobState::SUBMITTING;
    } else if(st.state == EMIES_STATE_PROCESSING_QUEUED_S) {
      return JobState::QUEUING;
    } else if(st.state == EMIES_STATE_PROCESSING_RUNNING_S) {
      return JobState::RUNNING;
    } else if(st.state == EMIES_STATE_POSTPROCESSING_S) {
      if(st.HasAttribute(EMIES_SATTR_CLIENT_STAGEIN_POSSIBLE_S)) return JobState::FINISHING;
      return JobState::OTHER;
    } else if(st.state == EMIES_STATE_TERMINAL_S) {
      if(st.HasAttribute(EMIES_SATTR_PREPROCESSING_CANCEL_S)) return JobState::FAILED;
      if(st.HasAttribute(EMIES_SATTR_PROCESSING_CANCEL_S)) return JobState::FAILED;
      if(st.HasAttribute(EMIES_SATTR_POSTPROCESSING_CANCEL_S)) return JobState::FAILED;
      if(st.HasAttribute(EMIES_SATTR_VALIDATION_FAILURE_S)) return JobState::FAILED;
      if(st.HasAttribute(EMIES_SATTR_PREPROCESSING_FAILURE_S)) return JobState::FAILED;
      if(st.HasAttribute(EMIES_SATTR_PROCESSING_FAILURE_S)) return JobState::FAILED;
      if(st.HasAttribute(EMIES_SATTR_POSTPROCESSING_FAILURE_S)) return JobState::FAILED;
      if(st.HasAttribute(EMIES_SATTR_APP_FAILURE_S)) return JobState::FAILED;
      if(st.HasAttribute(EMIES_SATTR_EXPIRED_S)) return JobState::DELETED;
      return JobState::FINISHED;
    } else if(st.state == "") {
      return JobState::UNDEFINED;
    }
    return JobState::OTHER;
  }

}
