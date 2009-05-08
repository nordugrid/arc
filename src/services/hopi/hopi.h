#ifndef __ARC_Hopi_H__
#define __ARC_Hopi_H__

#include <arc/infosys/RegisteredService.h>
#include <arc/message/Message.h>
#include <arc/Logger.h>
#include <arc/XMLNode.h>
#include <string>

namespace Hopi {

class Hopi: public Arc::RegisteredService {
    protected:
        std::string doc_root;
        bool slave_mode;
        Arc::PayloadRawInterface* Get(const std::string &path, const std::string &base_url);
        Arc::MCC_Status Put(const std::string &path, Arc::MessagePayload &buf);
    public:
        static Arc::Logger logger;
        Hopi(Arc::Config *cfg);
        virtual ~Hopi(void);
        virtual Arc::MCC_Status process(Arc::Message &inmsg, Arc::Message &outmsg);
        bool RegistrationCollector(Arc::XMLNode &doc);
};

} // namespace Hopi

#endif // __ARC_Hopi_H__
