#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <cstring>

#include <openssl/err.h>
#include <openssl/asn1_mac.h>
#include <openssl/objects.h>

#include "Proxycertinfo.h"

namespace ArcLib {

/* PROXYPOLICY function */

PROXYPOLICY * PROXYPOLICY_new() {
  ASN1_CTX                            c;
  PROXYPOLICY *                       ret;
  ret = NULL;

  M_ASN1_New_Malloc(ret, PROXYPOLICY);
  ret->policy_language = OBJ_nid2obj(OBJ_sn2nid(IMPERSONATION_PROXY_SN));
  ret->policy = NULL;
  return (ret);
  M_ASN1_New_Error(ASN1_F_PROXYPOLICY_NEW);
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
  ASN1_OBJECT_free(policy->policy_language);
  M_ASN1_OCTET_STRING_free(policy->policy);
  OPENSSL_free(policy);
}

/* duplicate */
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

/* set policy language */
int PROXYPOLICY_set_policy_language(PROXYPOLICY * policy, ASN1_OBJECT * policy_language) {
  if(policy_language != NULL) {
    ASN1_OBJECT_free(policy->policy_language);
    policy->policy_language = OBJ_dup(policy_language);
    return 1;
  }
  return 0;
}

/* get policy language */
ASN1_OBJECT * PROXYPOLICY_get_policy_language(PROXYPOLICY * policy)
{
  return policy->policy_language;
}

/* set policy */
int PROXYPOLICY_set_policy(PROXYPOLICY * proxypolicy, unsigned char * policy, int length) {
  if(policy != NULL) {
    /* perchè questa copia? */
    unsigned char* copy = (unsigned char*) malloc(length);
    memcpy(copy, policy, length);
    /* if member policy of proxypolicy non set */
    if(!proxypolicy->policy)
      proxypolicy->policy = ASN1_OCTET_STRING_new();
    /* set member policy of proxypolicy */
    ASN1_OCTET_STRING_set(proxypolicy->policy, copy, length);
  }
  else if(proxypolicy->policy) 
    ASN1_OCTET_STRING_free(proxypolicy->policy);
  return 1;
}

/* get policy */
unsigned char * PROXYPOLICY_get_policy(PROXYPOLICY * proxypolicy, int * length) {
  /* assure field policy is set */
  if(proxypolicy->policy) {
    *length = proxypolicy->policy->length;
    /* assure ASN1_OCTET_STRING is full */
    if (*length>0 && proxypolicy->policy->data) {
      unsigned char * copy = (unsigned char*) malloc(*length);
      memcpy(copy, proxypolicy->policy->data, *length);
      return copy;
    }
  }
  /* else return NULL */
  return NULL;
}

/* internal to der conversion */
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
}

PROXYPOLICY * d2i_PROXYPOLICY(PROXYPOLICY ** a, unsigned char ** pp, long length) {
  M_ASN1_D2I_vars(a, PROXYPOLICY *, PROXYPOLICY_new);
  M_ASN1_D2I_Init();
  M_ASN1_D2I_start_sequence();
  M_ASN1_D2I_get(ret->policy_language, d2i_ASN1_OBJECT);

  /* need to try getting the policy using
   *     a) a call expecting no tags
   *     b) a call expecting tags
   * one of which should succeed
   */
    
  M_ASN1_D2I_get_opt(ret->policy, d2i_ASN1_OCTET_STRING, V_ASN1_OCTET_STRING);
  M_ASN1_D2I_get_IMP_opt(ret->policy, d2i_ASN1_OCTET_STRING, 0, V_ASN1_OCTET_STRING);
  M_ASN1_D2I_Finish(a, PROXYPOLICY_free, ASN1_F_D2I_PROXYPOLICY);
}

