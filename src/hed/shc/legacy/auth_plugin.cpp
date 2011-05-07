#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string>

#include <arc/StringConv.h>
#include <arc/Run.h>

#include "auth.h"

namespace ArcSHCLegacy {

static void AuthUserSubst(std::string& str,AuthUser& it) {
  int l = str.length();
  // Substitutions: %D, %P
  for(int i=0;i<l;i++) {
    if(str[i] == '%') {
      if(i<(l-1)) {
        switch(str[i+1]) {
          case 'D': {
            const char* s = it.DN();
            int s_l = strlen(s);
            str.replace(i,2,s);
            i+=(s_l-2-1);
          }; break;
          case 'P': {
            const char* s = it.proxy();
            int s_l = strlen(s);
            str.replace(i,2,s);
            i+=(s_l-2-1);
          }; break;
          default: {
            i++;
          }; break;
        };
      };
    };
  };
}

int AuthUser::match_plugin(const char* line) {
  // return AAA_NO_MATCH;
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
          arg != args.end();++arg) AuthUserSubst(*arg,*this);
  Arc::Run run(args);
  if(!run.Start()) return AAA_NO_MATCH;
  if(!run.Wait()) return AAA_NO_MATCH;
  if(run.Result() != 0) return AAA_NO_MATCH;
  return AAA_POSITIVE_MATCH;
}

} // namespace ArcSHCLegacy

