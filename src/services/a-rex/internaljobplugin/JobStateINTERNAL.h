#ifndef __ARC_JOBSTATEINTERNAL_H__
#define __ARC_JOBSTATEINTERNAL_H__

#include <arc/compute/JobState.h>





namespace ARexINTERNAL {

  class JobStateINTERNAL
    : public Arc::JobState {
  public:
    JobStateINTERNAL(const std::string& state)
      : Arc::JobState(state, &StateMap) {}
    static JobState::StateType StateMap(const std::string& state);
  };

}


#endif // __ARC_JOBSTATEINTERNAL_H__
