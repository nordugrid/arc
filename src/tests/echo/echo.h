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
        /**Helper method to store the request and policy information for pdp
        *Basically, in ARC1, the authorization decesion is based on each incoming or outgoing message
        * (Here, message is the container for socket stream, soap message, or http message) 
        * Reasonablly, it should be each service which would care the security-related attributes 
        * inside the message (or from other attribute sources) which will be used for authorization decision, 
        * therefore, each service should be responsible for creating request context and 
        * input it to the PDP, specificly, the service is responsible for mapping the valid attributes
        * (such as DN from authentication result, or voms attributes which is signed by some attribute authority) 
        * into the correct format for passing to the PDP, also be responsible for marshalling the attributes that 
        * describe the user's request action, target resouces and any environment attribute, placing these attribute
        * into the formated request which will be sent to PDP.
        * Here, assume the echo service cares about the DN and host address, and HTTP:METHTOD of the incoming requestor,  
        * get_pdpconfig() will get all of the three interested attributes, and marshall them into some common 
        * internal structure which is defined in PDP.h
        * PDPConfigContext is a specific MessageContextElement which is attatched to the message, and acts as a container
        * for the above internal structure, and PDP's policy location as well.
        * The PDP (specificly, here ArcPDP is used) will parse the PDPConfigContext object when it gets the message, then 
        * compose the formated xml structure for pdp request by using the internal structrue inside the PDPConfigContext 
        * object, and make authorization decision according to the formated request and policies.
        */
        ArcSec::PDPConfigContext* get_pdpconfig(Arc::Message& inmsg, std::string& label);
        /** Service request processing routine */
        virtual Arc::MCC_Status process(Arc::Message&,Arc::Message&);
};

} // namespace Echo

#endif /* __ARC_ECHO_H__ */