STACK_OF(CONF_VALUE) * i2v_PROXYPOLICY( struct v3_ext_method* /* method */, PROXYPOLICY*  ext, STACK_OF(CONF_VALUE)* extlist) {
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


/** PROXYCERTINFO function */

PROXYCERTINFO * PROXYCERTINFO_new() {
  PROXYCERTINFO *                     ret;
  ASN1_CTX                            c;
  ret = NULL;
  M_ASN1_New_Malloc(ret, PROXYCERTINFO);
  memset(ret, 0, sizeof(PROXYCERTINFO));
  ret->path_length      = NULL;
  ret->proxypolicy      = PROXYPOLICY_new();
  return (ret);
  M_ASN1_New_Error(ASN1_F_PROXYCERTINFO_NEW);
}

void PROXYCERTINFO_free(PROXYCERTINFO * proxycertinfo) {
  if(proxycertinfo == NULL) return;
  ASN1_INTEGER_free(proxycertinfo->path_length);
  PROXYPOLICY_free(proxycertinfo->proxypolicy);
  OPENSSL_free(proxycertinfo);
}

int PROXYCERTINFO_print(BIO* bp, PROXYCERTINFO* cert_info) {
  STACK_OF(CONF_VALUE)* values = NULL;
  values = i2v_PROXYCERTINFO(PROXYCERTINFO_v4_x509v3_ext_meth(), cert_info, NULL);
  X509V3_EXT_val_prn(bp, values, 0, 1);
  sk_CONF_VALUE_pop_free(values, X509V3_conf_free);
  return 1;
}

int PROXYCERTINFO_print_fp(FILE* fp, PROXYCERTINFO* cert_info) {
  int ret;
  BIO* bp;
  bp = BIO_new(BIO_s_file());  
  BIO_set_fp(bp, fp, BIO_NOCLOSE);
  ret =  PROXYCERTINFO_print(bp, cert_info);
  BIO_free(bp);
  return (ret);
}   

/* set path_length */
int PROXYCERTINFO_set_path_length(PROXYCERTINFO * proxycertinfo, long path_length) {
  /* assure proxycertinfo is not empty */
  if(proxycertinfo != NULL) {
    if(path_length != -1) {
      /* if member path_length is empty allocate memory the set */
      if(proxycertinfo->path_length == NULL)
	proxycertinfo->path_length = ASN1_INTEGER_new();
      return ASN1_INTEGER_set(proxycertinfo->path_length, path_length);
    }
    else 
      if(proxycertinfo->path_length != NULL) {
	ASN1_INTEGER_free(proxycertinfo->path_length);
	proxycertinfo->path_length = NULL;
      }
    return 1;
  }
  return 0;
}

int PROXYCERTINFO_set_version(PROXYCERTINFO * proxycertinfo, int version) {
  if (proxycertinfo != NULL) {
    proxycertinfo->version = version;
    return 1;
  }
  return 0;
}

int PROXYCERTINFO_get_version(PROXYCERTINFO * proxycertinfo) {
  if (proxycertinfo)
    return proxycertinfo->version;
  return -1;
}

/* get path length */
long PROXYCERTINFO_get_path_length(PROXYCERTINFO * proxycertinfo) {
  if(proxycertinfo && proxycertinfo->path_length)
    return ASN1_INTEGER_get(proxycertinfo->path_length);
  else return -1;
}

/* set policy */
int PROXYCERTINFO_set_proxypolicy(PROXYCERTINFO * proxycertinfo, PROXYPOLICY * proxypolicy) {
  PROXYPOLICY_free(proxycertinfo->proxypolicy);
  if(proxypolicy != NULL)
    proxycertinfo->proxypolicy = PROXYPOLICY_dup(proxypolicy);
  else
    proxycertinfo->proxypolicy = NULL;
  return 1;
}

/* get policy */
PROXYPOLICY * PROXYCERTINFO_get_proxypolicy(PROXYCERTINFO * proxycertinfo) {
  if(proxycertinfo)
    return proxycertinfo->proxypolicy;
  return NULL;
}

/* internal to der conversion */
int i2d_PROXYCERTINFO_v3(PROXYCERTINFO * proxycertinfo, unsigned char ** pp) {
  int v1;
  M_ASN1_I2D_vars(proxycertinfo);
  v1 = 0;
  M_ASN1_I2D_len(proxycertinfo->proxypolicy, i2d_PROXYPOLICY);
  M_ASN1_I2D_len_EXP_opt(proxycertinfo->path_length,i2d_ASN1_INTEGER, 1, v1);
  M_ASN1_I2D_seq_total();
  M_ASN1_I2D_put(proxycertinfo->proxypolicy, i2d_PROXYPOLICY);
  M_ASN1_I2D_put_EXP_opt(proxycertinfo->path_length, i2d_ASN1_INTEGER, 1, v1);
  M_ASN1_I2D_finish();
}

int i2d_PROXYCERTINFO_v4(PROXYCERTINFO * proxycertinfo, unsigned char ** pp) {
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
}

int i2d_PROXYCERTINFO(PROXYCERTINFO * proxycertinfo, unsigned char ** pp) {
  switch(proxycertinfo->version) {
  case 3:
    return i2d_PROXYCERTINFO_v3(proxycertinfo, pp);
    break;

  case 4:
    return i2d_PROXYCERTINFO_v4(proxycertinfo, pp);
    break;

  default:
    return -1;
    break;
  }
}

PROXYCERTINFO * d2i_PROXYCERTINFO_v3(PROXYCERTINFO ** cert_info, unsigned char ** pp, long length) {
  M_ASN1_D2I_vars(cert_info, PROXYCERTINFO *, PROXYCERTINFO_new);
  M_ASN1_D2I_Init();
  M_ASN1_D2I_start_sequence();
  //M_ASN1_D2I_get(ret->proxypolicy, (unsigned char**)d2i_PROXYPOLICY);
  c.q=c.p;
  if (d2i_PROXYPOLICY(&(ret->proxypolicy),(unsigned char**)&c.p,c.slen) == NULL)
   {c.line=__LINE__; goto err; } 
  c.slen-=(c.p-c.q);

  M_ASN1_D2I_get_EXP_opt(ret->path_length, d2i_ASN1_INTEGER, 1);
  ret->version = 3;
  M_ASN1_D2I_Finish(cert_info, PROXYCERTINFO_free, ASN1_F_D2I_PROXYCERTINFO);
}

PROXYCERTINFO * d2i_PROXYCERTINFO_v4(PROXYCERTINFO ** cert_info, unsigned char ** pp, long length) {
  M_ASN1_D2I_vars(cert_info, PROXYCERTINFO *, PROXYCERTINFO_new);
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
  M_ASN1_D2I_Finish(cert_info, PROXYCERTINFO_free, ASN1_F_D2I_PROXYCERTINFO);
}

PROXYCERTINFO * d2i_PROXYCERTINFO(PROXYCERTINFO ** cert_info, unsigned char ** pp, long length) {
  PROXYCERTINFO *info = d2i_PROXYCERTINFO_v3(cert_info, pp, length);
  if (!info)
    info = d2i_PROXYCERTINFO_v4(cert_info, pp, length);
  return info;
}

STACK_OF(CONF_VALUE) * i2v_PROXYCERTINFO(struct v3_ext_method* /* method */, PROXYCERTINFO* ext, STACK_OF(CONF_VALUE)* extlist) {
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
  if(PROXYCERTINFO_get_path_length(ext) > -1) {
    memset(tmp_string, 0, len);
    BIO_snprintf(tmp_string, len, " %lu (0x%lx)",
        PROXYCERTINFO_get_path_length(ext),
        PROXYCERTINFO_get_path_length(ext));
    X509V3_add_value("Path Length", tmp_string, &extlist);
  }
  if(PROXYCERTINFO_get_proxypolicy(ext)) {
    i2v_PROXYPOLICY(PROXYPOLICY_x509v3_ext_meth(), PROXYCERTINFO_get_proxypolicy(ext), extlist);
  }
  return extlist;
}

/*
//The i2r_PROXYCERTINFO and r2i_PROXYCERTINFO are introduced from openssl-0.9.8e (crypto/x509v3/V3_pci.c)
int i2r_PROXYCERTINFO(X509V3_EXT_METHOD *method, PROXYCERTINFO *ext, BIO *out, int indent) {
  BIO_printf(out, "%*sPath Length Constraint: ", indent, "");
  if (ext->path_length)
    i2a_ASN1_INTEGER(out, ext->path_length);
  else 
    BIO_printf(out, "infinite");
  BIO_puts(out, "\n");
  BIO_printf(out, "%*sPolicy Language: ", indent, "");
  i2a_ASN1_OBJECT(out, ext->proxypolicy->policy_language);
  BIO_puts(out, "\n");
  if (ext->proxypolicy->policy && ext->proxypolicy->policy->data)
    BIO_printf(out, "%*sPolicy Text: %s\n", indent, "", ext->proxypolicy->policy->data);
  return 1;
}

static int process_pci_value(CONF_VALUE *val, ASN1_OBJECT **language, ASN1_INTEGER **pathlen, ASN1_OCTET_STRING **policy) {
  int free_policy = 0;

  if (strcmp(val->name, "language") == 0) {
    if (*language) {
      X509V3err(X509V3_F_PROCESS_PCI_VALUE,X509V3_R_POLICY_LANGUAGE_ALREADTY_DEFINED);
      X509V3_conf_err(val);
      return 0;
    }
    if (!(*language = OBJ_txt2obj(val->value, 0))) {
      X509V3err(X509V3_F_PROCESS_PCI_VALUE,X509V3_R_INVALID_OBJECT_IDENTIFIER);
      X509V3_conf_err(val);
      return 0;
    }
  }
  else if (strcmp(val->name, "pathlen") == 0) {
    if (*pathlen) {
      X509V3err(X509V3_F_PROCESS_PCI_VALUE,X509V3_R_POLICY_PATH_LENGTH_ALREADTY_DEFINED);
      X509V3_conf_err(val);
      return 0;
    }
    if (!X509V3_get_value_int(val, pathlen)) {
      X509V3err(X509V3_F_PROCESS_PCI_VALUE,X509V3_R_POLICY_PATH_LENGTH);
      X509V3_conf_err(val);
      return 0;
    }
  }
  else if (strcmp(val->name, "policy") == 0) {
    unsigned char *tmp_data = NULL;
    long val_len;
    if (!*policy) {
      *policy = ASN1_OCTET_STRING_new();
      if (!*policy) {
        X509V3err(X509V3_F_PROCESS_PCI_VALUE,ERR_R_MALLOC_FAILURE);
	X509V3_conf_err(val);
	return 0;
      }
      free_policy = 1;
    }
    if (strncmp(val->value, "hex:", 4) == 0) {
      unsigned char *tmp_data2 = string_to_hex(val->value + 4, &val_len);
      if (!tmp_data2) goto err;

      tmp_data = (unsigned char*) OPENSSL_realloc((*policy)->data, (*policy)->length + val_len + 1);
      if (tmp_data) {
        (*policy)->data = tmp_data;
	memcpy(&(*policy)->data[(*policy)->length], tmp_data2, val_len);
	(*policy)->length += val_len;
	(*policy)->data[(*policy)->length] = '\0';
      }
    }
    else if (strncmp(val->value, "file:", 5) == 0) {
      unsigned char buf[2048];
      int n;
      BIO *b = BIO_new_file(val->value + 5, "r");
      if (!b) {
        X509V3err(X509V3_F_PROCESS_PCI_VALUE,ERR_R_BIO_LIB);
        X509V3_conf_err(val);
        goto err;
      }
      while((n = BIO_read(b, buf, sizeof(buf))) > 0 || (n == 0 && BIO_should_retry(b))){ 
        if (!n) continue;
        tmp_data = (unsigned char*) OPENSSL_realloc((*policy)->data, (*policy)->length + n + 1);
        if (!tmp_data) break;
        (*policy)->data = tmp_data;
        memcpy(&(*policy)->data[(*policy)->length], buf, n);
        (*policy)->length += n;
        (*policy)->data[(*policy)->length] = '\0';
      }

      if (n < 0) {
        X509V3err(X509V3_F_PROCESS_PCI_VALUE,ERR_R_BIO_LIB);
        X509V3_conf_err(val);
        goto err;
      }
    }
    else if (strncmp(val->value, "text:", 5) == 0) {
      val_len = strlen(val->value + 5);
      tmp_data = (unsigned char*) OPENSSL_realloc((*policy)->data,
      (*policy)->length + val_len + 1);
      if (tmp_data) {
        (*policy)->data = tmp_data;
        memcpy(&(*policy)->data[(*policy)->length], val->value + 5, val_len);
        (*policy)->length += val_len;
        (*policy)->data[(*policy)->length] = '\0';
      }
    }
    else {
      X509V3err(X509V3_F_PROCESS_PCI_VALUE,X509V3_R_INCORRECT_POLICY_SYNTAX_TAG);
      X509V3_conf_err(val);
      goto err;
    }
    if (!tmp_data) {
      X509V3err(X509V3_F_PROCESS_PCI_VALUE,ERR_R_MALLOC_FAILURE);
      X509V3_conf_err(val);
      goto err;
    }
  }
  return 1;
err:
  if (free_policy) {
    ASN1_OCTET_STRING_free(*policy);
    *policy = NULL;
  }
  return 0;
}
*/

/*
PROXYCERTINFO *r2i_PROXYCERTINFO(X509V3_EXT_METHOD *method, X509V3_CTX *ctx, char *value) {
  PROXYCERTINFO *ext = NULL;
  STACK_OF(CONF_VALUE) *vals;
  ASN1_OBJECT *language = NULL;
  ASN1_INTEGER *pathlen = NULL;
  ASN1_OCTET_STRING *policy = NULL;
  int i, j;

  vals = X509V3_parse_list(value);
  for (i = 0; i < sk_CONF_VALUE_num(vals); i++) {
    CONF_VALUE *cnf = sk_CONF_VALUE_value(vals, i);
    if (!cnf->name || (*cnf->name != '@' && !cnf->value)) {
      X509V3err(X509V3_F_R2I_PCI,X509V3_R_INVALID_PROXY_POLICY_SETTING);
      X509V3_conf_err(cnf);
      goto err;
    }
    if (*cnf->name == '@') {
      STACK_OF(CONF_VALUE) *sect;
      int success_p = 1;
      sect = X509V3_get_section(ctx, cnf->name + 1);
      if (!sect) {
        X509V3err(X509V3_F_R2I_PCI,X509V3_R_INVALID_SECTION);
	X509V3_conf_err(cnf);
	goto err;
      }
      for (j = 0; success_p && j < sk_CONF_VALUE_num(sect); j++) {
        success_p = process_pci_value(sk_CONF_VALUE_value(sect, j), &language, &pathlen, &policy);
      }
      X509V3_section_free(ctx, sect);
      if (!success_p) goto err;
    }
    else {
      if (!process_pci_value(cnf, &language, &pathlen, &policy)) {
        X509V3_conf_err(cnf);
        goto err;
      }
    }
  }

  // Language is mandatory 
  if (!language) {
    X509V3err(X509V3_F_R2I_PCI,X509V3_R_NO_PROXY_CERT_POLICY_LANGUAGE_DEFINED);
    goto err;
  }
  i = OBJ_obj2nid(language);
  if ((i == NID_Independent || i == NID_id_ppl_inheritAll) && policy) {
    X509V3err(X509V3_F_R2I_PCI,X509V3_R_POLICY_WHEN_PROXY_LANGUAGE_REQUIRES_NO_POLICY);
    goto err;
  }
  ext = PROXYCERTINFO_new();
  if (!ext) {
    X509V3err(X509V3_F_R2I_PCI,ERR_R_MALLOC_FAILURE);
    goto err;
  }
  ext->proxypolicy = PROXYPOLICY_new();
  if (!ext->proxypolicy) {
    X509V3err(X509V3_F_R2I_PCI,ERR_R_MALLOC_FAILURE);
    goto err;
  }

  ext->proxypolicy->policy_language = language; language = NULL;
  ext->proxypolicy->policy = policy; policy = NULL;
  ext->path_length = pathlen; pathlen = NULL;
  goto end;
err:
  if (language) { ASN1_OBJECT_free(language); language = NULL; }
  if (pathlen) { ASN1_INTEGER_free(pathlen); pathlen = NULL; }
  if (policy) { ASN1_OCTET_STRING_free(policy); policy = NULL; }
  if (ext && ext->proxypolicy) {
    PROXYPOLICY_free(ext->proxypolicy);
    ext->proxypolicy = NULL;
  }
  if (ext) { PROXYCERTINFO_free(ext); ext = NULL; }
end:
  sk_CONF_VALUE_pop_free(vals, X509V3_conf_free);
  return ext;
}
*/

X509V3_EXT_METHOD * PROXYCERTINFO_v4_x509v3_ext_meth() {
    static X509V3_EXT_METHOD proxycertinfo_v4_x509v3_ext_meth =
    {
        -1,
        X509V3_EXT_MULTILINE,
        NULL,
        (X509V3_EXT_NEW) PROXYCERTINFO_new,
        (X509V3_EXT_FREE) PROXYCERTINFO_free,
        (X509V3_EXT_D2I) d2i_PROXYCERTINFO_v4,
        (X509V3_EXT_I2D) i2d_PROXYCERTINFO_v4,
        NULL, NULL,
        (X509V3_EXT_I2V) i2v_PROXYCERTINFO,
        NULL,
        NULL, //(X509V3_EXT_I2R) i2r_PROXYCERTINFO,
        NULL, //(X509V3_EXT_R2I) r2i_PROXYCERTINFO,
        NULL
    };
    return (&proxycertinfo_v4_x509v3_ext_meth);
}

X509V3_EXT_METHOD * PROXYCERTINFO_v3_x509v3_ext_meth() {
  static X509V3_EXT_METHOD proxycertinfo_v3_x509v3_ext_meth =
  {
    -1,
    X509V3_EXT_MULTILINE,
    NULL,
    (X509V3_EXT_NEW) PROXYCERTINFO_new,
    (X509V3_EXT_FREE) PROXYCERTINFO_free,
    (X509V3_EXT_D2I) d2i_PROXYCERTINFO_v3,
    (X509V3_EXT_I2D) i2d_PROXYCERTINFO_v3,
    NULL, NULL,
    (X509V3_EXT_I2V) i2v_PROXYCERTINFO,
    NULL,
    NULL, //(X509V3_EXT_I2R) i2r_PROXYCERTINFO,
    NULL, //(X509V3_EXT_R2I) r2i_PROXYCERTINFO,
    NULL
  };
  return (&proxycertinfo_v3_x509v3_ext_meth);
}

} //namespace ArcLib
