/**Provide functions for maintaining information about proxy certificates*/

/**The code is derived from globus gsi, voms, and openssl-0.9.8e
 It seems the existing code about maintaining proxy certificate information in new openssl version is not enough for
 globus-compatible certificate, so here the globus code is introduced.
*/

#ifndef ARC_PROXYCERTINFO_H
#define ARC_PROXYCERTINFO_H

#include <openssl/x509.h>
#include <openssl/x509v3.h>
#include <string>

namespace ArcLib {
/* predefined policy language */
#define IMPERSONATION_PROXY_OID         "1.3.6.1.5.5.7.21.1"
#define IMPERSONATION_PROXY_SN          "IMPERSONATION_PROXY"
#define IMPERSONATION_PROXY_LN          "GSI impersonation proxy"

#define INDEPENDENT_PROXY_OID           "1.3.6.1.5.5.7.21.2"
#define INDEPENDENT_PROXY_SN            "INDEPENDENT_PROXY"
#define INDEPENDENT_PROXY_LN            "GSI independent proxy"

/* generic policy language */
#define GLOBUS_GSI_PROXY_GENERIC_POLICY_OID "1.3.6.1.4.1.3536.1.1.1.8"

#define LIMITED_PROXY_OID               "1.3.6.1.4.1.3536.1.1.1.9"
#define LIMITED_PROXY_SN                "LIMITED_PROXY"
#define LIMITED_PROXY_LN                "GSI limited proxy"

/* error handling */
#define ASN1_F_PROXYPOLICY_NEW          450
#define ASN1_F_D2I_PROXYPOLICY          451
#define ASN1_F_PROXYCERTINFO_NEW        430
#define ASN1_F_D2I_PROXYCERTINFO        431

/* Error codes for the X509V3 functions. */
/* Function codes. */
#define X509V3_F_PROCESS_PCI_VALUE			 150
#define X509V3_F_R2I_PCI				 155

/* Reason Code */
#define X509V3_R_INVALID_PROXY_POLICY_SETTING		 153
#define X509V3_R_NO_PROXY_CERT_POLICY_LANGUAGE_DEFINED	 154
#define X509V3_R_POLICY_WHEN_PROXY_LANGUAGE_REQUIRES_NO_POLICY 159

/* data structure */

typedef struct PROXYPOLICY_st {
    ASN1_OBJECT *                       policy_language;
    ASN1_OCTET_STRING *                 policy;
} PROXYPOLICY;

typedef struct PROXYCERTINFO_st {
  ASN1_INTEGER * path_length;
  PROXYPOLICY * proxypolicy;
  int version;
} PROXYCERTINFO;


/* PROXYPOLICY function */

/* allocating and free memory */
PROXYPOLICY * PROXYPOLICY_new();
void PROXYPOLICY_free(PROXYPOLICY * proxypolicy);

/* duplicate */
PROXYPOLICY * PROXYPOLICY_dup(PROXYPOLICY * policy);

/* set policy language */
int PROXYPOLICY_set_policy_language(PROXYPOLICY * policy, ASN1_OBJECT * policy_language);

/* get policy language */
ASN1_OBJECT * PROXYPOLICY_get_policy_language(PROXYPOLICY * policy);

/* set policy contents */
int PROXYPOLICY_set_policy(PROXYPOLICY * proxypolicy, unsigned char * policy, int length);

/* get policy contents */
unsigned char * PROXYPOLICY_get_policy(PROXYPOLICY * policy, int * length);

/* internal to der conversion */
int i2d_PROXYPOLICY(PROXYPOLICY * policy, unsigned char ** pp);

/* der to internal conversion */
PROXYPOLICY * d2i_PROXYPOLICY(PROXYPOLICY ** policy, unsigned char ** pp, long length);

X509V3_EXT_METHOD * PROXYPOLICY_x509v3_ext_meth();

STACK_OF(CONF_VALUE) * i2v_PROXYPOLICY(struct v3_ext_method * method, PROXYPOLICY * ext, STACK_OF(CONF_VALUE) * extlist);

/*PROXYCERTINFO function */

/* allocating and free memory */
PROXYCERTINFO * PROXYCERTINFO_new();
void PROXYCERTINFO_free(PROXYCERTINFO * proxycertinfo);

int PROXYCERTINFO_print_fp(FILE* fp, PROXYCERTINFO* cert_info);

/* set path_length */
int PROXYCERTINFO_set_path_length(PROXYCERTINFO * proxycertinfo, long path_length);

/* get ptah length */
long PROXYCERTINFO_get_path_length(PROXYCERTINFO * proxycertinfo);

/* set proxypolicy */
int PROXYCERTINFO_set_proxypolicy(PROXYCERTINFO * proxycertinfo, PROXYPOLICY * proxypolicy);

/* get proxypolicy */
PROXYPOLICY * PROXYCERTINFO_get_proxypolicy(PROXYCERTINFO * proxycertinfo);

/* internal to der conversion */
int i2d_PROXYCERTINFO(PROXYCERTINFO * proxycertinfo, unsigned char ** pp);

/* der to internal conversion */
PROXYCERTINFO * d2i_PROXYCERTINFO(PROXYCERTINFO ** cert_info, unsigned char ** a, long length);

int PROXYCERTINFO_set_version(PROXYCERTINFO *cert_info, int version);

STACK_OF(CONF_VALUE) * i2v_PROXYCERTINFO(
    struct v3_ext_method *              method,
    PROXYCERTINFO *                     ext,
    STACK_OF(CONF_VALUE) *              extlist);

int i2r_PROXYCERTINFO(X509V3_EXT_METHOD *method, PROXYCERTINFO *ext, BIO *out, int indent);

PROXYCERTINFO *r2i_PROXYCERTINFO(X509V3_EXT_METHOD *method, X509V3_CTX *ctx, char *value);

X509V3_EXT_METHOD * PROXYCERTINFO_v3_x509v3_ext_meth();

X509V3_EXT_METHOD * PROXYCERTINFO_v4_x509v3_ext_meth();

} //namespace ArcLib

#endif
