#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string>

#include <arc/StringConv.h>
#include <arc/Run.h>

#include "auth.h"

namespace ArcSHCLegacy {

static Arc::Logger logger(Arc::Logger::getRootLogger(),"AuthUser");

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

