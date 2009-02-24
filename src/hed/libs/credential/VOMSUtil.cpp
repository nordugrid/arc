#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <fstream>
#include <glibmm/fileutils.h>
#include <unistd.h>

#ifndef HAVE_GETDOMAINNAME
#include <sys/systeminfo.h>
#endif

#include <arc/ArcRegex.h>

#include <arc/credential/VOMSAttribute.h>
#include <arc/credential/VOMSUtil.h>
#include "listfunc.h"

#ifdef WIN32
#include <arc/win32.h>
int gethostname_mingw (char *, size_t);
int gethostname_mingw (char *name, size_t len) {
  DWORD dlen = (len <= (DWORD)~0 ? len : (DWORD)~0);
  return (GetComputerNameEx(ComputerNameDnsHostname, name, &dlen) ? 0 : -1);
}
#define gethostname gethostname_mingw

int getdomainname_mingw (char *, size_t);
int getdomainname_mingw (char *name, size_t len) {
  DWORD dlen = (len <= (DWORD)~0 ? len : (DWORD)~0);
  return (GetComputerNameEx(ComputerNameDnsDomain, name, &dlen) ? 0 : -1);
}
#define getdomainname getdomainname_mingw
#endif


using namespace ArcCredential;

namespace Arc {

  VOMSTrustList::VOMSTrustList(const std::vector<std::string>& encoded_list) {
    VOMSTrustChain chain;
    for(std::vector<std::string>::const_iterator i = encoded_list.begin();
                                i != encoded_list.end(); ++i) {
      if((*i).find("NEXT CHAIN") != std::string::npos) {
        if(chain.size() > 0) {
          if(chain.size() > 1) { // More than one item in chain means DN list
            AddChain(chain);
          } else {
            // Trying to find special symbols
            if((chain[0].find('^') != std::string::npos) ||
               (chain[0].find('$') != std::string::npos) ||
               (chain[0].find('*') != std::string::npos)) {
              AddRegex(chain[0]);
            } else {
              AddChain(chain);
            };
          }
          chain.clear();
        }
        continue;
      }
      chain.push_back(*i);
    }
    if(chain.size() > 0) {
      if(chain.size() > 1) { // More than one item in chain means DN list
        AddChain(chain);
      } else {
        // Trying to find special symbols
        if((chain[0].find('^') != std::string::npos) ||
           (chain[0].find('$') != std::string::npos) ||
           (chain[0].find('*') != std::string::npos)) {
          AddRegex(chain[0]);
        } else {
          AddChain(chain);
        };
      }
      chain.clear();
    }
  }

  VOMSTrustList::VOMSTrustList(const std::vector<VOMSTrustChain>& chains,
    const std::vector<VOMSTrustRegex>& regexs):chains_(chains) {
    for(std::vector<VOMSTrustRegex>::const_iterator r = regexs.begin();
                    r != regexs.end();++r) {
      AddRegex(*r);
    }
  }

  VOMSTrustList::~VOMSTrustList(void) {
    for(std::vector<RegularExpression*>::iterator r = regexs_.begin();
                    r != regexs_.end();++r) {
      if(*r) delete *r; *r=NULL;
    }
  }

  VOMSTrustChain& VOMSTrustList::AddChain(void) {
    VOMSTrustChain chain;
    return *chains_.insert(chains_.end(),chain);
  }

  VOMSTrustChain& VOMSTrustList::AddChain(const VOMSTrustChain& chain) {
    return *chains_.insert(chains_.end(),chain);
  }

  RegularExpression& VOMSTrustList::AddRegex(const VOMSTrustRegex& reg) {
    RegularExpression* r = new RegularExpression(reg);
    regexs_.insert(regexs_.end(),r);
    return *r;
  }


  void InitVOMSAttribute(void) {
    #define idpkix                "1.3.6.1.5.5.7"
    #define idpkcs9               "1.2.840.113549.1.9"
    #define idpe                  idpkix ".1"
    #define idce                  "2.5.29"
    #define idaca                 idpkix ".10"
    #define idat                  "2.5.4"
    #define idpeacauditIdentity   idpe ".4"
    #define idcetargetInformation idce ".55"
    #define idceauthKeyIdentifier idce ".35"
    #define idceauthInfoAccess    idpe ".1"
    #define idcecRLDistPoints     idce ".31"
    #define idcenoRevAvail        idce ".56"
    #define idceTargets           idce ".55"
    #define idacaauthentInfo      idaca ".1"
    #define idacaaccessIdentity   idaca ".2"
    #define idacachargIdentity    idaca ".3"
    #define idacagroup            idaca ".4"
    #define idatclearance         "2.5.1.5.5"
    #define voms                  "1.3.6.1.4.1.8005.100.100.1"
    #define incfile               "1.3.6.1.4.1.8005.100.100.2"
    #define vo                    "1.3.6.1.4.1.8005.100.100.3"
    #define idatcap               "1.3.6.1.4.1.8005.100.100.4"

    #define attribs            "1.3.6.1.4.1.8005.100.100.11"
    #define acseq                 "1.3.6.1.4.1.8005.100.100.5"
    #define order                 "1.3.6.1.4.1.8005.100.100.6"
    #define certseq               "1.3.6.1.4.1.8005.100.100.10"
    #define email                 idpkcs9 ".1"

    #define OBJC(c,n) OBJ_create(c,n,#c)

    X509V3_EXT_METHOD *vomsattribute_x509v3_ext_meth;
    static int done=0;
    if (done)
      return;

    done=1;

    /* VOMS Attribute related objects*/
    //OBJ_create(email, "Email", "Email");
    OBJC(idatcap,"idatcap");

    OBJC(attribs,"attributes");
    OBJC(idcenoRevAvail, "idcenoRevAvail");
    OBJC(idceauthKeyIdentifier, "authKeyId");
    OBJC(idceTargets, "idceTargets");
    OBJC(acseq, "acseq");
    OBJC(order, "order");
    OBJC(voms, "voms");
    OBJC(incfile, "incfile");
    OBJC(vo, "vo");
    OBJC(certseq, "certseq");

    vomsattribute_x509v3_ext_meth = VOMSAttribute_auth_x509v3_ext_meth();
    if (vomsattribute_x509v3_ext_meth) {
      vomsattribute_x509v3_ext_meth->ext_nid = OBJ_txt2nid("authKeyId");
      X509V3_EXT_add(vomsattribute_x509v3_ext_meth);
    }

    vomsattribute_x509v3_ext_meth = VOMSAttribute_avail_x509v3_ext_meth();
    if (vomsattribute_x509v3_ext_meth) {
      vomsattribute_x509v3_ext_meth->ext_nid = OBJ_txt2nid("idcenoRevAvail");
      X509V3_EXT_add(vomsattribute_x509v3_ext_meth);
    }

    vomsattribute_x509v3_ext_meth = VOMSAttribute_targets_x509v3_ext_meth();
    if (vomsattribute_x509v3_ext_meth) {
      vomsattribute_x509v3_ext_meth->ext_nid = OBJ_txt2nid("idceTargets");
      X509V3_EXT_add(vomsattribute_x509v3_ext_meth);
    }

    vomsattribute_x509v3_ext_meth = VOMSAttribute_acseq_x509v3_ext_meth();
    if (vomsattribute_x509v3_ext_meth) {
      vomsattribute_x509v3_ext_meth->ext_nid = OBJ_txt2nid("acseq");
      X509V3_EXT_add(vomsattribute_x509v3_ext_meth);
    }

    vomsattribute_x509v3_ext_meth = VOMSAttribute_certseq_x509v3_ext_meth();
    if (vomsattribute_x509v3_ext_meth) {
      vomsattribute_x509v3_ext_meth->ext_nid = OBJ_txt2nid("certseq");
      X509V3_EXT_add(vomsattribute_x509v3_ext_meth);
    }

    vomsattribute_x509v3_ext_meth = VOMSAttribute_attribs_x509v3_ext_meth();
    if (vomsattribute_x509v3_ext_meth) {
      vomsattribute_x509v3_ext_meth->ext_nid = OBJ_txt2nid("attributes");
      X509V3_EXT_add(vomsattribute_x509v3_ext_meth);
    }
  }

