#ifndef __ARC_JOB_STATUS_H__
#define __ARC_JOB_STATUS_H__

#include <string>
#include <arc/StringConv.h>

namespace Arc
{

enum SchedJobStatus {
    JOB_STATUS_SCHED_NEW, 
    JOB_STATUS_SCHED_STARTING, 
    JOB_STATUS_SCHED_RUNNING, 
    JOB_STATUS_SCHED_CANCELLED, 
    JOB_STATUS_SCHED_FAILED, 
    JOB_STATUS_SCHED_FINISHED, 
    JOB_STATUS_SCHED_KILLED, 
    JOB_STATUS_SCHED_KILLING,
    JOB_STATUS_SCHED_UNKNOWN 
};

enum ARexJobStatus
{
    JOB_STATUS_AREX_ACCEPTING, 
    JOB_STATUS_AREX_ACCEPTED, 
    JOB_STATUS_AREX_PREPARING, 
    JOB_STATUS_AREX_PREPARED, 
    JOB_STATUS_AREX_SUBMITTING, 
    JOB_STATUS_AREX_EXECUTING, 
    JOB_STATUS_AREX_KILLING, 
    JOB_STATUS_AREX_EXECUTED, 
    JOB_STATUS_AREX_FINISHING, 
    JOB_STATUS_AREX_FAILED, 
    JOB_STATUS_AREX_HELD,
    JOB_STATUS_AREX_UNKNOWN
};

enum PaulJobStatus
{
    JOB_STATUS_PAUL_NEW,
    JOB_STATUS_PAUL_STARTING,
    JOB_STATUS_PAUL_STAGEIN,
    JOB_STATUS_PAUL_RUNNING,
    JOB_STATUS_PAUL_STAGEOUT,
    JOB_STATUS_PAUL_FAILDE,
    JOB_STATUS_PAUL_KILLING,
    JOB_STATUS_PAUL_KILLED,
    JOB_STATUS_PAUL_FINISHED,
    JOB_STATUS_PAUL_UNKNOWN
};

std::string sched_status_to_string(SchedJobStatus l);
SchedJobStatus sched_status_from_string(const std::string &s);
PaulJobStatus paul_status_from_string(const std::string &s);
ARexJobStatus arex_status_from_string(const std::string &s);

} // namespace arc

#endif
