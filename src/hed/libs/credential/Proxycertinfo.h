#ifndef ARC_PROXY_CERT_INFO_EXTENSION_H
#define ARC_PROXY_CERT_INFO_EXTENSION_H

//#include <openssl/asn1t.h>
//#include <openssl/x509.h>
#include <openssl/x509v3.h>
//#include <string>

/// Internal code for low-level credential handling.
namespace ArcCredential {
/**
  Functions and constants for maintaining proxy certificates
*/

/**
 The code is derived from globus gsi, voms, and openssl-0.9.8e.
 The existing code for maintaining proxy certificates in OpenSSL only
 covers standard proxies and does not cover old Globus proxies, so 
 here the Globus code is introduced.
*/

/* predefined policy language */
#define ANYLANGUAGE_PROXY_OBJ         OBJ_id_ppl_anyLanguage
#define ANYLANGUAGE_PROXY_NID         NID_id_ppl_anyLanguage
#define ANYLANGUAGE_PROXY_SN          SN_id_ppl_anyLanguage
#define ANYLANGUAGE_PROXY_LN          LN_id_ppl_anyLanguage

#define IMPERSONATION_PROXY_OBJ       OBJ_id_ppl_inheritAll 
#define IMPERSONATION_PROXY_NID       NID_id_ppl_inheritAll
#define IMPERSONATION_PROXY_SN        SN_id_ppl_inheritAll
#define IMPERSONATION_PROXY_LN        LN_id_ppl_inheritAll

#define INDEPENDENT_PROXY_OBJ         OBJ_Independent
#define INDEPENDENT_PROXY_NID         NID_Independent
#define INDEPENDENT_PROXY_SN          SN_Independent
#define INDEPENDENT_PROXY_LN          LN_Independent

/* generic policy language */
/*
#define GLOBUS_GSI_PROXY_GENERIC_POLICY_OID "1.3.6.1.4.1.3536.1.1.1.8"

#define LIMITED_PROXY_OID               "1.3.6.1.4.1.3536.1.1.1.9"
#define LIMITED_PROXY_SN                "LIMITED_PROXY"
#define LIMITED_PROXY_LN                "GSI limited proxy"
*/

/* error handling */
/*
#define ASN1_F_PROXYPOLICY_NEW          450
#define ASN1_F_D2I_PROXYPOLICY          451
#define ASN1_F_PROXY_CERT_INFO_EXTENSION_NEW        430
#define ASN1_F_D2I_PROXY_CERT_INFO_EXTENSION        431
*/

/* Error codes for the X509V3 functions. */
/* Function codes. */
/*
#define X509V3_F_PROCESS_PCI_VALUE			 150
#define X509V3_F_R2I_PCI				 155
*/

/* Reason Code */
/*
#define X509V3_R_INVALID_PROXY_POLICY_SETTING		 153
#define X509V3_R_NO_PROXY_CERT_POLICY_LANGUAGE_DEFINED	 154
#define X509V3_R_POLICY_WHEN_PROXY_LANGUAGE_REQUIRES_NO_POLICY 159
*/

/* data structure */
/*
typedef struct PROXYPOLICY_st {
    ASN1_INTEGER* dummy;
    ASN1_INTEGER* dummy2;
    ASN1_OBJECT *                       policy_language;
    ASN1_OCTET_STRING *                 policy;
} PROXYPOLICY;


ASN1_SEQUENCE(PROXYPOLICY) = {
  ASN1_EXP_OPT(PROXYPOLICY, dummy, ASN1_INTEGER, 1),
  ASN1_OPT(PROXYPOLICY, dummy2, ASN1_INTEGER),
  ASN1_EXP_OPT(PROXYPOLICY, policy_language, ASN1_OBJECT, 1),
  ASN1_EXP_OPT(PROXYPOLICY, policy, ASN1_OCTET_STRING, 1)
} ASN1_SEQUENCE_END(PROXYPOLICY)

DECLARE_ASN1_FUNCTIONS(PROXYPOLICY)

IMPLEMENT_ASN1_FUNCTIONS(PROXYPOLICY)
*/

/*
typedef struct PROXY_CERT_INFO_EXTENSION_st {
  ASN1_INTEGER * path_length;
  PROXYPOLICY * proxypolicy;
  int version;
} PROXY_CERT_INFO_EXTENSION;
/// \endcond
*/

/* PROXYPOLICY function */

/* allocating and free memory */
//PROXYPOLICY * PROXYPOLICY_new();
//void PROXYPOLICY_free(PROXYPOLICY * proxypolicy);

/* duplicate */
//PROXYPOLICY * PROXYPOLICY_dup(PROXYPOLICY * policy);

/* set policy language */
int PROXY_POLICY_set_policy_language(PROXY_POLICY * policy, ASN1_OBJECT * policy_language);

/* Returns policy language object from policy */
ASN1_OBJECT * PROXY_POLICY_get_policy_language(PROXY_POLICY * policy);

/* set policy contents */
int PROXY_POLICY_set_policy(PROXY_POLICY * proxypolicy, unsigned char * policy, int length);

/* get policy contents */
unsigned char * PROXY_POLICY_get_policy(PROXY_POLICY * policy, int * length);

/* internal to der conversion */
//int i2d_PROXYPOLICY(PROXYPOLICY * policy, unsigned char ** pp);

/* der to internal conversion */
//PROXYPOLICY * d2i_PROXYPOLICY(PROXYPOLICY ** policy, unsigned char ** pp, long length);

//X509V3_EXT_METHOD * PROXYPOLICY_x509v3_ext_meth();

//STACK_OF(CONF_VALUE) * i2v_PROXYPOLICY(struct v3_ext_method * method, PROXYPOLICY * ext, STACK_OF(CONF_VALUE) * extlist);

/*PROXY_CERT_INFO_EXTENSION function */

/* allocating and free memory */
//PROXY_CERT_INFO_EXTENSION * PROXY_CERT_INFO_EXTENSION_new();
//void PROXY_CERT_INFO_EXTENSION_free(PROXY_CERT_INFO_EXTENSION * proxycertinfo);

/* duplicate */
//PROXY_CERT_INFO_EXTENSION * PROXY_CERT_INFO_EXTENSION_dup(PROXY_CERT_INFO_EXTENSION * proxycertinfo);

//int PROXY_CERT_INFO_EXTENSION_print_fp(FILE* fp, PROXY_CERT_INFO_EXTENSION* cert_info);

/* set path_length */
int PROXY_CERT_INFO_EXTENSION_set_path_length(PROXY_CERT_INFO_EXTENSION * proxycertinfo, long path_length);

/* get path length */
long PROXY_CERT_INFO_EXTENSION_get_path_length(PROXY_CERT_INFO_EXTENSION * proxycertinfo);

/* set proxypolicy */
//int PROXY_CERT_INFO_EXTENSION_set_proxypolicy(PROXY_CERT_INFO_EXTENSION * proxycertinfo, PROXYPOLICY * proxypolicy);

/* get proxypolicy */
PROXY_POLICY * PROXY_CERT_INFO_EXTENSION_get_proxypolicy(PROXY_CERT_INFO_EXTENSION * proxycertinfo);

/* internal to der conversion */
//int i2d_PROXY_CERT_INFO_EXTENSION(PROXY_CERT_INFO_EXTENSION * proxycertinfo, unsigned char ** pp);

/* der to internal conversion */
//PROXY_CERT_INFO_EXTENSION * d2i_PROXY_CERT_INFO_EXTENSION(PROXY_CERT_INFO_EXTENSION ** cert_info, unsigned char ** a, long length);

//int PROXY_CERT_INFO_EXTENSION_set_version(PROXY_CERT_INFO_EXTENSION *cert_info, int version);

//STACK_OF(CONF_VALUE) * i2v_PROXY_CERT_INFO_EXTENSION(
//    struct v3_ext_method *              method,
//    PROXY_CERT_INFO_EXTENSION *                     ext,
//    STACK_OF(CONF_VALUE) *              extlist);

//int i2r_PROXY_CERT_INFO_EXTENSION(X509V3_EXT_METHOD *method, PROXY_CERT_INFO_EXTENSION *ext, BIO *out, int indent);

//PROXY_CERT_INFO_EXTENSION *r2i_PROXY_CERT_INFO_EXTENSION(X509V3_EXT_METHOD *method, X509V3_CTX *ctx, char *value);

//X509V3_EXT_METHOD * PROXY_CERT_INFO_EXTENSION_v3_x509v3_ext_meth();

//X509V3_EXT_METHOD * PROXY_CERT_INFO_EXTENSION_v4_x509v3_ext_meth();

} //namespace ArcCredential

#endif
