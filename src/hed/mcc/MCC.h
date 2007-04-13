#ifndef __ARC_MCC_H__
#define __ARC_MCC_H__

#include "common/ArcConfig.h"
#include "../Message.h"

namespace Arc {

/** This class defines interface for communication between MCC, Service and Plexer objects.
  Interface is made of method process() which is called by previous MCC in chain. */
class MCCInterface
{
    public:
        /** Method for processing of information.
           This method is called by previos MCC in chain.
           This method must call similar method of next MCC in chain unless 
          any failure happens. Result returned by call to next MCC should
          be processed and passed back to previous MCC.
           In case of failure this method is expected to generate valid 
          error response and return it back to previous MCC without calling
          the next one. */
        virtual  Message process(Message msg)  = 0;
/*
    //! The method that processes an incoming request
    *! This method is called by the preceeding MCC in a chain when an
      incoming request needs to be processed.
      The advantage of sending out the response through a reference
      parameter is that no new Message object is created, as would be
      the case if the response was sent as a return value. The problem
      with creating new message objects is that it either involves a
      shallow copy of the payload (which may result in memory leaks or
      "dangling" pointers when the copy is deallocated) or a deep copy
      of the payload (which may be an expensive operation or even
      impossible in case of streams).
      \param request The incoming request that needs to be processed.
      \param response A message object that will contain the response
      of the request when the method returns.
      \return An object (integer) representing the status of the call,
      zero if everything was ok and non-zero if an error occurred. The
      precise meaning of non-zero values have to be decided.
     *
    virtual MCC_Status process(Message& request, Message& response) = 0;
*/



};

/** Message Chain Component - base class for every MCC plugin.
  This is partialy virtual class which defines interface and common 
 functionality for every MCC plugin needed for managing of component
 in a chain.
 */
class MCC: public MCCInterface
{
    protected:
        std::map<std::string,MCCInterface*> next_;
    public:
        /** Example contructor - MCC takes at least it's configuration subtree */
        MCC(Arc::Config *cfg) { };
        virtual ~MCC(void) { };
        /** Add reference to next MCC in chain */
        virtual void Next(MCCInterface* next,const std::string& label = "");
        virtual  Message process(Message msg) { return Message(); };
};

} // namespace Arc

#endif /* __ARC_MCC_H__ */
