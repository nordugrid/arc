#ifndef __ARC_Hopi_H__
#define __ARC_Hopi_H__

#include <arc/message/Service.h>
#include <arc/message/PayloadRaw.h>
#include <arc/Logger.h>
#include <string>

namespace Hopi {

class Hopi: public Arc::Service {
    protected:
        Arc::Logger logger;
        std::string doc_root;
        std::string slave_mode;
        Arc::PayloadRawInterface* Get(const std::string &path, const std::string &base_url);
        Arc::MCC_Status Put(const std::string &path, Arc::PayloadRawInterface &buf);
    public:
        Hopi(Arc::Config *cfg);
        virtual ~Hopi(void);
        virtual Arc::MCC_Status process(Arc::Message &inmsg, Arc::Message &outmsg);
};

} // namespace Hopi

#endif // __ARC_Hopi_H__
