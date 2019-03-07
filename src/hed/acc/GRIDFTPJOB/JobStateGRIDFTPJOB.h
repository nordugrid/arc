#ifndef __ARC_JOBSTATEGRIDFTPJOB_H__
#define __ARC_JOBSTATEGRIDFTPJOB_H__

#include <arc/compute/JobState.h>

namespace Arc {

  class JobStateGRIDFTPJOB
    : public JobState {
  public:
    JobStateGRIDFTPJOB(const std::string& state)
      : JobState(state, &StateMap) {}
    static JobState::StateType StateMap(const std::string& state);
  };

}

#endif // __ARC_JOBSTATEGRIDFTPJOB_H__
