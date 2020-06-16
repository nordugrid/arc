#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string>

#include <arc/ArcConfigIni.h>
#include "auth.h"

AuthResult AuthUser::match_subject(const char* line) {
  std::string s(line);
  if(strcmp(subject.c_str(),s.c_str()) == 0) {
    return AAA_POSITIVE_MATCH;
  };
  return AAA_NO_MATCH;
}