  int createVOMSAC(X509 *issuer, STACK_OF(X509) *issuerstack, X509 *holder, EVP_PKEY *pkey, BIGNUM *serialnum,
             std::vector<std::string> &fqan, std::vector<std::string> &targets, std::vector<std::string>& attrs, 
             AC **ac, std::string voname, std::string uri, int lifetime) {
    #define ERROR(e) do { err = (e); goto err; } while (0)
    AC *a = NULL;
    X509_NAME *subname = NULL, *issname = NULL;
    GENERAL_NAME *dirn1 = NULL, *dirn2 = NULL;
    ASN1_INTEGER  *serial = NULL, *holdserial = NULL, *version = NULL;
    ASN1_BIT_STRING *uid = NULL;
    AC_ATTR *capabilities = NULL;
    AC_IETFATTR *capnames = NULL;
    AC_FULL_ATTRIBUTES *ac_full_attrs = NULL;
    ASN1_OBJECT *cobj = NULL, *aobj = NULL;
    X509_ALGOR *alg1, *alg2;
    ASN1_GENERALIZEDTIME *time1 = NULL, *time2 = NULL;
    X509_EXTENSION *norevavail = NULL, *targetsext = NULL, *auth = NULL, *certstack = NULL;
    AC_ATT_HOLDER *ac_att_holder = NULL;
    //char *qual = NULL, *name = NULL, *value = NULL, *tmp1 = NULL, *tmp2 = NULL;
    STACK_OF(X509) *stk = NULL;
    int err = AC_ERR_UNKNOWN;
    time_t curtime;

    InitVOMSAttribute();

    if (!issuer || !holder || !serialnum || fqan.empty() || !ac || !pkey)
      return AC_ERR_PARAMETERS;

    a = *ac;
    subname = X509_NAME_dup(X509_get_issuer_name(holder)); //old or new version?
    issname = X509_NAME_dup(X509_get_subject_name(issuer));

    time(&curtime);
    time1 = ASN1_GENERALIZEDTIME_set(NULL, curtime);
    time2 = ASN1_GENERALIZEDTIME_set(NULL, curtime+lifetime);

    dirn1  = GENERAL_NAME_new();
    dirn2  = GENERAL_NAME_new();
    holdserial      = M_ASN1_INTEGER_dup(holder->cert_info->serialNumber);
    serial          = BN_to_ASN1_INTEGER(serialnum, NULL);
    version         = BN_to_ASN1_INTEGER((BIGNUM *)(BN_value_one()), NULL);
    capabilities    = AC_ATTR_new();
    cobj            = OBJ_txt2obj("idatcap",0);
    aobj            = OBJ_txt2obj("attributes",0);
    capnames        = AC_IETFATTR_new();
    ac_full_attrs   = AC_FULL_ATTRIBUTES_new();
    ac_att_holder   = AC_ATT_HOLDER_new();

    std::string buffer, complete;

    GENERAL_NAME *g = GENERAL_NAME_new();
    ASN1_IA5STRING *tmpr = ASN1_IA5STRING_new();


    if (!subname || !issuer || !dirn1 || !dirn2 || !holdserial || !serial ||
      !capabilities || !cobj || !capnames || !time1 || !time2 || !ac_full_attrs || !ac_att_holder)
      ERROR(AC_ERR_MEMORY);

    for (std::vector<std::string>::iterator i = targets.begin(); i != targets.end(); i++) {
      if (i == targets.begin()) complete = (*i);
      else complete.append(",").append(*i);
    }

    // prepare AC_IETFATTR
    for (std::vector<std::string>::iterator i = fqan.begin(); i != fqan.end(); i++) {
      ASN1_OCTET_STRING *tmpc = ASN1_OCTET_STRING_new();
      if (!tmpc) {
        ASN1_OCTET_STRING_free(tmpc);
        ERROR(AC_ERR_MEMORY);
      }

      CredentialLogger.msg(VERBOSE,"VOMS: create FQAN: %s",*i);

#ifdef HAVE_OPENSSL_OLDRSA
      ASN1_OCTET_STRING_set(tmpc, (unsigned char*)((*i).c_str()), (*i).length());
#else
      ASN1_OCTET_STRING_set(tmpc, (const unsigned char*)((*i).c_str()), (*i).length());
#endif

      sk_AC_IETFATTRVAL_push(capnames->values, (AC_IETFATTRVAL *)tmpc);
    }
 
    //GENERAL_NAME *g = GENERAL_NAME_new();
    //ASN1_IA5STRING *tmpr = ASN1_IA5STRING_new();
    buffer.append(voname);
    buffer.append("://");
    buffer.append(uri);
    if (!tmpr || !g) {
      GENERAL_NAME_free(g);
      ASN1_IA5STRING_free(tmpr);
      ERROR(AC_ERR_MEMORY);
    }

    ASN1_STRING_set(tmpr, buffer.c_str(), buffer.size());
    g->type  = GEN_URI;
    g->d.ia5 = tmpr;
    sk_GENERAL_NAME_push(capnames->names, g);
 
    // stuff the created AC_IETFATTR in ietfattr (values) and define its object
    sk_AC_IETFATTR_push(capabilities->ietfattr, capnames);
    capabilities->get_type = GET_TYPE_FQAN;
    ASN1_OBJECT_free(capabilities->type);
    capabilities->type = cobj;

    // prepare AC_FULL_ATTRIBUTES
    for (std::vector<std::string>::iterator i = attrs.begin(); i != attrs.end(); i++) {
      std::string qual, name, value;

      CredentialLogger.msg(VERBOSE,"VOMS: create attribute: %s",*i); 
 
      AC_ATTRIBUTE *ac_attr = AC_ATTRIBUTE_new();
      if (!ac_attr) {
        AC_ATTRIBUTE_free(ac_attr);
        ERROR(AC_ERR_MEMORY);
      }

      //Accoding to the definition of voms, the attributes will be like "qualifier::name=value" or "::name=value"
      size_t pos =(*i).find_first_of("::");
      if (pos != std::string::npos) {
        qual = (*i).substr(0, pos);
        pos += 2;
      }
      else { pos = 2; } 

      size_t pos1 = (*i).find_first_of("=");
      if (pos1 == std::string::npos) {
        ERROR(AC_ERR_PARAMETERS);
      }
      else {
        name = (*i).substr(pos, pos1 - pos);
        value = (*i).substr(pos1 + 1);
      }

#ifdef HAVE_OPENSSL_OLDRSA
      if (!qual.empty())
        ASN1_OCTET_STRING_set(ac_attr->qualifier, (unsigned char*)(qual.c_str()), qual.length());
      else
        ASN1_OCTET_STRING_set(ac_attr->qualifier, (unsigned char*)(voname.c_str()), voname.length());

      ASN1_OCTET_STRING_set(ac_attr->name, (unsigned char*)(name.c_str()), name.length());
      ASN1_OCTET_STRING_set(ac_attr->value, (unsigned char*)(value.c_str()), value.length());
#else
      if (!qual.empty())
        ASN1_OCTET_STRING_set(ac_attr->qualifier, (const unsigned char*)(qual.c_str()), qual.length());
      else
        ASN1_OCTET_STRING_set(ac_attr->qualifier, (const unsigned char*)(voname.c_str()), voname.length());

      ASN1_OCTET_STRING_set(ac_attr->name, (const unsigned char*)(name.c_str()), name.length());
      ASN1_OCTET_STRING_set(ac_attr->value, (const unsigned char*)(value.c_str()), value.length());
#endif

      sk_AC_ATTRIBUTE_push(ac_att_holder->attributes, ac_attr);
    }

    if (attrs.empty()) 
      AC_ATT_HOLDER_free(ac_att_holder);
    else {
      GENERAL_NAME *g = GENERAL_NAME_new();
      ASN1_IA5STRING *tmpr = ASN1_IA5STRING_new();
      if (!tmpr || !g) {
        GENERAL_NAME_free(g);
        ASN1_IA5STRING_free(tmpr);
        ERROR(AC_ERR_MEMORY);
      }
    
      std::string buffer(voname);
      buffer.append("://");
      buffer.append(uri);

      ASN1_STRING_set(tmpr, buffer.c_str(), buffer.length());
      g->type  = GEN_URI;
      g->d.ia5 = tmpr;
      sk_GENERAL_NAME_push(ac_att_holder->grantor, g);

      sk_AC_ATT_HOLDER_push(ac_full_attrs->providers, ac_att_holder);
    }  
  
    // push both AC_ATTR into STACK_OF(AC_ATTR)
    sk_AC_ATTR_push(a->acinfo->attrib, capabilities);

    if (ac_full_attrs) {
      X509_EXTENSION *ext = NULL;

      ext = X509V3_EXT_conf_nid(NULL, NULL, OBJ_txt2nid("attributes"), (char *)(ac_full_attrs->providers));
      AC_FULL_ATTRIBUTES_free(ac_full_attrs);
      if (!ext)
        ERROR(AC_ERR_NO_EXTENSION);

      sk_X509_EXTENSION_push(a->acinfo->exts, ext);
      ac_full_attrs = NULL;
    }

    stk = sk_X509_new_null();
    
    if (issuerstack) {
      for (int j =0; j < sk_X509_num(issuerstack); j++)
        sk_X509_push(stk, X509_dup(sk_X509_value(issuerstack, j)));
    }

   //for (int i=0; i < sk_X509_num(stk); i++) 
   //  fprintf(stderr, "stk[%i] = %s\n", i , 
   //  X509_NAME_oneline(X509_get_subject_name((X509 *)sk_X509_value(stk, i)), NULL, 0)); 

#ifdef HAVE_OPENSSL_OLDRSA
    sk_X509_push(stk, (X509 *)ASN1_dup((int (*)())i2d_X509,
          (char*(*)())d2i_X509, (char *)issuer));
#else
    sk_X509_push(stk, (X509 *)ASN1_dup((int (*)(void*, unsigned char**))i2d_X509,
          (void*(*)(void**, const unsigned char**, long int))d2i_X509, (char *)issuer));
#endif

   //for(int i=0; i<sk_X509_num(stk); i++)
   //  fprintf(stderr, "stk[%i] = %d  %s\n", i , sk_X509_value(stk, i),  
   //  X509_NAME_oneline(X509_get_subject_name((X509 *)sk_X509_value(stk, i)), NULL, 0));

    certstack = X509V3_EXT_conf_nid(NULL, NULL, OBJ_txt2nid("certseq"), (char*)stk);
    sk_X509_pop_free(stk, X509_free);

    /* Create extensions */
    norevavail = X509V3_EXT_conf_nid(NULL, NULL, OBJ_txt2nid("idcenoRevAvail"), (char*)"loc");
    if (!norevavail)
      ERROR(AC_ERR_NO_EXTENSION);
    X509_EXTENSION_set_critical(norevavail, 0); 

    auth = X509V3_EXT_conf_nid(NULL, NULL, OBJ_txt2nid("authKeyId"), (char *)issuer);
    if (!auth)
      ERROR(AC_ERR_NO_EXTENSION);
    X509_EXTENSION_set_critical(auth, 0); 

    if (!complete.empty()) {
      targetsext = X509V3_EXT_conf_nid(NULL, NULL, OBJ_txt2nid("idceTargets"), (char*)(complete.c_str()));
      if (!targetsext)
        ERROR(AC_ERR_NO_EXTENSION);

      X509_EXTENSION_set_critical(targetsext,1);
      sk_X509_EXTENSION_push(a->acinfo->exts, targetsext);
    }

    sk_X509_EXTENSION_push(a->acinfo->exts, norevavail);
    sk_X509_EXTENSION_push(a->acinfo->exts, auth);
    if (certstack)
      sk_X509_EXTENSION_push(a->acinfo->exts, certstack);

    alg1 = X509_ALGOR_dup(issuer->cert_info->signature);
    alg2 = X509_ALGOR_dup(issuer->sig_alg);

    if (issuer->cert_info->issuerUID)
      if (!(uid = M_ASN1_BIT_STRING_dup(issuer->cert_info->issuerUID)))
        ERROR(AC_ERR_MEMORY);

    ASN1_INTEGER_free(a->acinfo->holder->baseid->serial);
    ASN1_INTEGER_free(a->acinfo->serial);
    ASN1_INTEGER_free(a->acinfo->version);
    ASN1_GENERALIZEDTIME_free(a->acinfo->validity->notBefore);
    ASN1_GENERALIZEDTIME_free(a->acinfo->validity->notAfter);
    dirn1->d.dirn = subname;
    dirn1->type = GEN_DIRNAME;
    sk_GENERAL_NAME_push(a->acinfo->holder->baseid->issuer, dirn1);
    dirn2->d.dirn = issname;
    dirn2->type = GEN_DIRNAME;
    sk_GENERAL_NAME_push(a->acinfo->form->names, dirn2);
    a->acinfo->holder->baseid->serial = holdserial;
    a->acinfo->serial = serial;
    a->acinfo->version = version;
    a->acinfo->validity->notBefore = time1;
    a->acinfo->validity->notAfter  = time2;
    a->acinfo->id = uid;
    X509_ALGOR_free(a->acinfo->alg);
    a->acinfo->alg = alg1;
    X509_ALGOR_free(a->sig_alg);
    a->sig_alg = alg2;

#ifdef HAVE_OPENSSL_OLDRSA
    ASN1_sign((int (*)())i2d_AC_INFO, a->acinfo->alg, a->sig_alg, a->signature,
	    (char *)a->acinfo, pkey, EVP_md5());
#else
    ASN1_sign((int (*)(void*, unsigned char**))i2d_AC_INFO, a->acinfo->alg, a->sig_alg, a->signature,
            (char *)a->acinfo, pkey, EVP_md5());
#endif

    *ac = a;
    return 0;

err:
    X509_EXTENSION_free(auth);
    X509_EXTENSION_free(norevavail);
    X509_EXTENSION_free(targetsext);
    X509_EXTENSION_free(certstack);
    X509_NAME_free(subname);
    X509_NAME_free(issname);
    GENERAL_NAME_free(dirn1);
    GENERAL_NAME_free(dirn2);
    ASN1_INTEGER_free(holdserial);
    ASN1_INTEGER_free(serial);
    AC_ATTR_free(capabilities);
    ASN1_OBJECT_free(cobj);
    AC_IETFATTR_free(capnames);
    ASN1_UTCTIME_free(time1);
    ASN1_UTCTIME_free(time2);
    AC_ATT_HOLDER_free(ac_att_holder);
    AC_FULL_ATTRIBUTES_free(ac_full_attrs);
    return err;
  }

