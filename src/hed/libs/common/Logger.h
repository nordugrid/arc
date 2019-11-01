// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_LOGGER__
#define __ARC_LOGGER__

#include <string>
#include <list>
#include <map>
#include <iostream>
#include <fstream>

#include <arc/Thread.h>
#include <arc/IString.h>

namespace Arc {

  /** \addtogroup common
   *  @{ */
  /// Logging levels for tagging and filtering log messages.
  enum LogLevel {
    DEBUG = 1,   ///< DEBUG level designates finer-grained informational events
                 ///< which should only be used for debugging purposes.
    VERBOSE = 2, ///< VERBOSE level designates fine-grained informational events
                 ///< that will give additional information about the application.
    INFO = 4,    ///< INFO level designates informational messages that highlight
                 ///< the progress of the application at coarse-grained level.
    WARNING = 8, ///< WARNING level designates potentially harmful situations.
    ERROR = 16,  ///< ERROR level designates error events that might still allow
                 ///< the application to continue running.
    FATAL = 32   ///< FATAL level designates very severe error events that will
                 ///< presumably lead the application to abort.
  };

  /// Output formats. Defines prefix for each message.
  enum LogFormat {
    /// All information about message is printed
    LongFormat,
    /// All information except domain is printed
    /**
     * \since Added in 4.0.0.
     **/
    MediumFormat,
    /// Only message level is printed
    ShortFormat,
    /// Message time (microsecond precision) and time difference
    /// from previous message are printed. This format is mostly
    /// meant for profiling.
    DebugFormat,
    /// Only message is printed
    EmptyFormat
  };

  /// Struct to contain LogFormat, to use with operator<<(std::ostream&, const LoggerFormat&)
  struct LoggerFormat {
    /// Make a new LoggerFormat with the given LogFormat.
    LoggerFormat(LogFormat format)
      : format(format) {};
    LogFormat format;
  };

  /// Allows printing of messages to streams using ARC Logger formatting.
  std::ostream& operator<<(std::ostream& os, const LoggerFormat& format);

  /// Printing of LogLevel values to ostreams.
  /** Output operator so that LogLevel values can be printed in a
     nicer way.
   */
  std::ostream& operator<<(std::ostream& os, LogLevel level);
  /// Convert string to a LogLevel.
  LogLevel string_to_level(const std::string& str);
  /// Convert string case-insensitively to a LogLevel
  /**
   * \since Added in 4.1.0.
   **/
  LogLevel istring_to_level(const std::string& llStr);
  /// Case-insensitive parsing of a string to a LogLevel with error response.
  /**
   * The method will try to parse (case-insensitive) the argument string
   * to a corresponding LogLevel. If the method succeeds, true will
   * be returned and the argument \a ll will be set to
   * the parsed LogLevel. If the parsing fails \c false will be
   * returned. The parsing succeeds if \a llStr match
   * (case-insensitively) one of the names of the LogLevel members.
   *
   * @param llStr a string which should be parsed to a Arc::LogLevel.
   * @param ll a Arc::LogLevel reference which will be set to the
   *        matching Arc::LogLevel upon successful parsing.
   * @return \c true in case of successful parsing, otherwise \c false.
   * @see LogLevel
   */
  bool istring_to_level(const std::string& llStr, LogLevel& ll);
  /// Same as istring_to_level except it is case-sensitive.
  bool string_to_level(const std::string& str, LogLevel& ll);
  /// Convert LogLevel to a string.
  std::string level_to_string(const LogLevel& level);
  /// Convert an old-style log level (int from 0 to 5) to a LogLevel.
  LogLevel old_level_to_level(unsigned int old_level);



  /// A class for log messages.
  /** This class is used to represent log messages internally. It
     contains the time the message was created, its level, from which
     domain it was sent, an identifier and the message text itself.
     \headerfile Logger.h arc/Logger.h
   */
  class LogMessage {
  public:

    /// Creates a LogMessage with the specified level and message text.
    /** This constructor creates a LogMessage with the specified level
       and message text. The time is set automatically, the domain is
       set by the Logger to which the LogMessage is sent and the
       identifier is composed from the process ID and the address of
       the Thread object corresponding to the calling thread.
       @param level The level of the LogMessage.
       @param message The message text.
     */
    LogMessage(LogLevel level,
               const IString& message);

    /// Creates a LogMessage with the specified attributes.
    /** This constructor creates a LogMessage with the specified
       level, message text and identifier. The time is set
       automatically and the domain is set by the Logger to which the
       LogMessage is sent.
       @param level The level of the LogMessage.
       @param message The message text.
       @param identifier The identifier of the LogMessage.
     */
    LogMessage(LogLevel level,
               const IString& message,
               const std::string& identifier);

