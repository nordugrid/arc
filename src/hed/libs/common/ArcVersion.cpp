// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <iostream>
#include <cstring>
#include <cstdlib>

#include "ArcVersion.h"

namespace Arc {

  static unsigned int extract_subversion(const char* ver,unsigned int pos) {
    const char* p = ver;
    if(!p) return 0;
    for(;pos;--pos) {
      p = strchr(p,'.');
      if(!p) return 0;
      ++p;
    }
    return (unsigned int)strtoul(p,NULL,10);
  }

  ArcVersion::ArcVersion(const char* ver):
                            Major(extract_subversion(ver,0)),
                            Minor(extract_subversion(ver,1)),
                            Patch(extract_subversion(ver,2)) {
  }

  const ArcVersion Version(PACKAGE_VERSION);

} // namespace Arc
