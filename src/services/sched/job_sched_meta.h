#ifndef SCHED_SCHED_METADATA
#define SCHED_SCHED_METADATA

#include <string>
#include <list>
#include <arc/XMLNode.h>
#include <arc/DateTime.h>

namespace Arc
{

class JobSchedMetaData {

    private:
        int reruns;
        Arc::Time start_time;
        Arc::Time end_time;
    public:
        JobSchedMetaData(void);
        virtual ~JobSchedMetaData(void);
};

}; // namespace Arc

#endif // SCHED_SCHED_METADATA
