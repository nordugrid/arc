#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string>
#include <fstream>
#include <iostream>

#include <arc/StringConv.h>
#include <arc/Logger.h>

#include "auth.h"

namespace ArcSHCLegacy {

static Arc::Logger logger(Arc::Logger::getRootLogger(),"AuthUser");

AuthResult AuthUser::match_file(const char* line) {
  std::string token = Arc::trim(line);
  std::ifstream f(token.c_str());
  if(!f.is_open()) {
    logger.msg(Arc::ERROR, "Failed to read file %s", token);
    return AAA_FAILURE;
  };
  for(;f.good();) {
    std::string buf;
    getline(f,buf);
    buf = Arc::trim(buf);
    if(buf.empty()) continue;
    AuthResult res = match_subject(buf.c_str());
    if(res != AAA_NO_MATCH) {
      f.close();
      return res;
    };
  };
  f.close();
  return AAA_NO_MATCH;
}

} // namespace ArcSHCLegacy

