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

#include "VOMSAttribute.h"
#include "voms_util.h"
extern "C" {
#include "listfunc.h"
} 

using namespace Arc;

namespace ArcLib{

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
    OBJ_create(email, "Email", "Email");
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

      std::cout<<"FQAN: "<<(*i)<<std::endl;

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

      std::cout<<"Attribute: "<<(*i)<<std::endl; 
 
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
    
    std::cout<<"++++++++ cert chain number: "<<sk_X509_num(issuerstack)<<std::endl;

    if (issuerstack) {
      for (int j =0; j < sk_X509_num(issuerstack); j++)
        sk_X509_push(stk, X509_dup(sk_X509_value(issuerstack, j)));
    }

   int i;
   for (i=0; i < sk_X509_num(stk); i++) 
     fprintf(stderr, "stk[%i] = %s\n", i , X509_NAME_oneline(X509_get_subject_name((X509 *)sk_X509_value(stk, i)), NULL, 0)); 

#ifdef HAVE_OPENSSL_OLDRSA
    sk_X509_push(stk, (X509 *)ASN1_dup((int (*)())i2d_X509,
          (char*(*)())d2i_X509, (char *)issuer));
#else
    sk_X509_push(stk, (X509 *)ASN1_dup((int (*)(void*, unsigned char**))i2d_X509,
          (void*(*)(void**, const unsigned char**, long int))d2i_X509, (char *)issuer));
#endif

