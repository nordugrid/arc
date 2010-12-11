// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <sstream>
#include <fstream>

#include <unistd.h>

#include <arc/DateTime.h>
#include <arc/StringConv.h>

#include <unistd.h>
#ifdef WIN32
#include <process.h>
#endif

#include "Logger.h"

#undef rootLogger

namespace Arc {

  static std::string list_to_domain(const std::list<std::string>& subdomains) {
    std::string domain;
    for(std::list<std::string>::const_iterator subdomain = subdomains.begin();
        subdomain != subdomains.end();++subdomain) {
      domain += "."+(*subdomain);
    }
    return domain;
  }

  std::ostream& operator<<(std::ostream& os, LogLevel level) {
    if (level == DEBUG)
      os << "DEBUG";
    else if (level == VERBOSE)
      os << "VERBOSE";
    else if (level == INFO)
      os << "INFO";
    else if (level == WARNING)
      os << "WARNING";
    else if (level == ERROR)
      os << "ERROR";
    else if (level == FATAL)
      os << "FATAL";
    // There should be no more alternative!
    return os;
  }

  LogLevel string_to_level(const std::string& str) {
    if (str == "DEBUG")
      return DEBUG;
    else if (str == "VERBOSE")
      return VERBOSE;
    else if (str == "INFO")
      return INFO;
    else if (str == "WARNING")
      return WARNING;
    else if (str == "ERROR")
      return ERROR;
    else if (str == "FATAL")
      return FATAL;
    else  // should not happen...
      return FATAL;
  }

  bool string_to_level(const std::string& str, LogLevel& ll) {
    if (str == "DEBUG")
      ll = DEBUG;
    else if (str == "VERBOSE")
      ll = VERBOSE;
    else if (str == "INFO")
      ll = INFO;
    else if (str == "WARNING")
      ll = WARNING;
    else if (str == "ERROR")
      ll = ERROR;
    else if (str == "FATAL")
      ll = FATAL;
    else  // should not happen...
      return false;

    return true;
  }

  bool istring_to_level(const std::string& llStr, LogLevel& ll) {
    const std::string str = upper(llStr);
    if (str == "DEBUG")
      ll = DEBUG;
    else if (str == "VERBOSE")
      ll = VERBOSE;
    else if (str == "INFO")
      ll = INFO;
    else if (str == "WARNING")
      ll = WARNING;
    else if (str == "ERROR")
      ll = ERROR;
    else if (str == "FATAL")
      ll = FATAL;
    else
      return false;

    return true;
  }

  std::string level_to_string(const LogLevel& level) {
    switch (level) {
      case DEBUG:
        return "DEBUG";
      case VERBOSE:
        return "VERBOSE";
      case INFO:
        return "INFO";
      case WARNING:
        return "WARNING";
      case ERROR:
        return "ERROR";
      case FATAL:
        return "FATAL";
      default:  // should not happen...
        return "";
    }
  }

  LogLevel old_level_to_level(unsigned int old_level) {
    if (old_level >= 5)
      return DEBUG;
    else if (old_level == 4)
      return VERBOSE;
    else if (old_level == 3)
      return INFO;
    else if (old_level == 2)
      return WARNING;
    else if (old_level == 1)
      return ERROR;
    else if (old_level == 0)
      return FATAL;
    else  // cannot happen...
      return FATAL;
  }

  LogMessage::LogMessage(LogLevel level,
                         const IString& message)
    : time(TimeStamp()),
      level(level),
      domain("---"),
      identifier(getDefaultIdentifier()),
      message(message) {}

  LogMessage::LogMessage(LogLevel level,
                         const IString& message,
                         const std::string& identifier)
    : time(TimeStamp()),
      level(level),
      domain("---"),
      identifier(identifier),
      message(message) {}

  LogLevel LogMessage::getLevel() const {
    return level;
  }

  void LogMessage::setIdentifier(std::string identifier) {
    this->identifier = identifier;
  }

  std::string LogMessage::getDefaultIdentifier() {
    std::ostringstream sout;
#ifdef HAVE_GETPID
    sout << getpid() << "/"
         << (unsigned long int)(void*)Glib::Thread::self();
#else
    sout << (unsigned long int)(void*)Glib::Thread::self();
#endif
    return sout.str();
  }

  void LogMessage::setDomain(std::string domain) {
    this->domain = domain;
  }

  static const int formatindex = std::ios_base::xalloc();

  std::ostream& operator<<(std::ostream& os, const LoggerFormat& format) {
    os.iword(formatindex) = format.format;
    return os;
  }

