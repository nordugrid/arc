#ifndef __ARC_SP_H__
#define __ARC_SP_H__

#include <map>
#include <arc/message/Service.h>
#include <arc/Logger.h>

namespace SPService {

/** This is service which accepts HTTP request from user agent (web browser) in the 
 * client side and processes the same functionality as Service Provider in SAML2 SSO
 * profile, including composing <AuthnRequest/>, verifying saml response, etc.
 */

class Service_SP: public Arc::Service
{
    protected:
        Arc::NS ns_;
        Arc::MCC_Status make_fault(Arc::Message& outmsg);
        Arc::Logger logger;
        Arc::XMLNode metadata_node_;
    public:
        /** Constructor*/
        Service_SP(Arc::Config *cfg);
        virtual ~Service_SP(void);
        /** Service request processing routine */
        virtual Arc::MCC_Status process(Arc::Message&,Arc::Message&);
};

} // namespace SPService

#endif /* __ARC_SP_H__ */
