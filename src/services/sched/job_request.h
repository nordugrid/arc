#ifndef __ARC_JOB_REQUEST_H__
#define __ARC_JOB_REQUEST_H__

#include <string>
#include <arc/XMLNode.h>
#include <arc/ByteArray.h>

namespace Arc
{

class JobRequest {

    private:
        Arc::XMLNode request_;
        // for serialization
        ByteArray buffer_;        
    public:
        JobRequest();
        JobRequest(const JobRequest &r); // copy constructor
        JobRequest(Arc::XMLNode &d);
        JobRequest(ByteArray &buffer); // unserialization
        ByteArray &serialize(void);
        ~JobRequest(void);
        Arc::XMLNode &getJSDL(void) { return request_; };
        Arc::XMLNode operator[](const std::string &key) { return request_[key]; };
        operator std::string(void) const;
};

}

#endif // __ARC_JOB_REQUEST__
