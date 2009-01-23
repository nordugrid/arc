#ifndef __ARC_LOGGER__
#define __ARC_LOGGER__

#include <string>
#include <iostream>
#include <glibmm/thread.h>

#include <arc/IString.h>

namespace Arc {

  //! Logging levels.
  /*! Logging levels for tagging and filtering log messages.
	FATAL level designates very severe error events that will 
	  presumably lead the application to abort.
	ERROR level designates error events that might still allow 
	  the application to continue running.
	WARNING level designates potentially harmful situations.
	INFO level designates informational messages that highlight 
	  the progress of the application at coarse-grained level.
	DEBUG level designates fine-grained informational events 
	  that are most useful to debug an application.
	VERBOSE level designates finer-grained informational events 
	than the DEBUG
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
  LogLevel string_to_level(const std::string& str);



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
               const IString& message);

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
               const IString& message,
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

    //! Composes a default identifier.
    /*! This method composes a default identifier by combining the the
      process ID and the address of the Thread object corresponding to
      the calling thread.
      @return A default identifier.
     */
    static std::string getDefaultIdentifier();

    //! Sets the domain of the LogMessage
    /*! This method sets the domain (origin) of the LogMessage. It is
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
    IString message;

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

    virtual ~LogDestination() {
std::cerr<<"~LogDestination: "<<(unsigned int)this<<std::endl;
};

   protected:

    //! Default constructor.
    /*! This destination will use the default locale.
     */
    LogDestination();

    //! Constructor with specific locale.
    /*! This destination will use the specified locale.
     */
    LogDestination(const std::string& locale);

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

   protected:

    std::string locale;
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

    //! Creates a LogStream connected to an ostream.
    /*! Creates a LogStream connected to the specified ostream.
      The output will be localised to the specified locale.
     */
    LogStream(std::ostream& destination, const std::string& locale);

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
    //static Logger rootLogger;
    static Logger& getRootLogger();

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

    //! Removes all LogDestinations.
    void removeDestinations(void);

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

    //! Logs a message text.
    /*! Logs a message text string at the specified LogLevel. This is
      a convenience method to save some typing. It simply creates a
      LogMessage and sends it to the other msg() method.
      @param level The level of the message.
      @param str The message text.
     */
    void msg(LogLevel level, const std::string& str) {
      msg(LogMessage(level, IString(str)));
    }

    template <class T0>
    void msg(LogLevel level, const std::string& str,
             const T0& t0) {
      msg(LogMessage(level, IString(str, t0)));
    }

    template <class T0, class T1>
    void msg(LogLevel level, const std::string& str,
             const T0& t0, const T1& t1) {
      msg(LogMessage(level, IString(str, t0, t1)));
    }

    template <class T0, class T1, class T2>
    void msg(LogLevel level, const std::string& str,
             const T0& t0, const T1& t1, const T2& t2) {
      msg(LogMessage(level, IString(str, t0, t1, t2)));
    }

    template <class T0, class T1, class T2, class T3>
    void msg(LogLevel level, const std::string& str,
             const T0& t0, const T1& t1, const T2& t2, const T3& t3) {
      msg(LogMessage(level, IString(str, t0, t1, t2, t3)));
    }

    template <class T0, class T1, class T2, class T3, class T4>
    void msg(LogLevel level, const std::string& str,
             const T0& t0, const T1& t1, const T2& t2, const T3& t3,
             const T4& t4) {
      msg(LogMessage(level, IString(str, t0, t1, t2, t3, t4)));
    }

    template <class T0, class T1, class T2, class T3, class T4,
              class T5>
    void msg(LogLevel level, const std::string& str,
             const T0& t0, const T1& t1, const T2& t2, const T3& t3,
             const T4& t4, const T5& t5) {
      msg(LogMessage(level, IString(str, t0, t1, t2, t3, t4, t5)));
    }

    template <class T0, class T1, class T2, class T3, class T4,
              class T5, class T6>
    void msg(LogLevel level, const std::string& str,
             const T0& t0, const T1& t1, const T2& t2, const T3& t3,
             const T4& t4, const T5& t5, const T6& t6) {
      msg(LogMessage(level, IString(str, t0, t1, t2, t3, t4, t5, t6)));
    }

    template <class T0, class T1, class T2, class T3, class T4,
              class T5, class T6, class T7>
    void msg(LogLevel level, const std::string& str,
             const T0& t0, const T1& t1, const T2& t2, const T3& t3,
             const T4& t4, const T5& t5, const T6& t6, const T7& t7) {
      msg(LogMessage(level, IString(str, t0, t1, t2, t3, t4, t5, t6, t7)));
    }

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
    Logger *parent;

    //! The domain of this logger.
    std::string domain;

    //! A list of pointers to LogDestinations.
    std::list<LogDestination*> destinations;

    //! The threshold of this Logger.
    LogLevel threshold;

#define rootLoggerMagic (0xF6569201)
    static Logger* rootLogger;
    static unsigned int rootLoggerMark;
  };

} // namespace Arc

#define rootLogger getRootLogger()

#define LOG(LGR,THR,FSTR,...) { if((LGR).getThreshold() >= (THR) (LGR).msg((THR),(FSTR),...); }

#endif // __ARC_LOGGER__
