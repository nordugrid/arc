#ifndef __ARC_JOBSTATEBES_H__
#define __ARC_JOBSTATEBES_H__

#include <arc/client/JobState.h>

namespace Arc {

  class JobStateBES
    : public JobState {
  public:
    JobStateBES(const std::string& state)
      : JobState(state, &StateMap) {}
    static JobState::StateType StateMap(const std::string& state);
  };

}

#endif // __ARC_JOBSTATEBES_H__
