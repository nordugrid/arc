#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <sstream>

#include "Logger.h"
#include "DateTime.h"

#include <unistd.h>
#ifdef WIN32
#include <process.h>
#endif

#undef rootLogger

namespace Arc {

  std::ostream& operator<<(std::ostream& os, LogLevel level) {
    if(level == VERBOSE)
      os << "VERBOSE";
    else if(level == DEBUG)
      os << "DEBUG";
    else if(level == INFO)
      os << "INFO";
    else if(level == WARNING)
      os << "WARNING";
    else if(level == ERROR)
      os << "ERROR";
    else if(level == FATAL)
      os << "FATAL";
    // There should be no more alternative!
    return os;
  }

  LogLevel string_to_level(const std::string& str) {
    if(str == "VERBOSE")
      return VERBOSE;
    else if(str == "DEBUG")
      return DEBUG;
    else if(str == "INFO")
      return INFO;
    else if(str == "WARNING")
      return WARNING;
    else if(str == "ERROR")
      return ERROR;
    else if(str == "FATAL")
      return FATAL;
    else  // should not happen...
      return FATAL;
  }

  LogMessage::LogMessage(LogLevel level,
                         const IString& message) :
    time(TimeStamp()),
    level(level),
    domain("---"),
    identifier(getDefaultIdentifier()),
    message(message) {}

  LogMessage::LogMessage(LogLevel level,
                         const IString& message,
                         const std::string& identifier) :
    time(TimeStamp()),
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

  LogDestination::LogDestination(const std::string& locale) : locale(locale) {}

  LogDestination::LogDestination(const LogDestination&) {
    // Executing this code should be impossible!
    exit(EXIT_FAILURE);
  }

  void LogDestination::operator=(const LogDestination&) {
    // Executing this code should be impossible!
    exit(EXIT_FAILURE);
  }

  LogStream::LogStream(std::ostream& destination) : destination(destination) {}

  LogStream::LogStream(std::ostream& destination,
                       const std::string& locale) : LogDestination(locale),
                                                    destination(destination) {}

  void LogStream::log(const LogMessage& message) {
    Glib::Mutex::Lock lock(mutex);
    char* loc = setlocale(LC_ALL, NULL);
    setlocale(LC_ALL, locale.c_str());
    destination << message << std::endl;
    setlocale(LC_ALL, loc);
  }

  LogStream::LogStream(const LogStream&) : LogDestination(),
                                           destination(std::cerr) {
    // Executing this code should be impossible!
    exit(EXIT_FAILURE);
  }

  void LogStream::operator=(const LogStream&) {
    // Executing this code should be impossible!
    exit(EXIT_FAILURE);
  }

  Logger* Logger::rootLogger = NULL;
  unsigned int Logger::rootLoggerMark = ~rootLoggerMagic;

  Logger& Logger::getRootLogger(void) {
    if((rootLogger == NULL) || (rootLoggerMark != rootLoggerMagic)) {
      rootLogger = new Logger();
      rootLoggerMark = rootLoggerMagic;
    }
    return *rootLogger;
  }

  // LogStream Logger::cerr(std::cerr);

  Logger::Logger(Logger& parent,
                 const std::string& subdomain) :
    parent(&parent),
    domain(parent.getDomain() + "." + subdomain),
    threshold(parent.getThreshold()) {
    // Nothing else needs to be done.
  }

  Logger::Logger(Logger& parent,
                 const std::string& subdomain,
                 LogLevel threshold) :
    parent(&parent),
    domain(parent.getDomain() + "." + subdomain),
    threshold(threshold) {
    // Nothing else needs to be done.
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

  Logger::Logger() :
    parent(0),
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
    if(message.getLevel() >= threshold) {
      std::list<LogDestination*>::iterator dest;
      std::list<LogDestination*>::iterator begin = destinations.begin();
      std::list<LogDestination*>::iterator end = destinations.end();
      for(dest = begin; dest != end; ++dest) {
        (*dest)->log(message);
      }
      if(parent != 0) {
        parent->log(message);
      }
    }
  }

}
