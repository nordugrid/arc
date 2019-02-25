#ifndef __ARC_JOBSTATEARC1_H__
#define __ARC_JOBSTATEARC1_H__

#include <arc/compute/JobState.h>

namespace Arc {

  class JobStateARC1
    : public JobState {
  public:
    JobStateARC1(const std::string& state)
      : JobState(state, &StateMap) {}
    static JobState::StateType StateMap(const std::string& state);
  };

}

#endif // __ARC_JOBSTATEARC1_H__