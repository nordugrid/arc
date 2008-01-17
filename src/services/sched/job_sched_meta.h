#ifndef SCHED_SCHED_METADATA
#define SCHED_SCHED_METADATA

#include <string>
#include <list>
#include <arc/XMLNode.h>
#include <arc/DateTime.h>

class JobSchedMetaData {

    private:
        int reruns;
        Arc::Time start_time;
        Arc::Time end_time;
    public:
        JobSchedMetaData();
        virtual JobSchedMetaData& operator=(const JobSchedMetaData& j);
        virtual ~JobSchedMetaData(void);
};

#endif // SCHED_SCHED_METADATA
