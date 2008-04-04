#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "job_status.h"

namespace Paul
{

SchedStatusFactory::SchedStatusFactory(void)
{
    str_map[NEW] = "NEW";
    str_map[STARTING] = "STARTING";
    str_map[RUNNING] = "RUNNING"; 
    str_map[CANCELLED] = "CANCELLED"; 
    str_map[FAILED] = "FAILED"; 
    str_map[FINISHED] = "FINISHED"; 
    str_map[KILLED] = "KILLED"; 
    str_map[KILLING] = "KILLING";
    str_map[UNKNOWN] = "UNKNOWN"; 
}

SchedStatus SchedStatusFactory::get(const std::string &state)
{
    std::map<SchedStatusLevel, std::string>::iterator it;
    for (it = str_map.begin(); it != str_map.end(); it++) {
        if (state == it->second) {
            return SchedStatus(it->first, it->second);
        }
    }
    return get(UNKNOWN);
}

SchedStatus SchedStatusFactory::getFromARexStatus(const std::string &arex_state) 
{
    if (arex_state == "ACCEPTED") {
        return get(STARTING);
    } else if(arex_state == "PREPARING") {
        return get(STARTING);
    } else if(arex_state == "SUBMITING") {
        return get(STARTING);
    } else if(arex_state == "EXECUTING") {
        return get(RUNNING);
    } else if(arex_state == "FINISHING") {
        return get(RUNNING);
    } else if(arex_state == "FINISHED") {
        return get(FINISHED);
    } else if(arex_state == "DELETED") {
        return get(CANCELLED);
    } else if(arex_state == "KILLING") {
        return get(CANCELLED);
    }
    return get(UNKNOWN);
}

SchedStatus SchedStatusFactory::getFromARexStatus(ARexStatusLevel al)
{
    // XXX  missing mapping
    get(UNKNOWN);
}

}; // namespace Paul
