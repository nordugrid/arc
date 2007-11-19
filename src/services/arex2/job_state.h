#ifndef __AREX2_JOB_STATE_H__
#define __AREX2_JOB_STATE_H__

#include <string>

namespace ARex2 
{

// Job States
typedef enum {
  JOB_STATE_ACCEPTED  = 0,
  JOB_STATE_PREPARING = 1,
  JOB_STATE_SUBMITING = 2,
  JOB_STATE_INLRMS    = 3,
  JOB_STATE_FINISHING = 4,
  JOB_STATE_FINISHED  = 5,
  JOB_STATE_DELETED   = 6,
  JOB_STATE_CANCELING = 7,
  JOB_STATE_UNDEFINED = 8
} job_state_t;

/** Represents the state of job. It includes error messages as well */
class JobState
{
    protected:
        job_state_t state;
        /* explanation of job's failure */
        std::string failure_reason;
    public:
        JobState(job_state_t s);
        ~JobState(void);
        const std::string &GetFailure(void) { return failure_reason; };

};

}; // namespace ARex2

#endif

