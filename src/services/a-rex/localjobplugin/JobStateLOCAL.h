#ifndef __ARC_JOBSTATELOCAL_H__
#define __ARC_JOBSTATELOCAL_H__

#include <arc/compute/JobState.h>





namespace ARexLOCAL {

  class JobStateLOCAL
    : public Arc::JobState {
  public:
    JobStateLOCAL(const std::string& state)
      : Arc::JobState(state, &StateMap) {}
    static JobState::StateType StateMap(const std::string& state);
  };

}


#endif // __ARC_JOBSTATELOCAL_H__