    /// Returns the level of the LogMessage.
    /** Returns the level of the LogMessage.
       @return The level of the LogMessage.
     */
    LogLevel getLevel() const;

  protected:

    /// Sets the identifier of the LogMessage.
    /** The purpose of this method is to allow subclasses (in case
       there are any) to set the identifier of a LogMessage.
       @param identifier The identifier.
     */
    void setIdentifier(std::string identifier);

  private:

    /// Composes a default identifier.
    /** This method composes a default identifier by combining the the
       process ID and the address of the Thread object corresponding to
       the calling thread.
       @return A default identifier.
     */
    static std::string getDefaultIdentifier();

    /// Sets the domain of the LogMessage
    /** This method sets the domain (origin) of the LogMessage. It is
       called by the Logger to which the LogMessage is sent.
       @param domain The domain.
     */
    void setDomain(std::string domain);

    /// The time when the LogMessage was created.
    std::string time;

    /// The level (severity) of the LogMessage.
    LogLevel level;

    /// The domain (origin) of the LogMessage.
    std::string domain;

    /// An identifier that may be used for filtering.
    std::string identifier;

    /// The message text.
    IString message;

    /// Printing of LogMessages to ostreams.
    /** Output operator so that LogMessages can be printed
       conveniently by LogDestinations.
     */
    friend std::ostream& operator<<(std::ostream& os,
                                    const LogMessage& message);

    /// The Logger class is a friend.
    /** The Logger class must have some privileges (e.g. ability to
       call the setDomain() method), therefore it is a friend.
     */
    friend class Logger;

  };



  /// A base class for log destinations.
  /** This class defines an interface for LogDestinations.
     LogDestination objects will typically contain synchronization
     mechanisms and should therefore never be copied. If setlocale()
     has been called with a supported locale, log messages will be
     logged in that locale.
     \headerfile Logger.h arc/Logger.h
   */
  class LogDestination {
  public:

    /// Logs a LogMessage to this LogDestination.
    virtual void log(const LogMessage& message) = 0;

    virtual ~LogDestination() {}

    /// Set format for this log destination.
    void setFormat(const LogFormat& newformat);

    /// Returns currently assigned format
    LogFormat getFormat() const;

    /// Set format for any new log destination.
    static void setDefaultFormat(const LogFormat& newformat);

    /// Returns currently assigned default format
    static LogFormat getDefaultFormat();

    /// Set a prefix for this log destination to be logged before messages.
    /** The prefix is prepended to messages in all formats except EmptyFormat.
     * \since Added in 4.0.0.
     */
    void setPrefix(const std::string& prefix);

    /// Returns currently assignd prefix
    std::string getPrefix() const;

  protected:

    /// Default constructor. Protected since subclasses should be used instead.
    LogDestination();

  private:

    /// Private copy constructor
    /** LogDestinations should never be copied, therefore the copy
       constructor is private.
     */
    LogDestination(const LogDestination& unique);

    /// Private assignment operator
    /** LogDestinations should never be assigned, therefore the
       assignment operator is private.
     */
    void operator=(const LogDestination& unique);

    /// Sets iword and pword for format and prefix
    friend std::ostream& operator<<(std::ostream& os, const LogDestination& dest);

  protected:
    /// Format to use in this LogDestination.
    LogFormat format;

    /// Prefix to use in this log destination.
    /**
     * \since Added in 4.0.0.
     **/
    std::string prefix;
  };



