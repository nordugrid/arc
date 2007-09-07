#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

// Plexer.cpp

#include "Plexer.h"

namespace Arc {

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
      if (iter->label.match(path, unmatched)) {
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

  //XXX: workaround because the python bindig segmentation fault
  // Arc::Logger Arc::Plexer::logger(Arc::MCC::logger,"Plexer");

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
