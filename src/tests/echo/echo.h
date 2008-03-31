#ifndef __ARC_ECHO_H__
#define __ARC_ECHO_H__

#include <map>
#include <arc/message/Service.h>
#include <arc/Logger.h>
#include <arc/security/PDP.h>

namespace Echo {

/** This is a test service which accepts SOAP requests and produces response
 as described in echo.wsdl. Response contains string passed in request with 
 prefix_ and suffix_ added. */
class Service_Echo: public Arc::Service
{
    protected:
        std::string prefix_;
        std::string suffix_;
        std::map<std::string, std::string> pdpinfo_;
        Arc::NS ns_;
        Arc::MCC_Status make_fault(Arc::Message& outmsg);
	static Arc::Logger logger;
    public:
        /** Constructor accepts configuration describing content of prefix and suffix */
        Service_Echo(Arc::Config *cfg);
        virtual ~Service_Echo(void);
        /**Helper method to store the request and policy information for pdp*/
        ArcSec::PDPConfigContext* get_pdpconfig(Arc::Message& inmsg, std::string& label);
        /** Service request processing routine */
        virtual Arc::MCC_Status process(Arc::Message&,Arc::Message&);
};

} // namespace Echo

#endif /* __ARC_ECHO_H__ */
