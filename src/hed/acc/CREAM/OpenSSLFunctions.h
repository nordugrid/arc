#include <iostream>
#include <fstream>
#include <sstream>
#include <ctime>

#include <unistd.h>
#include <sys/types.h>

#ifndef HAVE_TIMEGM
time_t timegm (struct tm *tm);
#endif

#include <openssl/pem.h>
#include <openssl/asn1.h>
#include <openssl/x509.h>
#include <openssl/evp.h>
#include <openssl/bio.h>

time_t ASN1_UTCTIME_get(const ASN1_UTCTIME *s);
const long getCertTimeLeft( std::string pxfile );
time_t GRSTasn1TimeToTimeT(char *asn1time, size_t len);
int makeProxyCert(char **proxychain, char *reqtxt, char *cert, char *key, int minutes);
std::string checkPath(std::string p);
std::string getProxy();
std::string getTrustedCerts();
