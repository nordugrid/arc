#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <ctype.h>
#include <algorithm>
#include "Logger.h"

namespace Arc {

  Logger stringLogger(Logger::getRootLogger(), "StringConv");

  std::string upper(const std::string &s) {
    std::string ret = s;
    std::transform(ret.begin(), ret.end(), ret.begin(), (int(*)(int)) std::toupper);
    return ret;
  }
} // namespace Arc
