#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

//@ #include "../std.h"
#include <string>
#include "canonical_dir.h"

int canonical_dir(std::string &name,bool leading_slash) {
  std::string::size_type i,ii,n;
  ii=0; i=0;
  for(;i<name.length();) {
    name[ii]=name[i];
    if(name[i] == '/') {
      n=i+1;
      if(n >= name.length()) {
        n=i; break;
      }
      else if(name[n] == '.') {
        n++;
        if(name[n] == '.') {
          n++;
          if((n >= name.length()) || (name[n] == '/')) {
            i=n;
            /* go 1 directory up */
            for(;;) {
              ii--;
              if(ii == std::string::npos) {
                 /* bad dir */
                return 1;
              };
              if(name[i] == '/') break;
            };
          };
        }
        else if((n >= name.length()) || (name[n] == '/')) {
          i=n;
        };  
      }
      else if(name[n] == '/') {
        i=n;
      };
    };
    n=i; i++; ii++;
  };
  if(leading_slash) {
    if((name[0] != '/') || (ii==0)) { name="/"+name.substr(0,ii); }
    else { name=name.substr(0,ii); };
  }
  else {
    if((name[0] != '/') || (ii==0)) { name=name.substr(0,ii); }
    else { name=name.substr(1,ii-1); };
  };
  return 0;
}