  std::ostream& operator<<(std::ostream& os, const LogMessage& message) {
    switch (os.iword(formatindex)) {
    case LongFormat:
      os << "[" << message.time << "] "
         << "[" << message.domain << "] "
         << "[" << message.level << "] "
         << "[" << message.identifier << "] "
         << message.message;
      break;
    case ShortFormat:
      os << message.level << ": " << message.message;
      break;
    }
    return os;
  }

  LogDestination::LogDestination()
    : format(LongFormat) {}

  LogDestination::LogDestination(const std::string& locale)
    : locale(locale),
      format(LongFormat) {}

  LogDestination::LogDestination(const LogDestination&) {
    // Executing this code should be impossible!
    exit(EXIT_FAILURE);
  }

  void LogDestination::operator=(const LogDestination&) {
    // Executing this code should be impossible!
    exit(EXIT_FAILURE);
  }

  void LogDestination::setFormat(const LogFormat& newformat) {
    format = newformat;
  }

  LogStream::LogStream(std::ostream& destination)
    : destination(destination) {}

  LogStream::LogStream(std::ostream& destination,
                       const std::string& locale)
    : LogDestination(locale),
      destination(destination) {}

  void LogStream::log(const LogMessage& message) {
    Glib::Mutex::Lock lock(mutex);
    const char *loc = NULL;
    if (!locale.empty()) {
      loc = setlocale(LC_ALL, NULL);
      setlocale(LC_ALL, locale.c_str());
    }
    destination << LoggerFormat(format) << message << std::endl;
    if (!locale.empty()) setlocale(LC_ALL, loc);
  }

  LogStream::LogStream(const LogStream&)
    : LogDestination(),
      destination(std::cerr) {
    // Executing this code should be impossible!
    exit(EXIT_FAILURE);
  }

  void LogStream::operator=(const LogStream&) {
    // Executing this code should be impossible!
    exit(EXIT_FAILURE);
  }

  LogFile::LogFile(const std::string& path)
    : LogDestination(),
      path(path),
      destination(),
      maxsize(-1),
      backups(-1),
      reopen(false) {
    if(path.empty()) {
      //logger.msg(Arc::ERROR,"Log file path is not specified");
      return;
    }
    destination.open(path.c_str(), std::fstream::out | std::fstream::app);
    if(!destination.is_open()) {
      //logger.msg(Arc::ERROR,"Failed to open log file: %s",path);
      return;
    }
  }

  LogFile::LogFile(const std::string& path, const std::string& locale)
    : LogDestination(locale),
      path(path),
      destination(),
      maxsize(-1),
      backups(-1),
      reopen(false) {
    if(path.empty()) {
      //logger.msg(Arc::ERROR,"Log file path is not specified");
      return;
    }
    destination.open(path.c_str(), std::fstream::out | std::fstream::app);
    if(!destination.is_open()) {
      //logger.msg(Arc::ERROR,"Failed to open log file: %s",path);
      return;
    }
  }
  void LogFile::setMaxSize(int newsize) {
    maxsize = newsize;
  }

  void LogFile::setBackups(int newbackups) {
    backups = newbackups;
  }
  void LogFile::setReopen(bool newreopen) {
    reopen = newreopen;
    if(reopen) {
      destination.close();
    } else {
      if(!destination.is_open()) {
        destination.open(path.c_str(), std::fstream::out | std::fstream::app);
      }
    }
  }

  LogFile::LogFile(void)
    : LogDestination(), maxsize(-1), backups(-1), reopen(false) {
    // Executing this code should be impossible!
    exit(EXIT_FAILURE);
  }

  LogFile::LogFile(const LogFile&)
    : LogDestination(), maxsize(-1), backups(-1), reopen(false) {
    // Executing this code should be impossible!
    exit(EXIT_FAILURE);
  }

  void LogFile::operator=(const LogFile&) {
    // Executing this code should be impossible!
    exit(EXIT_FAILURE);
  }
  LogFile::operator bool(void) {
    Glib::Mutex::Lock lock(mutex);
    return destination.is_open();
  }

  bool LogFile::operator!(void) {
    Glib::Mutex::Lock lock(mutex);
    return !destination.is_open();
  }

