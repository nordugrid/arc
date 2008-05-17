#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <cstring>

#include "jobdesc_util.h"

std::ostream& operator<<(std::ostream &o,const value_for_shell &s) {
  if(s.str == NULL) return o;
  if(s.quote) o<<"'";
  const char* p = s.str;
  for(;;) {
    char* pp = strchr(p,'\'');
    if(pp == NULL) { o<<p; if(s.quote) o<<"'"; break; };
    o.write(p,pp-p); o<<"'\\''"; p=pp+1;
  };
  return o;
}

std::ostream& operator<<(std::ostream &o,const numvalue_for_shell &s) {
  o<<s.n;
  return o;
}
  
