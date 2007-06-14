// Logger.cpp

#include <sstream>
#include <glib.h>
#include "Logger.h"

namespace Arc {
  
  std::ostream& operator<<(std::ostream& os, LogLevel level){
    if (level==VERBOSE)
      os << "VERBOSE";
    else if (level==DEBUG)
      os << "DEBUG";
    else if (level==INFO)
      os << "INFO";
    else if (level==WARNING)
      os << "WARNING";
    else if (level==ERROR)
      os << "ERROR";
    else if (level==FATAL)
      os << "FATAL";
    else  // There should be no more alternative!
      ;
  }
  
  LogLevel string_to_level(std::string& str) 
  {
    if (str == "VERBOSE") {
        return VERBOSE;
    } else if (str == "DEBUG") {
        return DEBUG;
    } else if (str == "INFO") {
        return INFO;
    } else if (str == "WARNING") {
        return WARNING;
    } else if (str == "ERROR") {
        return ERROR;
    } else if (str == "FATAL") {
        return FATAL;
    }
  }

  LogMessage::LogMessage(LogLevel level,
			 const std::string& message) :
    time(getCurrentTime()),
    level(level),
    domain("---"),
    identifier(getDefaultIdentifier()),
    message(message)
  {
    // Nothing else needs to be done.
  }

  LogMessage::LogMessage(LogLevel level,
			 const std::string& message,
			 const std::string& identifier) :
    time(getCurrentTime()),
    level(level),
    domain("---"),
    identifier(identifier),
    message(message)
  {
    // Nothing else needs to be done.
  }
  
  LogLevel LogMessage::getLevel() const{
    return level;
  }

  void LogMessage::setIdentifier(std::string identifier){
    this->identifier = identifier;
  }
  
  std::string LogMessage::getCurrentTime(){
    Glib::TimeVal tv;
    gchar tc[51];
    g_get_current_time(&tv);
    strftime(tc, 50, "%Y-%m-%d %H:%M:%S", gmtime(&(tv.tv_sec)));
    return std::string(tc);
  }

  std::string LogMessage::getDefaultIdentifier(){
    std::ostringstream sout;
    sout << getpid() << "/"
	 << (unsigned long int)(void*)Glib::Thread::self();
    return sout.str();
  }

  void LogMessage::setDomain(std::string domain){
    this->domain = domain;
  }
  
  std::ostream& operator<<(std::ostream& os, const LogMessage& message){
    // TODO: Format the time in some human-readable way!
    os << "[" << message.time << "] "
       << "[" << message.domain << "] "
       << "[" << message.level << "] "
       << "[" << message.identifier << "] "
       << message.message;
    /*
    os << "Time:       " << message.time << std::endl
       << "Level:      " << message.level << std::endl
       << "Domain:     " << message.domain << std::endl
       << "Identifier: " << message.identifier << std::endl
       << message.message << std::endl;
    */
  }

  LogDestination::LogDestination(){
    // Nothing needs to be done here.
  }

  LogDestination::LogDestination(const LogDestination& unique){
    // Executing this code should be impossible!
    exit(EXIT_FAILURE);    
  }

  void LogDestination::operator=(const LogDestination& unique){
    // Executing this code should be impossible!
    exit(EXIT_FAILURE);
  }

  LogStream::LogStream(std::ostream& destination) : destination(destination) {
    // Nothing else needs to be done.
  }

  void LogStream::log(const LogMessage& message){
    Glib::Mutex::Lock lock(mutex);
    destination << message << std::endl;
  }

  LogStream::LogStream(const LogStream& unique) : destination(std::cerr) {
    // Executing this code should be impossible!
    exit(EXIT_FAILURE);
  }

  void LogStream::operator=(const LogStream& unique){
    // Executing this code should be impossible!
    exit(EXIT_FAILURE);
  }
  
  Logger Logger::rootLogger;

  // LogStream Logger::cerr(std::cerr);

  Logger::Logger(Logger& parent,
		 const std::string& subdomain) :
    parent(&parent),
    domain(parent.getDomain()+"."+subdomain),
    threshold(parent.getThreshold())
  {
    // Nothing else needs to be done.
  }

  Logger::Logger(Logger& parent,
		 const std::string& subdomain,
		 LogLevel threshold) :
    parent(&parent),
    domain(parent.getDomain()+"."+subdomain),
    threshold(threshold)
  {
    // Nothing else needs to be done.
  }

  void Logger::addDestination(LogDestination& destination){
    destinations.push_back(&destination);
  }

  void Logger::setThreshold(LogLevel threshold){
    this->threshold=threshold;
  }

  LogLevel Logger::getThreshold() const{
    return threshold;
  }

  void Logger::msg(LogMessage message){
    message.setDomain(domain);
    log(message);
  }
  
  void Logger::msg(LogLevel level, const char *str) {
    msg(LogMessage(level, str)); 
  }

  Logger::Logger() :
    parent(0),
    domain("Arc"),
    threshold(VERBOSE)
  {
    // addDestination(cerr);
  }

  Logger::Logger(const Logger& unique){
    // Executing this code should be impossible!
    exit(EXIT_FAILURE);
  }

  void Logger::operator=(const Logger& unique){
    // Executing this code should be impossible!
    exit(EXIT_FAILURE);
  }

  std::string Logger::getDomain(){
    return domain;
  }

  void Logger::log(const LogMessage& message){
    if (message.getLevel()>=threshold){
      std::list<LogDestination*>::iterator dest;
      std::list<LogDestination*>::iterator begin=destinations.begin();
      std::list<LogDestination*>::iterator end=destinations.end();
      for (dest=begin; dest!=end; ++dest) {
        (*dest)->log(message);
      }
      if (parent!=0) { 
        parent->log(message); 
      }
    }
  }

}
