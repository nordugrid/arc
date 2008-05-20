#ifndef SCHED_METADATA
#define SCHED_METADATA

#include <arc/ByteArray.h>
#include <string>

namespace Arc {

class JobSchedMetaData {

    private:
        std::string resource_id_;
        std::string failure_;
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
        operator std::string(void) const;
};

} // namespace Arc

#endif // SCHED_METADATA
