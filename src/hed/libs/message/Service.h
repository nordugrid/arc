#ifndef __ARC_SERVICE_H__
#define __ARC_SERVICE_H__

#include "common/ArcConfig.h"
#include "../message/MCC.h"

namespace Arc {

/** Service - last plugin in a Message Chain.
  This is virtual class which defines interface (in a future also 
 common functionality) for every Service plugin. Interface is made of 
 method process() which is called by Plexer or MCC class.
  There is one Service object created for every service description 
 processed by Loader class objects. Classes derived from Service class 
 must implement process() method of MCCInterface. 
  It is up to developer how internal state of service is stored and 
 communicated to other services and external utilites.
  Service is free to expect any type of payload passed to it and generate
 any payload as well. Useful types depend on MCCs in chain which leads to 
 that service. For example if service is expected to by linked to SOAP
 MCC it must accespt and generate messages with PayloadSOAP payload.
  Method process() of class derived from Service class may be called 
 concurently in multiple threads. Developers must take that into account 
 and write thread-safe implementation.
  Simple example of service is provided in /src/tests/echo/echo.cpp .
  The way to write client couterpart of corresponding service is 
 undefined. For example see /src/tests/echo/test.cpp .
 */
class Service: public MCCInterface
{
    protected:
        std::map<std::string,std::list<AuthNHandler*> > authn_;
        std::map<std::string,std::list<AuthZHandler*> > authz_;
    public:
        /** Example contructor - Server takes at least it's configuration subtree */
        Service(Arc::Config *cfg) { };
        virtual ~Service(void) { };
        virtual void AuthN(AuthNHandler* authn,const std::string& label = "");
        virtual void AuthZ(AuthZHandler* authz,const std::string& label = "");
};

} // namespace Arc

#endif /* __ARC_SERVICE_H__ */
