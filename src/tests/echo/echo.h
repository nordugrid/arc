#ifndef __ARC_ECHO_H__
#define __ARC_ECHO_H__

#include <map>
#include <arc/infosys/RegisteredService.h>
#include <arc/Logger.h>
#include <arc/security/PDP.h>
#include <arc/infosys/InformationInterface.h>

namespace Echo {

/** This is a test service which accepts SOAP requests and produces response
 as described in echo.wsdl. Response contains string passed in request with 
 prefix_ and suffix_ added. */

/** About the policy decision, here the echo service is used as an example to demostrate how to 
* implement and deploy it. 
* For service developer, he is supposed to marshall the pdp request into a internal structure
* For service deployer, he is supposed to do the following two things:
* a, write the policy according to its requirement, and based on the Policy.xsd schema.
* b, configure the service.xml, put the pdp configuration into a <SecHandler/>
     <PDP name="arc.pdp" policylocation="Policy_Example.xml"/>
     The "name" attribute is the identifier for dynamic loading the ArcPDP object.
     The "policylocation" attribute is for the configuration of ArcPDP's policy 
*/

class Service_Echo: public Arc::RegisteredService
{
    protected:
        std::string prefix_;
        std::string suffix_;
        std::string serviceid_;
        std::string endpoint_;
        std::string expiration_;
        std::string policylocation_;
        Arc::NS ns_;
        Arc::MCC_Status make_fault(Arc::Message& outmsg,const std::string& txtmsg = "");
        Arc::Logger logger;
        Arc::InformationContainer infodoc;
    public:
        /** Constructor accepts configuration describing content of prefix and suffix */
        Service_Echo(Arc::Config *cfg);
        virtual ~Service_Echo(void);
        /** Service request processing routine */
        virtual Arc::MCC_Status process(Arc::Message&,Arc::Message&);

        bool RegistrationCollector(Arc::XMLNode &doc);
};

} // namespace Echo

#endif /* __ARC_ECHO_H__ */