  bool createVOMSAC(std::string &codedac, Credential &issuer_cred, Credential &holder_cred, 
             std::vector<std::string> &fqan, std::vector<std::string> &targets, 
             std::vector<std::string>& attributes, std::string &voname, std::string &uri, int lifetime) {

    EVP_PKEY* issuerkey = NULL;
    X509* holder = NULL;
    X509* issuer = NULL;
    STACK_OF(X509)* issuerchain = NULL;

    issuer = issuer_cred.GetCert();
    issuerchain = issuer_cred.GetCertChain();
    issuerkey = issuer_cred.GetPrivKey();
    holder = holder_cred.GetCert();

    AC* ac = NULL;
    ac = AC_new();

    if(createVOMSAC(issuer, issuerchain, holder, issuerkey, (BIGNUM *)(BN_value_one()),
             fqan, targets, attributes, &ac, voname, uri, lifetime)){
      AC_free(ac); return false;
    }

    unsigned int len = i2d_AC(ac, NULL);
    unsigned char *tmp = (unsigned char *)OPENSSL_malloc(len);
    unsigned char *ttmp = tmp;

    if (tmp) {
      i2d_AC(ac, &tmp);
      //codedac = std::string((char *)ttmp, len);
      codedac.append((const char*)ttmp, len);
    }
    free(ttmp);

    AC_free(ac);
  
    return true;
  }


