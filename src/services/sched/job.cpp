#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/GUID.h>
#include "job.h"

namespace Arc {

Job::Job(void):buffer_(0)
{
    // NOP
}

Job::Job(JobRequest &r, JobSchedMetaData &m):buffer_(0)
{
    id_ = Arc::UUID();
    request_ = new JobRequest(r);
    sched_meta_ = new JobSchedMetaData(m);
    status_ = JOB_STATUS_SCHED_NEW;
}

Job::Job(ByteArray &buffer)
{
    size_t buf_len = 0;
    char *buf = buffer.data();
    
    // get id
    id_ = buf;
    buf_len = id_.size() + 1;
    // get status
    status_ = *((SchedJobStatus *)(buf + buf_len));
    buf_len += sizeof(SchedJobStatus);
    // get request class
    size_t bs = *((size_t *)(buf + buf_len));
    buf_len += sizeof(size_t);
    ByteArray a1(bs);
    a1.append(buf + buf_len, bs);
    buf_len += bs;
    request_ = new JobRequest(a1);
    // get sched meta class
    bs = *((size_t *)(buf + buf_len));
    buf_len += sizeof(size_t);
    ByteArray a2(bs);
    a2.append(buf + buf_len, bs);
    buf_len += bs;
    sched_meta_ = new JobSchedMetaData(a2);
}

ByteArray &Job::serialize(void)
{
    buffer_.clean();
    buffer_.append(id_);
    buffer_.append(&status_, sizeof(SchedJobStatus));
    ByteArray &a1 = request_->serialize();
    buffer_.append(a1);
    ByteArray &a2 = sched_meta_->serialize();
    buffer_.append(a2);
    return buffer_;
}

Job::~Job(void) 
{
    delete request_;
    delete sched_meta_;
}

Job::operator std::string(void) const
{
    std::string out =   "ID: " + id_ + "\n"
                      + "Status: " + sched_status_to_string(status_) + "\n"
                      + (std::string)*request_ 
                      + (std::string)*sched_meta_;
    return out;
}

}
