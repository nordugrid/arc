#ifndef __ARC_ECHO_H__
#define __ARC_ECHO_H__

#include "../../hed/libs/loader/Service.h"

namespace Echo {

class Service_Echo: public Arc::Service
{
    protected:
        std::string prefix_;
        std::string suffix_;
        Arc::XMLNode::NS ns_;
        Arc::MCC_Status make_fault(Arc::Message& outmsg);
    public:
        Service_Echo(Arc::Config *cfg);
        virtual ~Service_Echo(void);
        virtual Arc::MCC_Status process(Arc::Message&,Arc::Message&);
};

} // namespace Echo

#endif /* __ARC_ECHO_H__ */
