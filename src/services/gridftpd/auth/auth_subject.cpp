#include <string>

#include "../misc/escaped.h"
#include "auth.h"

int AuthUser::match_subject(const char* line) {
  for(;;) {
    std::string s("");
    int n = gridftpd::input_escaped_string(line,s,' ','"');
    if(n == 0) break;
    line+=n;
    if(strcmp(subject.c_str(),s.c_str()) == 0) {
      return AAA_POSITIVE_MATCH;
    };
  };
  return AAA_NO_MATCH;
}
