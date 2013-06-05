#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "JobState.h"

namespace Arc {
#ifdef JOBSTATE_X
#undef JOBSTATE_X
#endif

const std::string JobState::StateTypeString[] = { "Undefined", "Accepted", "Preparing", "Submitting", "Hold", "Queuing", "Running", "Finishing", "Finished", "Killed", "Failed", "Deleted", "Other" };

JobState::StateType JobState::GetStateType(const std::string& stateStr) {
    if (stateStr == "Accepted") { return ACCEPTED; }
    if (stateStr == "Preparing") { return PREPARING; }
    if (stateStr == "Submitting") { return SUBMITTING; }
    if (stateStr == "Hold") { return HOLD; }
    if (stateStr == "Queuing") { return QUEUING; }
    if (stateStr == "Running") { return RUNNING; }
    if (stateStr == "Finishing") { return FINISHING; }
    if (stateStr == "Finished") { return FINISHED; }
    if (stateStr == "Killed") { return KILLED; }
    if (stateStr == "Failed") { return FAILED; }
    if (stateStr == "Deleted") { return DELETED; }
    if (stateStr == "Other") { return OTHER; }
    return UNDEFINED;
}

} // namespace Arc
