// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "ArcRegex.h"
#include <iostream>

namespace Arc {

  RegularExpression::RegularExpression()
    : pattern(""), status(-1) {
    regcomp(&preg, pattern.c_str(), REG_EXTENDED);
  }

  RegularExpression::RegularExpression(std::string pattern, bool ignoreCase)
    : pattern(pattern) {
    status = regcomp(&preg, pattern.c_str(), REG_EXTENDED | (REG_ICASE * ignoreCase) );
  }

  RegularExpression::RegularExpression(const RegularExpression& regex)
    : pattern(regex.pattern) {
    status = regcomp(&preg, pattern.c_str(), 0);
  }

  RegularExpression::~RegularExpression() {
    regfree(&preg);
  }

  RegularExpression& RegularExpression::operator=(const RegularExpression& regex) {
    regfree(&preg);
    pattern = regex.pattern;
    status = regcomp(&preg, pattern.c_str(), 0);
    return *this;
  }

  bool RegularExpression::isOk() {
    return status == 0;
  }

  bool RegularExpression::hasPattern(std::string str) {
    return pattern == str;
  }

  bool RegularExpression::match(const std::string& str) const {
    std::list<std::string> unmatched;
    std::list<std::string> matched;
    return match(str, unmatched, matched) && (unmatched.empty());
  }

  bool RegularExpression::match(const std::string& str, std::vector<std::string>& matched) const {
    if (status != 0) return false;
    
    regmatch_t rm[preg.re_nsub+1];
    if (regexec(&preg, str.c_str(), preg.re_nsub+1, rm, 0) != 0) return false;
    for (int n = 1; n <= preg.re_nsub; ++n) {
      if (rm[n].rm_so == -1) {
        matched.push_back("");
      }
      else {
        matched.push_back(str.substr(rm[n].rm_so, rm[n].rm_eo - rm[n].rm_so));
      }
    }
    return true;
  }

  bool RegularExpression::match(const std::string& str, std::list<std::string>& unmatched, std::list<std::string>& matched) const {
    if (status == 0) {
      int st;
      regmatch_t rm[256];
      unmatched.clear();
      matched.clear();
      st = regexec(&preg, str.c_str(), 256, rm, 0);
      if (st != 0)
        return false;
      regoff_t p = 0;
      for (int n = 0; n < 256; ++n) {
        if (rm[n].rm_so == -1)
          break;
        matched.push_back(str.substr(rm[n].rm_so, rm[n].rm_eo - rm[n].rm_so));
        if (rm[n].rm_so > p)
          unmatched.push_back(str.substr(p, rm[n].rm_so - p));
        p = rm[n].rm_eo;
      }
      if (p < str.length())
        unmatched.push_back(str.substr(p));
      return true;
    }
    else
      return false;
  }

  std::string RegularExpression::getPattern() const {
    return pattern;
  }

}
