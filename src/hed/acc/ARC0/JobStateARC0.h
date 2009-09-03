#ifndef __ARC_JOBSTATEARC0_H__
#define __ARC_JOBSTATEARC0_H__

#include <arc/client/JobState.h>

namespace Arc {

  class JobStateARC0
    : public JobState {
  public:
    JobStateARC0(const std::string& state)
      : JobState(state, &StateMap) {}
    static JobState::StateType StateMap(const std::string& state);
  };

}

#endif // __ARC_JOBSTATEARC0_H__
