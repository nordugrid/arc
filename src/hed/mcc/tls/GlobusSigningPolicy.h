#include <iostream>
#include <string>
#include <list>

#include <openssl/ssl.h>

namespace ArcMCCTLS {

class GlobusSigningPolicy {
  public:
    GlobusSigningPolicy(): stream_(NULL) { };
    ~GlobusSigningPolicy() { close(); };
    bool open(const X509_NAME* issuer_subject,const std::string& ca_path);
    void close() { delete stream_; stream_ = NULL; };
    bool match(const X509_NAME* issuer_subject,const X509_NAME* subject);
  private:
    GlobusSigningPolicy(GlobusSigningPolicy const &);
    GlobusSigningPolicy& operator=(GlobusSigningPolicy const &);
    std::istream* stream_;
};

} // namespace ArcMCCTLS

