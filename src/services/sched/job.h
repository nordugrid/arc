#ifndef SCHED_JOB
#define SCHED_JOB

#include <string>

#include <arc/ByteArray.h>

#include "job_request.h"
#include "job_sched_meta.h"
#include "job_status.h"

namespace Arc {

class Job {

    private:
        std::string id_;
        JobRequest *request_;
        JobSchedMetaData *sched_meta_;
        SchedJobStatus status_;
        // for serialization
        ByteArray buffer_;
    public:
        Job(void);
        Job(JobRequest &d, JobSchedMetaData &m);
        Job(ByteArray &buf); // unserialize
        ByteArray &serialize(void);
        ~Job(void);
        const std::string &getID(void) { return id_; };
        void setFailure(const std::string &f) { sched_meta_->setFailure(f); }
        const std::string &getFailure(void) { return sched_meta_->getFailure(); };
        void setStatus(SchedJobStatus s) { status_ = s; };
        SchedJobStatus getStatus(void) { return status_; };
        Arc::XMLNode &getJSDL(void) { return request_->getJSDL(); };
        JobSchedMetaData *getJobSchedMetaData(void) { return sched_meta_; };
        operator bool(void) { return (id_.empty() ? false : true ); };
        bool operator!(void) { return (id_.empty() ? true : false ); };
        operator std::string(void) const;
};

} // namespace Arc

#endif // SCHED_JOB
