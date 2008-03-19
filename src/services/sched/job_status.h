#ifndef __ARC_JOB_STATUS_H__
#define __ARC_JOB_STATUS_H__

#include <map>
#include <string>
#include <arc/StringConv.h>

namespace GridScheduler
{

enum SchedStatusLevel {
    NEW, 
    STARTING, 
    RUNNING, 
    CANCELLED, 
    FAILED, 
    FINISHED, 
    KILLED, 
    KILLING,
    UNKNOWN 
};

enum ARexStatusLevel 
{
    ACCEPTING, 
    ACCEPTED, 
    PREPARING, 
    PREPARED, 
    SUBMITTING, 
    EXECUTING, 
    AREX_KILLING, 
    EXECUTED, 
    FINISHING, 
    AREX_FAILED, 
    HELD,
    AREX_UNKNOWN
};

class SchedStatus
{
    private:
        SchedStatusLevel level;
        std::string level_str;
    public:
        SchedStatus(void) { level = UNKNOWN; level_str = "UNKONWN"; };
        SchedStatus(SchedStatusLevel l, const std::string &str);
        operator std::string(void) const { return level_str; };
        bool operator==(SchedStatusLevel l) const { return (l == level); };
        bool operator==(const SchedStatus &s) const { return (s.level == level); };
        bool operator==(const std::string &s) const { return (Arc::upper(s) == level_str); };
        bool operator!=(SchedStatusLevel l) const { return (l != level); };
        bool operator!=(const SchedStatus &s) const { return (s.level != level); };
        bool operator!=(const std::string &s) const { return (Arc::upper(s) != level_str); };
};

class SchedStatusFactory
{
    private:
        std::map<SchedStatusLevel, std::string> str_map;
    public:
        SchedStatusFactory(void);
        SchedStatus get(SchedStatusLevel l) { return SchedStatus(l, str_map[l]);};
        SchedStatus get(const std::string &str);
        SchedStatus getFromARexStatus(ARexStatusLevel l);
        SchedStatus getFromARexStatus(const std::string &str);
}; // sched status factory

} // namespace Paul

#endif
