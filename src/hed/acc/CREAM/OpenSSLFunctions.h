#ifndef __ARC_OPENSSLFUNCTIONS_H__
#define __ARC_OPENSSLFUNCTIONS_H__
#endif

namespace Arc {
  namespace Cream {
    long getCertTimeLeft(const std::string& pxfile);
    int makeProxyCert(char **proxychain, char *reqtxt,
		      char *cert, char *key, int minutes);
  }
}
