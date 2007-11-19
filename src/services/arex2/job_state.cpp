#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "job_state.h"

namespace ARex2
{

JobState::JobState(job_state_t s)
{
    state = s;
}

}; // namespace ARex2

