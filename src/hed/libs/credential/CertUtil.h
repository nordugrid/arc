#ifndef __ARC_CERTUTIL_H__
#define __ARC_CERTUTIL_H__

#include <string>
#include <openssl/pem.h>
#include <openssl/x509.h>
#include <openssl/stack.h>

#include <arc/credential/Proxycertinfo.h>

namespace ArcCredential {
  
    /// Certificate Types
    /** \ingroup credential */
    typedef enum {
      /** A end entity certificate */
      CERT_TYPE_EEC,
      /** A CA certificate */
      CERT_TYPE_CA,
      /** A X.509 Proxy Certificate Profile (pre-RFC) compliant impersonation proxy - obsolete */
      CERT_TYPE_GSI_3_IMPERSONATION_PROXY,
      /** A X.509 Proxy Certificate Profile (pre-RFC) compliant independent proxy - obsolete */
      CERT_TYPE_GSI_3_INDEPENDENT_PROXY,
      /** A X.509 Proxy Certificate Profile (pre-RFC) compliant limited proxy - obsolete */
      CERT_TYPE_GSI_3_LIMITED_PROXY,
      /** A X.509 Proxy Certificate Profile (pre-RFC) compliant restricted proxy - obsolete */
      CERT_TYPE_GSI_3_RESTRICTED_PROXY,
      /** A legacy Globus impersonation proxy - obsolete */
      CERT_TYPE_GSI_2_PROXY,
      /** A legacy Globus limited impersonation proxy - obsolete */
      CERT_TYPE_GSI_2_LIMITED_PROXY,
      /** A X.509 Proxy Certificate Profile RFC compliant impersonation proxy; RFC inheritAll proxy */
      CERT_TYPE_RFC_IMPERSONATION_PROXY,
      /** A X.509 Proxy Certificate Profile RFC compliant independent proxy; RFC independent proxy */
      CERT_TYPE_RFC_INDEPENDENT_PROXY,
      /** A X.509 Proxy Certificate Profile RFC compliant limited proxy */
      CERT_TYPE_RFC_LIMITED_PROXY,
      /** A X.509 Proxy Certificate Profile RFC compliant restricted proxy */
      CERT_TYPE_RFC_RESTRICTED_PROXY,
      /** RFC anyLanguage proxy */
      CERT_TYPE_RFC_ANYLANGUAGE_PROXY
    } certType; 

    /** True if certificate type is one of proxy certificates */
    #define CERT_IS_PROXY(cert_type) \
        (cert_type == CERT_TYPE_RFC_IMPERSONATION_PROXY || \
         cert_type == CERT_TYPE_RFC_INDEPENDENT_PROXY || \
         cert_type == CERT_TYPE_RFC_LIMITED_PROXY || \
         cert_type == CERT_TYPE_RFC_RESTRICTED_PROXY || \
         cert_type == CERT_TYPE_RFC_ANYLANGUAGE_PROXY)

    /** True if certificate type is one of standard proxy certificates */
    #define CERT_IS_RFC_PROXY(cert_type) \
        (cert_type == CERT_TYPE_RFC_IMPERSONATION_PROXY || \
         cert_type == CERT_TYPE_RFC_INDEPENDENT_PROXY || \
         cert_type == CERT_TYPE_RFC_LIMITED_PROXY || \
         cert_type == CERT_TYPE_RFC_RESTRICTED_PROXY || \
         cert_type == CERT_TYPE_RFC_ANYLANGUAGE_PROXY)

    #define CERT_IS_INDEPENDENT_PROXY(cert_type) \
        (cert_type == CERT_TYPE_RFC_INDEPENDENT_PROXY)

    #define CERT_IS_RESTRICTED_PROXY(cert_type) \
        (cert_type == CERT_TYPE_RFC_RESTRICTED_PROXY)

    #define CERT_IS_LIMITED_PROXY(cert_type) \
        (cert_type == CERT_TYPE_RFC_LIMITED_PROXY)

    #define CERT_IS_IMPERSONATION_PROXY(cert_type) \
        (cert_type == CERT_TYPE_RFC_IMPERSONATION_PROXY || \
         cert_type == CERT_TYPE_RFC_LIMITED_PROXY)

    int verify_cert_chain(X509* cert, STACK_OF(X509)** certchain, std::string const& ca_file, std::string const& ca_dir, std::string& proxy_policy);
    int collect_cert_chain(X509* cert, STACK_OF(X509)** certchain, std::string& proxy_policy);
    bool check_cert_type(X509* cert, certType& type);
    const char* certTypeToString(certType type);

}

#endif // __ARC_CERTUTIL_H__

