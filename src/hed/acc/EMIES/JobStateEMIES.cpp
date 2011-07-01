#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/StringConv.h>

#include "JobStateEMIES.h"

namespace Arc {

  JobState::StateType JobStateEMIES::StateMap(const std::string& st) {
    EMIESJobState st_;
    st_ = XMLNode(st);
    return StateMapInt(st_);
  }

  JobState::StateType JobStateEMIES::StateMapInt(const EMIESJobState& st) {
    if(st.state == "ACCEPTED") {
      return JobState::ACCEPTED;
    } else if(st.state == "PREPROCESSING") {
      if(st.HasAttribute("CLIENT-STAGEIN-POSSIBLE")) return JobState::PREPARING;
      return JobState::ACCEPTED;
    } else if(st.state == "PROCESSING") {
      return JobState::QUEUING;
    } else if(st.state == "PROCESSING-ACCEPTING") {
      return JobState::SUBMITTING;
    } else if(st.state == "PROCESSING-QUEUED") {
      return JobState::QUEUING;
    } else if(st.state == "PROCESSING-RUNNING") {
      return JobState::RUNNING;
    } else if(st.state == "POSTPROCESSING") {
      if(st.HasAttribute("CLIENT-STAGEIN-POSSIBLE")) return JobState::FINISHING;
      return JobState::OTHER;
    } else if(st.state == "TERMINAL") {
      if(st.HasAttribute("PREPROCESSING-CANCEL")) return JobState::FAILED;
      if(st.HasAttribute("PROCESSING-CANCEL")) return JobState::FAILED;
      if(st.HasAttribute("POSTPROCESSING-CANCEL")) return JobState::FAILED;
      if(st.HasAttribute("VALIDATION-FAILURE")) return JobState::FAILED;
      if(st.HasAttribute("PREPROCESSING-FAILURE")) return JobState::FAILED;
      if(st.HasAttribute("PROCESSING-FAILURE")) return JobState::FAILED;
      if(st.HasAttribute("POSTPROCESSING-FAILURE")) return JobState::FAILED;
      if(st.HasAttribute("APP-FAILURE")) return JobState::FAILED;
      return JobState::FINISHED;
    }
    return JobState::OTHER;
  }

}
