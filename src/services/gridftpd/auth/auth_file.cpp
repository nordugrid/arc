#include <string>
#include <fstream>
#include <iostream>

#include "../misc/escaped.h"
#include "auth.h"

#define olog std::cerr

int AuthUser::match_file(const char* line) {
  for(;;) {
    std::string s("");
    int n = gridftpd::input_escaped_string(line,s,' ','"');
    if(n == 0) break;
    line+=n;
    std::ifstream f(s.c_str());
    if(!f.is_open()) {
      olog<<"Failed to read file "<<s<<std::endl;
      return AAA_FAILURE;
    };
    for(;!f.eof();) {
      std::string buf;
      getline(f,buf);
      int res = evaluate(buf.c_str());
      if(res != AAA_NO_MATCH) {
        f.close();
        return res;
      };
    };
    f.close();
  };
  return AAA_NO_MATCH;
}
