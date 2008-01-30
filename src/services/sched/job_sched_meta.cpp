#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "job_sched_meta.h"

namespace GridScheduler
{

JobSchedMetaData::JobSchedMetaData(void)
{
    reruns = 0;
}

JobSchedMetaData::~JobSchedMetaData(void)
{
    // nop
}

};
