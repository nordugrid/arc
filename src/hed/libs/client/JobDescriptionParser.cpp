// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/Logger.h>

#include "JobDescriptionParser.h"

namespace Arc {

  Logger JobDescriptionParser::logger(Logger::getRootLogger(),
                                      "JobDescriptionParser");

  JobDescriptionParser::JobDescriptionParser() {}

  JobDescriptionParser::~JobDescriptionParser() {}

  void JobDescriptionParser::AddHint(const std::string& key,const std::string& value) {
    if(key.empty()) return;
    hints[key] = value;
  }
 
  std::string JobDescriptionParser::GetHint(const std::string& key) const {
    std::map<std::string,std::string>::const_iterator h = hints.find(key);
    if(h == hints.end()) return "";
    return h->second;
  }

  void JobDescriptionParser::SetHints(const std::map<std::string,std::string>& hints) {
    this->hints = hints;
  }


} // namespace Arc
