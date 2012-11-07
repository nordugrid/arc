#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "JobState.h"

namespace Arc {
#ifdef JOBSTATE_X
#undef JOBSTATE_X
#endif

#define JOBSTATE_X(a, b) , b
const std::string JobState::StateTypeString[] = { "Undefined" JOBSTATE_TABLE };
#undef JOBSTATE_X

JobState::StateType JobState::GetStateType(const std::string& stateStr) {
#define JOBSTATE_X(a, b) if (stateStr == b) { return a; }
JOBSTATE_TABLE
return UNDEFINED;
#undef JOBSTATE_X
}
}