  /// A class for logging to ostreams.
  /** This class is used for logging to ostreams (cout, cerr, files).
     It provides synchronization in order to prevent different
     LogMessages to appear mixed with each other in the stream. In
     order not to break the synchronization, LogStreams should never be
     copied. Therefore the copy constructor and assignment operator are
     private. Furthermore, it is important to keep a LogStream object
     as long as the Logger to which it has been registered.
     \headerfile Logger.h arc/Logger.h
   */
  class LogStream
    : public LogDestination {
  public:

    /// Creates a LogStream connected to an ostream.
    /** Creates a LogStream connected to the specified ostream. In
       order not to break synchronization, it is important not to
       connect more than one LogStream object to a certain stream.
       @param destination The ostream to which to write LogMessages.
     */
    LogStream(std::ostream& destination);

    /// Writes a LogMessage to the stream.
    /** This method writes a LogMessage to the ostream that is
       connected to this LogStream object. It is synchronized so that
       not more than one LogMessage can be written at a time.
       @param message The LogMessage to write.
     */
    virtual void log(const LogMessage& message);

  private:

    /// Private copy constructor
    /** LogStreams should never be copied, therefore the copy
       constructor is private.
     */
    LogStream(const LogStream& unique);

    /// Private assignment operator
    /** LogStreams should never be assigned, therefore the assignment
       operator is private.
     */
    void operator=(const LogStream& unique);

    /// The ostream to which to write LogMessages
    /** This is the ostream to which LogMessages sent to this
       LogStream will be written.
     */
    std::ostream& destination;

    /// A mutex for synchronization.
    /** This mutex is locked before a LogMessage is written and it is
       not unlocked until the entire message has been written and the
       stream flushed. This is done in order to prevent LogMessages to
       appear mixed in the stream.
     */
    Glib::Mutex mutex;

  };

  /// A class for logging to files.
  /** This class is used for logging to files.
     It provides synchronization in order to prevent different
     LogMessages to appear mixed with each other in the stream.
     It is possible to limit size of created file. Whenever
     specified size is exceeded file is deleted and new one is
     created. Old files may be moved into backup files instead of
     being deleted. Those files have names same as initial file with
     additional number suffix - similar to those found in /var/log
     of many Unix-like systems.
     \headerfile Logger.h arc/Logger.h
   */
  class LogFile
    : public LogDestination {
  public:

    /// Creates a LogFile connected to a file.
    /** Creates a LogFile connected to the file located at
       specified path. In order not to break synchronization,
       it is important not to connect more than one LogFile object
       to a certain file. If file does not exist it will be created.
       @param path The path to file to which to write LogMessages.
     */
    LogFile(const std::string& path);

    /// Ordinary destructor
    ~LogFile();

    /// Set maximal allowed size of file.
    /** Set maximal allowed size of file. This value is not
       obeyed exactly. Specified size may be exceeded by amount
       of one LogMessage. To disable limit specify -1.
       @param newsize Max size of log file.
     */
    void setMaxSize(int newsize);

    /// Set number of backups to store.
    /** Set number of backups to store. When file size exceeds one
       specified with setMaxSize() file is closed and moved to one
       named path.1. If path.1 exists it is moved to path.2 and so
       on. Number of path.# files is one set in newbackup.
       @param newbackup Number of backup files.
     */
    void setBackups(int newbackup);

    /// Set file reopen on every write.
    /** Set file reopen on every write. If set to true file is opened
       before writing every log record and closed afterward.
       @param newreopen If file to be reopened for every log record.
     */
    void setReopen(bool newreopen);

    /// Reopen file if currently open.
    void Reopen();

    /// Reopen all LogFile objects.
    static void ReopenAll();

    /// Returns true if this instance is valid.
    operator bool(void);

    /// Returns true if this instance is invalid.
    bool operator!(void);

    /// Writes a LogMessage to the file.
    /** This method writes a LogMessage to the file that is
       connected to this LogFile object. If after writing
       size of file exceeds one set by setMaxSize() file is moved
       to backup and new one is created.
       @param message The LogMessage to write.
     */
    virtual void log(const LogMessage& message);
  private:
    LogFile(void);
    LogFile(const LogFile& unique);
    void operator=(const LogFile& unique);
    void backup(void);
    std::string path;
    std::ofstream destination;
    int maxsize;
    int backups;
    bool reopen;
    Glib::Mutex mutex;
  };

  class LoggerContextRef;

  /** \cond Container for internal logger configuration.
     \headerfile Logger.h arc/Logger.h */
  class LoggerContext {
    friend class Logger;
    friend class LoggerContextRef;
    private:
      /// This counts how many threads are using this object.
      int usage_count;

      /// Protection for usage_count.
      Glib::Mutex mutex;

      /// A list of pointers to LogDestinations.
      std::list<LogDestination*> destinations;

      /// The threshold of Logger.
      LogLevel threshold;

      LoggerContext(LogLevel thr):usage_count(0),threshold(thr) { };

      LoggerContext(const LoggerContext& ctx):
             usage_count(0),destinations(ctx.destinations),threshold(ctx.threshold) { };

      ~LoggerContext(void);

      void Acquire(void);

      void Release(void);
  };
  /** \endcond */
 

