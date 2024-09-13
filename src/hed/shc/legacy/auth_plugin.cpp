#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string>

#include <arc/StringConv.h>
#include <arc/Run.h>

#include "auth.h"

namespace ArcSHCLegacy {

static Arc::Logger logger(Arc::Logger::getRootLogger(),"AuthUser");

void AuthUser::add_auth_environment(Arc::Run& run) const {
  if(!subject_.empty()) run.AddEnvironment("X509_SUBJECT_NAME",subject_);
  for(int tokenIdx = 0; tokenIdx < otokens_data_.size(); ++tokenIdx) {
    otokens_t const & token = otokens_data_[tokenIdx];
    if(!token.subject.empty()) run.AddEnvironment("BEARER_TOKEN_"+Arc::tostring(tokenIdx)+"_SUBJECT",token.subject);
    if(!token.issuer.empty()) run.AddEnvironment("BEARER_TOKEN_"+Arc::tostring(tokenIdx)+"_ISSUER",token.issuer);
    int idx = 0;
    for(std::list<std::string>::const_iterator it = token.audiences.begin(); it != token.audiences.end(); ++it) {
      run.AddEnvironment("BEARER_TOKEN_"+Arc::tostring(tokenIdx)+"_AUDIENCE_"+Arc::tostring(idx),*it);
      ++idx;
    }
    idx = 0;
    for(std::list<std::string>::const_iterator it = token.scopes.begin(); it != token.scopes.end(); ++it) {
      run.AddEnvironment("BEARER_TOKEN_"+Arc::tostring(tokenIdx)+"_SCOPE_"+Arc::tostring(idx),*it);
      ++idx;
    }
    idx = 0;
    for(std::list<std::string>::const_iterator it = token.groups.begin(); it != token.groups.end(); ++it) {
      run.AddEnvironment("BEARER_TOKEN_"+Arc::tostring(tokenIdx)+"_GROUP_"+Arc::tostring(idx),*it);
      ++idx;
    }
    for(std::map< std::string,std::list<std::string> >::const_iterator claim = token.claims.begin(); claim != token.claims.end(); ++claim) {
      idx = 0;
      for(std::list<std::string>::const_iterator it = claim->second.begin(); it != claim->second.end(); ++it) {
        run.AddEnvironment("BEARER_TOKEN_"+Arc::tostring(tokenIdx)+"_CLAIM_"+claim->first+"_"+Arc::tostring(idx),*it);
        ++idx;
      }
    }
  }
  // TODO std::vector<struct voms_t> voms_data_; // VOMS information extracted from message
}

AuthResult AuthUser::match_plugin(const char* line) {
  // plugin = timeout path [argument ...] - Run external executable or
  if(!line) return AAA_NO_MATCH;
  for(;*line;line++) if(!isspace(*line)) break;
  if(*line == 0) return AAA_NO_MATCH;
  char* p;
  long int to = strtol(line,&p,0);
  if(p == line) return AAA_NO_MATCH;
  if(to < 0) return AAA_NO_MATCH;
  line=p;
  for(;*line;line++) if(!isspace(*line)) break;
  if(*line == 0) return AAA_NO_MATCH;
  std::list<std::string> args;
  Arc::tokenize(line,args," ");
  if(args.size() <= 0) return AAA_NO_MATCH;
  for(std::list<std::string>::iterator arg = args.begin();
          arg != args.end();++arg) {
    subst(*arg);
  };

  std::string stdout_str;
  std::string stderr_str;
  Arc::Run run(args);
  add_auth_environment(run);
  run.AssignStdout(stdout_str);
  run.AssignStderr(stderr_str);
  if(run.Start()) {
    if(run.Wait(to)) {
      if(run.Result() == 0) {
        return AAA_POSITIVE_MATCH;
      } else {
        logger.msg(Arc::ERROR,"Plugin %s returned: %u",args.front(),run.Result());
      };
    } else {
      run.Kill(1);
      logger.msg(Arc::ERROR,"Plugin %s timeout after %u seconds",args.front(),to);
    };
  } else {
    logger.msg(Arc::ERROR,"Plugin %s failed to start",args.front());
  };
  if(!stdout_str.empty()) logger.msg(Arc::INFO,"Plugin %s printed: %s",args.front(),stdout_str);
  if(!stderr_str.empty()) logger.msg(Arc::ERROR,"Plugin %s error: %s",args.front(),stderr_str);
  return AAA_NO_MATCH; // ??
}

} // namespace ArcSHCLegacy

