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
}
