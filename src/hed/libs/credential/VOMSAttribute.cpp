#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <cstring>

#include "VOMSAttribute.h"

#include "VOMSUtil.h"

namespace ArcCredential {


IMPLEMENT_ASN1_FUNCTIONS(AC_SEQ)
ASN1_SEQUENCE(AC_SEQ) = {
  ASN1_SEQUENCE_OF(AC_SEQ, acs, AC)
} ASN1_SEQUENCE_END(AC_SEQ)

IMPLEMENT_ASN1_FUNCTIONS(AC_CERTS)
ASN1_SEQUENCE(AC_CERTS) = {
  ASN1_SEQUENCE_OF(AC_CERTS, stackcert, X509)
} ASN1_SEQUENCE_END(AC_CERTS)

IMPLEMENT_ASN1_FUNCTIONS(AC_ATTRIBUTE)
ASN1_SEQUENCE(AC_ATTRIBUTE) = {
  ASN1_SIMPLE(AC_ATTRIBUTE, name,      ASN1_OCTET_STRING),
  ASN1_SIMPLE(AC_ATTRIBUTE, value,     ASN1_OCTET_STRING),
  ASN1_SIMPLE(AC_ATTRIBUTE, qualifier, ASN1_OCTET_STRING)
} ASN1_SEQUENCE_END(AC_ATTRIBUTE)

IMPLEMENT_ASN1_FUNCTIONS(AC_ATT_HOLDER)
ASN1_SEQUENCE(AC_ATT_HOLDER) = {
  ASN1_SIMPLE(AC_ATT_HOLDER, grantor, GENERAL_NAMES),
  ASN1_SEQUENCE_OF(AC_ATT_HOLDER, attributes, AC_ATTRIBUTE)
} ASN1_SEQUENCE_END(AC_ATT_HOLDER)

IMPLEMENT_ASN1_FUNCTIONS(AC_FULL_ATTRIBUTES)
ASN1_SEQUENCE(AC_FULL_ATTRIBUTES) = {
  ASN1_SEQUENCE_OF(AC_FULL_ATTRIBUTES, providers, AC_ATT_HOLDER)
} ASN1_SEQUENCE_END(AC_FULL_ATTRIBUTES)


static char *norep()
{
  static char buffer[] = "";
  return buffer;
}



#if (OPENSSL_VERSION_NUMBER < 0x30400000L)
// --------------------------------
// Definitions for OpenSSL < 3.4.0
// --------------------------------

IMPLEMENT_ASN1_FUNCTIONS(AC_DIGEST)
ASN1_SEQUENCE(AC_DIGEST) = {
  ASN1_SIMPLE(AC_DIGEST, type, ASN1_ENUMERATED),
  ASN1_SIMPLE(AC_DIGEST, oid, ASN1_OBJECT),
  ASN1_SIMPLE(AC_DIGEST, algor, X509_ALGOR),
  ASN1_SIMPLE(AC_DIGEST, digest, ASN1_BIT_STRING)
} ASN1_SEQUENCE_END(AC_DIGEST)

IMPLEMENT_ASN1_FUNCTIONS(AC_IS)
ASN1_SEQUENCE(AC_IS) = {
  ASN1_SIMPLE(AC_IS, issuer, GENERAL_NAMES),
  ASN1_SIMPLE(AC_IS, serial, ASN1_INTEGER),
  ASN1_IMP_OPT(AC_IS, uid, ASN1_BIT_STRING, V_ASN1_BIT_STRING)
} ASN1_SEQUENCE_END(AC_IS)

IMPLEMENT_ASN1_FUNCTIONS(AC_FORM)
ASN1_SEQUENCE(AC_FORM) = {
  ASN1_SIMPLE(AC_FORM, names, GENERAL_NAMES),
  ASN1_IMP_OPT(AC_FORM, is, AC_IS, 0),
  ASN1_IMP_OPT(AC_FORM, digest, AC_DIGEST, 1)
} ASN1_SEQUENCE_END(AC_FORM)

IMPLEMENT_ASN1_FUNCTIONS(AC_ACI)
ASN1_SEQUENCE(AC_ACI) = {
  ASN1_IMP_OPT(AC_ACI, form, AC_FORM, 0)
} ASN1_SEQUENCE_END(AC_ACI)

IMPLEMENT_ASN1_FUNCTIONS(AC_HOLDER)
ASN1_SEQUENCE(AC_HOLDER) = {
  ASN1_IMP_OPT(AC_HOLDER, baseid, AC_IS, 0),
  ASN1_IMP_OPT(AC_HOLDER, name, GENERAL_NAMES, 1),
  ASN1_IMP_OPT(AC_HOLDER, digest, AC_DIGEST, 2)
} ASN1_SEQUENCE_END(AC_HOLDER)

IMPLEMENT_ASN1_FUNCTIONS(AC_VAL)
ASN1_SEQUENCE(AC_VAL) = {
  ASN1_SIMPLE(AC_VAL, notBefore, ASN1_GENERALIZEDTIME),
  ASN1_SIMPLE(AC_VAL, notAfter,  ASN1_GENERALIZEDTIME)
} ASN1_SEQUENCE_END(AC_VAL)

IMPLEMENT_ASN1_FUNCTIONS(AC_IETFATTR)
ASN1_SEQUENCE(AC_IETFATTR) = {
  ASN1_IMP_SEQUENCE_OF_OPT(AC_IETFATTR, names, GENERAL_NAME, 0),
  ASN1_SEQUENCE_OF(AC_IETFATTR, values, ASN1_ANY)
} ASN1_SEQUENCE_END(AC_IETFATTR)
  /*ASN1_IMP_OPT(AC_IETFATTR, names, GENERAL_NAMES, 0),*/
/*  ASN1_SEQUENCE_OF(AC_IETFATTR, values, AC_IETFATTRVAL) */

IMPLEMENT_ASN1_FUNCTIONS(AC_TARGET)
ASN1_SEQUENCE(AC_TARGET) = {
  ASN1_EXP_OPT(AC_TARGET, name, GENERAL_NAME, 0),
  ASN1_EXP_OPT(AC_TARGET, group, GENERAL_NAME, 1),
  ASN1_EXP_OPT(AC_TARGET, cert, AC_IS, 2)
} ASN1_SEQUENCE_END(AC_TARGET)

IMPLEMENT_ASN1_FUNCTIONS(AC_TARGETS)
ASN1_SEQUENCE(AC_TARGETS) = {
  ASN1_SEQUENCE_OF(AC_TARGETS, targets, AC_TARGET)
} ASN1_SEQUENCE_END(AC_TARGETS)

IMPLEMENT_ASN1_FUNCTIONS(AC_ATTR)
ASN1_SEQUENCE(AC_ATTR) = {
  ASN1_SIMPLE(AC_ATTR, type, ASN1_OBJECT),
  ASN1_SET_OF_OPT(AC_ATTR, ietfattr, AC_IETFATTR)
/*
  if (!i2t_ASN1_OBJECT(text,999,a->type))
    return 0;
  else if (!((strcmp(text, "idacagroup") == 0) || (strcmp(text,"idatcap") == 0)))
    return 0;


  ASN1_OBJECT * type;
  int get_type;
  STACK_OF(AC_IETFATTR) *ietfattr;
  STACK_OF(AC_FULL_ATTRIBUTES) *fullattributes;

*/
} ASN1_SEQUENCE_END(AC_ATTR)

IMPLEMENT_ASN1_FUNCTIONS(AC_INFO)
ASN1_SEQUENCE(AC_INFO) = {
  ASN1_SIMPLE(AC_INFO, version, ASN1_INTEGER),
  ASN1_SIMPLE(AC_INFO, holder, AC_HOLDER),
  ASN1_IMP_OPT(AC_INFO, form, AC_FORM, 0), /*V_ASN1_SEQUENCE*/
  ASN1_SIMPLE(AC_INFO, alg, X509_ALGOR),
  ASN1_SIMPLE(AC_INFO, serial, ASN1_INTEGER),
  ASN1_SIMPLE(AC_INFO, validity, AC_VAL),
  ASN1_SEQUENCE_OF(AC_INFO, attrib, AC_ATTR),
  ASN1_IMP_OPT(AC_INFO, id, ASN1_BIT_STRING, V_ASN1_BIT_STRING),
  ASN1_SEQUENCE_OF_OPT(AC_INFO, exts, X509_EXTENSION)
} ASN1_SEQUENCE_END(AC_INFO)

IMPLEMENT_ASN1_FUNCTIONS(AC)
ASN1_SEQUENCE(AC) = {
  ASN1_SIMPLE(AC, acinfo, AC_INFO),
  ASN1_SIMPLE(AC, sig_alg, X509_ALGOR),
  ASN1_SIMPLE(AC, signature, ASN1_BIT_STRING)
} ASN1_SEQUENCE_END(AC)

char *targets_i2s(struct v3_ext_method*, void*)
{
  return norep();
}

char *null_i2s(struct v3_ext_method*, void*)
{
  return norep();
}

void *targets_s2i(struct v3_ext_method*, struct v3_ext_ctx*, char *data)
{
  char* list = strdup(data);
  char* pos = list;
  AC_TARGETS *a = AC_TARGETS_new();

  while(pos) {
    char* cpos = strchr(pos, ',');
    if (cpos) *cpos = '\0';
    {
      GENERAL_NAME *g = GENERAL_NAME_new();
      ASN1_IA5STRING *tmpr = ASN1_IA5STRING_new();
      AC_TARGET *targ = AC_TARGET_new();

      if (!g || !tmpr || !targ) {
        GENERAL_NAME_free(g);
        ASN1_IA5STRING_free(tmpr);
        AC_TARGET_free(targ);
        goto err;
      }
      ASN1_STRING_set(tmpr, pos, strlen(list));
      g->type = GEN_URI;
      g->d.ia5 = tmpr;
      targ->name = g;
      sk_AC_TARGET_push(a->targets, targ);
    }
    pos = cpos;
    if (pos) ++pos;
  };
  free(list);

  return a;

 err:
  free(list);
  AC_TARGETS_free(a);
  return NULL;
}

void *null_s2i(struct v3_ext_method*, struct v3_ext_ctx*, char*)
{
  return ASN1_NULL_new();
}

char *authkey_i2s(struct v3_ext_method*, void*)
{
  return norep();
}

void *authkey_s2i(struct v3_ext_method*, struct v3_ext_ctx* ctx, char *data)
{
  AUTHORITY_KEYID* keyid = NULL;
  X509* cert = ctx ? ctx->issuer_cert : NULL;
  if(cert) {
    ASN1_BIT_STRING* pkeystr = X509_get0_pubkey_bitstr(cert);
    if(pkeystr) {
      ASN1_OCTET_STRING *str = ASN1_OCTET_STRING_new();
      if(str) {
        keyid = AUTHORITY_KEYID_new();
        if(keyid) {
          char digest[21];
          SHA1(pkeystr->data,
              pkeystr->length,
              (unsigned char*)digest);
          ASN1_OCTET_STRING_set(str, (unsigned char*)digest, 20);
          if(keyid->keyid) ASN1_OCTET_STRING_free(keyid->keyid);
          keyid->keyid = str; str = NULL;
        }
        if (str) ASN1_OCTET_STRING_free(str);
      }
    }
  }
  return keyid;
}

X509V3_EXT_METHOD * VOMSAttribute_auth_x509v3_ext_meth() {
  static X509V3_EXT_METHOD vomsattribute_auth_x509v3_ext_meth =
  {
    -1,
    0,
    NULL,
    (X509V3_EXT_NEW) AUTHORITY_KEYID_new,
    (X509V3_EXT_FREE) AUTHORITY_KEYID_free,
    (X509V3_EXT_D2I) d2i_AUTHORITY_KEYID,
    (X509V3_EXT_I2D) i2d_AUTHORITY_KEYID,
    (X509V3_EXT_I2S) authkey_i2s,
    (X509V3_EXT_S2I) authkey_s2i,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL
  };
  return (&vomsattribute_auth_x509v3_ext_meth);
}

X509V3_EXT_METHOD * VOMSAttribute_avail_x509v3_ext_meth() {
  static X509V3_EXT_METHOD vomsattribute_avail_x509v3_ext_meth =
  {
    -1,
    0,
    NULL,
    (X509V3_EXT_NEW) ASN1_NULL_new,
    (X509V3_EXT_FREE) ASN1_NULL_free,
    (X509V3_EXT_D2I) d2i_ASN1_NULL,
    (X509V3_EXT_I2D) i2d_ASN1_NULL,
    (X509V3_EXT_I2S) null_i2s,
    (X509V3_EXT_S2I) null_s2i,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL
  };
  return (&vomsattribute_avail_x509v3_ext_meth);
}

X509V3_EXT_METHOD * VOMSAttribute_targets_x509v3_ext_meth() {
  static X509V3_EXT_METHOD vomsattribute_targets_x509v3_ext_meth =
  {
    -1,
    0,
    NULL,
    (X509V3_EXT_NEW) AC_TARGETS_new,
    (X509V3_EXT_FREE) AC_TARGETS_free,
    (X509V3_EXT_D2I) d2i_AC_TARGETS,
    (X509V3_EXT_I2D) i2d_AC_TARGETS,
    (X509V3_EXT_I2S) targets_i2s,
    (X509V3_EXT_S2I) targets_s2i,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL
  };
  return (&vomsattribute_targets_x509v3_ext_meth);
}


#endif // (OPENSSL_VERSION_NUMBER < 0x30400000L)


void *acseq_s2i(struct v3_ext_method*, struct v3_ext_ctx*, char *data)
{
  AC_SEQ* acseq = NULL;
  AC** aclist = NULL;
  std::string acseq_str;
  std::string ac_str;
  if(data == NULL) return NULL;
  acseq_str = data;

  std::string::size_type pos1 = 0, pos2 = 0; 
  while(pos1 < acseq_str.length()) {
    pos1 = acseq_str.find(VOMS_AC_HEADER, pos1);
    if(pos1 == std::string::npos) break;
    pos1 = acseq_str.find_first_of("\r\n", pos1);
    if(pos1 == std::string::npos) break;
    pos2 = acseq_str.find(VOMS_AC_TRAILER, pos1);
    if(pos2 == std::string::npos) break;
    ac_str.clear();
    ac_str = acseq_str.substr(pos1+1, (pos2-1) - (pos1+1));

    pos2 = acseq_str.find_first_of("\r\n", pos2);
    if(pos2 == std::string::npos) pos2 = acseq_str.length();
    pos1 = pos2+1;

    // decode the AC string
    int size;
    char* dec = NULL;
    std::string decodedac;
    dec = Arc::VOMSDecode((char*)(ac_str.c_str()), ac_str.length(), &size);
    if (dec != NULL) {
      decodedac.append(dec, size);
      free(dec);
      dec = NULL;
    }
    // TODO: is the ac order required?
    std::string acorder;      
    Arc::addVOMSAC(aclist, acorder, decodedac);
  }

  if (!aclist) return NULL;

  AC** ac = aclist;

  acseq = AC_SEQ_new();
  if(acseq) {
    while (*ac) {
      if(!sk_AC_push(acseq->acs, *ac)) break;
      ++ac;
    }
  }

  if((*ac) || (!acseq)) {
    // Not (all) ACs were transfered - error
    while (*ac) { AC_free(*ac); ++ac; };
    free(aclist);
    if(acseq) AC_SEQ_free(acseq);
    return NULL;
  }

  free(aclist);

  return (void *)acseq;
}

char *acseq_i2s(struct v3_ext_method*, void* data)
{
  AC_SEQ* acseq = NULL;
  acseq = (AC_SEQ *)data;
  if(!acseq) return NULL;
  std::string encoded_acseq;

  AC *item = NULL;
  int num = sk_AC_num(acseq->acs);
  for (int i =0; i < num; i++) {
    item = sk_AC_value(acseq->acs, i);
    unsigned int len = i2d_AC(item, NULL);
    unsigned char *tmp = (unsigned char *)OPENSSL_malloc(len);
    std::string ac_str;
    if(tmp) {
      unsigned char *ttmp = tmp;
      i2d_AC(item, &ttmp);
      //ac_str = std::string((char *)tmp, len);
      ac_str.append((const char*)tmp, len);
      free(tmp);
    }

    // encode the AC string
    int size;
    char* enc = NULL;
    std::string encodedac;
    enc = Arc::VOMSEncode((char*)(ac_str.c_str()), ac_str.length(), &size);
    if (enc != NULL) {
      encodedac.append(enc, size);
      free(enc);
      enc = NULL;
    }
    encoded_acseq.append(VOMS_AC_HEADER).append("\n");
    encoded_acseq.append(encodedac).append("\n");
    encoded_acseq.append(VOMS_AC_TRAILER).append("\n");
  }
  
  char* ret = NULL;
  int len = encoded_acseq.length();
  if(len) {
    ret = (char*)OPENSSL_malloc(len + 1);
    memset(ret, 0, len + 1);
    memcpy(ret, encoded_acseq.c_str(), len);
  }
  return (char *) ret;
}

void *certs_s2i(struct v3_ext_method*, struct v3_ext_ctx*, char *data)
{
  STACK_OF(X509) *certs =
    (STACK_OF(X509) *)data;
  int i = 0;

  if (data) {
    AC_CERTS *a = AC_CERTS_new();

    sk_X509_pop_free(a->stackcert, X509_free);
    a->stackcert = sk_X509_new_null();

    for (i =0; i < sk_X509_num(certs); i++)
      sk_X509_push(a->stackcert, X509_dup(sk_X509_value(certs, i)));

    return a;
  }

  return NULL;    
}

char *certs_i2s(struct v3_ext_method*, void*)
{
  return norep();
}

void *attributes_s2i(struct v3_ext_method*, struct v3_ext_ctx*, char *data)
{
  int i = 0;

  STACK_OF(AC_ATT_HOLDER) *stack =
    (STACK_OF(AC_ATT_HOLDER) *)data;

  if (data) {
    AC_FULL_ATTRIBUTES *a = AC_FULL_ATTRIBUTES_new();
    sk_AC_ATT_HOLDER_pop_free(a->providers, AC_ATT_HOLDER_free);
    a->providers = sk_AC_ATT_HOLDER_new_null();
    for (i = 0; i < sk_AC_ATT_HOLDER_num(stack); i++) {
      sk_AC_ATT_HOLDER_push(a->providers,
           ASN1_dup_of(AC_ATT_HOLDER, i2d_AC_ATT_HOLDER,
           d2i_AC_ATT_HOLDER,
           sk_AC_ATT_HOLDER_value(stack, i)));
    };
    return a;
  }
  return NULL;
}

char *attributes_i2s(struct v3_ext_method*, void*)
{
  return norep();
}


X509V3_EXT_METHOD * VOMSAttribute_acseq_x509v3_ext_meth() {
  static X509V3_EXT_METHOD vomsattribute_acseq_x509v3_ext_meth =
  {
    -1,
    0,  
    NULL,
    (X509V3_EXT_NEW) AC_SEQ_new,
    (X509V3_EXT_FREE) AC_SEQ_free,
    (X509V3_EXT_D2I) d2i_AC_SEQ,
    (X509V3_EXT_I2D) i2d_AC_SEQ,
    (X509V3_EXT_I2S) acseq_i2s, 
    (X509V3_EXT_S2I) acseq_s2i,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL
  };
  return (&vomsattribute_acseq_x509v3_ext_meth);
}  

X509V3_EXT_METHOD * VOMSAttribute_certseq_x509v3_ext_meth() {
  static X509V3_EXT_METHOD vomsattribute_certseq_x509v3_ext_meth =
  {
    -1,
    0,  
    NULL,
    (X509V3_EXT_NEW) AC_CERTS_new,
    (X509V3_EXT_FREE) AC_CERTS_free,
    (X509V3_EXT_D2I) d2i_AC_CERTS,
    (X509V3_EXT_I2D) i2d_AC_CERTS,
    (X509V3_EXT_I2S) certs_i2s, 
    (X509V3_EXT_S2I) certs_s2i,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL
  };
  return (&vomsattribute_certseq_x509v3_ext_meth);
}  

X509V3_EXT_METHOD * VOMSAttribute_attribs_x509v3_ext_meth() {
  static X509V3_EXT_METHOD vomsattribute_attribs_x509v3_ext_meth =
  {
    -1,
    0,
    NULL,
    (X509V3_EXT_NEW) AC_FULL_ATTRIBUTES_new,
    (X509V3_EXT_FREE) AC_FULL_ATTRIBUTES_free,
    (X509V3_EXT_D2I) d2i_AC_FULL_ATTRIBUTES,
    (X509V3_EXT_I2D) i2d_AC_FULL_ATTRIBUTES,
    (X509V3_EXT_I2S) attributes_i2s,  
    (X509V3_EXT_S2I) attributes_s2i,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL
  };
  return (&vomsattribute_attribs_x509v3_ext_meth);
}


} //namespace ArcCredential
