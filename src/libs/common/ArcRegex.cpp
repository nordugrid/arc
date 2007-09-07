#include "ArcRegex.h"

namespace Arc {
  
  RegularExpression::RegularExpression(std::string pattern) :
    pattern(pattern)
  {
    status=regcomp(&preg, pattern.c_str(), REG_EXTENDED);
  }
  
  RegularExpression::RegularExpression(const RegularExpression& regex) :
    pattern(regex.pattern)
  {
    status=regcomp(&preg, pattern.c_str(), 0);
  }
  
  RegularExpression::~RegularExpression(){
    regfree(&preg);
  }
  
  const RegularExpression&
  RegularExpression::operator=(const RegularExpression& regex){
    regfree(&preg);
    pattern=regex.pattern;
    status=regcomp(&preg, pattern.c_str(), 0);
    return *this;
  }
  
  bool RegularExpression::isOk(){
    return status==0;
  }
  
  bool RegularExpression::hasPattern(std::string str){
    return pattern==str;
  }
  
  bool RegularExpression::match(const std::string& str) const {
    std::list<std::string> unmatched;
    return match(str,unmatched) && (unmatched.size() == 0);
  }

  bool RegularExpression::match(const std::string& str,std::list<std::string>& unmatched) const {
    if (status==0){
      int st;
      regmatch_t rm[256];
      unmatched.clear();
      st = regexec(&preg, str.c_str(), 256, rm, 0);
      if(st != 0) return false;
      regoff_t p = 0;
      for(int n = 0; n<256; ++n) {
        if(rm[n].rm_so == -1) break;
        if(rm[n].rm_so > p) {
          unmatched.push_back(str.substr(p,rm[n].rm_so-p));
        };
        p=rm[n].rm_eo;
      };
      if(p < str.length()) unmatched.push_back(str.substr(p));
      return true;
    }
    else {
      return false;
    }
  }

  std::string RegularExpression::getPattern() {
    return pattern;
  }

}
