#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef HAVE_GLIBMM_GETENV
#include <glibmm/miscutils.h>
#else
#include <stdlib.h>
#endif

#ifdef HAVE_GLIBMM_SETENV
#include <glibmm/miscutils.h>
#else
#include <stdlib.h>
#endif

#ifdef HAVE_GLIBMM_UNSETENV
#include <glibmm/miscutils.h>
#else
#include <stdlib.h>
#endif

#include <string.h>

#include <arc/StringConv.h>

#ifndef BUFLEN
#define BUFLEN 1024
#endif

namespace Arc {

  std::string GetEnv(const std::string& var) {
#ifdef HAVE_GLIBMM_GETENV
    return Glib::getenv(var);
#else
    char val* = getenv(var.c_str());
    return val ? val : "";
#endif
  }

  bool SetEnv(const std::string& var, const std::string& value) {
#ifdef HAVE_GLIBMM_SETENV
    return Glib::setenv(var, value);
#else
#ifdef HAVE_SETENV
    return (setenv(var.c_str(), value.c_str(), 1) == 0);
#else
    return (putenv(strdup((var + "=" + value).c_str())) == 0);
#endif
#endif
  }

  void UnsetEnv(const std::string& var) {
#ifdef HAVE_GLIBMM_UNSETENV
    Glib::unsetenv(var);
#else
#ifdef HAVE_UNSETENV
    unsetenv(var.c_str());
#else
    putenv(strdup(var.c_str()));
#endif
#endif
  }

  std::string StrError(int errnum) {
#ifdef HAVE_STRERROR_R
    char errbuf[BUFLEN];
#ifdef STRERROR_R_CHAR_P
    return strerror_r(errnum, errbuf, sizeof(errbuf));
#else
    if (strerror_r(errnum, errbuf, sizeof(errbuf)) == 0)
      return errbuf;
    else
      return "Unknown error " + tostring(errnum);
#endif
#else
    return strerror(errnum);
#endif
  }

} // namespace Arc
