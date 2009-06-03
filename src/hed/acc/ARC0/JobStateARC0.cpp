#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "JobStateARC0.h"

namespace Arc {

JobState::StateType JobStateARC0::StateMap(const std::string& state)
{
  if (state == "ACCEPTED") return JobState::ACCEPTED;
  else if (state == "PREPARING") return JobState::PREPARING;
  else if (state == "SUBMIT") return JobState::SUBMITTING;
  else if (state == "INLRMS:Q") return JobState::QUEUING;
  else if (state == "INLRMS:R") return JobState::RUNNING;
  else if (state == "FINISHING") return JobState::FINISHING;
  else if (state == "FINISHED") return JobState::FINISHED;
  else if (state == "KILLED") return JobState::KILLED;
  else if (state == "FAILED") return JobState::FAILED;
  else if (state == "DELETED") return JobState::DELETED;
  else return JobState::UNDEFINED;
};

}
