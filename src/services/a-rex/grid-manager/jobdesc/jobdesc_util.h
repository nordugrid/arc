#ifndef __ARC_JOBDESC_UTIL_H__
#define __ARC_JOBDESC_UTIL_H__
#include <string>
#include <iostream>

class value_for_shell {
 friend std::ostream& operator<<(std::ostream&,const value_for_shell&);
 private:
  const char* str;
  bool quote;
 public:
  value_for_shell(const char *str_,bool quote_):str(str_),quote(quote_) { };
  value_for_shell(const std::string &str_,bool quote_):str(str_.c_str()),quote(quote_) { };
};
std::ostream& operator<<(std::ostream &o,const value_for_shell &s);

class numvalue_for_shell {
 friend std::ostream& operator<<(std::ostream&,const numvalue_for_shell&);
 private:
  long int n;
 public:
  numvalue_for_shell(const char *str) { n=0; sscanf(str,"%li",&n); };
  numvalue_for_shell(int n_) { n=n_; };
  numvalue_for_shell operator/(int d) { return numvalue_for_shell(n/d); };
  numvalue_for_shell operator*(int d) { return numvalue_for_shell(n*d); };
};
std::ostream& operator<<(std::ostream &o,const numvalue_for_shell &s);

#define NG_RSL_DEFAULT_STDIN      const_cast<char*>("/dev/null")
#define NG_RSL_DEFAULT_STDOUT     const_cast<char*>("/dev/null")
#define NG_RSL_DEFAULT_STDERR     const_cast<char*>("/dev/null")

#endif
