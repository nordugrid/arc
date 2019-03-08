#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string>
#include <fstream>
#include <iostream>

#include <arc/Logger.h>
#include <arc/ArcConfigIni.h>
#include <arc/StringConv.h>

#include "auth.h"

static Arc::Logger logger(Arc::Logger::getRootLogger(),"AuthUserFile");

AuthResult AuthUser::match_file(const char* line) {
  std::string s = Arc::trim(line);
  if(!s.empty()) {
    std::ifstream f(s.c_str());
    if(!f.is_open()) {
      logger.msg(Arc::ERROR, "Failed to read file %s", s);
      return AAA_FAILURE;
    };
    for(;f.good();) {
      std::string buf;
      getline(f,buf);
      //buf = Arc::trim(buf);
      std::string::size_type p = 0;
      for(;p<buf.length();++p) if(!isspace(buf[p])) break;
      // Extract subject from the line.
      // It is either till white space or quoted string.
      // There are also comment lines starting from # and empty lines.
      if(p>=buf.length()) continue;
      if(buf[p] == '#') continue;
      std::string subj;
      p = Arc::get_token(subj,buf,p," ","\"","\"");
      if(subj.empty()) continue; // can't match empty subject - it is dangerous
      if(subject != subj) continue;
      f.close();
      return AAA_POSITIVE_MATCH;
    };
    f.close();
  };
  return AAA_NO_MATCH;
}
