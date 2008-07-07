#ifndef __ARC_HTTPD_H__
#define __ARC_HTTPD_H__

#include <arc/message/Service.h>
#include <arc/message/PayloadRaw.h>
#include <arc/Logger.h>
#include <string>

namespace HTTPD {

class HTTPD: public Arc::Service {
    protected:
        Arc::Logger logger;
        std::string doc_root;
        std::string slave_mode;
        std::string path_prefix;
        Arc::PayloadRawInterface* Get(const std::string &path);
        Arc::MCC_Status Put(const std::string &path, Arc::PayloadRawInterface &buf);
    public:
        HTTPD(Arc::Config *cfg);
        virtual ~HTTPD(void);
        virtual Arc::MCC_Status process(Arc::Message &inmsg, Arc::Message &outmsg);
};

} // namespace HTTPD

#endif // __ARC_HTTPD_H__