  /// A logger class.
  /** This class defines a Logger to which LogMessages can be sent.

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

     Logger messages may be localised according to the current locale. Some
     locales are better supported than others.

     Example code for setting up logger in main():
     @code
  // Set up stderr as a log stream
  Arc::LogStream logcerr(std::cerr);
  // Log message is prefixed by level only
  logcerr.setFormat(Arc::ShortFormat);
  // Add the stderr destination to the root logger
  Arc::Logger::getRootLogger().addDestination(logcerr);
  // Set the logging threshold to WARNING
  Arc::Logger::getRootLogger().setThreshold(Arc::WARNING);

  // Logger to use in main() - it inherits all properties from the root Logger
  Arc::Logger logger(Arc::Logger::getRootLogger(), "main");
  // this message will not be logged since it is below the threshold
  logger.msg(Arc::INFO, "main started");
  int i = 5;
  // This message will be logged
  logger.msg(Arc::ERROR, "Oops, an error occurred when i was %i", i);
     @endcode
   */
  class Logger {
  public:

    /// The root Logger.
    /** This is the root Logger. It is an ancestor of any other Logger
       and always exists.
     */
    //static Logger rootLogger;
    static Logger& getRootLogger();

    /// Creates a logger.
    /** The threshold is inherited from its parent Logger.
       @param parent The parent Logger of the new Logger.
       @param subdomain The subdomain of the new logger.
     */
    Logger(Logger& parent,
           const std::string& subdomain);

    /// Creates a logger.
    /** @param parent The parent Logger of the new Logger.
       @param subdomain The subdomain of the new logger.
       @param threshold The threshold of the new logger.
     */
    Logger(Logger& parent,
           const std::string& subdomain,
           LogLevel threshold);

    /// Destroys a logger.
    ~Logger();

    /// Adds a LogDestination.
    /** Adds a LogDestination to which to forward LogMessages sent to
       this logger (if they pass the threshold). Since LogDestinations
       should not be copied, the new LogDestination is passed by
       reference and a pointer to it is kept for later use. It is
       therefore important that the LogDestination passed to this
       Logger exists at least as long as the Logger itself.
     */
    void addDestination(LogDestination& destination);

    /// Adds LogDestinations.
    /** See addDestination(LogDestination& destination).
     */
    void addDestinations(const std::list<LogDestination*>& destinations);

    /// Set LogDestinations.
    /** A safe atomic way to remove and add LogDestinations.
     */
    void setDestinations(const std::list<LogDestination*>& destinations);

    /// Obtains current LogDestinations.
    /** Returns list of pointers to LogDestination objects. Returned result
       refers directly to internal member of Logger instance. Hence it
       should not be used after this Logger is destroyed.
     */
    const std::list<LogDestination*>& getDestinations(void) const;

    /// Removes all LogDestinations.
    void removeDestinations(void);

    /// Remove all LogDestinations and delete LogDestination objects.
    /** A LogDestination to not delete can be specified in the exclude parameter.
     * @param exclude Do not delete this destination
     */
    void deleteDestinations(LogDestination* exclude=NULL);

    /// Sets the logging threshold.
    /** This method sets the threshold of the Logger. Any message sent
       to this Logger that has a level below this threshold will be
       discarded.
       @param threshold The threshold
     */
    void setThreshold(LogLevel threshold);

    /// Sets the threshold for domain.
    /** This method sets the default threshold of the domain. All new loggers
       created with specified domain will have specified threshold set by default.
       The subdomains of all loggers in chain are matched against list of provided
       subdomains.
       @param threshold The threshold
       @param subdomains The subdomains of all loggers in chain
     */
    static void setThresholdForDomain(LogLevel threshold,
                                      const std::list<std::string>& subdomains);

    /// Sets the threshold for domain.
    /** This method sets the default threshold of the domain. All new loggers
       created with specified domain will have specified threshold set by default.
       The domain is composed of all subdomains of all loggers in chain by merging
       them with '.' as separator.
       @param threshold The threshold
       @param domain The domain of logger
     */
    static void setThresholdForDomain(LogLevel threshold,
                                      const std::string& domain);

    /// Returns the threshold of this logger.
    LogLevel getThreshold() const;

    /// Creates per-thread context.
    /** Creates new context for this logger which becomes effective
       for operations initiated by this thread. All new threads 
       started by this one will inherit new context. Context
       stores current threshold and pointers to destinations.
       Hence new context is identical to current one. One can 
       modify new context using setThreshold(), removeDestinations()
       and addDestination(). All such operations will not affect
       old context.
     */
    void setThreadContext(void);

    /// Sends a LogMessage.
    /** @param message The LogMessage to send.
     */
    void msg(LogMessage message);

