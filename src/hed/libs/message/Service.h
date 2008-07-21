#ifndef __ARC_SERVICE_H__
#define __ARC_SERVICE_H__

#include <arc/ArcConfig.h>
#include <arc/Logger.h>
#include <arc/message/MCC.h>
#include <arc/security/SecHandler.h>

namespace Arc {

/// Service - last component in a Message Chain.
/**  This is virtual class which defines interface (in a future also 
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
 MCC it must accept and generate messages with PayloadSOAP payload.
  Method process() of class derived from Service class may be called 
 concurently in multiple threads. Developers must take that into account 
 and write thread-safe implementation.
  Simple example of service is provided in /src/tests/echo/echo.cpp of
 source tree.
  The way to write client couterpart of corresponding service is 
 undefined yet. For example see /src/tests/echo/test.cpp .
 */
class Service: public MCCInterface
{
    protected:
        /** Set of labeled authentication and authorization handlers.
        MCC calls sequence of handlers at specific point depending
        on associated identifier. in most aces those are "in" and "out"
        for incoming and outgoing messages correspondingly. */
        std::map<std::string,std::list<ArcSec::SecHandler*> > sechandlers_;
        static Logger logger;

        /** Executes security handlers of specified queue.
          For more information please see description of MCC::ProcessSecHandlers */
        bool ProcessSecHandlers(Arc::Message& message,const std::string& label = "");

    public:
        /** Example contructor - Server takes at least it's configuration subtree */
        Service(Arc::Config*) { };

        virtual ~Service(void) { };

        /** Add security components/handlers to this MCC.
          For more information please see description of MCC::AddSecHandler */
        virtual void AddSecHandler(Arc::Config *cfg,ArcSec::SecHandler* sechandler,const std::string& label = "");
        
        /** Service specific registartion collector, 
            used for generate service registartions */
        virtual bool RegistrationCollector(Arc::XMLNode &doc);

        /** Service may implement own service identitifer gathering method */
        virtual std::string getID() { return ""; };
};

} // namespace Arc

#endif /* __ARC_SERVICE_H__ */
