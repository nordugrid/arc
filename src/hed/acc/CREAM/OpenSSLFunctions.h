#ifndef __ARC_OPENSSLFUNCTIONS_H__
#define __ARC_OPENSSLFUNCTIONS_H__
#endif

namespace Arc {
    long getCertTimeLeft(const std::string& pxfile);
    int makeProxyCert(char **proxychain, char *reqtxt,
		      char *cert, char *key, int minutes);
}