    /// Logs a message text.
    /** Logs a message text string at the specified LogLevel. This is
       a convenience method to save some typing. It simply creates a
       LogMessage and sends it to the other msg() methods. It is also possible
       to use msg() with multiple arguments and printf-style string formatting,
       for example
       @code
       logger.msg(INFO, "Operation no %i failed: %s", number, reason);
       @endcode
       @param level The level of the message.
       @param str The message text.
     */
    void msg(LogLevel level, const std::string& str) {
      msg(LogMessage(level, IString(str)));
    }

    template<class T0>
    void msg(LogLevel level, const std::string& str,
             const T0& t0) {
      msg(LogMessage(level, IString(str, t0)));
    }

    template<class T0, class T1>
    void msg(LogLevel level, const std::string& str,
             const T0& t0, const T1& t1) {
      msg(LogMessage(level, IString(str, t0, t1)));
    }

    template<class T0, class T1, class T2>
    void msg(LogLevel level, const std::string& str,
             const T0& t0, const T1& t1, const T2& t2) {
      msg(LogMessage(level, IString(str, t0, t1, t2)));
    }

    template<class T0, class T1, class T2, class T3>
    void msg(LogLevel level, const std::string& str,
             const T0& t0, const T1& t1, const T2& t2, const T3& t3) {
      msg(LogMessage(level, IString(str, t0, t1, t2, t3)));
    }

    template<class T0, class T1, class T2, class T3, class T4>
    void msg(LogLevel level, const std::string& str,
             const T0& t0, const T1& t1, const T2& t2, const T3& t3,
             const T4& t4) {
      msg(LogMessage(level, IString(str, t0, t1, t2, t3, t4)));
    }

    template<class T0, class T1, class T2, class T3, class T4,
             class T5>
    void msg(LogLevel level, const std::string& str,
             const T0& t0, const T1& t1, const T2& t2, const T3& t3,
             const T4& t4, const T5& t5) {
      msg(LogMessage(level, IString(str, t0, t1, t2, t3, t4, t5)));
    }

    template<class T0, class T1, class T2, class T3, class T4,
             class T5, class T6>
    void msg(LogLevel level, const std::string& str,
             const T0& t0, const T1& t1, const T2& t2, const T3& t3,
             const T4& t4, const T5& t5, const T6& t6) {
      msg(LogMessage(level, IString(str, t0, t1, t2, t3, t4, t5, t6)));
    }

    template<class T0, class T1, class T2, class T3, class T4,
             class T5, class T6, class T7>
    void msg(LogLevel level, const std::string& str,
             const T0& t0, const T1& t1, const T2& t2, const T3& t3,
             const T4& t4, const T5& t5, const T6& t6, const T7& t7) {
      msg(LogMessage(level, IString(str, t0, t1, t2, t3, t4, t5, t6, t7)));
    }

  private:

    /// A private constructor.
    /** Every Logger (except the root logger) must have a parent,
       therefore the default constructor (which does not specify a
       parent) is private to prevent accidental use. It is only used
       when creating the root logger.
     */
    Logger();

    /// Private copy constructor
    /** Loggers should never be copied, therefore the copy constructor
       is private.
     */
    Logger(const Logger& unique);

    /// Private assignment operator
    /** Loggers should never be assigned, therefore the assignment
       operator is private.
     */
    void operator=(const Logger& unique);

    /// Returns the domain.
    /** This method returns the domain of this logger, i.e. the string
       that is attached to all LogMessages sent from this logger to
       indicate their origin.
     */
    std::string getDomain();

    /// Forwards a log message.
    /** This method is called by the msg() method and by child
       Loggers. It filters messages based on their level and forwards
       them to the parent Logger and any LogDestination that has been
       added to this Logger.
       @param message The message to send.
     */
    void log(const LogMessage& message);

    /// A pointer to the parent of this logger.
    Logger *parent;

    /// The domain of this logger.
    std::string domain;

    /// Per-trhread storage id for context;
    std::string context_id;

    LoggerContext context;

    LoggerContext& getContext(void);

    Glib::Mutex mutex;

#define rootLoggerMagic (0xF6569201)
    static Logger *rootLogger;
    static std::map<std::string,LogLevel>* defaultThresholds;
    static unsigned int rootLoggerMark;
  };

  /** @} */

} // namespace Arc

#define rootLogger getRootLogger()

#define LOG(LGR, THR, FSTR, ...) { if ((LGR).getThreshold() >= (THR)(LGR).msg((THR), (FSTR), ...); }

#endif // __ARC_LOGGER__
