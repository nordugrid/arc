#ifndef __ARC_MCC_H__
#define __ARC_MCC_H__

#include <list>
#include <map>
#include <arc/ArcConfig.h>
#include <arc/Logger.h>
#include <arc/security/SecHandler.h>
#include "Message.h"
#include "MCC_Status.h"

namespace Arc {

/// Interface for communication between MCC, Service and Plexer objects.
/** The Interface is made of method process() which is called by previous MCC in chain. 
  For memory management policies please read description of Message class. */
class MCCInterface
{
    public:
        /** Method for processing of requests and responses.
           This method is called by preceeding MCC in chain when a request 
           needs to be processed.
           This method must call similar method of next MCC in chain unless 
          any failure happens. Result returned by call to next MCC should
          be processed and passed back to previous MCC.
           In case of failure this method is expected to generate valid 
          error response and return it back to previous MCC without calling
          the next one.
          \param request The request that needs to be processed.
          \param response A Message object that will contain the response
          of the request when the method returns.
          \return An object representing the status of the call.
        */
        virtual Arc::MCC_Status process(Arc::Message& request, Arc::Message& response) = 0;
        virtual ~MCCInterface() {};
};

/// Message Chain Component - base class for every MCC plugin.
/** This is partialy virtual class which defines interface and common 
 functionality for every MCC plugin needed for managing of component
 in a chain. */
class MCC: public Arc::MCCInterface
{
 protected:
  /** Set of labeled "next" components. 
      Each implemented MCC must call process() metthod of 
      corresponding MCCInterface from this set in own process() method. */
  std::map<std::string,Arc::MCCInterface*> next_;
  Arc::MCCInterface* Next(const std::string& label = "");
  
  /** Set o flabeled authentication and authorization handlers.
      MCC calls sequence of handlers at specific point depending
      on associated identifier. in most aces those are "in" and "out"
      for incoming and outgoing messages correspondingly. */
  std::map<std::string,std::list<ArcSec::SecHandler*> > sechandlers_;
  
  //! A logger for MCCs.
  /*! A logger intended to be the parent of loggers in the different
    MCCs.
  */
  static Arc::Logger logger;
  
    public:
        /** Example contructor - MCC takes at least it's configuration
            subtree */
        MCC(Arc::Config*) { };
        virtual ~MCC(void) { };
        /** Add reference to next MCC in chain.
          This method is called by Loader for every potentially labeled link to next 
         component which implements MCCInterface. If next is set NULL corresponding
         link is removed.  */
        
        virtual void Next(Arc::MCCInterface* next,const std::string& label = "");
        
        /** SecHandler */
        virtual void AddSecHandler(Arc::Config *cfg,ArcSec::SecHandler* sechandler,const std::string& label = "");
        
        /** Removing all links. Useful for destroying chains. */
        virtual void Unlink(void);
        /** Dummy Message processing method. Just a placeholder. */
        virtual Arc::MCC_Status process(Arc::Message&, Arc::Message&) { return MCC_Status(); };

};

} // namespace Arc

#endif /* __ARC_MCC_H__ */
