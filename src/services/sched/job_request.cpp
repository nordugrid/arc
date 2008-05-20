#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "job_request.h"

namespace Arc {

JobRequest::JobRequest():buffer_(0) 
{
    // NOP
}

JobRequest::~JobRequest() 
{
    // NOP
}

JobRequest::JobRequest(Arc::XMLNode &r):buffer_(0)
{
    r.New(request_);
}

JobRequest::JobRequest(const JobRequest &r):buffer_(0)
{
    r.request_.New(request_);
}

JobRequest::JobRequest(ByteArray &buffer)
{
    int buf_len = 0;
    char *buf = buffer.data();
    std::string xml_str = buf;
    buf_len = xml_str.size() + 1;
    Arc::XMLNode n(xml_str);
    n.New(request_);
}

ByteArray &JobRequest::serialize(void)
{
    buffer_.clean();
    std::string xml_str;
    request_.GetXML(xml_str);
    buffer_.append(xml_str);
    return buffer_;
}

JobRequest::operator std::string(void) const
{
    std::string xml_str;
    request_.GetXML(xml_str);
    std::string out = "Request XML:\n" + xml_str + "\n";
    return out;
}

} //namespace
