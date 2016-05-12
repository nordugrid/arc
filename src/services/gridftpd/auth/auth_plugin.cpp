#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string>

#include <arc/Logger.h>

#include "../run/run_plugin.h"
#include "auth.h"

static Arc::Logger logger(Arc::Logger::getRootLogger(),"AuthUserPlugin");

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

AuthResult AuthUser::match_plugin(const char* line) {
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
  if(run.run(subst_arg,this)) {
    if(run.result() == 0) {
      return AAA_POSITIVE_MATCH;
    } else {
      logger.msg(Arc::ERROR,"Plugin %s returned: %u",run.cmd(),(unsigned int)run.result());
    };
  } else {
    logger.msg(Arc::ERROR,"Plugin %s failed to run",run.cmd());
  };
  logger.msg(Arc::INFO,"Plugin %s printed: %u",run.cmd(),run.stdout_channel());
  logger.msg(Arc::ERROR,"Plugin %s error: %u",run.cmd(),run.stderr_channel());
  return AAA_NO_MATCH; // It is safer to return no-match (??)
}