  void LogFile::log(const LogMessage& message) {
    Glib::Mutex::Lock lock(mutex);
    const char *loc = NULL;
    if (reopen) {
      destination.open(path.c_str(), std::fstream::out | std::fstream::app);
    }
    if(!destination.is_open()) return;
    if (!locale.empty()) {
      loc = setlocale(LC_ALL, NULL);
      setlocale(LC_ALL, locale.c_str());
    }
    destination << LoggerFormat(format) << message << std::endl;
    if (!locale.empty()) setlocale(LC_ALL, loc);
    if (reopen) destination.close();
    backup();
  }
  void LogFile::backup(void) {
    if(maxsize <= 0) return;
    if(destination.tellp() < maxsize) return;
    bool backup_done = true;
    // Not sure if this will work on windows, but glibmm
    // has no functions for removing and renaming files
    if(backups > 0) {
      std::string backup_path = path+"."+tostring(backups);
      ::unlink(backup_path.c_str());
      for(int n = backups;n>0;--n) {
        std::string old_backup_path = (n>1)?(path+"."+tostring(n-1)):path;
        if(::rename(old_backup_path.c_str(),backup_path.c_str()) != 0) {
          if(n == 1) backup_done=false;
        }
        backup_path = old_backup_path;
      }
    } else {
      if(::unlink(path.c_str()) != 0) backup_done=false;
    }
    if((backup_done) && (!reopen)) {
      destination.close();
      destination.open(path.c_str(), std::fstream::out | std::fstream::app);
    }
  }

  class LoggerThreadRef: public ThreadDataItem {
  private:
    Logger* logger_;
  public:
    LoggerThreadRef(Logger* logger);
    virtual void Dup(void);
    operator Logger*(void) { return logger_; };
  };

  LoggerThreadRef::LoggerThreadRef(Logger* logger):
              ThreadDataItem("ARC:LOGGER"),logger_(logger) {
  }

  void LoggerThreadRef::Dup(void) {
    new LoggerThreadRef(logger_);
  }

  Logger* Logger::rootLogger = NULL;
  std::map<std::string,LogLevel>* Logger::defaultThresholds = NULL;
  unsigned int Logger::rootLoggerMark = ~rootLoggerMagic;

  Logger& Logger::getRootLogger(void) {
    if ((rootLogger == NULL) || (rootLoggerMark != rootLoggerMagic)) {
      rootLogger = new Logger();
      defaultThresholds = new std::map<std::string,LogLevel>;
      rootLoggerMark = rootLoggerMagic;
      new LoggerThreadRef(rootLogger);
    }
    return *rootLogger;
  }

  Logger::Logger(Logger& parent,
                 const std::string& subdomain)
    : parent(&parent),
      domain(parent.getDomain() + "." + subdomain),
      threshold((LogLevel)0) {
    std::map<std::string,LogLevel>::const_iterator thr =
                                   defaultThresholds->find(domain);
    if(thr != defaultThresholds->end()) {
      threshold = thr->second;
    }
  }

  Logger::Logger(Logger& parent,
                 const std::string& subdomain,
                 LogLevel threshold)
    : parent(&parent),
      domain(parent.getDomain() + "." + subdomain),
      threshold(threshold) {
  }

  Logger::~Logger() {
  }

  void Logger::addDestination(LogDestination& destination) {
    destinations.push_back(&destination);
  }

  void Logger::removeDestinations(void) {
    destinations.clear();
  }

  void Logger::setThreshold(LogLevel threshold) {
    this->threshold = threshold;
  }

  void Logger::setThresholdForDomain(LogLevel threshold,
                                     const std::list<std::string>& subdomains) {
    setThresholdForDomain(threshold, list_to_domain(subdomains));
  }

  void Logger::setThresholdForDomain(LogLevel threshold,
                                     const std::string& domain) {
    getRootLogger();
    if(domain.empty() || (domain == "Arc")) {
      getRootLogger().setThreshold(threshold);
    } else {
      (*defaultThresholds)[domain] = threshold;
    }
  }

  LogLevel Logger::getThreshold() const {
    if(threshold != (LogLevel)0) return threshold;
    if(parent) return parent->getThreshold();
    return (LogLevel)0;
  }

  void Logger::msg(LogMessage message) {
    message.setDomain(domain);
    if (message.getLevel() >= getThreshold()) {
      log(message);
    }
  }

  Logger::Logger()
    : parent(0),
      domain("Arc"),
      threshold(DEBUG) {
    // addDestination(cerr);
  }

  Logger::Logger(const Logger&) {
    // Executing this code should be impossible!
    exit(EXIT_FAILURE);
  }

  void Logger::operator=(const Logger&) {
    // Executing this code should be impossible!
    exit(EXIT_FAILURE);
  }

  std::string Logger::getDomain() {
    return domain;
  }

  void Logger::log(const LogMessage& message) {
    std::list<LogDestination*>::iterator dest;
    std::list<LogDestination*>::iterator begin = destinations.begin();
    std::list<LogDestination*>::iterator end = destinations.end();
    for (dest = begin; dest != end; ++dest)
      (*dest)->log(message);
    if (parent)
      parent->log(message);
  }

}
