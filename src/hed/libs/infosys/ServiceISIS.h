#ifndef __ARC_SERVICEISIS_H__
#define __ARC_SERVICEISIS_H__

#include <arc/message/Service.h>
#include <arc/ArcConfig.h>
#include <arc/Logger.h>
#include <arc/message/MCC.h>
#include <arc/loader/Plugin.h>
#include <arc/security/SecHandler.h>

namespace Arc {

/// Service - last component in a Message Chain.
/**  This class which defines interface and common functionality for every 
 Service plugin. Interface is made of method process() which is called by 
 Plexer or MCC class.
  There is one Service object created for every service description 
 processed by Loader class objects. Classes derived from Service class 
 must implement process() method of MCCInterface. 
  It is up to developer how internal state of service is stored and 
 communicated to other services and external utilites.
  Service is free to expect any type of payload passed to it and generate
 any payload as well. Useful types depend on MCCs in chain which leads to 
 that service. For example if service is expected to by linked to SOAP
 MCC it must accept and generate messages with PayloadSOAP payload.
  Method process() of class derived from Service class may be called 
 concurently in multiple threads. Developers must take that into account 
 and write thread-safe implementation.
  Simple example of service is provided in /src/tests/echo/echo.cpp of
 source tree.
  The way to write client couterpart of corresponding service is 
 undefined yet. For example see /src/tests/echo/test.cpp .
 */
class ServiceISIS: public Service
{
    public:
        /** Example contructor - Server takes at least it's configuration subtree */
        ServiceISIS(Arc::Config*);

        virtual ~ServiceISIS(void) { };
};


} // namespace Arc

#endif /* __ARC_SERVICEISIS_H__ */
