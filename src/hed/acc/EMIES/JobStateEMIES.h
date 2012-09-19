#ifndef __ARC_JOBSTATEEMIES_H__
#define __ARC_JOBSTATEEMIES_H__

#include <arc/client/JobState.h>

#include "EMIESClient.h"

namespace Arc {

#define EMIES_STATE_ACCEPTED_S "accepted"
#define EMIES_STATE_PREPROCESSING_S "preprocessing"
#define EMIES_STATE_PROCESSING_S "processing"
#define EMIES_STATE_PROCESSING_ACCEPTING_S "processing-accepting"
#define EMIES_STATE_PROCESSING_QUEUED_S "processing-queued"
#define EMIES_STATE_PROCESSING_RUNNING_S "processing-running"
#define EMIES_STATE_POSTPROCESSING_S "postprocessing"
#define EMIES_STATE_TERMINAL_S "terminal"

#define EMIES_SATTR_CLIENT_STAGEIN_POSSIBLE_S "client-stagein-possible"
#define EMIES_SATTR_CLIENT_STAGEOUT_POSSIBLE_S "client-stageout-possible"
#define EMIES_SATTR_PREPROCESSING_CANCEL_S "preprocessing-cancel"
#define EMIES_SATTR_PREPROCESSING_FAILURE_S "preprocessing-failure"
#define EMIES_SATTR_PROCESSING_CANCEL_S "processing-cancel"
#define EMIES_SATTR_PROCESSING_FAILURE_S "processing-failure"
#define EMIES_SATTR_POSTPROCESSING_CANCEL_S "postprocessing-cancel"
#define EMIES_SATTR_POSTPROCESSING_FAILURE_S "postprocessing-failure"
#define EMIES_SATTR_VALIDATION_FAILURE_S "validation-failure"
#define EMIES_SATTR_APP_FAILURE_S "app-failure"

  class JobStateEMIES
    : public JobState {
  public:
    JobStateEMIES(const std::string& state): JobState(state, &StateMapS, FormatSpecificState) {}
    // TODO: extremely suboptimal
    JobStateEMIES(XMLNode state): JobState(xml_to_string(state), &StateMapX, FormatSpecificState) {}
    static JobState::StateType StateMapS(const std::string& state);
    static JobState::StateType StateMapX(const std::string& state);
    static JobState::StateType StateMapInt(const EMIESJobState& st);
    
    static std::string FormatSpecificState(const std::string& state);
  private:
    std::string xml_to_string(XMLNode xml) {
      std::string s;
      xml.GetXML(s);
      return s;
    };
  };

}

#endif // __ARC_JOBSTATEEMIES_H__
