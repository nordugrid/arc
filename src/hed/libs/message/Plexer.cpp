#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

// Plexer.cpp

#include "Plexer.h"

namespace Arc {

  PlexerEntry::PlexerEntry(const RegularExpression& label,
			   MCCInterface* mcc) :
    label(label),
    mcc(mcc)
  {
  }

  Plexer::Plexer(Config *cfg) : MCC(cfg) {
  }

  Plexer::~Plexer(){
  }

  void Plexer::Next(MCCInterface* next, const std::string& label){
    std::list<PlexerEntry>::iterator iter;
    if (next!=0) {
      RegularExpression regex(label);
      if (regex.isOk()) {
        mccs.push_front(PlexerEntry(regex,next));
      } else {
        logger.msg(WARNING, "Bad label: \"%s\"", label);
      }
    } else {
      for (iter=mccs.begin(); iter!=mccs.end();) {
        if (iter->label.hasPattern(label)) {
          iter = mccs.erase(iter);
        } else {
          ++iter;
        }
      }
    }
  }
  
  MCC_Status Plexer::process(Message& request, Message& response){
    std::string ep = request.Attributes()->get("ENDPOINT");
    std::string path = getPath(ep);
    logger.msg(VERBOSE, "Operation on path \"%s\"",path);
    std::list<PlexerEntry>::iterator iter;
    for (iter=mccs.begin(); iter!=mccs.end(); ++iter) {
      std::list<std::string> unmatched, matched;
      if (iter->label.match(path, unmatched, matched)) {
        request.Attributes()->set("PLEXER:PATTERN",iter->label.getPattern());
        request.Attributes()->set("PLEXER:EXTENSION", "");
        if(unmatched.size() > 0) {
          request.Attributes()->set("PLEXER:EXTENSION",*(--unmatched.end()));
        };
        return iter->mcc->process(request, response);
      }
    }
    logger.msg(WARNING, "No next MCC or Service at path \"%s\"",path);
    return MCC_Status(UNKNOWN_SERVICE_ERROR,
			   (std::string)("MCC Plexer"),
			   path);  
  }

  //XXX: workaround because the python bindig segmentation fault
  Logger Plexer::logger(Logger::rootLogger,"Plexer");

  std::string Plexer::getPath(std::string url){
    // TODO: Need even more reliable URL detection
    std::string::size_type ds, ps;
    ds=url.find("://");
    if (ds==std::string::npos)
      ps=url.find("/");
    else
      ps=url.find("/", ds+3);
    if (ps==std::string::npos)
      return "";
    else
      return url.substr(ps);
  }
  
}
