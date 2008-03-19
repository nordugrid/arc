#ifndef SCHED_METADATA
#define SCHED_METADATA

#include <string>
#include <list>
#include <arc/XMLNode.h>
#include <arc/DateTime.h>

namespace GridScheduler {

class JobSchedMetaData {

    private:
        int reruns;
        Arc::Time start_time;
        Arc::Time end_time;
        unsigned last_check_time;
        int timeout;
        std::map<std::string,std::string> data; //scheduling data
        std::map<std::string,std::string> blacklisted_hosts; //host names
        std::string resource_id;
        std::string resource_job_id;
        std::string failure;
    public:
        JobSchedMetaData();
        JobSchedMetaData(int r);
        virtual ~JobSchedMetaData(void);
        void setResourceID(const std::string &id) { resource_id = id; };
        const std::string& getResourceID(void) { return resource_id; };
        void setResourceJobID(const std::string &id) { resource_job_id = id; };
        const std::string& getResourceJobID(void) { return resource_job_id; };
        const std::string& getFailure(void) { return failure; };
};

} // namespace Arc

#endif // SCHED_METADATA
