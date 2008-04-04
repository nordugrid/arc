#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "job_sched_meta.h"

namespace Paul {

JobSchedMetaData::JobSchedMetaData() 
{
    reruns = 5;
}

JobSchedMetaData::~JobSchedMetaData(void) 
{
    // nop
}


JobSchedMetaData::JobSchedMetaData(int r) 
{
    reruns = r;
}


} //namespace
