// Logger.h

#ifndef __Logger__
#define __Logger__

#include <set>
#include <string>
#include <iostream>
#include <glibmm/thread.h>

namespace Arc {

  enum LogLevel {
    VERBOSE = 1,
    DEBUG = 2,
    INFO = 4,
    WARNING = 8,
    ERROR = 16,
    FATAL = 32
  };

  std::ostream& operator<<(std::ostream& os, LogLevel level);
  
  class LogMessage {
  public:
    LogMessage(LogLevel level,
	       const std::string& message);
    LogMessage(LogLevel level,
	       const std::string& message,
	       const std::string& identifier);
    LogLevel getLevel() const;
  protected:
    void setIdentifier(std::string identifier);
  private:
    static std::string getCurrentTime();
    static std::string getDefaultIdentifier();
    void setDomain(std::string domain);
    std::string time;
    LogLevel level;
    std::string domain;
    std::string identifier;
    std::string message;
    friend class Logger;
    friend std::ostream& operator<<(std::ostream& os,
				    const LogMessage& message);
  };

  class LogDestination {
  public:
    virtual void log(const LogMessage& message) = 0;
  protected:
    LogDestination();
  private:
    LogDestination(const LogDestination& unique);
    void operator=(const LogDestination& unique);
  };

  class LogStream : public LogDestination {
  public:
    LogStream(std::ostream& destination);
    virtual void log(const LogMessage& message);
  private:
    LogStream(const LogStream& unique);
    void operator=(const LogStream& unique);
    std::ostream& destination;
    Glib::Mutex mutex;
  };

  class Logger {
  public:
    static Logger rootLogger;
    static LogStream cerr;
    Logger(Logger& parent,
	   const std::string& subdomain);
    Logger(Logger& parent,
	   const std::string& subdomain,
	   LogLevel treshold);
    void addDestination(LogDestination& destination);
    void setTreshold(LogLevel treshold);
    LogLevel getTreshold() const;
    void msg(LogMessage message);
  private:
    Logger();
    Logger(const Logger& unique);
    void operator=(const Logger& unique);
    std::string getDomain();
    void log(const LogMessage& message);
    Logger* parent;
    std::string domain;
    std::list<LogDestination*> destinations;
    LogLevel treshold;
  };

}

#define MSG(logger, level, message) {		\
  if (level>=logger.getTreshold())			\
    logger.msg(Arc::LogMessage(level,message));}

#endif
