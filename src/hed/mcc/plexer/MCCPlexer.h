// MCCPlexer.h

#ifndef __ARC_MCC_PLEXER__
#define __ARC_MCC_PLEXER__

#include <map>
#include <string>
#include "../../libs/message/MCC.h"
#include "../../libs/message/Message.h"
#include "../../libs/message/ArcConfig.h"

namespace Arc {
  
  //! The Plexer class, used for routing messages to services.
  /*! This is the Plexer class. Its purpose is to rout incoming
    messages to appropriate services.
  */
  class MCC_Plexer: public MCC {
  public:

    //! The constructor.
    /*! This is the constructor. Since all member variables are
      instances of "well-behaving" STL classes, nothing needs to be
      done.
     */
    MCC_Plexer(Config *cfg);

    //! The destructor.
    /*! This is the destructor. Since all member variables are
      instances of "well-behaving" STL classes, nothing needs to be
      done.
     */
    virtual ~MCC_Plexer();

    //! Add reference to next MCC in chain.
    /*! This method is called by Loader for every potentially labeled
      link to next component which implements MCCInterface. If next is
      set NULL corresponding link is removed.
     */
    virtual void Next(MCCInterface* next, const std::string& label);

    //! Rout request messages to appropriate services.
    /*! Routs the request message to the appropriate service.
      Currently routing is based on the value of the "Request-URI"
      attribute, but that may be replaced by some other attribute once
      the attributes discussion is finished.
    */
    virtual MCC_Status process(Message& request, Message& response);
  private:

    //! The map of services.
    /*! This is a map that maps service labels (strings) to services
      (with MCC interface). It is used for routing messages.
    */
    std::map<std::string,MCCInterface*> services_;
  };

}

#endif
