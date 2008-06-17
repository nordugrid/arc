#ifndef SCHED_METADATA
#define SCHED_METADATA

#include <arc/ByteArray.h>
#include <string>
#include <arc/DateTime.h>

namespace Arc {

class JobSchedMetaData {

    private:
        std::string resource_id_;
        std::string failure_;
        Arc::Time last_checked_;
        Arc::Time last_updated_;
        Arc::Time created_;
        Arc::Time start_time_;
        Arc::Time end_time_;
        // for serialization
        ByteArray buffer_;
    public:
        JobSchedMetaData(void);
        JobSchedMetaData(const JobSchedMetaData &s); // copy constructor
        JobSchedMetaData(ByteArray &buffer); // unserialize
        ByteArray &serialize(void);
        ~JobSchedMetaData(void);
        void setResourceID(const std::string &id) { resource_id_ = id; };
        const std::string& getResourceID(void) { return resource_id_; };
        void setFailure(const std::string &f) { failure_ = f; };
        const std::string& getFailure(void) { return failure_; };
        void setLastChecked(const Arc::Time &last_checked) { last_checked_ = last_checked; };
        Arc::Time &getLastChecked(void) { return last_checked_; };
        void setLastUpdated(const Arc::Time &last_updated) { last_updated_ = last_updated; };
        Arc::Time &getLastUpdated(void) { return last_updated_; };
        void setCreatedTime(const Arc::Time &created) { created_ = created; };
        Arc::Time &getCreatedTime(void) { return created_; };
        void setStartTime(const Arc::Time &start_time) { start_time_ = start_time; };
        Arc::Time &getStartTime(void) { return start_time_; };
        void setEndTime(const Arc::Time &end_time) { end_time_ = end_time; };
        Arc::Time &getEndTime(void) { return end_time_; };
        operator std::string(void) const;
};

} // namespace Arc

#endif // SCHED_METADATA
