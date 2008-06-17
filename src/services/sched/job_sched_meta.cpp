#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <iomanip>
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
    last_checked_ = s.last_checked_;
    last_updated_ = s.last_updated_;
    created_ = s.created_;
    start_time_ = s.start_time_;
    end_time_ = s.end_time_;
}

JobSchedMetaData::JobSchedMetaData(ByteArray &buffer)
{
    int buf_len = 0;
    char *buf = buffer.data();
    time_t t;
    
    resource_id_ = buf;
    buf_len = resource_id_.size() + 1;
    failure_ = buf + buf_len;
    buf_len += failure_.size() + 1;
    
    t = *(time_t *)(buf + buf_len);
    buf_len += sizeof(t);
    last_checked_.SetTime(t);
    
    t = *(time_t *)(buf + buf_len);
    buf_len += sizeof(t);
    last_updated_.SetTime(t);
    
    t = *(time_t *)(buf + buf_len);
    buf_len += sizeof(t);
    created_.SetTime(t);
    
    t = *(time_t *)(buf + buf_len);
    buf_len += sizeof(t);
    start_time_.SetTime(t);
    
    t = *(time_t *)(buf + buf_len);
    buf_len += sizeof(t);
    end_time_.SetTime(t);
}

ByteArray &JobSchedMetaData::serialize(void)
{
    time_t t;
    buffer_.clean();
    buffer_.append(resource_id_);
    buffer_.append(failure_);
    t = last_updated_.GetTime();
    buffer_.append(&t, sizeof(t));
    t = last_checked_.GetTime();
    buffer_.append(&t, sizeof(t));
    t = created_.GetTime();
    buffer_.append(&t, sizeof(t));
    t = start_time_.GetTime();
    buffer_.append(&t, sizeof(t));
    t = end_time_.GetTime();
    buffer_.append(&t, sizeof(t));
    
    return buffer_;
}

JobSchedMetaData::~JobSchedMetaData(void) 
{
    // NOP
}

JobSchedMetaData::operator std::string(void) const
{
    std::string out =   "ResourceID: " + resource_id_ + "\n"
                      + "Failure: " + failure_ + "\n"
                      + "Last Checked: " + (std::string)last_checked_ + "\n"
                      + "Last Updated: " + (std::string)last_updated_ + "\n"
                      + "Created: " + (std::string)created_ + "\n"
                      + "Start Time: " + (std::string)start_time_ + "\n"
                      + "End Time: " + (std::string)end_time_ + "\n";
    return out;
}

} //namespace arc
