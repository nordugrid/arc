#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdlib.h>
#include <sys/time.h>

#ifdef HAVE_STDINT_H
#include <stdint.h>
#endif

#include <arc/StringConv.h>

#include "uid.h"

namespace ARex {

std::string rand_uid64(void) {
  static unsigned int cnt;
  struct timeval t;
  gettimeofday(&t,NULL);
  uint64_t id =
      (((uint64_t)((cnt++) & 0xffff))   << 48) |
      (((uint64_t)(t.tv_sec & 0xffff))  << 32) |
      (((uint64_t)(t.tv_usec & 0xffff)) << 16) |
      (((uint64_t)(rand() & 0xffff))    << 0);
  return Arc::inttostr(id,16,16);
}  

} // namespace ARex