  bool addVOMSAC(AC** &aclist, std::string &codedac) {
    AC* received_ac;
    AC** actmplist = NULL;
    char *p, *pp;

    InitVOMSAttribute();

    int l = codedac.size();

    pp = (char *)malloc(codedac.size());
    if(!pp) {
      CredentialLogger.msg(ERROR,"VOMS: Can not allocate memory for parsing AC");
      return false; 
    }

    pp = (char *)memcpy(pp, codedac.data(), codedac.size());
    p = pp;

    //Parse the AC, and insert it into an AC list
    if((received_ac = d2i_AC(NULL, (unsigned char **)&p, l))) {
      actmplist = (AC **)listadd((char **)aclist, (char *)received_ac, sizeof(AC *));
      if (actmplist) {
        aclist = actmplist; free(pp); return true;
      }
      else {
        listfree((char **)aclist, (freefn)AC_free);  free(pp); return false;
      }
    }
    else {
      CredentialLogger.msg(ERROR,"VOMS: Can not parse AC");
      free(pp); return false;
    }
  }

  static int cb(int ok, X509_STORE_CTX *ctx) {
    if (!ok) {
      if (ctx->error == X509_V_ERR_CERT_HAS_EXPIRED) ok=1;
      /* since we are just checking the certificates, it is
       * ok if they are self signed. But we should still warn
       * the user.
       */
      if (ctx->error == X509_V_ERR_DEPTH_ZERO_SELF_SIGNED_CERT) ok=1;
      /* Continue after extension errors too */
      if (ctx->error == X509_V_ERR_INVALID_CA) ok=1;
      if (ctx->error == X509_V_ERR_PATH_LENGTH_EXCEEDED) ok=1;
      if (ctx->error == X509_V_ERR_CERT_CHAIN_TOO_LONG) ok=1;
      if (ctx->error == X509_V_ERR_DEPTH_ZERO_SELF_SIGNED_CERT) ok=1;
    }
    return(ok);
  }

  static bool check_cert(X509 *cert, const std::string& ca_cert_dir, const std::string& ca_cert_file) {
    X509_STORE *ctx = NULL;
    X509_STORE_CTX *csc = NULL;
    X509_LOOKUP *lookup = NULL;
    int i = 0;

    if(ca_cert_dir.empty() && ca_cert_file.empty()) {
      CredentialLogger.msg(ERROR,"VOMS: CA directory or CA file must be provided");
      return false;
    }

    csc = X509_STORE_CTX_new();
    ctx = X509_STORE_new();
    if (ctx && csc) {
      X509_STORE_set_verify_cb_func(ctx,cb);
//#ifdef SIGPIPE
//      signal(SIGPIPE,SIG_IGN);
//#endif
      CRYPTO_malloc_init();
      if (!(ca_cert_dir.empty()) && (lookup = X509_STORE_add_lookup(ctx,X509_LOOKUP_hash_dir()))) {
        X509_LOOKUP_add_dir(lookup, ca_cert_dir.c_str(), X509_FILETYPE_PEM);
        ERR_clear_error();
        X509_STORE_CTX_init(csc,ctx,cert,NULL);
        i = X509_verify_cert(csc);
      }
      else if (!(ca_cert_file.empty()) && (lookup = X509_STORE_add_lookup(ctx, X509_LOOKUP_file()))) {
        X509_LOOKUP_load_file(lookup, NULL, X509_FILETYPE_PEM);
        ERR_clear_error();
        X509_STORE_CTX_init(csc,ctx,cert,NULL);
        i = X509_verify_cert(csc);
      }
    }
    if (ctx) X509_STORE_free(ctx);
    if (csc) X509_STORE_CTX_free(csc);

    return (i != 0);
  }

  static bool check_cert(STACK_OF(X509) *stack, const std::string& ca_cert_dir, const std::string& ca_cert_file) {
    X509_STORE *ctx = NULL;
    X509_STORE_CTX *csc = NULL;
    X509_LOOKUP *lookup = NULL;
    int index = 0;

    if(ca_cert_dir.empty() && ca_cert_file.empty()) {
      CredentialLogger.msg(ERROR,"VOMS: CA direcory or CA file must be provided");
      return false;
    }

    csc = X509_STORE_CTX_new();
    ctx = X509_STORE_new();
    if (ctx && csc) {
      X509_STORE_set_verify_cb_func(ctx,cb);
//#ifdef SIGPIPE
//      signal(SIGPIPE,SIG_IGN);
//#endif
      CRYPTO_malloc_init();

      if (!(ca_cert_dir.empty()) && (lookup = X509_STORE_add_lookup(ctx,X509_LOOKUP_hash_dir()))) {
        X509_LOOKUP_add_dir(lookup, ca_cert_dir.c_str(), X509_FILETYPE_PEM);
        //Check the AC issuer certificate's chain
        for (int i = sk_X509_num(stack)-1; i >=0; i--) {
          //Firstly, try to verify the certificate which is issues by CA;
          //Then try to verify the next one; the last one is the certificate
          //(voms server certificate) which issues AC.
          //Normally the voms server certificate is directly issued by a CA,
          //in this case, sk_X509_num(stack) should be 1.
          //On the other hand, if the voms server certificate is issued by a CA
          //which is issued by an parent CA, and so on, then the AC issuer should 
          //put those CA certificates (except the root CA certificate which has 
          //been configured to be trusted on the AC consumer side) together with 
          //the voms server certificate itself in the 'certseq' part of AC.
          //
          //The CA certificates are checked one by one: the certificate which
          //is signed by root CA is checked firstly; the voms server certificate
          //is checked lastly.
          //
          X509_STORE_CTX_init(csc, ctx, sk_X509_value(stack, i), NULL);
          index = X509_verify_cert(csc);
          if(!index) break;
          //If the 'i'th certificate is verified, then add it as trusted certificate,
          //then 'i'th certificate will be used as 'trusted certificate' to check
          //the 'i-1'th certificate
          X509_STORE_add_cert(ctx,sk_X509_value(stack, i));
        }
/*
        for (int i = 1; i < sk_X509_num(stack); i++)
          X509_STORE_add_cert(ctx,sk_X509_value(stack, i)); 
        ERR_clear_error();
        X509_STORE_CTX_init(csc, ctx, sk_X509_value(stack, 0), NULL);
        index = X509_verify_cert(csc);
*/
      }
      else if (!(ca_cert_file.empty()) && (lookup = X509_STORE_add_lookup(ctx, X509_LOOKUP_file()))) {
        X509_LOOKUP_load_file(lookup, NULL, X509_FILETYPE_PEM);
        for (int i = sk_X509_num(stack)-1; i >=0; i--) {
          X509_STORE_CTX_init(csc, ctx, sk_X509_value(stack, i), NULL);
          index = X509_verify_cert(csc);
          if(!index) break;
          X509_STORE_add_cert(ctx,sk_X509_value(stack, i));
        }
/*
        for (int i = 1; i < sk_X509_num(stack); i++)
          X509_STORE_add_cert(ctx,sk_X509_value(stack, i));
        ERR_clear_error();
        X509_STORE_CTX_init(csc, ctx, sk_X509_value(stack, 0), NULL);
        index = X509_verify_cert(csc);
*/
      }
    }
    if (ctx) X509_STORE_free(ctx);
    if (csc) X509_STORE_CTX_free(csc);

    return (index != 0);
  }
  
  static bool check_sig_ac(X509* cert, AC* ac){
    if (!cert || !ac) return false;

    EVP_PKEY *key = X509_extract_key(cert);
    if (!key) return false;

    int res;
#ifdef HAVE_OPENSSL_OLDRSA
    res = ASN1_verify((int (*)())i2d_AC_INFO, ac->sig_alg, ac->signature,
                        (char *)ac->acinfo, key);
#else
    res = ASN1_verify((int (*)(void*, unsigned char**))i2d_AC_INFO, ac->sig_alg, ac->signature,
                        (char *)ac->acinfo, key);
#endif

    if (!res) CredentialLogger.msg(ERROR,"VOMS: unable to verify AC signature");
  
    EVP_PKEY_free(key);
    return (res == 1);
  }

  static bool regex_match(std::string& label, std::string& value) {
    bool match=false;
    Arc::RegularExpression regex(label);
    if(regex.isOk()){
      std::list<std::string> unmatched, matched;
      if(regex.match(value, unmatched, matched))
        match=true;
    }
    return match;
  }

