#include <iostream>
#include <string>
#include <list>

#include <openssl/ssl.h>

namespace Arc {

std::istream* open_globus_policy(const X509_NAME* issuer_subject,const std::string& ca_path);
bool match_globus_policy(std::istream& in,const X509_NAME* issuer_subject,const X509_NAME* subject);

}

