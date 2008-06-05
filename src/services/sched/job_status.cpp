#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "job_status.h"

namespace Arc
{

std::string sched_status_to_string(SchedJobStatus l)
{
    if (l == JOB_STATUS_SCHED_NEW) {
        return "NEW";
    } else if (l == JOB_STATUS_SCHED_STARTING) {
        return "STARTING";
    } else if (l == JOB_STATUS_SCHED_RUNNING) {
        return "RUNNING";
    } else if (l == JOB_STATUS_SCHED_CANCELLED) {
        return "CANCELLED";
    } else if (l == JOB_STATUS_SCHED_FAILED) {
        return "FAILED";
    } else if (l == JOB_STATUS_SCHED_FINISHED) {
        return "FINISHED";
    } else if (l == JOB_STATUS_SCHED_KILLED) {
        return "KILLED";
    } else if (l == JOB_STATUS_SCHED_KILLING) {
        return "KILLING";
    } 

    return "UNKNOWN";
}

SchedJobStatus sched_status_from_string(const std::string &s)
{
    if (s == "NEW") { 
        return JOB_STATUS_SCHED_NEW;
    } else if (s == "STARTING") {
        return JOB_STATUS_SCHED_STARTING;
    } else if (s == "RUNNING") {
        return JOB_STATUS_SCHED_RUNNING;
    } else if (s == "CANCELLED") {
        return JOB_STATUS_SCHED_CANCELLED;
    } else if (s == "FAILED") { 
        return JOB_STATUS_SCHED_FAILED;
    } else if (s == "FINISHED") {
        return JOB_STATUS_SCHED_FINISHED;
    } else if (s == "KILLED") {
        return JOB_STATUS_SCHED_KILLED;
    } else if (s == "KILLING")  {
        return JOB_STATUS_SCHED_KILLING;
    }

    return JOB_STATUS_SCHED_UNKNOWN;
}

ARexJobStatus arex_status_from_string(const std::string&)
{
    return JOB_STATUS_AREX_UNKNOWN;
}

PaulJobStatus paul_status_from_string(const std::string&)
{
    return JOB_STATUS_PAUL_UNKNOWN;
}

} // namespace Arc
