#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string>

#include <arc/StringConv.h>
#include <arc/Run.h>

#include "auth.h"

namespace ArcSHCLegacy {

int AuthUser::match_plugin(const char* line) {
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
  Arc::tokenize(line,args," ","\"","\"");
  if(args.size() <= 0) return AAA_NO_MATCH;
  for(std::list<std::string>::iterator arg = args.begin();
          arg != args.end();++arg) subst(*arg);
  Arc::Run run(args);
  if(!run.Start()) return AAA_NO_MATCH;
  if(!run.Wait(to)) return AAA_NO_MATCH;
  if(run.Result() != 0) return AAA_NO_MATCH;
  return AAA_POSITIVE_MATCH;
}

} // namespace ArcSHCLegacy