  static bool check_trust(const VOMSTrustChain& chain,STACK_OF(X509)* certstack) {
    int n = 0;
    X509 *current = NULL;
    //A trusted chain is like following:
    // /O=Grid/O=NorduGrid/CN=host/arthur.hep.lu.se
    // /O=Grid/O=NorduGrid/CN=NorduGrid Certification Authority
    // ----NEXT CHAIN----
    if(chain.size()-1 > (sk_X509_num(certstack)+1)) return false;
#if 0
    for(;n < sk_X509_num(certstack);++n) {
      if(n >= chain.size()) return true;
      current = sk_X509_value(certstack,n);
      if(!current) return false;
      if(chain[n] != X509_NAME_oneline(X509_get_subject_name(current),NULL,0)) {
        return false;
      }
    }
    if(n < chain.size()) {
      if(!current) return false;
      if(chain[n] != X509_NAME_oneline(X509_get_issuer_name(current),NULL,0)) {
        return false;
      }
    }
#endif

    for(;n < sk_X509_num(certstack);++n) {
      if((n+1) >= chain.size()) return true;
      current = sk_X509_value(certstack,n);
      if(!current) return false;
      if(chain[n] != X509_NAME_oneline(X509_get_subject_name(current),NULL,0)) {
        CredentialLogger.msg(ERROR,"VOMS: the DN does in certificate: %s does not match that in trusted DN list: %s",
          X509_NAME_oneline(X509_get_subject_name(current),NULL,0), chain[n]);
        return false;
      }
      if(chain[n+1] != X509_NAME_oneline(X509_get_issuer_name(current),NULL,0)) {
        CredentialLogger.msg(ERROR,"VOMS: the Issuer identity does in certificate: %s does not match that in trusted DN list: %s",
          X509_NAME_oneline(X509_get_issuer_name(current),NULL,0), chain[n+1]);
        return false;
      }
    }

    return true;
  }

  static bool check_trust(const RegularExpression& reg,STACK_OF(X509)* certstack) {
    if(sk_X509_num(certstack) <= 0) return false;
    X509 *current = sk_X509_value(certstack,0);
#if 0
    std::string subject(X509_NAME_oneline(X509_get_subject_name(current),NULL,0));
    std::list<std::string> unmatched, matched;
    return reg.match(subject,unmatched,matched);
#endif
    std::string subject(X509_NAME_oneline(X509_get_subject_name(current),NULL,0));
    std::string issuer(X509_NAME_oneline(X509_get_issuer_name(current),NULL,0));
    std::list<std::string> unmatched, matched;
    return (reg.match(subject,unmatched,matched) && reg.match(issuer,unmatched,matched));

  }

