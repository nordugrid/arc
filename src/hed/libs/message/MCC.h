#ifndef __ARC_MCC_H__
#define __ARC_MCC_H__

#include "common/ArcConfig.h"
#include "Message.h"

namespace Arc {

class AuthNHandler;
class AuthZHandler;

/** This class represents status of Message processing.
  Currently it's just a placeholder for int code with 0 meaning
 there were no errors during processing. It's methods allow
 it to be treated as ordinary int as well.
  The precise meaning of non-zero values and other extensions have to 
 be decided.
  The purpose of such object is to indicate if message was processed
 at endpoint Service or it hasn't reached it due to error in it's 
 path. */
class MCC_Status {
    protected:
        int code_;
    public:
        MCC_Status(int code = 0):code_(code) { };
        operator int(void) { return code_; };
        MCC_Status& operator=(int code) { code_=code; return *this; };
        operator bool(void) { return (code_ == 0); };
        bool operator!(void) { return (code_ != 0); };
};

/** This class defines interface for communication between MCC, Service and Plexer objects.
  Interface is made of method process() which is called by previous MCC in chain. 
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
        virtual  MCC_Status process(Message& request, Message& response)  = 0;
};

/** Message Chain Component - base class for every MCC plugin.
  This is partialy virtual class which defines interface and common 
 functionality for every MCC plugin needed for managing of component
 in a chain. */
class MCC: public MCCInterface
{
    protected:
        /** Set of labeled "next" components. 
          Each implemented MCC must call process() metthod of 
         corresponding MCCInterface from this set in own process() method. */
        std::map<std::string,MCCInterface*> next_;
        MCCInterface* Next(const std::string& label = "");
        /** Set o flabeled authentication and authorization handlers.
          MCC calls sequence of handlers at specific point depending
         on associated identifier. in most aces those are "in" and "out"
         for incoming and outgoing messages correspondingly. */
        std::map<std::string,std::list<AuthNHandler*> > authn_;
        std::map<std::string,std::list<AuthZHandler*> > authz_;
    public:
        /** Example contructor - MCC takes at least it's configuration
	    subtree */
        MCC(Arc::Config *cfg) { };
        virtual ~MCC(void) { };
        /** Add reference to next MCC in chain.
          This method is called by Loader for every potentially labeled link to next 
         component which implements MCCInterface. If next is set NULL corresponding
         link is removed.  */
        virtual void Next(MCCInterface* next,const std::string& label = "");
        virtual void AuthN(AuthNHandler* authn,const std::string& label = "");
        virtual void AuthZ(AuthZHandler* authz,const std::string& label = "");
        /** Removing all links. 
          Useful for destroying chains. */
        virtual void Unlink(void);
        /** Dummy Message processing method. Just a placeholder. */
        virtual  MCC_Status process(Message& request, Message& response) { return MCC_Status(-1); };
};

} // namespace Arc

#endif /* __ARC_MCC_H__ */
