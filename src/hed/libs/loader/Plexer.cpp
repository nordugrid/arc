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
    std::list<std::string> unmatched;
    return match(str,unmatched) && (unmatched.size() == 0);
  }

  bool RegularExpression::match(const std::string& str,std::list<std::string>& unmatched) const {
    if (status==0){
      int st;
      regmatch_t rm[256];
      unmatched.clear();
      st = regexec(&preg, str.c_str(), 256, rm, 0);
      if(st != 0) return false;
      regoff_t p = 0;
      for(int n = 0; n<256; ++n) {
        if(rm[n].rm_so == -1) break;
        if(rm[n].rm_so > p) {
          unmatched.push_back(str.substr(p,rm[n].rm_so-p));
        };
        p=rm[n].rm_eo;
      };
      if(p < str.length()) unmatched.push_back(str.substr(p));
      return true;
    }
    else {
      return false;
    }
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
	logger.msg(WARNING, "Bad label: \"%s\"", label.c_str());
    }
    else
      for (iter=services.begin(); iter!=services.end(); ++iter)
	if (iter->label.hasPattern(label))
	  services.erase(iter);
  }
  
  MCC_Status Plexer::process(Message& request, Message& response){
    std::string ep = request.Attributes()->get("ENDPOINT");
    std::string path = getPath(ep);
    logger.msg(DEBUG, "Operation on path \"%s\"",path.c_str());
    std::list<PlexerEntry>::iterator iter;
    for (iter=services.begin(); iter!=services.end(); ++iter) {
      std::list<std::string> unmatched;
      if (iter->label.match(path)) {
        request.Attributes()->set("PLEXER:PATTERN",iter->label.getPattern());
        if(unmatched.size() > 0) {
          request.Attributes()->set("PLEXER:EXTENSION",*(unmatched.end()--));
        };
        return iter->service->process(request, response);
      }
    }
    logger.msg(WARNING, "No service at path \"%s\"",path.c_str());
    return Arc::MCC_Status(Arc::UNKNOWN_SERVICE_ERROR,
			   (std::string)("MCC Plexer"),
			   path);  
  }

  Arc::Logger Arc::Plexer::logger(Arc::MCC::logger,"Plexer");

  std::string Plexer::getPath(std::string url){
    std::string::size_type ds, ps;
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
