/// \cond
/**Borrow the code about Attribute Certificate from VOMS*/

/**The VOMSAttribute.h and VOMSAttribute.cpp are integration about code written by VOMS project, 
 *so here the original license follows. 
 */

#ifndef ARC_VOMSATTRIBUTE_H
#define ARC_VOMSATTRIBUTE_H

#include <openssl/asn1t.h>
#include <openssl/safestack.h>
#include <openssl/x509v3.h>


#define VOMS_AC_HEADER "-----BEGIN VOMS AC-----"
#define VOMS_AC_TRAILER "-----END VOMS AC-----"

namespace ArcCredential {

#define ASN1_F_D2I_AC_ATTR          5000
#define AC_F_ATTR_New               5001
#define ASN1_F_D2I_AC_ROLE          5002
#define AC_F_ROLE_New               5003
#define ASN1_F_D2I_AC_IETFATTR      5004
#define AC_F_IETFATTR_New           5005
#define ASN1_F_D2I_AC_IETFATTRVAL   5006
#define ASN1_F_D2I_AC_DIGEST        5007
#define AC_F_DIGEST_New             5008
#define ASN1_F_D2I_AC_IS            5009
#define AC_F_AC_IS_New              5010
#define ASN1_F_D2I_AC_FORM          5011
#define AC_F_AC_FORM_New            5012
#define ASN1_F_D2I_AC_ACI           5013
#define ASN1_F_AC_ACI_New           5014
#define ASN1_F_D2I_AC_HOLDER        5015
#define ASN1_F_AC_HOLDER_New        5016
#define ASN1_F_AC_VAL_New           5017
#define AC_F_AC_INFO_NEW            5018
#define AC_F_D2I_AC                 5019
#define AC_F_AC_New                 5020
#define ASN1_F_I2D_AC_IETFATTRVAL   5021
#define AC_F_D2I_AC_DIGEST          5022
#define AC_F_AC_DIGEST_New          5023
#define AC_F_D2I_AC_IS              5024
#define AC_ERR_UNSET                5025
#define AC_ERR_SET                  5026
#define AC_ERR_SIGNATURE            5027
#define AC_ERR_VERSION              5028
#define AC_ERR_HOLDER_SERIAL        5029
#define AC_ERR_HOLDER               5030
#define AC_ERR_UID_MISMATCH         5031
#define AC_ERR_ISSUER_NAME          5032
#define AC_ERR_SERIAL               5033
#define AC_ERR_DATES                5034
#define AC_ERR_ATTRIBS              5035
#define AC_F_AC_TARGET_New          5036
#define ASN1_F_D2I_AC_TARGET        5037
#define AC_F_AC_TARGETS_New         5036
#define ASN1_F_D2I_AC_TARGETS       5037
#define ASN1_F_D2I_AC_SEQ           5038
#define AC_F_AC_SEQ_new             5039
#define AC_ERR_ATTRIB_URI           5040
#define AC_ERR_ATTRIB_FQAN          5041
#define AC_ERR_EXTS_ABSENT          5042
#define AC_ERR_MEMORY               5043
#define AC_ERR_EXT_CRIT             5044
#define AC_ERR_EXT_TARGET           5045
#define AC_ERR_EXT_KEY              5046
#define AC_ERR_UNKNOWN              5047

#define AC_ERR_PARAMETERS           5048
#define X509_ERR_ISSUER_NAME        5049
#define X509_ERR_HOLDER_NAME        5050
#define AC_ERR_NO_EXTENSION         5051

#define ASN1_F_D2I_AC_CERTS         5052
#define AC_F_X509_New               5053

#define AC_F_D2I_AC_ATTRIBUTE       5054
#define AC_F_ATTRIBUTE_New          5055
#define ASN1_F_D2I_AC_ATT_HOLDER    5056
#define AC_F_AC_ATT_HOLDER_New      5057
#define ASN1_F_D2I_AC_FULL_ATTRIBUTES 5058
#define AC_F_AC_FULL_ATTRIBUTES_New 5059
#define ASN1_F_D2I_AC_ATTRIBUTEVAL  5060
#define ASN1_F_I2D_AC_ATTRIBUTEVAL  5061
#define AC_F_AC_ATTRIBUTEVAL_New    5062
#define AC_ERR_ATTRIB               5063

typedef struct ACDIGEST {
  ASN1_ENUMERATED *type;
  ASN1_OBJECT     *oid;
  X509_ALGOR      *algor;
  ASN1_BIT_STRING *digest;
} AC_DIGEST;

DECLARE_ASN1_FUNCTIONS(AC_DIGEST)

typedef struct ACIS {
  STACK_OF(GENERAL_NAME) *issuer;
  ASN1_INTEGER  *serial;
  ASN1_BIT_STRING *uid;
} AC_IS;

DECLARE_ASN1_FUNCTIONS(AC_IS)

typedef struct ACFORM {
  STACK_OF(GENERAL_NAME) *names;
  AC_IS         *is;
  AC_DIGEST     *digest;
} AC_FORM;

DECLARE_ASN1_FUNCTIONS(AC_FORM)

typedef struct ACACI {
  STACK_OF(GENERAL_NAME) *names;
  AC_FORM       *form;
} AC_ACI;

DECLARE_ASN1_FUNCTIONS(AC_ACI)

typedef struct ACHOLDER {
  AC_IS         *baseid;
  STACK_OF(GENERAL_NAME) *name;
  AC_DIGEST     *digest;
} AC_HOLDER;

DECLARE_ASN1_FUNCTIONS(AC_HOLDER)

typedef struct ACVAL {
  ASN1_GENERALIZEDTIME *notBefore;
  ASN1_GENERALIZEDTIME *notAfter;
} AC_VAL;

DECLARE_ASN1_FUNCTIONS(AC_VAL)

//typedef struct asn1_string_st AC_IETFATTRVAL;
//typedef ASN1_TYPE AC_IETFATTRVAL;
#define AC_IETFATTRVAL ASN1_TYPE
#define AC_IETFATTRVAL_new ASN1_TYPE_new
#define AC_IETFATTRVAL_free ASN1_TYPE_free
#define sk_AC_IETFATTRVAL_push sk_ASN1_TYPE_push
#define stack_st_AC_IETFATTRVAL stack_st_ASN1_TYPE
#define sk_AC_IETFATTRVAL_num sk_ASN1_TYPE_num
#define sk_AC_IETFATTRVAL_value sk_ASN1_TYPE_value
#define sk_AC_IETFATTRVAL_new_null sk_ASN1_TYPE_new_null

typedef struct ACIETFATTR {
  STACK_OF(GENERAL_NAME)   *names;
  STACK_OF(AC_IETFATTRVAL) *values;
} AC_IETFATTR;

DECLARE_ASN1_FUNCTIONS(AC_IETFATTR)

typedef struct ACTARGET {
  GENERAL_NAME *name;
  GENERAL_NAME *group;
  AC_IS        *cert;
} AC_TARGET;
 
DECLARE_ASN1_FUNCTIONS(AC_TARGET)

typedef struct ACTARGETS {
  STACK_OF(AC_TARGET) *targets;
} AC_TARGETS;

DECLARE_ASN1_FUNCTIONS(AC_TARGETS)

typedef struct ACATTR {
  ASN1_OBJECT * type;
  //int get_type;
  STACK_OF(AC_IETFATTR) *ietfattr;
  //STACK_OF(AC_FULL_ATTRIBUTES) *fullattributes;
} AC_ATTR;
#define GET_TYPE_FQAN 1
#define GET_TYPE_ATTRIBUTES 2

DECLARE_ASN1_FUNCTIONS(AC_ATTR)

typedef struct ACINFO {
  ASN1_INTEGER        *version;
  AC_HOLDER           *holder;
  AC_FORM             *form;
  X509_ALGOR          *alg;
  ASN1_INTEGER        *serial;
  AC_VAL              *validity;
  STACK_OF(AC_ATTR)   *attrib;
  ASN1_BIT_STRING     *id;
  STACK_OF(X509_EXTENSION) *exts;
} AC_INFO;

DECLARE_ASN1_FUNCTIONS(AC_INFO)

typedef struct ACC {
  AC_INFO         *acinfo;
  X509_ALGOR      *sig_alg;
  ASN1_BIT_STRING *signature;
} AC;

DECLARE_ASN1_FUNCTIONS(AC)

typedef struct ACSEQ {
  STACK_OF(AC) *acs;
} AC_SEQ;

DECLARE_ASN1_FUNCTIONS(AC_SEQ)

typedef struct ACCERTS {
  STACK_OF(X509) *stackcert;
} AC_CERTS;

DECLARE_ASN1_FUNCTIONS(AC_CERTS)

typedef struct ACATTRIBUTE {
  ASN1_OCTET_STRING *name;
  ASN1_OCTET_STRING *qualifier;
  ASN1_OCTET_STRING *value;
} AC_ATTRIBUTE;

DECLARE_ASN1_FUNCTIONS(AC_ATTRIBUTE)

typedef struct ACATTHOLDER {
  STACK_OF(GENERAL_NAME) *grantor;
  STACK_OF(AC_ATTRIBUTE) *attributes;
} AC_ATT_HOLDER;

DECLARE_ASN1_FUNCTIONS(AC_ATT_HOLDER)

typedef struct ACFULLATTRIBUTES {
  STACK_OF(AC_ATT_HOLDER) *providers;
} AC_FULL_ATTRIBUTES;

DECLARE_ASN1_FUNCTIONS(AC_FULL_ATTRIBUTES)


DEFINE_STACK_OF(AC_TARGET)
DEFINE_STACK_OF(AC_TARGETS)
DEFINE_STACK_OF(AC_IETFATTR)
//DEFINE_STACK_OF(AC_IETFATTRVAL)
DEFINE_STACK_OF(AC_ATTR)
DEFINE_STACK_OF(AC)
DEFINE_STACK_OF(AC_INFO)
DEFINE_STACK_OF(AC_VAL)
DEFINE_STACK_OF(AC_HOLDER)
DEFINE_STACK_OF(AC_ACI)
DEFINE_STACK_OF(AC_FORM)
DEFINE_STACK_OF(AC_IS)
DEFINE_STACK_OF(AC_DIGEST)
DEFINE_STACK_OF(AC_CERTS)
DEFINE_STACK_OF(AC_ATTRIBUTE)
DEFINE_STACK_OF(AC_ATT_HOLDER)
DEFINE_STACK_OF(AC_FULL_ATTRIBUTES)


X509V3_EXT_METHOD * VOMSAttribute_auth_x509v3_ext_meth();
X509V3_EXT_METHOD * VOMSAttribute_avail_x509v3_ext_meth();
X509V3_EXT_METHOD * VOMSAttribute_targets_x509v3_ext_meth();
X509V3_EXT_METHOD * VOMSAttribute_acseq_x509v3_ext_meth();
X509V3_EXT_METHOD * VOMSAttribute_certseq_x509v3_ext_meth();
X509V3_EXT_METHOD * VOMSAttribute_attribs_x509v3_ext_meth();

} // namespace ArcCredential

#endif
/// \endcond
