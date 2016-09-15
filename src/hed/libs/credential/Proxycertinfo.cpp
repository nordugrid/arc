#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <cstring>

/*
#include <openssl/err.h>
#include <openssl/asn1.h>
#include <openssl/objects.h>
*/

#include "Proxycertinfo.h"

namespace ArcCredential {

/* PROXYPOLICY function */

/*
PROXYPOLICY * PROXYPOLICY_new() {
  PROXYPOLICY* ret = (PROXYPOLICY*)OPENSSL_malloc(sizeof(PROXYPOLICY));
  if(ret != NULL) {
    ret->policyLanguage = OBJ_nid2obj(OBJ_sn2nid(IMPERSONATION_PROXY_SN));
    ret->policy = NULL;
  } else {
    ASN1err(ASN1_F_PROXYPOLICY_NEW, ERR_R_MALLOC_FAILURE);
  }
  return ret;
}

#ifndef NO_OLD_ASN1
char *ASN1_dup(int (*i2d)(void*, unsigned char**), char *(*d2i)(void**, const unsigned char**, long int), char *x) {
  unsigned char *b,*p;
  long i;
  char *ret;
  if (x == NULL) return(NULL);
  i=(long)i2d(x,NULL);
  b=(unsigned char *)OPENSSL_malloc((unsigned int)i+10);
  if (b == NULL) { ASN1err(ASN1_F_ASN1_DUP,ERR_R_MALLOC_FAILURE); return(NULL); }
  p= b;
  i=i2d(x,&p);
  p= b;
  ret=d2i(NULL,(const unsigned char**)&p,i);
  OPENSSL_free(b);
  return(ret);
  }
#endif

void PROXYPOLICY_free(PROXYPOLICY * policy) {
  if(policy == NULL) return;
  ASN1_OBJECT_free(policy->policyLanguage);
  ASN1_OCTET_STRING_free(policy->policy);
  OPENSSL_free(policy);
}
*/

/* duplicate */
/*
PROXYPOLICY * PROXYPOLICY_dup(PROXYPOLICY * policy) {
  return ((PROXYPOLICY *) ASN1_dup((int (*)(void*, unsigned char**))i2d_PROXYPOLICY,
				   (char *(*)(void**, const unsigned char**, long int))d2i_PROXYPOLICY,
				   (char *)policy));
}

int PROXYPOLICY_print(BIO* bp, PROXYPOLICY* policy) {
  STACK_OF(CONF_VALUE)* values = NULL;
  values = i2v_PROXYPOLICY(PROXYPOLICY_x509v3_ext_meth(), policy, values);
  X509V3_EXT_val_prn(bp, values, 0, 1);
  sk_CONF_VALUE_pop_free(values, X509V3_conf_free);
  return 1;
}
*/

/* set policy language */
int PROXY_POLICY_set_policy_language(PROXY_POLICY * policy, ASN1_OBJECT * policy_language) {
  if(policy_language != NULL) {
    if(policy_language != policy->policyLanguage) {
      ASN1_OBJECT_free(policy->policyLanguage);
      policy->policyLanguage = OBJ_dup(policy_language);
    }
    return 1;
  }
  return 0;
}

/* get policy language */
ASN1_OBJECT * PROXY_POLICY_get_policy_language(PROXY_POLICY * policy)
{
  return policy->policyLanguage;
}

/* set policy */
int PROXY_POLICY_set_policy(PROXY_POLICY * proxypolicy, unsigned char * policy, int length) {
  if(policy != NULL) {
    /* if member policy of proxypolicy non set */
    if(!proxypolicy->policy)
      proxypolicy->policy = ASN1_OCTET_STRING_new();
    /* set member policy of proxypolicy */
    ASN1_OCTET_STRING_set(proxypolicy->policy, policy, length);
  }
  else if(proxypolicy->policy) {
    ASN1_OCTET_STRING_free(proxypolicy->policy);
    proxypolicy->policy = NULL;
  }
  return 1;
}

/* get policy */
unsigned char * PROXY_POLICY_get_policy(PROXY_POLICY * proxypolicy, int * length) {
  /* assure field policy is set */
  if(proxypolicy->policy) {
    *length = proxypolicy->policy->length;
    /* assure ASN1_OCTET_STRING is full */
    if (*length>0 && proxypolicy->policy->data) {
      unsigned char * copy = (unsigned char*) malloc(*length);
      if(copy) {
        memcpy(copy, proxypolicy->policy->data, *length);
        return copy;
      }
    }
  }
  /* else return NULL */
  return NULL;
}

/* internal to der conversion */
/*
int i2d_PROXYPOLICY(PROXYPOLICY * policy, unsigned char ** pp)  {
#if 0
  int  v1 = 0;
    
  M_ASN1_I2D_vars(policy);

  M_ASN1_I2D_len(policy->policy_language, i2d_ASN1_OBJECT);
  M_ASN1_I2D_len_EXP_opt(policy->policy, i2d_ASN1_OCTET_STRING, 0, v1);
  M_ASN1_I2D_seq_total();
  M_ASN1_I2D_put(policy->policy_language, i2d_ASN1_OBJECT);
  M_ASN1_I2D_put_EXP_opt(policy->policy, i2d_ASN1_OCTET_STRING, 0, v1);

  M_ASN1_I2D_finish();
#endif

  M_ASN1_I2D_vars(policy);

  M_ASN1_I2D_len(policy->policy_language, i2d_ASN1_OBJECT);

  if(policy->policy) { 
    M_ASN1_I2D_len(policy->policy, i2d_ASN1_OCTET_STRING);
  }
    
  M_ASN1_I2D_seq_total();
  M_ASN1_I2D_put(policy->policy_language, i2d_ASN1_OBJECT);
  if(policy->policy) { 
    M_ASN1_I2D_put(policy->policy, i2d_ASN1_OCTET_STRING);
  }
  M_ASN1_I2D_finish();
  return 0;
}

PROXYPOLICY * d2i_PROXYPOLICY(PROXYPOLICY ** a, unsigned char ** pp, long length) {
  M_ASN1_D2I_vars(a, PROXYPOLICY *, PROXYPOLICY_new);
  M_ASN1_D2I_Init();
  M_ASN1_D2I_start_sequence();
  M_ASN1_D2I_get(ret->policy_language, d2i_ASN1_OBJECT);

  * need to try getting the policy using
   *     a) a call expecting no tags
   *     b) a call expecting tags
   * one of which should succeed
   *
    
  M_ASN1_D2I_get_opt(ret->policy, d2i_ASN1_OCTET_STRING, V_ASN1_OCTET_STRING);
  M_ASN1_D2I_get_IMP_opt(ret->policy, d2i_ASN1_OCTET_STRING, 0, V_ASN1_OCTET_STRING);
  M_ASN1_D2I_Finish(a, PROXYPOLICY_free, ASN1_F_D2I_PROXYPOLICY);
}

STACK_OF(CONF_VALUE) * i2v_PROXYPOLICY( struct v3_ext_method* * method *, PROXYPOLICY*  ext, STACK_OF(CONF_VALUE)* extlist) {
  char* policy = NULL;
  char  policy_lang[128];
  char* tmp_string = NULL;
  char* index = NULL;
  int   nid;
  int   policy_length;

  X509V3_add_value("Proxy Policy:", NULL, &extlist);
  nid = OBJ_obj2nid(PROXYPOLICY_get_policy_language(ext));
  if(nid != NID_undef) { BIO_snprintf(policy_lang, 128, " %s", OBJ_nid2ln(nid)); }
  else {
    policy_lang[0] = ' ';
    i2t_ASN1_OBJECT(&policy_lang[1], 127, PROXYPOLICY_get_policy_language(ext));
  }
   
  X509V3_add_value("    Policy Language",  policy_lang, &extlist);    
  policy = (char *) PROXYPOLICY_get_policy(ext, &policy_length);
  if(!policy) {
    X509V3_add_value("    Policy", " EMPTY", &extlist);
  }
  else {
    X509V3_add_value("    Policy:", NULL, &extlist);
    tmp_string = policy;
    while(1) {
      index = strchr(tmp_string, '\n');
      if(!index) {
        int length;
        unsigned char* last_string;
        length = (policy_length - (tmp_string - policy)) + 9;
        last_string = (unsigned char*)  malloc(length);
        BIO_snprintf((char*)last_string, length, "%8s%s", "", tmp_string);
        X509V3_add_value(NULL, (const char*)last_string, &extlist);
        free(last_string);
        break;
      }      
      *index = '\0';
            
      X509V3_add_value(NULL, tmp_string, &extlist);
            
      tmp_string = index + 1;
    }      
    free(policy);
  }
  return extlist;
}

X509V3_EXT_METHOD * PROXYPOLICY_x509v3_ext_meth() {
  static X509V3_EXT_METHOD proxypolicy_x509v3_ext_meth =
  {
    -1,
    X509V3_EXT_MULTILINE,
    NULL,
    (X509V3_EXT_NEW) PROXYPOLICY_new,
    (X509V3_EXT_FREE) PROXYPOLICY_free,
    (X509V3_EXT_D2I) d2i_PROXYPOLICY,
    (X509V3_EXT_I2D) i2d_PROXYPOLICY,
    NULL, NULL,
    (X509V3_EXT_I2V) i2v_PROXYPOLICY,
    NULL,
    NULL, NULL,
    NULL
  };
  return (&proxypolicy_x509v3_ext_meth);
}
*/


/** PROXY_CERT_INFO_EXTENSION function */

/*
PROXY_CERT_INFO_EXTENSION * PROXY_CERT_INFO_EXTENSION_new() {
  PROXY_CERT_INFO_EXTENSION* ret = (PROXY_CERT_INFO_EXTENSION*)OPENSSL_malloc(sizeof(PROXY_CERT_INFO_EXTENSION));
  if(ret != NULL) {
    memset(ret, 0, sizeof(PROXY_CERT_INFO_EXTENSION));
    ret->path_length      = NULL;
    ret->proxypolicy      = PROXYPOLICY_new();
  } else {
    ASN1err(ASN1_F_PROXY_CERT_INFO_EXTENSION_NEW, ERR_R_MALLOC_FAILURE);
  }
  return (ret);
}

void PROXY_CERT_INFO_EXTENSION_free(PROXY_CERT_INFO_EXTENSION * proxycertinfo) {
  if(proxycertinfo == NULL) return;
  ASN1_INTEGER_free(proxycertinfo->path_length);
  PROXYPOLICY_free(proxycertinfo->proxypolicy);
  OPENSSL_free(proxycertinfo);
}

PROXY_CERT_INFO_EXTENSION * PROXY_CERT_INFO_EXTENSION_dup(PROXY_CERT_INFO_EXTENSION * proxycertinfo) {
  PROXY_CERT_INFO_EXTENSION * new_proxycertinfo = NULL;
  if(proxycertinfo == NULL) return NULL;
  new_proxycertinfo = PROXY_CERT_INFO_EXTENSION_new();
  if(new_proxycertinfo == NULL) return NULL;
  if(proxycertinfo->path_length) {
    new_proxycertinfo->path_length =
            ASN1_INTEGER_dup(proxycertinfo->path_length);
  }
  new_proxycertinfo->version = proxycertinfo->version;
  PROXY_CERT_INFO_EXTENSION_set_proxypolicy(new_proxycertinfo,proxycertinfo->proxypolicy);
  return new_proxycertinfo;
}

int PROXY_CERT_INFO_EXTENSION_print(BIO* bp, PROXY_CERT_INFO_EXTENSION* cert_info) {
  STACK_OF(CONF_VALUE)* values = NULL;
  values = i2v_PROXY_CERT_INFO_EXTENSION(PROXY_CERT_INFO_EXTENSION_v4_x509v3_ext_meth(), cert_info, NULL);
  X509V3_EXT_val_prn(bp, values, 0, 1);
  sk_CONF_VALUE_pop_free(values, X509V3_conf_free);
  return 1;
}

int PROXY_CERT_INFO_EXTENSION_print_fp(FILE* fp, PROXY_CERT_INFO_EXTENSION* cert_info) {
  int ret;
  BIO* bp;
  bp = BIO_new(BIO_s_file());  
  BIO_set_fp(bp, fp, BIO_NOCLOSE);
  ret =  PROXY_CERT_INFO_EXTENSION_print(bp, cert_info);
  BIO_free(bp);
  return (ret);
}   
*/

/* set path_length */
int PROXY_CERT_INFO_EXTENSION_set_path_length(PROXY_CERT_INFO_EXTENSION * proxycertinfo, long path_length) {
  /* assure proxycertinfo is not empty */
  if(proxycertinfo != NULL) {
    if(path_length != -1) {
      /* if member pcPathLengthConstraint is empty allocate memory the set */
      if(proxycertinfo->pcPathLengthConstraint == NULL)
	proxycertinfo->pcPathLengthConstraint = ASN1_INTEGER_new();
      return ASN1_INTEGER_set(proxycertinfo->pcPathLengthConstraint, path_length);
    }
    else 
      if(proxycertinfo->pcPathLengthConstraint != NULL) {
	ASN1_INTEGER_free(proxycertinfo->pcPathLengthConstraint);
	proxycertinfo->pcPathLengthConstraint = NULL;
      }
    return 1;
  }
  return 0;
}

/*
int PROXY_CERT_INFO_EXTENSION_set_version(PROXY_CERT_INFO_EXTENSION * proxycertinfo, int version) {
  if (proxycertinfo != NULL) {
    proxycertinfo->version = version;
    return 1;
  }
  return 0;
}

int PROXY_CERT_INFO_EXTENSION_get_version(PROXY_CERT_INFO_EXTENSION * proxycertinfo) {
  if (proxycertinfo)
    return proxycertinfo->version;
  return -1;
}
*/

/* get path length */
long PROXY_CERT_INFO_EXTENSION_get_path_length(PROXY_CERT_INFO_EXTENSION * proxycertinfo) {
  if(proxycertinfo && proxycertinfo->pcPathLengthConstraint)
    return ASN1_INTEGER_get(proxycertinfo->pcPathLengthConstraint);
  else return -1;
}

/*
* set policy *
int PROXY_CERT_INFO_EXTENSION_set_proxypolicy(PROXY_CERT_INFO_EXTENSION * proxycertinfo, PROXYPOLICY * proxypolicy) {
  if(proxypolicy != proxycertinfo->proxypolicy) {
    PROXYPOLICY_free(proxycertinfo->proxypolicy);
    if(proxypolicy != NULL)
      proxycertinfo->proxypolicy = PROXYPOLICY_dup(proxypolicy);
    else
      proxycertinfo->proxypolicy = NULL;
  }
  return 1;
}
*/

/* get policy */
PROXY_POLICY * PROXY_CERT_INFO_EXTENSION_get_proxypolicy(PROXY_CERT_INFO_EXTENSION * proxycertinfo) {
  if(proxycertinfo)
    return proxycertinfo->proxyPolicy;
  return NULL;
}

/*
* internal to der conversion *
int i2d_PROXY_CERT_INFO_EXTENSION_v3(PROXY_CERT_INFO_EXTENSION * proxycertinfo, unsigned char ** pp) {
  int v1;
  M_ASN1_I2D_vars(proxycertinfo);
  v1 = 0;
  M_ASN1_I2D_len(proxycertinfo->proxypolicy, i2d_PROXYPOLICY);
  M_ASN1_I2D_len_EXP_opt(proxycertinfo->path_length,i2d_ASN1_INTEGER, 1, v1);
  M_ASN1_I2D_seq_total();
  M_ASN1_I2D_put(proxycertinfo->proxypolicy, i2d_PROXYPOLICY);
  M_ASN1_I2D_put_EXP_opt(proxycertinfo->path_length, i2d_ASN1_INTEGER, 1, v1);
  M_ASN1_I2D_finish();
  return 0;
}

int i2d_PROXY_CERT_INFO_EXTENSION_v4(PROXY_CERT_INFO_EXTENSION * proxycertinfo, unsigned char ** pp) {
  M_ASN1_I2D_vars(proxycertinfo);
  if(proxycertinfo->path_length) { 
    M_ASN1_I2D_len(proxycertinfo->path_length, i2d_ASN1_INTEGER);
  }
  M_ASN1_I2D_len(proxycertinfo->proxypolicy, i2d_PROXYPOLICY);
  M_ASN1_I2D_seq_total();
  if(proxycertinfo->path_length) { 
    M_ASN1_I2D_put(proxycertinfo->path_length, i2d_ASN1_INTEGER);
  }
  M_ASN1_I2D_put(proxycertinfo->proxypolicy, i2d_PROXYPOLICY);
  M_ASN1_I2D_finish();
  return 0;
}

int i2d_PROXY_CERT_INFO_EXTENSION(PROXY_CERT_INFO_EXTENSION * proxycertinfo, unsigned char ** pp) {
  switch(proxycertinfo->version) {
  case 3:
    return i2d_PROXY_CERT_INFO_EXTENSION_v3(proxycertinfo, pp);
    break;

  case 4:
    return i2d_PROXY_CERT_INFO_EXTENSION_v4(proxycertinfo, pp);
    break;

  default:
    return -1;
    break;
  }
  return 0;
}

PROXY_CERT_INFO_EXTENSION * d2i_PROXY_CERT_INFO_EXTENSION_v3(PROXY_CERT_INFO_EXTENSION ** cert_info, unsigned char ** pp, long length) {
  M_ASN1_D2I_vars(cert_info, PROXY_CERT_INFO_EXTENSION *, PROXY_CERT_INFO_EXTENSION_new);
  M_ASN1_D2I_Init();
  M_ASN1_D2I_start_sequence();
  //M_ASN1_D2I_get(ret->proxypolicy, (unsigned char**)d2i_PROXYPOLICY);
  c.q=c.p;
  if (d2i_PROXYPOLICY(&(ret->proxypolicy),(unsigned char**)&c.p,c.slen) == NULL)
   {c.line=__LINE__; goto err; } 
  c.slen-=(c.p-c.q);

  M_ASN1_D2I_get_EXP_opt(ret->path_length, d2i_ASN1_INTEGER, 1);
  ret->version = 3;
  M_ASN1_D2I_Finish(cert_info, PROXY_CERT_INFO_EXTENSION_free, ASN1_F_D2I_PROXY_CERT_INFO_EXTENSION);
}

PROXY_CERT_INFO_EXTENSION * d2i_PROXY_CERT_INFO_EXTENSION_v4(PROXY_CERT_INFO_EXTENSION ** cert_info, unsigned char ** pp, long length) {


DECLARE_ASN1_FUNCTIONS





  M_ASN1_D2I_vars(cert_info, PROXY_CERT_INFO_EXTENSION *, PROXY_CERT_INFO_EXTENSION_new);
  M_ASN1_D2I_Init();
  M_ASN1_D2I_start_sequence();
  M_ASN1_D2I_get_EXP_opt(ret->path_length, d2i_ASN1_INTEGER, 1);
  M_ASN1_D2I_get_opt(ret->path_length, d2i_ASN1_INTEGER, V_ASN1_INTEGER);
  //M_ASN1_D2I_get(ret->proxypolicy, (unsigned char**)d2i_PROXYPOLICY);
  c.q=c.p;
  if (d2i_PROXYPOLICY(&(ret->proxypolicy),(unsigned char**)&c.p,c.slen) == NULL)
   {c.line=__LINE__; goto err; }
  c.slen-=(c.p-c.q);

  ret->version = 4;
  M_ASN1_D2I_Finish(cert_info, PROXY_CERT_INFO_EXTENSION_free, ASN1_F_D2I_PROXY_CERT_INFO_EXTENSION);
}

PROXY_CERT_INFO_EXTENSION * d2i_PROXY_CERT_INFO_EXTENSION(PROXY_CERT_INFO_EXTENSION ** cert_info, unsigned char ** pp, long length) {
  PROXY_CERT_INFO_EXTENSION *info = d2i_PROXY_CERT_INFO_EXTENSION_v3(cert_info, pp, length);
  if (!info)
    info = d2i_PROXY_CERT_INFO_EXTENSION_v4(cert_info, pp, length);
  return info;
}

STACK_OF(CONF_VALUE) * i2v_PROXY_CERT_INFO_EXTENSION(struct v3_ext_method* * method *, 
    PROXY_CERT_INFO_EXTENSION* ext, STACK_OF(CONF_VALUE)* extlist) {
  int     len = 128;
  char    tmp_string[128];
  if(!ext) {
    extlist = NULL;
    return extlist;
  }
  if(extlist == NULL) {
    extlist = sk_CONF_VALUE_new_null();
    if(extlist == NULL) { 
      return NULL;
    }
  }
  if(PROXY_CERT_INFO_EXTENSION_get_path_length(ext) > -1) {
    memset(tmp_string, 0, len);
    BIO_snprintf(tmp_string, len, " %lu (0x%lx)",
        PROXY_CERT_INFO_EXTENSION_get_path_length(ext),
        PROXY_CERT_INFO_EXTENSION_get_path_length(ext));
    X509V3_add_value("Path Length", tmp_string, &extlist);
  }
  if(PROXY_CERT_INFO_EXTENSION_get_proxypolicy(ext)) {
    i2v_PROXYPOLICY(PROXYPOLICY_x509v3_ext_meth(), PROXY_CERT_INFO_EXTENSION_get_proxypolicy(ext), extlist);
  }
  return extlist;
}

X509V3_EXT_METHOD * PROXY_CERT_INFO_EXTENSION_v4_x509v3_ext_meth() {
    static X509V3_EXT_METHOD proxycertinfo_v4_x509v3_ext_meth =
    {
        -1,
        X509V3_EXT_MULTILINE,
        NULL,
        (X509V3_EXT_NEW) PROXY_CERT_INFO_EXTENSION_new,
        (X509V3_EXT_FREE) PROXY_CERT_INFO_EXTENSION_free,
        (X509V3_EXT_D2I) d2i_PROXY_CERT_INFO_EXTENSION_v4,
        (X509V3_EXT_I2D) i2d_PROXY_CERT_INFO_EXTENSION_v4,
        NULL, NULL,
        (X509V3_EXT_I2V) i2v_PROXY_CERT_INFO_EXTENSION,
        NULL,
        NULL, //(X509V3_EXT_I2R) i2r_PROXY_CERT_INFO_EXTENSION,
        NULL, //(X509V3_EXT_R2I) r2i_PROXY_CERT_INFO_EXTENSION,
        NULL
    };
    return (&proxycertinfo_v4_x509v3_ext_meth);
}

X509V3_EXT_METHOD * PROXY_CERT_INFO_EXTENSION_v3_x509v3_ext_meth() {
  static X509V3_EXT_METHOD proxycertinfo_v3_x509v3_ext_meth =
  {
    -1,
    X509V3_EXT_MULTILINE,
    NULL,
    (X509V3_EXT_NEW) PROXY_CERT_INFO_EXTENSION_new,
    (X509V3_EXT_FREE) PROXY_CERT_INFO_EXTENSION_free,
    (X509V3_EXT_D2I) d2i_PROXY_CERT_INFO_EXTENSION_v3,
    (X509V3_EXT_I2D) i2d_PROXY_CERT_INFO_EXTENSION_v3,
    NULL, NULL,
    (X509V3_EXT_I2V) i2v_PROXY_CERT_INFO_EXTENSION,
    NULL,
    NULL, //(X509V3_EXT_I2R) i2r_PROXY_CERT_INFO_EXTENSION,
    NULL, //(X509V3_EXT_R2I) r2i_PROXY_CERT_INFO_EXTENSION,
    NULL
  };
  return (&proxycertinfo_v3_x509v3_ext_meth);
}
*/

} //namespace ArcCredential
