// Logger.h

#ifndef __Logger__
#define __Logger__

#include <set>
#include <string>
#include <iostream>
#include <glibmm/thread.h>

namespace Arc {

  //! Logging levels.
  /*! Logging levels for tagging and filtering log messages.
   */
  enum LogLevel {
    VERBOSE = 1,
    DEBUG = 2,
    INFO = 4,
    WARNING = 8,
    ERROR = 16,
    FATAL = 32
  };

  //! Printing of LogLevel values to ostreams.
  /*! Output operator so that LogLevel values can be printed in a
    nicer way.
   */
  std::ostream& operator<<(std::ostream& os, LogLevel level);
  LogLevel string_to_level(std::string& str);


  
  //! A class for log messages.
  /*! This class is used to represent log messages internally. It
    contains the time the message was created, its level, from which
    domain it was sent, an identifier and the message text itself.
   */
  class LogMessage {
  public:

    //! Creates a LogMessage with  the specified level and message text.
    /*! This constructor creates a LogMessage with the specified level
      and message text. The time is set automatically, the domain is
      set by the Logger to which the LogMessage is sent and the
      identifier is composed from the process ID and the address of
      the Thread object corresponding to the calling thread.
      @param level The level of the LogMessage.
      @param message The message text.
     */
    LogMessage(LogLevel level,
	       const std::string& message);

    //! Creates a LogMessage with the specified attributes.
    /*! This constructor creates a LogMessage with the specified
      level, message text and identifier. The time is set
      automatically and the domain is set by the Logger to which the
      LogMessage is sent.
      @param level The level of the LogMessage.
      @param message The message text.
      @param ident The identifier of the LogMessage.
    */
    LogMessage(LogLevel level,
	       const std::string& message,
	       const std::string& identifier);

    //! Returns the level of the LogMessage.
    /*! Returns the level of the LogMessage.
      @return The level of the LogMessage.
     */
    LogLevel getLevel() const;

  protected:

    //! Sets the identifier of the LogMessage.
    /*! The purpose of this method is to allow subclasses (in case
      there are any) to set the identifier of a LogMessage.
      @param The identifier.
     */
    void setIdentifier(std::string identifier);

  private:

    //! Returns the current date and time nicely formated in a string.
    /*! This method gets the current date and time and formats it
      nicely in a string to be used by the constructors of this class.
      @return A string containing the current date and time.
     */
    static std::string getCurrentTime();

    //! Composes a default identifier.
    /*! This method composes a default identifier by combining the the
      process ID and the address of the Thread object corresponding to
      the calling thread.
      @return A default identifier.
     */
    static std::string getDefaultIdentifier();

    //! Sets the domain of the LogMessage
    /*! This method sets the domain (origin) of the LogMEssage. It is
      called by the Logger to which the LogMessage is sent.
      @param domain The domain.
    */
    void setDomain(std::string domain);

    //! The time when the LogMessage was created.
    std::string time;

    //! The level (severity) of the LogMessage.
    LogLevel level;

    //! The domain (origin) of the LogMessage.
    std::string domain;

    //! An identifier that may be used for filtering.
    std::string identifier;

    //! The message text.
    std::string message;

    //! Printing of LogMessages to ostreams.
    /*! Output operator so that LogMessages can be printed
      conveniently by LogDestinations.
    */
    friend std::ostream& operator<<(std::ostream& os,
				    const LogMessage& message);

    //! The Logger class is a friend.
    /*! The Logger class must have some privileges (e.g. ability to
      call the setDomain() method), therefore it is a friend.
     */
    friend class Logger;

  };



  //! A base class for log destinations.
  /*! This class defines an interface for LogDestinations.
    LogDestination objects will typically contain synchronization
    mechanisms and should therefore never be copied.
   */
  class LogDestination {
  public:

    //! Logs a LogMessage to this LogDestination.
    virtual void log(const LogMessage& message) = 0;

  protected:

    //! Default constructor.
    /*! The only constructor needed by subclasses, since the
      LogDestination class has no attributes.
     */
    LogDestination();

  private:

    //! Private copy constructor
    /*! LogDestinations should never be copied, therefore the copy
      constructor is private.
     */
    LogDestination(const LogDestination& unique);

    //! Private assignment operator
    /*! LogDestinations should never be assigned, therefore the
      assignment operator is private.
     */
    void operator=(const LogDestination& unique);

  };



  //! A class for logging to ostreams
  /*! This class is used for logging to ostreams (cout, cerr, files).
    It provides synchronization in order to prevent different
    LogMessages to appear mixed with each other in the stream. In
    order not to break the synchronization, LogStreams should never be
    copied. Therefore the copy constructor and assignment operator are
    private. Furthermore, it is important to keep a LogStream object
    as long as the Logger to which it has been registered.
   */
  class LogStream : public LogDestination {
  public:

    //! Creates a LogStream connected to an ostream.
    /*! Creates a LogStream connected to the specified ostream. In
      order not to break synchronization, it is important not to
      connect more than one LogStream object to a certain stream.
      @param destination The ostream to which to erite LogMessages.
     */
    LogStream(std::ostream& destination);

    //! Writes a LogMessage to the stream.
    /*! This method writes a LogMessage to the ostream that is
      connected to this LogStream object. It is synchronized so that
      not more than one LogMessage can be written at a time.
      @param message The LogMessage to write.
     */
    virtual void log(const LogMessage& message);

