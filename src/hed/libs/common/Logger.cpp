// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <sstream>
#include <fstream>

#include <unistd.h>

#include "Logger.h"
#include "DateTime.h"
#include "StringConv.h"

#include <unistd.h>
#ifdef WIN32
#include <process.h>
#endif

#undef rootLogger

namespace Arc {

  std::ostream& operator<<(std::ostream& os, LogLevel level) {
    if (level == VERBOSE)
      os << "VERBOSE";
    else if (level == DEBUG)
      os << "DEBUG";
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
    if (str == "VERBOSE")
      return VERBOSE;
    else if (str == "DEBUG")
      return DEBUG;
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

  std::string level_to_string(const LogLevel& level) {
    switch (level) {
      case VERBOSE:
        return "VERBOSE";
      case DEBUG:
        return "DEBUG";
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

  std::ostream& operator<<(std::ostream& os, const LogMessage& message) {
    os << "[" << message.time << "] "
                        << "[" << message.domain << "] "
                        << "[" << message.level << "] "
                        << "[" << message.identifier << "] "
                        << message.message;
    return os;
  }

  LogDestination::LogDestination() {}

  LogDestination::LogDestination(const std::string& locale)
    : locale(locale) {}

  LogDestination::LogDestination(const LogDestination&) {
    // Executing this code should be impossible!
    exit(EXIT_FAILURE);
  }

  void LogDestination::operator=(const LogDestination&) {
    // Executing this code should be impossible!
    exit(EXIT_FAILURE);
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
    destination << message << std::endl;
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
      backups(-1) {
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
      backups(-1) {
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

  LogFile::LogFile(void)
    : LogDestination(), maxsize(-1), backups(-1) {
    // Executing this code should be impossible!
    exit(EXIT_FAILURE);
  }

  LogFile::LogFile(const LogFile&)
    : LogDestination(), maxsize(-1), backups(-1) {
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
    if(!destination.is_open()) return;
    const char *loc = NULL;
    if (!locale.empty()) {
      loc = setlocale(LC_ALL, NULL);
      setlocale(LC_ALL, locale.c_str());
    }
    destination << message << std::endl;
    if (!locale.empty()) setlocale(LC_ALL, loc);
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
    if(backup_done) {
      destination.close();
      destination.open(path.c_str(), std::fstream::out | std::fstream::app);
    }
  }

  Logger*Logger::rootLogger = NULL;
  unsigned int Logger::rootLoggerMark = ~rootLoggerMagic;

  Logger& Logger::getRootLogger(void) {
    if ((rootLogger == NULL) || (rootLoggerMark != rootLoggerMagic)) {
      rootLogger = new Logger();
      rootLoggerMark = rootLoggerMagic;
    }
    return *rootLogger;
  }

  // LogStream Logger::cerr(std::cerr);

  Logger::Logger(Logger& parent,
                 const std::string& subdomain)
    : parent(&parent),
      domain(parent.getDomain() + "." + subdomain),
      threshold(parent.getThreshold()) {
    // Nothing else needs to be done.
  }

  Logger::Logger(Logger& parent,
                 const std::string& subdomain,
                 LogLevel threshold)
    : parent(&parent),
      domain(parent.getDomain() + "." + subdomain),
      threshold(threshold) {
    // Nothing else needs to be done.
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

  LogLevel Logger::getThreshold() const {
    return threshold;
  }

  void Logger::msg(LogMessage message) {
    message.setDomain(domain);
    log(message);
  }

  Logger::Logger()
    : parent(0),
      domain("Arc"),
      threshold(VERBOSE) {
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
    if (message.getLevel() >= threshold) {
      std::list<LogDestination*>::iterator dest;
      std::list<LogDestination*>::iterator begin = destinations.begin();
      std::list<LogDestination*>::iterator end = destinations.end();
      for (dest = begin; dest != end; ++dest)
        (*dest)->log(message);
      if (parent != 0)
        parent->log(message);
    }
  }

}
