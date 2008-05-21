#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "VOMSAttribute.h"
#include "voms_util.h"
extern "C" {
#include "listfunc.h"
} 

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
    int i = 0;
    int err = AC_ERR_UNKNOWN;
    time_t curtime;

    InitVOMSAttribute();

    if (!issuer || !holder || !serialnum || fqan.empty() || !ac || !pkey)
      return AC_ERR_PARAMETERS;

    a = *ac;
    subname = X509_NAME_dup(X509_get_subject_name(holder));
    issname = X509_NAME_dup(X509_get_issuer_name(holder));

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
    if (issuerstack) {
      for (int j =0; j < sk_X509_num(issuerstack); j++)
        sk_X509_push(stk, X509_dup(sk_X509_value(issuerstack, j)));
    }

/*   for (i=0; i <sk_X509_num(stk); i ++) */
/*     fprintf(stderr, "stk[%i] = %s\n", i , X509_NAME_oneline(X509_get_subject_name((X509 *)sk_X509_value(stk, i)), NULL, 0)); */

#ifdef HAVE_OPENSSL_OLDRSA
    sk_X509_push(stk, (X509 *)ASN1_dup((int (*)())i2d_X509,
          (char*(*)())d2i_X509, (char *)issuer));
#else
    sk_X509_push(stk, (X509 *)ASN1_dup((int (*)(void*, unsigned char**))i2d_X509,
          (void*(*)(void**, const unsigned char**, long int))d2i_X509, (char *)issuer));
#endif

/*   for (i=0; i <sk_X509_num(stk); i ++) */
/*     fprintf(stderr, "stk[%i] = %d\n", i , sk_X509_value(stk, i)); */

    certstack = X509V3_EXT_conf_nid(NULL, NULL, OBJ_txt2nid("certseq"), (char*)stk);
    sk_X509_pop_free(stk, X509_free);

    /* Create extensions */
    norevavail = X509V3_EXT_conf_nid(NULL, NULL, OBJ_txt2nid("idcenoRevAvail"), "loc");
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



} // namespace Arc

