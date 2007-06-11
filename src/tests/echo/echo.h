#ifndef __ARC_ECHO_H__
#define __ARC_ECHO_H__

#include "../../hed/libs/message/Service.h"

namespace Echo {

/** This is a test service which accepts SOAP requests and produces response
 as described in echo.wsdl. Response contains string passed in request with 
 prefix_ and suffix_ added. */
class Service_Echo: public Arc::Service
{
    protected:
        std::string prefix_;
        std::string suffix_;
        Arc::NS ns_;
        Arc::MCC_Status make_fault(Arc::Message& outmsg);
    public:
        /** Constructor accepts configuration describing content of prefix and suffix */
        Service_Echo(Arc::Config *cfg);
        virtual ~Service_Echo(void);
        /** Service request processing routine */
        virtual Arc::MCC_Status process(Arc::Message&,Arc::Message&);
};

} // namespace Echo

#endif /* __ARC_ECHO_H__ */
