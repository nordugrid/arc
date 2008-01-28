#ifndef __ARC_COUNT_H__
#define __ARC_COUNT_H__

#include <arc/message/Service.h>
#include <arc/Logger.h>

namespace Count {

class Service_Count: public Arc::Service
{
    protected:
        Arc::NS ns_;
        Arc::MCC_Status make_fault(Arc::Message& outmsg);
	static Arc::Logger logger;
    public:
        /** Constructor for accepting configuration */
        Service_Count(Arc::Config *cfg);
        virtual ~Service_Count(void);
        /** Service request processing routine */
        virtual Arc::MCC_Status process(Arc::Message&,Arc::Message&);
};

} // namespace Count

#endif /* __ARC_COUNT_H__ */
