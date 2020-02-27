#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <iostream>

#include <vector>

#include <arc/Logger.h>
#include <arc/StringConv.h>

#include "auth.h"

namespace ArcSHCLegacy {

static Arc::Logger logger(Arc::Logger::getRootLogger(),"AuthUserSciTokens");

AuthResult AuthUser::match_scitokens(const char* line) {
  // No need to process anything if no SciTokens is present
  if(scitokens_data_.empty()) return AAA_NO_MATCH;
  // parse line
  std::string subject("");
  std::string issuer("");
  std::string audience("");
  std::string scope("");
  std::string::size_type n = 0;
  n=Arc::get_token(subject,line,n," ","\"","\"");
  if((n == std::string::npos) && (subject.empty())) {
    logger.msg(Arc::ERROR, "Missing subject in configuration");
    return AAA_FAILURE;
  };
  n=Arc::get_token(issuer,line,n," ","\"","\"");
  if((n == std::string::npos) && (issuer.empty())) {
    logger.msg(Arc::ERROR, "Missing issuer in configuration");
    return AAA_FAILURE;
  };
  n=Arc::get_token(audience,line,n," ","\"","\"");
  if((n == std::string::npos) && (audience.empty())) {
    logger.msg(Arc::ERROR, "Missing audience in configuration");
    return AAA_FAILURE;
  };
  n=Arc::get_token(scope,line,n," ","\"","\"");
  if((n == std::string::npos) && (scope.empty())) {
    logger.msg(Arc::ERROR, "Missing scope in configuration");
    return AAA_FAILURE;
  };
  logger.msg(Arc::VERBOSE, "Rule: subject: %s", subject);
  logger.msg(Arc::VERBOSE, "Rule: issuer: %s", issuer);
  logger.msg(Arc::VERBOSE, "Rule: audience: %s", audience);
  logger.msg(Arc::VERBOSE, "Rule: scope: %s", scope);
  // analyse permissions
  for(std::vector<struct scitokens_t>::iterator v = scitokens_data_.begin();v!=scitokens_data_.end();++v) {
    logger.msg(Arc::DEBUG, "Match issuer: %s", v->issuer);
    if((issuer == "*") || (issuer == v->issuer)) {
      if((subject == "*") || (subject == v->subject)) {
        if((audience == "*") || (audience == v->audience)) {
          if((scope == "*") || (std::find(v->scopes.begin(),v->scopes.end(),scope) != v->scopes.end())) {
            logger.msg(Arc::VERBOSE, "Matched: %s %s %s",v->subject,v->issuer,v->audience,scope);
            default_scitokens_ = scitokens_t();
            default_scitokens_.subject = v->subject;
            default_scitokens_.issuer = v->issuer;
            default_scitokens_.audience = v->audience;
            if(scope != "*") default_scitokens_.scopes.push_back(scope);
            return AAA_POSITIVE_MATCH;
          };
        };
      };
    };
  };
  logger.msg(Arc::VERBOSE, "Matched nothing");
  return AAA_NO_MATCH;
}

} // namespace ArcSHCLegacy

