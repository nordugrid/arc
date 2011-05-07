#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <iostream>
//#include <globus_gsi_credential.h>

#include <vector>

//#include <arc/globusutils/GlobusErrorUtils.h>
//#include <arc/credential/VOMSUtil.h>
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
  bool auto_cert = false;
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
  if(!auto_c.empty()) {
    if(auto_c == "auto") auto_cert=true;
  };
  logger.msg(Arc::VERBOSE, "VOMS config: vo: %s", vo);
  logger.msg(Arc::VERBOSE, "VOMS config: group: %s", group);
  logger.msg(Arc::VERBOSE, "VOMS config: role: %s", role);
  logger.msg(Arc::VERBOSE, "VOMS config: capabilities: %s", capabilities);
  if(voms_data_.size() == 0) return AAA_NO_MATCH;
  // analyse permissions
  for(std::vector<struct voms>::iterator v = voms_data_.begin();v!=voms_data_.end();++v) {
    logger.msg(Arc::DEBUG, "match vo: %s", v->voname);
    if((vo == "*") || (vo == v->voname)) {
      for(std::vector<struct voms_attrs>::iterator d=v->attrs.begin();d!=v->attrs.end();++d) {
        logger.msg(Arc::VERBOSE, "match group: %s", d->group);
        logger.msg(Arc::VERBOSE, "match role: %s", d->role);
        logger.msg(Arc::VERBOSE, "match capabilities: %s", d->cap);
        if(((group == "*") || (group == d->group)) &&
           ((role == "*") || (role == d->role)) &&
           ((capabilities == "*") || (capabilities == d->cap))) {
          logger.msg(Arc::VERBOSE, "VOMS matched");
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
  logger.msg(Arc::VERBOSE, "VOMS matched nothing");
  return AAA_NO_MATCH;
}

} // namespace ArcSHCLegacy

