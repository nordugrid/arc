#ifndef __ARC_CERTUTIL_H__
#define __ARC_CERTUTIL_H__

#include <string>
#include <openssl/pem.h>
#include <openssl/x509.h>
#include <openssl/stack.h>

#include "Proxycertinfo.h"

namespace ArcLib {
   
    /* Certificate Types */
    typedef enum {
      /** A end entity certificate */
      CERT_TYPE_EEC,
      /** A CA certificate */
      CERT_TYPE_CA,
      /** A X.509 Proxy Certificate Profile (pre-RFC) compliant impersonation proxy */
      CERT_TYPE_GSI_3_IMPERSONATION_PROXY,
      /** A X.509 Proxy Certificate Profile (pre-RFC) compliant independent proxy */
      CERT_TYPE_GSI_3_INDEPENDENT_PROXY,
      /** A X.509 Proxy Certificate Profile (pre-RFC) compliant limited proxy */
      CERT_TYPE_GSI_3_LIMITED_PROXY,
      /** A X.509 Proxy Certificate Profile (pre-RFC) compliant restricted proxy */
      CERT_TYPE_GSI_3_RESTRICTED_PROXY,
      /** A legacy Globus impersonation proxy */
      CERT_TYPE_GSI_2_PROXY,
      /** A legacy Globus limited impersonation proxy */
      CERT_TYPE_GSI_2_LIMITED_PROXY,
      /** A X.509 Proxy Certificate Profile RFC compliant impersonation proxy */
      CERT_TYPE_RFC_IMPERSONATION_PROXY,
      /** A X.509 Proxy Certificate Profile RFC compliant independent proxy */
      CERT_TYPE_RFC_INDEPENDENT_PROXY,
      /** A X.509 Proxy Certificate Profile RFC compliant limited proxy */
      CERT_TYPE_RFC_LIMITED_PROXY,
      /** A X.509 Proxy Certificate Profile RFC compliant restricted proxy */
      CERT_TYPE_RFC_RESTRICTED_PROXY,
    } certType; 

    #define CERT_IS_PROXY(cert_type) \
        (cert_type == CERT_TYPE_GSI_3_IMPERSONATION_PROXY || \
         cert_type == CERT_TYPE_GSI_3_INDEPENDENT_PROXY || \
         cert_type == CERT_TYPE_GSI_3_LIMITED_PROXY || \
         cert_type == CERT_TYPE_GSI_3_RESTRICTED_PROXY || \
         cert_type == CERT_TYPE_RFC_IMPERSONATION_PROXY || \
         cert_type == CERT_TYPE_RFC_INDEPENDENT_PROXY || \
         cert_type == CERT_TYPE_RFC_LIMITED_PROXY || \
         cert_type == CERT_TYPE_RFC_RESTRICTED_PROXY || \
         cert_type == CERT_TYPE_GSI_2_PROXY || \
         cert_type == CERT_TYPE_GSI_2_LIMITED_PROXY)

    #define CERT_IS_RFC_PROXY(cert_type) \
        (cert_type == CERT_TYPE_RFC_IMPERSONATION_PROXY || \
         cert_type == CERT_TYPE_RFC_INDEPENDENT_PROXY || \
         cert_type == CERT_TYPE_RFC_LIMITED_PROXY || \
         cert_type == CERT_TYPE_RFC_RESTRICTED_PROXY)

    #define CERT_IS_GSI_3_PROXY(cert_type) \
        (cert_type == CERT_TYPE_GSI_3_IMPERSONATION_PROXY || \
         cert_type == CERT_TYPE_GSI_3_INDEPENDENT_PROXY || \
         cert_type == CERT_TYPE_GSI_3_LIMITED_PROXY || \
         cert_type == CERT_TYPE_GSI_3_RESTRICTED_PROXY)

    #define CERT_IS_GSI_2_PROXY(cert_type) \
        (cert_type == CERT_TYPE_GSI_2_PROXY || \
         cert_type == CERT_TYPE_GSI_2_LIMITED_PROXY)

    #define CERT_IS_INDEPENDENT_PROXY(cert_type) \
        (cert_type == CERT_TYPE_RFC_INDEPENDENT_PROXY || \
         cert_type == CERT_TYPE_GSI_3_INDEPENDENT_PROXY)

    #define CERT_IS_RESTRICTED_PROXY(cert_type) \
        (cert_type == CERT_TYPE_RFC_RESTRICTED_PROXY || \
         cert_type == CERT_TYPE_GSI_3_RESTRICTED_PROXY)

    #define CERT_IS_LIMITED_PROXY(cert_type) \
        (cert_type == CERT_TYPE_RFC_LIMITED_PROXY || \
         cert_type == CERT_TYPE_GSI_3_LIMITED_PROXY || \
         cert_type == CERT_TYPE_GSI_2_LIMITED_PROXY)

    #define CERT_IS_IMPERSONATION_PROXY(cert_type) \
        (cert_type == CERT_TYPE_RFC_IMPERSONATION_PROXY || \
         cert_type == CERT_TYPE_RFC_LIMITED_PROXY || \
         cert_type == CERT_TYPE_GSI_3_IMPERSONATION_PROXY || \
         cert_type == CERT_TYPE_GSI_3_LIMITED_PROXY || \
         cert_type == CERT_TYPE_GSI_2_PROXY || \
         cert_type == CERT_TYPE_GSI_2_LIMITED_PROXY)

    #define VERIFY_CTX_STORE_EX_DATA_IDX  1

    typedef struct {
      X509_STORE_CTX *                    cert_store;
      int                                 cert_depth;
      int                                 proxy_depth;
      int                                 max_proxy_depth;
      int                                 limited_proxy;
      certType                            cert_type;
      STACK_OF(X509) *                    cert_chain; /*  X509 */
      std::string                         cert_dir;
    } cert_verify_context;

    int verify_chain(X509* cert, STACK_OF(X509)* certchain, cert_verify_context* vctx);
    int verify_callback(int ok, X509_STORE_CTX* store_ctx);
    bool check_proxy_type(X509* cert, certType& type);
    int check_issued(X509_STORE_CTX* ctx, X509* x, X509* issuer);

}

#endif
