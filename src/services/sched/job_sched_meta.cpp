#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "job_sched_meta.h"

namespace Arc {

JobSchedMetaData::JobSchedMetaData():buffer_(0)
{
    // NOP;
}

JobSchedMetaData::JobSchedMetaData(const JobSchedMetaData &s):buffer_(0)
{
    resource_id_ = s.resource_id_;
    failure_ = s.failure_;
}

JobSchedMetaData::JobSchedMetaData(ByteArray &buffer)
{
    int buf_len = 0;
    char *buf = buffer.data();

    resource_id_ = buf;
    buf_len = resource_id_.size() + 1;
    failure_ = buf + buf_len;
    buf_len = failure_.size() + 1;
}

ByteArray &JobSchedMetaData::serialize(void)
{
    buffer_.clean();
    buffer_.append(resource_id_);
    buffer_.append(failure_);
    return buffer_;
}

JobSchedMetaData::~JobSchedMetaData(void) 
{
    // NOP
}

} //namespace arc