   for(i=0; i<sk_X509_num(stk); i++)
     fprintf(stderr, "stk[%i] = %d  %s\n", i , sk_X509_value(stk, i),  X509_NAME_oneline(X509_get_subject_name((X509 *)sk_X509_value(stk, i)), NULL, 0));

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
             std::vector<std::string> &fqan, std::vector<std::string> &targets, std::vector<std::string>& attributes, 
             std::string &voname, std::string &uri, int lifetime) {

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
    int l = codedac.size();

    pp = (char *)malloc(codedac.size());
    if(!pp) {std::cerr<<"Can not allocate memory for parsing ac"<<std::endl; return false; }

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
      std::cerr<<"Can not parse ac"<<std::endl;
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
      std::cerr<<"CA dir or CA file should be setup"<<std::endl;
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
      std::cerr<<"CA dir or CA file should be setup"<<std::endl;
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
        for (int i = 0; i < sk_X509_num(stack); i++) {
          X509_STORE_add_cert(ctx,sk_X509_value(stack, i));
          ERR_clear_error();
          X509_STORE_CTX_init(csc, ctx, sk_X509_value(stack, 0), NULL);
          index = X509_verify_cert(csc);
        }
      }
      else if (!(ca_cert_file.empty()) && (lookup = X509_STORE_add_lookup(ctx, X509_LOOKUP_file()))) {
        X509_LOOKUP_load_file(lookup, NULL, X509_FILETYPE_PEM);
        for (int i = 0; i < sk_X509_num(stack); i++) {
          X509_STORE_add_cert(ctx,sk_X509_value(stack, i));
          ERR_clear_error();
          X509_STORE_CTX_init(csc, ctx, sk_X509_value(stack, 0), NULL);
          index = X509_verify_cert(csc);
        }
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

    if (!res) std::cerr<<"Unable to verify AC signature"<<std::endl;
  
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

  static bool check_signature(AC* ac, std::string& voname, std::string& hostname, 
    const std::string& ca_cert_dir, const std::string& ca_cert_file, const std::vector<std::string>& vomscert_trust_dn, 
    X509** issuer_cert) {
    X509* issuer = NULL;

    int nid = OBJ_txt2nid("certseq");
    STACK_OF(X509_EXTENSION) *exts = ac->acinfo->exts;
    int pos = X509v3_get_ext_by_NID(exts, nid, -1);
    if (pos >= 0) {
      //Check if the DN/CA file is installed for a given VO.
      X509_EXTENSION* ext = sk_X509_EXTENSION_value(exts, pos);
      AC_CERTS* certs = (AC_CERTS *)X509V3_EXT_d2i(ext);
      STACK_OF(X509)* certstack = certs->stackcert;

      bool success = false;
      bool final = false;

      unsigned int k=0;
      do {
        success = true;
        for (int i = 0; i < sk_X509_num(certstack); i++) {
          X509 *current = sk_X509_value(certstack, i);

          std::string subject_name;
          std::string issuer_name;
          subject_name = vomscert_trust_dn[k];
          size_t pos = std::string::npos;
          pos = subject_name.find_first_of("^");
          std::string str;
          if(k < vomscert_trust_dn.size()-1) {
            str = vomscert_trust_dn[k+1];
          }
 
          //If this line ("k"th line) is the last line in this chain, then this line should be regular expression,
          //here only regex "^" is considered.
          //And this line is also considered to check the issuer
          //"k" will not be increased, then this line will be used for checking the next certificate
          if((k >= (vomscert_trust_dn.size()-1) || str.find("NEXT CHAIN") != std::string::npos) 
              && (pos != std::string::npos && pos == subject_name.find_first_not_of(" ")))
            issuer_name = subject_name;
          //If there is another line after this line, then that line ("k"+1) will be used to check the issuer.
          //And the "k" line will not be used any more for the next certificate.
          else if ((vomscert_trust_dn[k+1]).find("NEXT CHAIN") == std::string::npos) {
            issuer_name = vomscert_trust_dn[++k];
          }
          //If "k" line is the last line, but it is not regular expression. 
          else {
            std::cerr<<"Wrong definition in voms certificate DN"<<std::endl; return false;
          }

          /*
          if(subject_name.empty() || issuer_name.empty()) { 
            success = false;
            final = true;
            break;
          }
          */

          std::string realsubject(X509_NAME_oneline(X509_get_subject_name(current), NULL, 0));
          std::string realissuer(X509_NAME_oneline(X509_get_issuer_name(current), NULL, 0));

          //std::cout<<"Subject: "<<subject_name<<" Real Subject: "<<realsubject<<std::endl
          //<<"Issuer: "<<issuer_name<<" Real Issuer: "<<realissuer<<std::endl;

          bool sub_match=false, iss_match=false;
          bool sub_isregex=false, iss_isregex=false;
          if(pos != std::string::npos && pos == subject_name.find_first_not_of(" ")) {
            sub_isregex = true;
            sub_match = regex_match(subject_name, realsubject);
          }
          pos = issuer_name.find_first_of("^");
          if(pos != std::string::npos && pos == issuer_name.find_first_not_of(" ")) {
            iss_isregex = true;
            iss_match = regex_match(issuer_name, realissuer);
          }
          if(!sub_isregex) sub_match = (subject_name == realsubject) ? true : false;
          if(!iss_isregex) iss_match = (issuer_name == realissuer) ? true : false;          

          if(!sub_match || !iss_match) {
            do {
              subject_name = vomscert_trust_dn[++k];
              //std::cout<<"++++++  "<<subject_name<<std::endl;
            } while (k < vomscert_trust_dn.size() && subject_name.find("NEXT CHAIN") != std::string::npos);
            success = false;
            break;
          }
        }
        if (success || k >= vomscert_trust_dn.size()) final = true;
      } while (!final);

      if (!success) {
        AC_CERTS_free(certs);
        std::cerr<<"Unable to match certificate chain against voms trust DNs"<<std::endl;
        return false;
      }
                  
      /* check if able to find the DN in the vomscert_trust_dn which is corresponding to the signing certificate*/

#ifdef HAVE_OPENSSL_OLDRSA
      X509 *cert = (X509 *)ASN1_dup((int (*)())i2d_X509, (char * (*)())d2i_X509, (char *)sk_X509_value(certstack, 0));
#else
      X509 *cert = (X509 *)ASN1_dup((int (*)(void*, unsigned char**))i2d_X509, 
                   (void*(*)(void**, const unsigned char**, long int))d2i_X509, (char *)sk_X509_value(certstack, 0));
#endif

   for (int i=0; i <sk_X509_num(certstack); i ++)
     fprintf(stderr, "+++ stk[%i] = %d  %s\n", i , sk_X509_value(certstack, i),  X509_NAME_oneline(X509_get_subject_name((X509 *)sk_X509_value(certstack, i)), NULL, 0));

      bool found = false;

      if (check_sig_ac(cert, ac))
        found = true;
      else
        std::cerr<<"Unable to verify signature!"<<std::endl;

      if (found) {
        if (!check_cert(certstack, ca_cert_dir, ca_cert_file)) {
          X509_free(cert);
          cert = NULL;
          std::cerr<<"Unable to verify certificate chain"<<std::endl;
        }
      }
      else
        std::cerr<<"Cannot find certificate of AC issuer for vo: "<<voname<<std::endl;
 
      AC_CERTS_free(certs);
     
      if(cert != NULL)issuer = cert;
    }

#if 0 
    /* check if able to find the signing certificate 
     *among those specific for the vo or else in the vomsdir
     *directory 
     */
    if(issuer == NULL){
      bool found  = false;
      BIO * in = NULL;
      X509 * x = NULL;
      for(int i = 0; (i < 2 && !found); ++i) {
        std::string directory = vomsdir + (i ? "" : "/" + voname);
        std::cout<<"Directory to find trusted certificate: "<<directory<<std::endl;
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
      else std::cerr<<"Cannot find certificate of AC issuer for vo "<<voname<<std::endl;

      issuer = x;
    }
#endif

    if(issuer == NULL) { std::cerr<<"Can not verify AC signature"<<std::endl; return false; } 

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
    if (!(pos >=0)) { std::cerr<<"Can not find AC_ATTR with IETFATTR type"<<std::endl; return false; }
    caps = sk_AC_ATTR_value(atts, pos);
  
    /* check there's exactly one IETFATTR attribute */
    if (sk_AC_IETFATTR_num(caps->ietfattr) != 1) {
      std::cerr<<"Check there's exactly one IETFATTR attribute"<<std::endl; return false; 
    }

    /* retrieve the only AC_IETFFATTR */
    capattr = sk_AC_IETFATTR_value(caps->ietfattr, 0);
    values = capattr->values;
  
    /* check it has exactly one policyAuthority */
    if (sk_GENERAL_NAME_num(capattr->names) != 1) {
      std::cerr<<"Check there's exactly one policyAuthority"<<std::endl; return false;
    }

    /* store policyAuthority */
    data = sk_GENERAL_NAME_value(capattr->names, 0);
    if (data->type == GEN_URI) {
      std::string voname;
      voname.append("voname=").append((const char*)(data->d.ia5->data), data->d.ia5->length);
      attributes.push_back(voname);
    }
    else {
      std::cerr<<"The format of policyAuthority is wrong"<<std::endl; return false;
    }

    /* scan the stack of IETFATTRVAL to store attribute */
    for (int i=0; i<sk_AC_IETFATTRVAL_num(values); i++) {
      capname = sk_AC_IETFATTRVAL_value(values, i);

      if (!(capname->type == V_ASN1_OCTET_STRING)) {
        std::cerr<<"The IETFATTRVAL is not V_ASN1_OCTET_STRING"<<std::endl; return false;
      }

      std::string fqan((const char*)(capname->data), capname->length);
      std::string str("VO=");
      std::size_t pos = fqan.find("/",1);
      if(pos==std::string::npos) str.append(fqan.substr(1));
      else { 
        std::size_t pos1 = fqan.find("/Role=");
        if(pos1==std::string::npos) str.append(fqan.substr(1,pos-1)).append("/Group=").append(fqan.substr(pos+1));
        else str.append(fqan.substr(1,pos-1)).append("/Group=").append(fqan.substr(pos+1, pos1-pos-1)).append(fqan.substr(pos1));
      }
      attributes.push_back(str);      
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
    std::string name, value, qualifier, grantor;
    std::string attribute;
    GENERAL_NAME *gn = NULL;
    STACK_OF(AC_ATT_HOLDER) *providers = NULL;
    int i;

    providers = full_attr->providers;

    for (i = 0; i < sk_AC_ATT_HOLDER_num(providers); i++) {
      AC_ATT_HOLDER *holder = sk_AC_ATT_HOLDER_value(providers, i);
      STACK_OF(AC_ATTRIBUTE) *atts = holder->attributes;

      gn = sk_GENERAL_NAME_value(holder->grantor, 0);
      grantor.assign((const char*)(gn->d.ia5->data), gn->d.ia5->length);
      if(grantor.empty()) { std::cerr<<"The attribute grantor is empty"<<std::endl; return false; }

      for (int j = 0; j < sk_AC_ATTRIBUTE_num(atts); j++) {
        AC_ATTRIBUTE *at = sk_AC_ATTRIBUTE_value(atts, j);

        name.assign((const char*)(at->name->data), at->name->length);
        if(name.empty()) { std::cerr<<"The attribute name is empty"<<std::endl; return false; }
        value.assign((const char*)(at->value->data), at->value->length);
        if(value.empty()) { std::cerr<<"The attribute value is empty"<<std::endl; return false; }
        qualifier.assign((const char*)(at->qualifier->data), at->qualifier->length);
        if(qualifier.empty()) { std::cerr<<"The attribute qualifier is empty"<<std::endl; return false; }

        //attribute.append("grantor=").append(grantor).append("/").append("qualifier=").append(qualifier).append(":").append(name).append("/").append("value=").append(value);
        attribute.append("grantor=").append(grantor).append("/").append(qualifier).append(":").append(name).append("=").append(value);
        attributes.push_back(attribute);
        attribute.clear(); 
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
    if (pos1 < 0 || pos2 < 0) {
      std::cerr<<"noRevAvail, Authkeyid MUST be present"<<std::endl; return false;
    }

    /* The only critical extension allowed is idceTargets. */
    while (pos3 >=0) {
      X509_EXTENSION *ex;
      AC_TARGETS *targets;
      AC_TARGET *name;

      ex = sk_X509_EXTENSION_value(exts, pos3);
      if (pos3 == pos4) {
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
        if (!ok) { std::cerr<<"The fqdn does not match that in AC"<<std::endl; } // return false; }
      }
      else { std::cerr<<"The critical part of the AC extension can only be idceTargets"<<std::endl;  return false; }
      pos3 = X509v3_get_ext_by_critical(exts, 1, pos3);
    }

    if (pos5 >= 0) {
      X509_EXTENSION *ex = NULL;
      AC_FULL_ATTRIBUTES *full_attr = NULL;
      ex = sk_X509_EXTENSION_value(exts, pos5);
      full_attr = (AC_FULL_ATTRIBUTES *)X509V3_EXT_d2i(ex);
      if (full_attr) {
        if (!interpret_attributes(full_attr, output)) {
          std::cerr<<"Failed to parsed attributes from AC"<<std::endl; 
          AC_FULL_ATTRIBUTES_free(full_attr); return false; 
        }
      }
      AC_FULL_ATTRIBUTES_free(full_attr);
    }

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

      if(keyerr) { std::cerr<<"authorityKey is wrong"<<std::endl;  return false; }
    }

    return true;
  }

  static bool check_acinfo(X509* cert, X509* issuer, AC* ac, std::vector<std::string>& output) {
    if(!ac || !cert || !(ac->acinfo) || !(ac->acinfo->version) || !(ac->acinfo->holder) || (ac->acinfo->holder->digest)
       || !(ac->acinfo->form) || !(ac->acinfo->form->names) || (ac->acinfo->form->is) || (ac->acinfo->form->digest)
       || !(ac->acinfo->serial) || !(ac->acinfo->validity) || !(ac->acinfo->alg) || !(ac->acinfo->validity) 
       || !(ac->acinfo->validity->notBefore) || !(ac->acinfo->validity->notAfter) || !(ac->acinfo->attrib)
       || !(ac->sig_alg) || !(ac->signature)) return false;

    //Check the validity time  
    ASN1_GENERALIZEDTIME *start;
    ASN1_GENERALIZEDTIME *end;
    start = ac->acinfo->validity->notBefore;
    end = ac->acinfo->validity->notAfter;

    time_t ctime, dtime;
    time (&ctime);
    ctime += 300;
    dtime = ctime-600;

    if ((end->type != V_ASN1_GENERALIZEDTIME) || (end->type != V_ASN1_GENERALIZEDTIME)) {
      std::cerr<<"Time format in AC is wrong"<<std::endl; return false;
    }
    if (((X509_cmp_current_time(start) >= 0) &&
         (X509_cmp_time(start, &ctime) >= 0)) ||
        ((X509_cmp_current_time(end) <= 0) &&
         (X509_cmp_time(end, &dtime) <= 0))) {
      std::cerr<<"Current time is out of the scope of valid period of the AC"<<std::endl; return false;
    }

    STACK_OF(GENERAL_NAME) *names;
    GENERAL_NAME  *name;

    if (ac->acinfo->holder->baseid) {
      if(!(ac->acinfo->holder->baseid->serial) ||
         !(ac->acinfo->holder->baseid->issuer)) {
        std::cerr<<"The AC_IS is not complete"<<std::endl; return false;
      }

      if (ASN1_INTEGER_cmp(ac->acinfo->holder->baseid->serial, cert->cert_info->serialNumber)) {
        std::cerr<<"The holder serial number: "<<ASN1_INTEGER_get(cert->cert_info->serialNumber)<<" is not the same as that in AC: "<<ASN1_INTEGER_get(ac->acinfo->holder->baseid->serial)<<std::endl; //return false;
      }
       
      names = ac->acinfo->holder->baseid->issuer;
      if ((sk_GENERAL_NAME_num(names) != 1) || !(name = sk_GENERAL_NAME_value(names,0)) || (name->type != GEN_DIRNAME)) {
        std::cerr<<"The holder issuer information in AC is wrong"<<std::endl; return false;
      }
      
      //If the holder is self-signed, and the holder also self sign the AC
      std::cout<<"The holder DN in AC: "<<X509_NAME_oneline(name->d.dirn,NULL,0)<<std::endl;
      std::cout<<"The holder DN: "<<X509_NAME_oneline(cert->cert_info->subject,NULL,0)<<std::endl;
      std::cout<<"The holder's issuer DN: "<<X509_NAME_oneline(cert->cert_info->issuer,NULL,0)<<std::endl;
      if (X509_NAME_cmp(name->d.dirn, cert->cert_info->subject) && X509_NAME_cmp(name->d.dirn, cert->cert_info->issuer)) {
        std::cerr<<"The holder itself can not sign an AC by using a self-sign certificate"<<std::endl; return false;
      }

      if ((ac->acinfo->holder->baseid->uid && cert->cert_info->issuerUID) ||
          (!cert->cert_info->issuerUID && !ac->acinfo->holder->baseid->uid)) {
        if (ac->acinfo->holder->baseid->uid) {
          if (M_ASN1_BIT_STRING_cmp(ac->acinfo->holder->baseid->uid, cert->cert_info->issuerUID)) {
            std::cerr<<"The holder issuerUID is not the same as that in AC"<<std::endl; return false;
          }
        }
      }
      else { std::cerr<<"The holder issuerUID is not the same as that in AC"<<std::endl; return false; }
    }
    else if (ac->acinfo->holder->name) {
      names = ac->acinfo->holder->name;
      if ((sk_GENERAL_NAME_num(names) == 1) ||      //??? 
          ((name = sk_GENERAL_NAME_value(names,0))) ||
          (name->type != GEN_DIRNAME)) {
        if (X509_NAME_cmp(name->d.dirn, cert->cert_info->issuer)) {
          /* CHECK ALT_NAMES */
          /* in VOMS ACs, checking into alt names is assumed to always fail. */
          std::cerr<<"The holder issuer name is not the same as that in AC"<<std::endl; return false;
        }
      }
    }
  
    names = ac->acinfo->form->names;
    if ((sk_GENERAL_NAME_num(names) != 1) || !(name = sk_GENERAL_NAME_value(names,0)) || (name->type != GEN_DIRNAME) ||
       X509_NAME_cmp(name->d.dirn, issuer->cert_info->subject)) {
      std::cerr<<"The issuer's issuer name: "<<X509_NAME_oneline(issuer->cert_info->subject,NULL,0)<<" is not the same as that in AC: "<<X509_NAME_oneline(name->d.dirn,NULL,0)<<std::endl; return false;
    }

    if (ac->acinfo->serial->length > 20) {
      std::cerr<<"The serial number of ACINFO is too long"<<std::endl; return false;
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

  bool verifyVOMSAC(AC* ac, const std::string& ca_cert_dir, const std::string& ca_cert_file, 
        const std::vector<std::string>& vomscert_trust_dn, X509* holder, std::vector<std::string>& output) {
    //Extract name 
    STACK_OF(AC_ATTR) * atts = ac->acinfo->attrib;
    int nid = 0;
    int pos = 0;
    nid = OBJ_txt2nid("idatcap");
    pos = X509at_get_attr_by_NID(atts, nid, -1);
    if(!(pos >=0)) { std::cerr<<"Unable to extract vo name from AC"<<std::endl; return false; }

    AC_ATTR * caps = sk_AC_ATTR_value(atts, pos);
    if(!caps) { std::cerr<<"Unable to extract vo name from AC"<<std::endl; return false; }

    AC_IETFATTR * capattr = sk_AC_IETFATTR_value(caps->ietfattr, 0);
    if(!capattr) { std::cerr<<"Unable to extract vo name from AC"<<std::endl; return false; }

    GENERAL_NAME * name = sk_GENERAL_NAME_value(capattr->names, 0);
    if(!name) { std::cerr<<"Unable to extract vo name from AC"<<std::endl; return false; }

    std::string voname((const char *)name->d.ia5->data, 0, name->d.ia5->length);
    std::string::size_type cpos = voname.find("://");
    std::string hostname;
    if (cpos != std::string::npos) {
      std::string::size_type cpos2 = voname.find(":", cpos+1);
      if (cpos2 != std::string::npos)
        hostname = voname.substr(cpos+3, (cpos2 - cpos - 3));
      else {
        std::cerr<<"Unable to determine hostname from AC: "<<voname<<std::endl;
        return false;
      }
      voname = voname.substr(0, cpos);
    }
    else {
      std::cerr<<"Unable to extract vo name from AC"<<std::endl;
      return false;
    }
 
    X509* issuer = NULL;

    if(!check_signature(ac, voname, hostname, ca_cert_dir, ca_cert_file, vomscert_trust_dn, &issuer)) {
      std::cerr<<"Can not verify the signature of the AC"<<std::endl; return false; 
    }

    if(check_acinfo(holder, issuer, ac, output)) return true;
    else return false;
  }

  bool parseVOMSAC(X509* holder, const std::string& ca_cert_dir, const std::string& ca_cert_file, 
        const std::vector<std::string>& vomscert_trust_dn, std::vector<std::string>& output) {

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
      std::cerr<<"No AC in the proxy certificate"<<std::endl; return false;
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

  bool parseVOMSAC(Credential& holder_cred, const std::string& ca_cert_dir, const std::string& ca_cert_file,
         const std::vector<std::string>& vomscert_trust_dn, std::vector<std::string>& output) {
    X509* holder = holder_cred.GetCert();
    if(!holder) return false;
    bool res = parseVOMSAC(holder, ca_cert_dir, ca_cert_file, vomscert_trust_dn, output);
    X509_free(holder);
    return res;
  }

} // namespace Arc