  private:

    //! Private copy constructor
    /*! LogStreams should never be copied, therefore the copy
      constructor is private.
     */
    LogStream(const LogStream& unique);

    //! Private assignment operator
    /*! LogStreams should never be assigned, therefore the assignment
      operator is private.
     */
    void operator=(const LogStream& unique);

    //! The ostream to which to write LogMessages
    /*! This is the ostream to which LogMessages sent to this
      LogStream will be written.
     */
    std::ostream& destination;

    //! A mutex for synchronization.
    /*! This mutex is locked before a LogMessage is written and it is
      not unlocked until the entire message has been written and the
      stream flushed. This is done in order to prevent LogMessages to
      appear mioxed in the stream.
     */
    Glib::Mutex mutex;

  };



  //! A logger class
  /*! This class defines a Logger to which LogMessages can be sent.

    Every Logger (except for the rootLogger) has a parent Logger. The
    domain of a Logger (a string that indicates the origin of
    LogMessages) is composed by adding a subdomain to the domain of
    its parent Logger.

    A Logger also has a threshold. Every LogMessage that have a level
    that is greater than or equal to the threshold is forwarded to any
    LogDestination connected to this Logger as well as to the parent
    Logger.

    Typical usage of the Logger class is to declare a global Logger
    object for each library/module/component to be used by all classes
    and methods there.
   */
  class Logger {
  public:

    //! The root Logger
    /*! This is the root Logger. It is an ancestor of any other Logger
      and allways exists.
     */
    static Logger rootLogger;

    //! Currently not used?!?
    static LogStream cerr;

    //! Creates a logger.
    /*! Creates a logger. The threshold is inherited from its parent
      Logger.
      @param parent The parent Logger of the new Logger.
      @param subdomain The subdomain of the new logger.
     */
    Logger(Logger& parent,
	   const std::string& subdomain);

    //! Creates a logger.
    /*! Creates a logger.
      @param parent The parent Logger of the new Logger.
      @param subdomain The subdomain of the new logger.
      @param threshold The threshold of the new logger.
     */
    Logger(Logger& parent,
	   const std::string& subdomain,
	   LogLevel threshold);

    //! Adds a LogDestination.
    /*! Adds a LogDestination to which to forward LogMessages sent to
      this logger (if they pass the threshold). Since LogDestinatoins
      should not be copied, the new LogDestination is passed by
      reference and a pointer to it is kept for later use. It is
      therefore important that the LogDestination passed to this
      Logger exists at least as long as the Logger iteslf.
     */
    void addDestination(LogDestination& destination);

    //! Sets the threshold
    /*! This method sets the threshold of the Logger. Any message sent
      to this Logger that has a level below this threshold will be
      discarded.
      @param The threshold
     */
    void setThreshold(LogLevel threshold);

    //! Returns the threshold.
    /*! Returns the threshold.
      @return The threshold of this Logger.
    */
    LogLevel getThreshold() const;

    //! Sends a LogMessage.
    /*! Sends a LogMessage.
      @param The LogMessage to send.
    */
    void msg(LogMessage message);

    //! Loggs a message text.
    /*! Loggs a message text string at the specified LogLevel. This is
      a convenience method to save some typing. It simply creates a
      LogMessage and sends it to the other msg() method.
      @param level The level of the message.
      @param str The message text.
    */
    void msg(LogLevel level, const char *str);

  private:

    //! A private constructor.
    /*! Every Logger (except the root logger) must have a parent,
      therefore the default constructor (which does not specify a
      parent) is private to prevent accidental use. It is only used
      when creating the root logger.
     */
    Logger();

    //! Private copy constructor
    /*! Loggers should never be copied, therefore the copy constructor
      is private.
     */
    Logger(const Logger& unique);

    //! Private assignment operator
    /*! Loggers should never be assigned, therefore the assignment
      operator is private.
     */
    void operator=(const Logger& unique);

    //! Returns the domain.
    /*! This method returns the domain of this logger, i.e. the string
      that is attached to all LogMessages sent from this logger to
      indicate their origin.
     */
    std::string getDomain();

    //! Forwards a log message.
    /*! This method is called by the msg() method and by child
      Loggers. It filters messages based on their level and forwards
      them to the parent Logger and any LogDestination that has been
      added to this Logger.
      @param message The message to send.
     */
    void log(const LogMessage& message);

    //! A pointer to the parent of this logger.
    Logger* parent;

    //! The domain of this logger.
    std::string domain;

    //! A list of pointers to LogDestinations.
    std::list<LogDestination*> destinations;

    //! The threshold of this Logger.
    LogLevel threshold;

  };

}


//! A preprocessor macro for efficiency
/*! This preprocessor macro can be used to make logging more
  efficient. It eliminates the construction of the message text (which
  can be time consuming) if the level is below the threshold of the
  logger.
  @param logger The logegr to which to send the message.
  @param level The level of the message.
  @param message The message text, possibly in the form of an
  expression that evaluates to a string.
 */
#define MSG(logger, level, message) {			\
    if (level>=logger.getThreshold())			\
      logger.msg(Arc::LogMessage(level,message));}

#endif
