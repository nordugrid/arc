#ifndef __ARC_JOBSTATECREAM_H__
#define __ARC_JOBSTATECREAM_H__

#include <arc/compute/JobState.h>

namespace Arc {

  class JobStateCREAM
    : public JobState {
  public:
    JobStateCREAM(const std::string& state)
      : JobState(state, &StateMap) {}
    static JobState::StateType StateMap(const std::string& state);
  };

}

#endif // __ARC_JOBSTATECREAM_H__