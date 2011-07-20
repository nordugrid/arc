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

int AuthUser::match_voms(const char* line) {
  // parse line
  std::string vo("");
  std::string group("");
  std::string role("");
  std::string capabilities("");
  std::string auto_c("");
  std::string::size_type n = 0;
  n=Arc::get_token(vo,line,n," ","\"","\"");
  if((n == std::string::npos) && (vo.empty())) {
    logger.msg(Arc::ERROR, "Missing VO in configuration");
    return AAA_FAILURE;
  };
  n=Arc::get_token(group,line,n," ","\"","\"");
  if((n == std::string::npos) && (group.empty())) {
    logger.msg(Arc::ERROR, "Missing group in configuration");
    return AAA_FAILURE;
  };
  n=Arc::get_token(role,line,n," ","\"","\"");
  if((n == std::string::npos) && (role.empty())) {
    logger.msg(Arc::ERROR, "Missing role in configuration");
    return AAA_FAILURE;
  };
  n=Arc::get_token(capabilities,line,n," ","\"","\"");
  if((n == std::string::npos) && (capabilities.empty())) {
    logger.msg(Arc::ERROR, "Missing capabilities in configuration");
    return AAA_FAILURE;
  };
  n=Arc::get_token(auto_c,line,n," ","\"","\"");
  logger.msg(Arc::VERBOSE, "Rule: vo: %s", vo);
  logger.msg(Arc::VERBOSE, "Rule: group: %s", group);
  logger.msg(Arc::VERBOSE, "Rule: role: %s", role);
  logger.msg(Arc::VERBOSE, "Rule: capabilities: %s", capabilities);
  if(voms_data_.empty()) return AAA_NO_MATCH;
  // analyse permissions
  for(std::vector<struct voms>::iterator v = voms_data_.begin();v!=voms_data_.end();++v) {
    logger.msg(Arc::DEBUG, "Match vo: %s", v->voname);
    if((vo == "*") || (vo == v->voname)) {
      for(std::vector<struct voms_attrs>::iterator d=v->attrs.begin();d!=v->attrs.end();++d) {
        logger.msg(Arc::VERBOSE, "Match group: %s", d->group);
        logger.msg(Arc::VERBOSE, "Match role: %s", d->role);
        logger.msg(Arc::VERBOSE, "Match capabilities: %s", d->cap);
        if(((group == "*") || (group == d->group)) &&
           ((role == "*") || (role == d->role)) &&
           ((capabilities == "*") || (capabilities == d->cap))) {
          logger.msg(Arc::VERBOSE, "Match: %s %s %s %s",v->voname,d->group,d->role,d->cap);
          default_voms_=v->server.c_str();
          default_vo_=v->voname.c_str();
          default_role_=d->role.c_str();
          default_capability_=d->cap.c_str();
          default_vgroup_=d->group.c_str();
          return AAA_POSITIVE_MATCH;
        };
      };
    };
  };
  logger.msg(Arc::VERBOSE, "Matched nothing");
  return AAA_NO_MATCH;
}

} // namespace ArcSHCLegacy

