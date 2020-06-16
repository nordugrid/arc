#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <iostream>

#include <vector>

#include <arc/Logger.h>
#include <arc/StringConv.h>

#include "auth.h"

namespace ArcSHCLegacy {

static Arc::Logger logger(Arc::Logger::getRootLogger(),"AuthUserVOMS");

static bool match_value(const std::string& value, const std::vector<std::string>& seq) {
  for(std::vector<std::string>::const_iterator v = seq.begin(); v != seq.end(); ++v) {
    if(*v == value) {
      return true;
    }
  };
  return false;
}

AuthResult AuthUser::match_voms(const char* line) {
  // voms = vo group role capabilities

  // No need to process anything if no VOMS extensions are present
  if(voms_data_.empty()) return AAA_NO_MATCH;

  // parse line
  std::string vo("");
  std::string group("");
  std::string role("");
  std::string capabilities("");
  std::string auto_c("");
  std::string::size_type n = 0;
  n=Arc::get_token(vo,line,n," ");
  if((n == std::string::npos) && (vo.empty())) {
    logger.msg(Arc::ERROR, "Missing VO in configuration");
    return AAA_FAILURE;
  };
  n=Arc::get_token(group,line,n," ");
  if((n == std::string::npos) && (group.empty())) {
    logger.msg(Arc::ERROR, "Missing group in configuration");
    return AAA_FAILURE;
  };
  n=Arc::get_token(role,line,n," ");
  if((n == std::string::npos) && (role.empty())) {
    logger.msg(Arc::ERROR, "Missing role in configuration");
    return AAA_FAILURE;
  };
  n=Arc::get_token(capabilities,line,n," ");
  if((n == std::string::npos) && (capabilities.empty())) {
    logger.msg(Arc::ERROR, "Missing capabilities in configuration");
    return AAA_FAILURE;
  };
  n=Arc::get_token(auto_c,line,n," ");
  if(!auto_c.empty()) {
    logger.msg(Arc::ERROR, "Too many arguments in configuration");
    return AAA_FAILURE;
  };
  logger.msg(Arc::VERBOSE, "Rule: vo: %s", vo);
  logger.msg(Arc::VERBOSE, "Rule: group: %s", group);
  logger.msg(Arc::VERBOSE, "Rule: role: %s", role);
  logger.msg(Arc::VERBOSE, "Rule: capabilities: %s", capabilities);
  // analyse permissions
  for(std::vector<struct voms_t>::iterator v = voms_data_.begin();v!=voms_data_.end();++v) {
    logger.msg(Arc::DEBUG, "Match vo: %s", v->voname);
    if((vo == "*") || (vo == v->voname)) {
      bool matched = false;
      for(std::vector<struct voms_fqan_t>::iterator f = v->fqans.begin(); f != v->fqans.end(); ++f) {
        if(((group == "*") || (group == f->group)) &&
           ((role == "*") || (role == f->role)) &&
           ((capabilities == "*") || (capabilities == f->capability))) {
          logger.msg(Arc::VERBOSE, "Matched: %s %s %s %s",v->voname,f->group,f->role,f->capability);
          if(!matched) {
            default_voms_ = voms_t();
            default_voms_.voname = v->voname;
            default_voms_.server = v->server;
            matched = true;
          };
          default_voms_.fqans.push_back(*f);
        };
      };
      if(matched) {
        return AAA_POSITIVE_MATCH;
      };
    };
  };
  logger.msg(Arc::VERBOSE, "Matched nothing");
  return AAA_NO_MATCH;
}

} // namespace ArcSHCLegacy

