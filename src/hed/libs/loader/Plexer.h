// MCCPlexer.h

#ifndef __ARC_MCC_PLEXER__
#define __ARC_MCC_PLEXER__

#include <list>
#include <string>
#include <arc/ArcRegex.h>
#include <arc/ArcConfig.h>
#include <arc/message/MCC.h>
#include <arc/message/Message.h>

namespace Arc {

  //! A pair of label (regex) and pointer to service.
  /*! A helper class that stores a label (regex) and a pointer to a
    service.
  */
  class PlexerEntry {
  private:
    //! Constructor.
    /*! Constructs a PlexerEntry and initializes its attributes.
    */
    PlexerEntry(const RegularExpression& label,
		MCCInterface* service);
    RegularExpression label;
    MCCInterface* service;
    friend class Plexer;
  };


  //! The Plexer class, used for routing messages to services.
  /*! This is the Plexer class. Its purpose is to route incoming
    messages to appropriate Services and MCC chains.
  */
  class Plexer: public MCC {
  public:

    //! The constructor.
    /*! This is the constructor. Since all member variables are
      instances of "well-behaving" STL classes, nothing needs to be
      done.
     */
    Plexer(Config *cfg);

    //! The destructor.
    /*! This is the destructor. Since all member variables are
      instances of "well-behaving" STL classes, nothing needs to be
      done.
     */
    virtual ~Plexer();

    //! Add reference to next MCC in chain.
    /*! This method is called by Loader for every potentially labeled
      link to next component which implements MCCInterface. If next is
      set NULL corresponding link is removed.
     */
    virtual void Next(MCCInterface* next, const std::string& label);

    //! Route request messages to appropriate services.
    /*! Routes the request message to the appropriate service.
      Routing is based on the path part of value of the ENDPOINT 
      attribute. Routed message is assigned following attributes:
        PLEXER:PATTERN - matched pattern,
        PLEXER:EXTENSION - last unmatched part of ENDPOINT path.
    */
    virtual MCC_Status process(Message& request, Message& response);

  /* protected:
    XXX: workaround because the python segmentation fault */
    static Arc::Logger logger;

  private:

    //! Extracts the path part of an URL.
    static std::string getPath(std::string url);

    //! The map of services.
    /*! This is a map that maps service labels (strings) to services
      (with MCC interface). It is used for routing messages.
    */
    std::list<PlexerEntry> services;
  };

}

#endif
