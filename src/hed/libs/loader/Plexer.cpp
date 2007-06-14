// Plexer.cpp

#include "Plexer.h"

namespace Arc {
  
  RegularExpression::RegularExpression(std::string pattern) :
    pattern(pattern)
  {
    status=regcomp(&preg, pattern.c_str(), REG_EXTENDED);
  }
  
  RegularExpression::RegularExpression(const RegularExpression& regex) :
    pattern(regex.pattern)
  {
    status=regcomp(&preg, pattern.c_str(), 0);
  }
  
  RegularExpression::~RegularExpression(){
    regfree(&preg);
  }
  
  const RegularExpression&
  RegularExpression::operator=(const RegularExpression& regex){
    regfree(&preg);
    pattern=regex.pattern;
    status=regcomp(&preg, pattern.c_str(), 0);
    return *this;
  }
  
  bool RegularExpression::isOk(){
    return status==0;
  }
  
  bool RegularExpression::hasPattern(std::string str){
    return pattern==str;
  }
  
  bool RegularExpression::match(const std::string& str) const {
    if (status==0){
      int st;
      regmatch_t rm;
      st = regexec(&preg, str.c_str(), 1, &rm, 0);
      return st==0 and rm.rm_so==0 and rm.rm_eo==str.size();
    }
    else
      return false;
  }

  std::string RegularExpression::getPattern() {
    return pattern;
  }

  PlexerEntry::PlexerEntry(const RegularExpression& label,
			   MCCInterface* service) :
    label(label),
    service(service)
  {
  }

  Plexer::Plexer(Config *cfg) : MCC(cfg) {
  }

  Plexer::~Plexer(){
  }

  void Plexer::Next(MCCInterface* next, const std::string& label){
    std::list<PlexerEntry>::iterator iter;
    if (next!=0){
      RegularExpression regex(label);
      if (regex.isOk())
	services.push_front(PlexerEntry(regex,next));
      else
	logger.msg(LogMessage(WARNING, "Bad label: \""+label+"\""));
    }
    else
      for (iter=services.begin(); iter!=services.end(); ++iter)
	if (iter->label.hasPattern(label))
	  services.erase(iter);
  }
  
  MCC_Status Plexer::process(Message& request, Message& response){
    std::string ep = request.Attributes()->get("ENDPOINT");
    std::string path = getPath(ep);
    std::list<PlexerEntry>::iterator iter;
    for (iter=services.begin(); iter!=services.end(); ++iter) {
      if (iter->label.match(path)) {
	    return iter->service->process(request, response);
      }
    }
    return Arc::MCC_Status(Arc::UNKNOWN_SERVICE_ERROR,
			   (std::string)("MCC Plexer"),
			   path);  
  }

  Arc::Logger Arc::Plexer::logger(Arc::MCC::logger,"Plexer");

  std::string Plexer::getPath(std::string url){
    int ds, ps;
    ds=url.find("//");
    if (ds==std::string::npos)
      ps=url.find("/");
    else
      ps=url.find("/", ds+2);
    if (ps==std::string::npos)
      return "";
    else
      return url.substr(ps);
  }
  
}
