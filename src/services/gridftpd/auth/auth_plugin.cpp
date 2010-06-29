#include <string>
#include "../run/run_plugin.h"
#include "auth.h"

void AuthUserSubst(std::string& str,AuthUser& it) {
  int l = str.length();
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

static void subst_arg(std::string& str,void* arg) {
  AuthUser* it = (AuthUser*)arg;
  if(!it) return;
  AuthUserSubst(str,*it);
}

void func(void) {
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

  std::string s = line;
  gridftpd::RunPlugin run(s);
  run.timeout(to);
  if(!run.run(subst_arg,this)) {
    return AAA_NO_MATCH; // It is safer to return no-match
  };
  if(run.result() != 0) return AAA_NO_MATCH;
  return AAA_POSITIVE_MATCH;
}