  static bool check_signature(AC* ac, std::string& voname, 
    std::string& hostname, 
    const std::string& ca_cert_dir, const std::string& ca_cert_file, 
    const VOMSTrustList& vomscert_trust_dn, 
    //const std::vector<std::string>& vomscert_trust_dn, 
    X509** issuer_cert) {
    X509* issuer = NULL;

    int nid = OBJ_txt2nid("certseq");
    STACK_OF(X509_EXTENSION) *exts = ac->acinfo->exts;
    int pos = X509v3_get_ext_by_NID(exts, nid, -1);
    if (pos >= 0) {
      //Check if the DN/CA file is installed for a given VO.
      X509_EXTENSION* ext = sk_X509_EXTENSION_value(exts, pos);
      AC_CERTS* certs = (AC_CERTS *)X509V3_EXT_d2i(ext);
      //The relatively new version of VOMS server is supposed to
      //create AC which includes the certificate stack:
      //the certificate of voms server; the non-CA certificate/s 
      //(if there are) that signs the voms server' certificate.
      STACK_OF(X509)* certstack = certs->stackcert;

      bool success = false;

      //Check if the DN of those certificates in the certificate stack
      //corresponds to the trusted DN chain in the configuration 
      if(certstack) {
        for(int n = 0;n < vomscert_trust_dn.SizeChains();++n) {
          const VOMSTrustChain& chain = vomscert_trust_dn.GetChain(n);
          if(check_trust(chain,certstack)) {
            success = true;
            break;
          }
        }
        if(!success) for(int n = 0;n < vomscert_trust_dn.SizeRegexs();++n) {
          const RegularExpression& reg = vomscert_trust_dn.GetRegex(n);
          if(check_trust(reg,certstack)) {
            success = true;
            break;
          }
        }
      }

      if (!success) {
        AC_CERTS_free(certs);
        CredentialLogger.msg(ERROR,"VOMS: unable to match certificate chain against VOMS trusted DNs");
        return false;
      }
                  
      //If the certificate stack does correspond to some of the trusted DN chain, 
      //then check if the AC signature is valid by using the voms server 
      //certificate (voms server certificate is supposed to be the first on
      //in the certificate stack).
#ifdef HAVE_OPENSSL_OLDRSA
      X509 *cert = (X509 *)ASN1_dup((int (*)())i2d_X509, 
        (char * (*)())d2i_X509, (char *)sk_X509_value(certstack, 0));
#else
      X509 *cert = (X509 *)ASN1_dup((int (*)(void*, unsigned char**))i2d_X509, 
        (void*(*)(void**, const unsigned char**, long int))d2i_X509, (char *)sk_X509_value(certstack, 0));
#endif

      //for (int i=0; i <sk_X509_num(certstack); i ++)
      //fprintf(stderr, "+++ stk[%i] = %d  %s\n", i , sk_X509_value(certstack, i),  X509_NAME_oneline(X509_get_subject_name((X509 *)sk_X509_value(certstack, i)), NULL, 0));

      bool found = false;

      if (check_sig_ac(cert, ac))
        found = true;
      else
        CredentialLogger.msg(ERROR,"VOMS: unable to verify AC signature");
    
      //Check if those certificate in the certificate stack are trusted.
      if (found) {
        if (!check_cert(certstack, ca_cert_dir, ca_cert_file)) {
          X509_free(cert);
          cert = NULL;
          CredentialLogger.msg(ERROR,"VOMS: unable to verify certificate chain");
        }
      }
      else
        CredentialLogger.msg(ERROR,"VOMS: cannot find certificate of AC issuer for VO %s",voname);
 
      AC_CERTS_free(certs);
     
      if(cert != NULL)issuer = cert;
    }

#if 0 
    //For those old-stype voms configuration, there is no 
    //certificate stack in the AC. So there should be a local
    //directory which includes the voms server certificate.
    //It is deprecated here.
    /*check if able to find the signing certificate 
     among those specific for the vo or else in the vomsdir
     *directory 
     */
    if(issuer == NULL){
      bool found  = false;
      BIO * in = NULL;
      X509 * x = NULL;
      for(int i = 0; (i < 2 && !found); ++i) {
        std::string directory = vomsdir + (i ? "" : "/" + voname);
        CredentialLogger.msg(VERBOSE,"VOMS: directory for trusted service certificates: %s",directory);
        Glib::Dir dir(directory); 
        while(true){
          std::string filename = dir.read_name(); 
          if (!filename.empty()) {
            in = BIO_new(BIO_s_file());
            if (in) {
              std::string temp = directory + "/" + filename;
              if (BIO_read_filename(in, temp.c_str()) > 0) {
                x = PEM_read_bio_X509(in, NULL, 0, NULL);
                if (x) {
                  if (check_sig_ac(x, ac)) { found = true; break; }
                  else { X509_free(x); x = NULL; }
                }
              }
              BIO_free(in); in = NULL;
            }
          }
          else break;
        }
      }
      if (in) BIO_free(in);
      if (found) {
        if (!check_cert(x, ca_cert_dir, ca_cert_file)) { X509_free(x); x = NULL; }
      }
      else {
        CredentialLogger.msg(ERROR,"VOMS: Cannot find certificate of AC issuer for VO %s",voname);

      issuer = x;
    }
#endif

    if(issuer == NULL) {
      CredentialLogger.msg(ERROR,"VOMS: unable to verify AC signature");
      return false;
    } 

    *issuer_cert = issuer; return true; 
  }


  static bool checkAttributes(STACK_OF(AC_ATTR) *atts, std::vector<std::string>& attributes) {
    AC_ATTR *caps;
    STACK_OF(AC_IETFATTRVAL) *values;
    AC_IETFATTR *capattr;
    AC_IETFATTRVAL *capname;
    GENERAL_NAME *data;

    /* find AC_ATTR with IETFATTR type */
    int  nid = OBJ_txt2nid("idatcap");
    int pos = X509at_get_attr_by_NID(atts, nid, -1);
    if (!(pos >=0)) { 
      CredentialLogger.msg(ERROR,"VOMS: Can not find AC_ATTR with IETFATTR type");
      return false;
    }
    caps = sk_AC_ATTR_value(atts, pos);
  
    /* check there's exactly one IETFATTR attribute */
    if (sk_AC_IETFATTR_num(caps->ietfattr) != 1) {
      CredentialLogger.msg(ERROR,"VOMS: case of multiple IETFATTR attributes not supported");
      return false; 
    }

    /* retrieve the only AC_IETFFATTR */
    capattr = sk_AC_IETFATTR_value(caps->ietfattr, 0);
    values = capattr->values;
  
    /* check it has exactly one policyAuthority */
    if (sk_GENERAL_NAME_num(capattr->names) != 1) {
      CredentialLogger.msg(ERROR,"VOMS: case of multiple policyAuthority not supported");
      return false;
    }

    /* store policyAuthority */
    data = sk_GENERAL_NAME_value(capattr->names, 0);
    if (data->type == GEN_URI) {
      std::string voname("/voname=");
      voname.append((const char*)(data->d.ia5->data), data->d.ia5->length);
      std::string::size_type pos = voname.find("://");
      if(pos != std::string::npos) {
        voname.replace(pos,3,"/hostname=");
      }
      attributes.push_back(voname);
    }
    else {
      CredentialLogger.msg(ERROR,"VOMS: the format of policyAuthority is unsupported - expecting URI");
      return false;
    }

    /* scan the stack of IETFATTRVAL to store attribute */
    for (int i=0; i<sk_AC_IETFATTRVAL_num(values); i++) {
      capname = sk_AC_IETFATTRVAL_value(values, i);

      if (capname->type != V_ASN1_OCTET_STRING) {
        CredentialLogger.msg(ERROR,"VOMS: the format of IETFATTRVAL is not supported - expecting OCTET STRING");
        return false;
      }

      std::string fqan((const char*)(capname->data), capname->length);
      if(fqan[0] == '/') fqan.erase(0,1);
      std::size_t group_pos = fqan.find("/",0);
      std::size_t role_pos = std::string::npos;
      if(group_pos != std::string::npos) {
        role_pos = fqan.find("/Role=",group_pos);
        if(role_pos == group_pos) {
          fqan.insert(group_pos,"/Group=NULL");
        } else {
          fqan.insert(group_pos+1,"Group=");
        }
      }
      fqan.insert(0,"/VO=");

      attributes.push_back(fqan);
    }

    return true;
  }

#ifndef HAVE_GETDOMAINNAME
  static int getdomainname(char *name, int length) {
    char	szBuffer[256];
    long	nBufSize = sizeof(szBuffer);
    char	*pBuffer = szBuffer;      

    long result_len = sysinfo( SI_SRPC_DOMAIN, pBuffer, nBufSize );		

    if (result_len > length) {
      return -1;
    }

    memcpy (name, pBuffer, result_len);
    if (result_len < length)
      name[result_len] = '\0';
    //strcpy(name, pBuffer);
   
    return 0;
  }
#endif

  static std::string getfqdn(void) {
    std::string name;
    char hostname[256];
    char domainname[256];

    if ((!gethostname(hostname, 255)) && (!getdomainname(domainname, 255))) {
      name.append(hostname); 
      if(strcmp(domainname, "(none)")) {
        if (*domainname == '.')
          name.append(domainname);
        else {
          name.append(".").append(domainname);
        }
      }
    }
    return name;
  }

  static bool interpret_attributes(AC_FULL_ATTRIBUTES *full_attr, std::vector<std::string>& attributes) {
    std::string name, value, qualifier, grantor, voname, uri;
    GENERAL_NAME *gn = NULL;
    STACK_OF(AC_ATT_HOLDER) *providers = NULL;
    int i;

    providers = full_attr->providers;

    for (i = 0; i < sk_AC_ATT_HOLDER_num(providers); i++) {
      AC_ATT_HOLDER *holder = sk_AC_ATT_HOLDER_value(providers, i);
      STACK_OF(AC_ATTRIBUTE) *atts = holder->attributes;

      gn = sk_GENERAL_NAME_value(holder->grantor, 0);
      grantor.assign((const char*)(gn->d.ia5->data), gn->d.ia5->length);
      if(grantor.empty()) {
        CredentialLogger.msg(ERROR,"VOMS: the grantor attribute is empty");
        return false;
      }
      std::string::size_type pos = grantor.find("://");
      if(pos == std::string::npos) {
        voname = grantor; uri="NULL";
      } else {
        voname = grantor.substr(0,pos);
        uri = grantor.substr(pos+3);
      }

      for (int j = 0; j < sk_AC_ATTRIBUTE_num(atts); j++) {
        std::string attribute;
        AC_ATTRIBUTE *at = sk_AC_ATTRIBUTE_value(atts, j);

        name.assign((const char*)(at->name->data), at->name->length);
        if(name.empty()) {
          CredentialLogger.msg(ERROR,"VOMS: the attribute name is empty");
          return false;
        }
        value.assign((const char*)(at->value->data), at->value->length);
        if(value.empty()) {
          CredentialLogger.msg(ERROR,"VOMS: the attribute value is empty");
          return false;
        }
        qualifier.assign((const char*)(at->qualifier->data), at->qualifier->length);
        if(qualifier.empty()) {
          CredentialLogger.msg(ERROR,"VOMS: the attribute qualifier is empty");
          return false;
        }
        //attribute.append("/grantor=").append(grantor).append("/").append(qualifier).append(":").append(name).append("=").append(value);
        attribute.append("/voname=").append(voname).
                  append("/hostname=").append(uri).
                  append("/").append(qualifier).append(":").append(name).
                  append("=").append(value);
        attributes.push_back(attribute);
      }
      grantor.clear();
    }
    return true;
  }
 
  static bool checkExtensions(STACK_OF(X509_EXTENSION) *exts, X509 *iss, std::vector<std::string>& output) {
    int nid1 = OBJ_txt2nid("idcenoRevAvail");
    int nid2 = OBJ_txt2nid("authorityKeyIdentifier");
    int nid3 = OBJ_txt2nid("idceTargets");
    int nid5 = OBJ_txt2nid("attributes");

    int pos1 = X509v3_get_ext_by_NID(exts, nid1, -1);
    int pos2 = X509v3_get_ext_by_NID(exts, nid2, -1);
    int pos3 = X509v3_get_ext_by_critical(exts, 1, -1);
    int pos4 = X509v3_get_ext_by_NID(exts, nid3, -1);
    int pos5 = X509v3_get_ext_by_NID(exts, nid5, -1);

    /* noRevAvail, Authkeyid MUST be present */
    if ((pos1 < 0) || (pos2 < 0)) {
      CredentialLogger.msg(ERROR,"VOMS: both idcenoRevAvail and authorityKeyIdentifier certificate extensions must be present");
      return false;
    }

    //Check if the target fqan matches idceTargets
    while (pos3 >=0) {
      X509_EXTENSION *ex;
      AC_TARGETS *targets;
      AC_TARGET *name;

      ex = sk_X509_EXTENSION_value(exts, pos3);
      if (pos3 == pos4) {     //The only critical extension allowed is idceTargets,
        std::string fqdn = getfqdn();
        int ok = 0;
        int i;
        ASN1_IA5STRING* fqdns = ASN1_IA5STRING_new();
        if (fqdns) {
          ASN1_STRING_set(fqdns, fqdn.c_str(), fqdn.size());
          targets = (AC_TARGETS *)X509V3_EXT_d2i(ex);
          if (targets)
            for (i = 0; i < sk_AC_TARGET_num(targets->targets); i++) {
              name = sk_AC_TARGET_value(targets->targets, i);
              if (name->name && name->name->type == GEN_URI) {
                ok = !ASN1_STRING_cmp(name->name->d.ia5, fqdns);
                if (ok)
                  break;
              }
            }
          ASN1_STRING_free(fqdns);
        }
        if (!ok) {
          CredentialLogger.msg(WARNING,"VOMS: FQDN of this host %s does not match any target in AC", fqdn);
          // return false;
        }
      }
      else {
        CredentialLogger.msg(ERROR,"VOMS: the only supported critical extension of the AC is idceTargets");
        return false;
      }
      pos3 = X509v3_get_ext_by_critical(exts, 1, pos3);
    }

    //Parse the attributes
    if (pos5 >= 0) {
      X509_EXTENSION *ex = NULL;
      AC_FULL_ATTRIBUTES *full_attr = NULL;
      ex = sk_X509_EXTENSION_value(exts, pos5);
      full_attr = (AC_FULL_ATTRIBUTES *)X509V3_EXT_d2i(ex);
      if (full_attr) {
        if (!interpret_attributes(full_attr, output)) {
          CredentialLogger.msg(ERROR,"VOMS: failed to parse attributes from AC"); 
          AC_FULL_ATTRIBUTES_free(full_attr); return false; 
        }
      }
      AC_FULL_ATTRIBUTES_free(full_attr);
    }

    //Check the authorityKeyIdentifier
    if (pos2 >= 0) {
      X509_EXTENSION *ex;
      bool keyerr = false; 
      AUTHORITY_KEYID *key;
      ex = sk_X509_EXTENSION_value(exts, pos2);
      key = (AUTHORITY_KEYID *)X509V3_EXT_d2i(ex);
      if (key) {
        if (iss) {
          if (key->keyid) {
            unsigned char hashed[20];
            if (!SHA1(iss->cert_info->key->public_key->data,
                      iss->cert_info->key->public_key->length,
                      hashed))
              keyerr = true;
          
            if ((memcmp(key->keyid->data, hashed, 20) != 0) && 
                (key->keyid->length == 20))
              keyerr = true;
          }
          else {
            if (!(key->issuer && key->serial)) keyerr = true;
            if (M_ASN1_INTEGER_cmp((key->serial), (iss->cert_info->serialNumber))) keyerr = true;
            if (key->serial->type != GEN_DIRNAME) keyerr = true;
            if (X509_NAME_cmp(sk_GENERAL_NAME_value((key->issuer), 0)->d.dirn, (iss->cert_info->subject))) keyerr = true;
          }
        }
        AUTHORITY_KEYID_free(key);
      }
      else {
        keyerr = true;
      }

      if(keyerr) {
        CredentialLogger.msg(ERROR,"VOMS: authorityKey is wrong");
        return false;
      }
    }

    return true;
  }

  static bool check_acinfo(X509* cert, X509* issuer, AC* ac, std::vector<std::string>& output) {
    if(!ac || !cert || !(ac->acinfo) || !(ac->acinfo->version) || !(ac->acinfo->holder) 
       || (ac->acinfo->holder->digest) || !(ac->acinfo->form) || !(ac->acinfo->form->names) 
       || (ac->acinfo->form->is) || (ac->acinfo->form->digest) || !(ac->acinfo->serial) 
       || !(ac->acinfo->validity) || !(ac->acinfo->alg) || !(ac->acinfo->validity) 
       || !(ac->acinfo->validity->notBefore) || !(ac->acinfo->validity->notAfter) 
       || !(ac->acinfo->attrib) || !(ac->sig_alg) || !(ac->signature)) return false;

    //Check the validity time  
    ASN1_GENERALIZEDTIME *start;
    ASN1_GENERALIZEDTIME *end;
    start = ac->acinfo->validity->notBefore;
    end = ac->acinfo->validity->notAfter;

    time_t ctime, dtime;
    time (&ctime);
    ctime += 300;
    dtime = ctime-600;

    if ((start->type != V_ASN1_GENERALIZEDTIME) || (end->type != V_ASN1_GENERALIZEDTIME)) {
      CredentialLogger.msg(ERROR,"VOMS: unsupported time format format in AC - expecting GENERALIZED TIME");
      return false;
    }
    if ((X509_cmp_current_time(start) >= 0) &&
        (X509_cmp_time(start, &ctime) >= 0)) {
      CredentialLogger.msg(ERROR,"VOMS: AC is not yet valid");
      return false;
    }
    if ((X509_cmp_current_time(end) <= 0) &&
        (X509_cmp_time(end, &dtime) <= 0)) {
      CredentialLogger.msg(ERROR,"VOMS: AC has expired");
      return false;
    }

    STACK_OF(GENERAL_NAME) *names;
    GENERAL_NAME  *name;

    if (ac->acinfo->holder->baseid) {
      if(!(ac->acinfo->holder->baseid->serial) ||
         !(ac->acinfo->holder->baseid->issuer)) {
        CredentialLogger.msg(ERROR,"VOMS: AC is not complete - missing Serial or Issuer information");
        return false;
      }

      if (ASN1_INTEGER_cmp(ac->acinfo->holder->baseid->serial, cert->cert_info->serialNumber)) {
        CredentialLogger.msg(WARNING,"VOMS: the holder serial number %i is not the same as in AC - %i",
          ASN1_INTEGER_get(cert->cert_info->serialNumber),
          ASN1_INTEGER_get(ac->acinfo->holder->baseid->serial));
        // return false;
      }
       
      names = ac->acinfo->holder->baseid->issuer;
      if ((sk_GENERAL_NAME_num(names) != 1) || !(name = sk_GENERAL_NAME_value(names,0)) ||
        (name->type != GEN_DIRNAME)) {
        CredentialLogger.msg(ERROR,"VOMS: the holder issuer information in AC is wrong");
        return false;
      }
      
      //If the holder is self-signed, and the holder also self sign the AC
      CredentialLogger.msg(VERBOSE,"VOMS: DN of holder in AC: %s",X509_NAME_oneline(name->d.dirn,NULL,0));
      CredentialLogger.msg(VERBOSE,"VOMS: DN of holder: %s",X509_NAME_oneline(cert->cert_info->subject,NULL,0));
      CredentialLogger.msg(VERBOSE,"VOMS: DN of issuer: %s",X509_NAME_oneline(cert->cert_info->issuer,NULL,0));
      if (X509_NAME_cmp(name->d.dirn, cert->cert_info->subject) && 
        X509_NAME_cmp(name->d.dirn, cert->cert_info->issuer)) {
        CredentialLogger.msg(ERROR,"VOMS: the holder itself can not sign an AC by using a self-sign certificate"); 
        return false;
      }

      if ((ac->acinfo->holder->baseid->uid && cert->cert_info->issuerUID) ||
          (!cert->cert_info->issuerUID && !ac->acinfo->holder->baseid->uid)) {
        if (ac->acinfo->holder->baseid->uid) {
          if (M_ASN1_BIT_STRING_cmp(ac->acinfo->holder->baseid->uid, cert->cert_info->issuerUID)) {
            CredentialLogger.msg(ERROR,"VOMS: the holder issuerUID is not the same as that in AC");
            return false;
          }
        }
      }
      else {
        CredentialLogger.msg(ERROR,"VOMS: the holder issuerUID is not the same as that in AC");
        return false;
      }
    }
    else if (ac->acinfo->holder->name) {
      names = ac->acinfo->holder->name;
      if ((sk_GENERAL_NAME_num(names) == 1) ||      //??? 
          ((name = sk_GENERAL_NAME_value(names,0))) ||
          (name->type != GEN_DIRNAME)) {
        if (X509_NAME_cmp(name->d.dirn, cert->cert_info->issuer)) {
          /* CHECK ALT_NAMES */
          /* in VOMS ACs, checking into alt names is assumed to always fail. */
          CredentialLogger.msg(ERROR,"VOMS: the holder issuer name is not the same as that in AC");
          return false;
        }
      }
    }
  
    names = ac->acinfo->form->names;
    if ((sk_GENERAL_NAME_num(names) != 1) || !(name = sk_GENERAL_NAME_value(names,0)) || 
       (name->type != GEN_DIRNAME) || X509_NAME_cmp(name->d.dirn, issuer->cert_info->subject)) {
      CredentialLogger.msg(ERROR,"VOMS: the issuer name %s is not the same as that in AC - %s",
        X509_NAME_oneline(issuer->cert_info->subject,NULL,0),
        X509_NAME_oneline(name->d.dirn,NULL,0));
      return false;
    }

    if (ac->acinfo->serial->length > 20) {
      CredentialLogger.msg(ERROR,"VOMS: the serial number of AC INFO is too long - expecting no more than 20 octets");
      return false;
    }
  
    bool ret = false;
 
    //Check AC's extension
    ret = checkExtensions(ac->acinfo->exts, issuer, output);
    if(!ret)return false;
 
    //Check AC's attribute    
    checkAttributes(ac->acinfo->attrib, output);
    if(!ret)return false;

    return true;
  }

  static bool verifyVOMSAC(AC* ac,
        const std::string& ca_cert_dir, const std::string& ca_cert_file, 
        const VOMSTrustList& vomscert_trust_dn,
        //const std::vector<std::string>& vomscert_trust_dn,
        X509* holder, std::vector<std::string>& output) {
    //Extract name 
    STACK_OF(AC_ATTR) * atts = ac->acinfo->attrib;
    int nid = 0;
    int pos = 0;
    nid = OBJ_txt2nid("idatcap");
    pos = X509at_get_attr_by_NID(atts, nid, -1);
    if(!(pos >=0)) {
      CredentialLogger.msg(ERROR,"VOMS: unable to extract VO name from AC");
      return false;
    }

    AC_ATTR * caps = sk_AC_ATTR_value(atts, pos);
    if(!caps) {
      CredentialLogger.msg(ERROR,"VOMS: unable to extract vo name from AC");
      return false;
    }

    AC_IETFATTR * capattr = sk_AC_IETFATTR_value(caps->ietfattr, 0);
    if(!capattr) {
      CredentialLogger.msg(ERROR,"VOMS: unable to extract vo name from AC");
      return false;
    }

    GENERAL_NAME * name = sk_GENERAL_NAME_value(capattr->names, 0);
    if(!name) {
      CredentialLogger.msg(ERROR,"VOMS: unable to extract vo name from AC");
      return false;
    }

    std::string voname((const char *)name->d.ia5->data, 0, name->d.ia5->length);
    std::string::size_type cpos = voname.find("://");
    std::string hostname;
    if (cpos != std::string::npos) {
      std::string::size_type cpos2 = voname.find(":", cpos+1);
      if (cpos2 != std::string::npos)
        hostname = voname.substr(cpos+3, (cpos2 - cpos - 3));
      else {
        CredentialLogger.msg(ERROR,"VOMS: unable to determine hostname of AC from VO name: %s",voname);
        return false;
      }
      voname = voname.substr(0, cpos);
    }
    else {
      CredentialLogger.msg(ERROR,"VOMS: unable to extract VO name from AC");
      return false;
    }
 
    X509* issuer = NULL;

    if(!check_signature(ac, voname, hostname, ca_cert_dir, ca_cert_file, vomscert_trust_dn, &issuer)) {
      CredentialLogger.msg(ERROR,"VOMS: cannt verify the signature of the AC");
      return false; 
    }

    if(check_acinfo(holder, issuer, ac, output)) return true;
    else return false;
  }

  bool parseVOMSAC(X509* holder,
        const std::string& ca_cert_dir, const std::string& ca_cert_file, 
        const VOMSTrustList& vomscert_trust_dn,
        //const std::vector<std::string>& vomscert_trust_dn,
        std::vector<std::string>& output) {

    InitVOMSAttribute();

    //Search the extension
    int nid = 0;
    int position = 0;
    X509_EXTENSION * ext;
    AC_SEQ* aclist = NULL;
    nid = OBJ_txt2nid("acseq");
    position = X509_get_ext_by_NID(holder, nid, -1);
    if(position >= 0) {
      ext = X509_get_ext(holder, position);
      if (ext){
        aclist = (AC_SEQ *)X509V3_EXT_d2i(ext);
      }
    }    
    if(aclist == NULL) {
      ERR_clear_error();
      //while(ERR_get_error() != 0);
      //std::cerr<<"No AC in the proxy certificate"<<std::endl; return false;
      return true;
    }

    bool verified = false;
    int num = sk_AC_num(aclist->acs);
    for (int i = 0; i < num; i++) {
      AC *ac = (AC *)sk_AC_value(aclist->acs, i);
      if (verifyVOMSAC(ac, ca_cert_dir, ca_cert_file, vomscert_trust_dn, holder, output)) {
        verified = true;
      }
      if (!verified) break;
    } 
    ERR_clear_error();
    //while(ERR_get_error() != 0);

    return verified;
  }

  bool parseVOMSAC(Credential& holder_cred,
         const std::string& ca_cert_dir, const std::string& ca_cert_file,
         const VOMSTrustList& vomscert_trust_dn,
         //const std::vector<std::string>& vomscert_trust_dn,
         std::vector<std::string>& output) {
    X509* holder = holder_cred.GetCert();
    if(!holder) return false;
    bool res = parseVOMSAC(holder, ca_cert_dir, ca_cert_file, vomscert_trust_dn, output);
    X509_free(holder);
    return res;
  }

  static char trans2[128] = { 0,   0,  0,  0,  0,  0,  0,  0,
                            0,   0,  0,  0,  0,  0,  0,  0,
                            0,   0,  0,  0,  0,  0,  0,  0,
                            0,   0,  0,  0,  0,  0,  0,  0,
                            0,   0,  0,  0,  0,  0,  0,  0,
                            0,   0,  0,  0,  0,  0,  0,  0,
                            52, 53, 54, 55, 56, 57, 58, 59,
                            60, 61,  0,  0,  0,  0,  0,  0,
                            0,  26, 27, 28, 29, 30, 31, 32,
                            33, 34, 35, 36, 37, 38, 39, 40,
                            41, 42, 43, 44, 45, 46, 47, 48,
                            49, 50, 51, 62,  0, 63,  0,  0,
                            0,   0,  1,  2,  3,  4,  5,  6,
                            7,   8,  9, 10, 11, 12, 13, 14,
                            15, 16, 17, 18, 19, 20, 21, 22,
                            23, 24, 25,  0,  0,  0,  0,  0};

  static char *base64Decode(const char *data, int size, int *j) {
    BIO *b64 = NULL;
    BIO *in = NULL;

    char *buffer = (char *)malloc(size);
    if (!buffer)
      return NULL;

    memset(buffer, 0, size);

    b64 = BIO_new(BIO_f_base64());
    in = BIO_new_mem_buf((void*)data, size);
    in = BIO_push(b64, in);

    *j = BIO_read(in, buffer, size);

    BIO_free_all(in);
    return buffer;
  }

  static char *MyDecode(const char *data, int size, int *n) {
    int bit = 0;
    int i = 0;
    char *res;

    if (!data || !size) return NULL;

    if ((res = (char *)calloc(1, (size*3)/4 + 2))) {
      *n = 0;

      while (i < size) {
        char c  = trans2[(int)data[i]];
        char c2 = (((i+1) < size) ? trans2[(int)data[i+1]] : 0);

        switch(bit) {
        case 0:
          res[*n] = ((c & 0x3f) << 2) | ((c2 & 0x30) >> 4);
          if ((i+1) < size)
            (*n)++;
          bit=4;
          i++;
          break;
        case 4:
          res[*n] = ((c & 0x0f) << 4) | ((c2 & 0x3c) >> 2);
          if ((i+1) < size)
            (*n)++;
          bit=2;
          i++;
          break;
        case 2:
          res[*n] = ((c & 0x03) << 6) | (c2 & 0x3f);
          if ((i+1) < size)
            (*n)++;

          i += 2;
          bit = 0;
          break;
        }
      }

      return res;
    }
    return NULL;
  }

  char *VOMSDecode(const char *data, int size, int *j) {
    int i = 0;

    while (i < size)
      if (data[i++] == '\n')
        return base64Decode(data, size, j);

    return MyDecode(data, size, j);
  }

} // namespace Arc

