#include <openssl/ssl.h>
#include <string>

bool LoadCertificateFile(const std::string& certfile, X509* &x509, STACK_OF(X509)* &certchain);
bool LoadKeyFile(const std::string& keyfile, EVP_PKEY* &pkey);

