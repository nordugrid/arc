// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "win32.h"

std::string GetOsErrorMessage(void) {
  std::string rv;
  LPVOID lpMsgBuf;
  if (FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        GetLastError(),
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
        (LPTSTR)&lpMsgBuf,
        0,
        NULL))
    rv.assign(reinterpret_cast<const char*>(lpMsgBuf));
  else
    rv.assign("FormatMessage API failed");
  LocalFree(lpMsgBuf);
  return rv;
}
