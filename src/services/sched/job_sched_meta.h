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
        std::string arex_id;
    public:
        JobSchedMetaData();
        JobSchedMetaData(int& r);
        virtual ~JobSchedMetaData(void);
        bool setArexID(std::string &id);
        std::string& getArexID() { return arex_id;};
       // JobSchedMetaData& operator=(const JobSchedMetaData& j);
       // JobSchedMetaData( const JobSchedMetaData& j );
};

}; // namespace Arc

#endif // SCHED_METADATA
