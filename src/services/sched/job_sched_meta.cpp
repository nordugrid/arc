#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "job_sched_meta.h"

namespace GridScheduler {

JobSchedMetaData::JobSchedMetaData() {
    reruns = 5;
}

JobSchedMetaData::~JobSchedMetaData(void) {
    // nop
}


JobSchedMetaData::JobSchedMetaData(int& r) {
    reruns = r;
}

bool JobSchedMetaData::setArexID(std::string &id) {
    arex_id = id;
}

/*

JobSchedMetaData::JobSchedMetaData& operator=(const JobSchedMetaData& j) {
    //TODO

}


JobSchedMetaData::JobSchedMetaData(const JobSchedMetaData& j) {

    //TODO

}

*/

}; //namespace
