// MCCPlexer.h

#ifndef __ARC_MCC_PLEXER__
#define __ARC_MCC_PLEXER__

#include <list>
#include <string>
#include <regex.h>
#include "../../libs/message/MCC.h"
#include "../../libs/message/Message.h"
#include "../../../libs/common/ArcConfig.h"

namespace Arc {

  //! A regular expression class.
  /*! This class is a wrapper around the functions provided in
    regex.h.
  */
  class RegularExpression {
  public:

    //! Creates a reges from a pattern string.
    RegularExpression(std::string pattern);
    
    //! Copy constructor.
    RegularExpression(const RegularExpression& regex);

    //! Destructor
    ~RegularExpression();

    //! Assignment operator.
    const RegularExpression& operator=(const RegularExpression& regex);

    //! Returns true iff the pattern of this regex is ok.
    bool isOk();

    //! Returns true iff this regex has the pattern provided.
    bool hasPattern(std::string str);

    //! Returns true iff this regex matches the string provided.
    bool match(const std::string& str) const;

  private:
    std::string pattern;
    regex_t preg;
    int status;
  };
  

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
    friend class MCC_Plexer;
  };


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
