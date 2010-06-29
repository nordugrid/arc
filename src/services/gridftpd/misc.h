#ifdef __USE_POSIX
#include <time.h>
#else
#define __USE_POSIX
#include <time.h>
#undef __USE_POSIX
#endif

#include <string>

std::string timetostring(time_t t);
std::string dirstring(bool dir,long long unsigned int s,time_t t,const char *name);
