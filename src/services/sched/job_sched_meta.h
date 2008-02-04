#ifndef SCHED_METADATA
#define SCHED_METADATA

#include <string>
#include <list>
#include <arc/XMLNode.h>
#include <arc/DateTime.h>

namespace GridScheduler
{

class JobSchedMetaData {

    private:
        int reruns;
        Arc::Time start_time;
        Arc::Time end_time;
        std::map<std::string,std::string> data; //scheduling data
        std::map<std::string,std::string> blacklisted_hosts; //host names
    public:
        JobSchedMetaData();
        JobSchedMetaData(int& r);
        virtual ~JobSchedMetaData(void);
};

}; // namespace Arc

#endif // SCHED_METADATA
