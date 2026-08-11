#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <fstream>
#include <glibmm/fileutils.h>
#include <unistd.h>

#include <arc/DateTime.h>
#include <arc/ArcRegex.h>
#include <arc/Utils.h>
#include <arc/StringConv.h>
#include <arc/crypto/OpenSSL.h>

#include <arc/credential/VOMSAttribute.h>
#include <arc/credential/VOMSUtil.h>
#include "listfunc.h"

#include "glibmm-compat.h"

#if (OPENSSL_VERSION_NUMBER < 0x30400000L)
// --------------------------------
// Definitions for OpenSSL < 3.4.0
// --------------------------------
#define idpkixOID                "1.3.6.1.5.5.7"
#define idceOID                  "2.5.29"
#define idceauthKeyIdentifierOID idceOID ".35"
#define idcenoRevAvailOID        idceOID ".56"
#define idceTargetsOID           idceOID ".55"
#endif

#define vomsOID                  "1.3.6.1.4.1.8005.100.100.1"
#define incfileOID               "1.3.6.1.4.1.8005.100.100.2"
#define voOID                    "1.3.6.1.4.1.8005.100.100.3"
#define idatcapOID               "1.3.6.1.4.1.8005.100.100.4"
#define attributesOID            "1.3.6.1.4.1.8005.100.100.11"
#define acseqOID                 "1.3.6.1.4.1.8005.100.100.5"
#define orderOID                 "1.3.6.1.4.1.8005.100.100.6"
#define certseqOID               "1.3.6.1.4.1.8005.100.100.10"

static std::string default_vomsdir = std::string(G_DIR_SEPARATOR_S) + "etc" + G_DIR_SEPARATOR_S +"grid-security" + G_DIR_SEPARATOR_S + "vomsdir";


using namespace ArcCredential;

namespace Arc {

typedef Arc::Credential::OBJRef<AC,AC_free,AC_new> ACRef;
typedef Arc::Credential::OBJRef<AC_SEQ,AC_SEQ_free,AC_SEQ_new> AC_SEQRef;
typedef Arc::Credential::OBJRef<AC_CERTS,AC_CERTS_free,AC_CERTS_new> AC_CERTSRef;
typedef Arc::Credential::OBJRef<AC_IETFATTRVAL,AC_IETFATTRVAL_free,AC_IETFATTRVAL_new> AC_IETFATTRVALRef;
typedef Arc::Credential::OBJRef<AC_FULL_ATTRIBUTES,AC_FULL_ATTRIBUTES_free,AC_FULL_ATTRIBUTES_new> AC_FULL_ATTRIBUTESRef;
typedef Arc::Credential::OBJRef<AC_ATT_HOLDER,AC_ATT_HOLDER_free,AC_ATT_HOLDER_new> AC_ATT_HOLDERRef;
typedef Arc::Credential::OBJRef<AC_ATTRIBUTE,AC_ATTRIBUTE_free,AC_ATTRIBUTE_new> AC_ATTRIBUTERef;
typedef Arc::Credential::OBJRef<AC_ATTR,AC_ATTR_free,AC_ATTR_new> AC_ATTRRef;
typedef Arc::Credential::OBJRef<AC_IETFATTR,AC_IETFATTR_free,AC_IETFATTR_new> AC_IETFATTRRef;
typedef Arc::Credential::OBJRef<AUTHORITY_KEYID,AUTHORITY_KEYID_free,AUTHORITY_KEYID_new> AUTHORITY_KEYIDRef;
#if (OPENSSL_VERSION_NUMBER < 0x30400000L)
typedef Arc::Credential::OBJRef<AC_TARGETS,AC_TARGETS_free,AC_TARGETS_new> AC_TARGETSRef;
#else
typedef Arc::Credential::OBJRef<OSSL_TARGET,OSSL_TARGET_free,OSSL_TARGET_new> OSSL_TARGETRef;
typedef Arc::Credential::OBJRef<OSSL_TARGETS,OSSL_TARGETS_free,OSSL_TARGETS_new> OSSL_TARGETSRef;
typedef Arc::Credential::OBJRef<OSSL_TARGETING_INFORMATION,OSSL_TARGETING_INFORMATION_free,OSSL_TARGETING_INFORMATION_new> OSSL_TARGETING_INFORMATIONRef;
typedef Arc::Credential::OBJRef<OSSL_ISSUER_SERIAL,OSSL_ISSUER_SERIAL_free,OSSL_ISSUER_SERIAL_new> OSSL_ISSUER_SERIALRef;
#endif

#if (OPENSSL_VERSION_NUMBER < 0x30000000L)
#define OPENSSL_CONST
#else
#define OPENSSL_CONST const
#endif

  void VOMSTrustList::AddElement(const std::vector<std::string>& encoded_list) {
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

  VOMSTrustList::VOMSTrustList(const std::vector<std::string>& encoded_list) {
    AddElement(encoded_list);
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
      if(*r) { delete *r; *r=NULL; }
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

    #define OBJCREATE(c,n) { \
      if(OBJ_create(c,n,#c) == 0) { \
        unsigned long __err = ERR_get_error(); \
        if(ERR_GET_REASON(__err) != OBJ_R_OID_EXISTS) { \
          CredentialLogger.msg(ERROR, \
                 "Failed to create OpenSSL object %s %s - %u %s", \
                 c, n, ERR_GET_REASON(__err), ERR_error_string(__err,NULL)); \
          return; \
        }; \
      }; \
    }

    #define OBJSETNID(v,n) { v = OBJ_txt2nid(n); if(v == NID_undef) CredentialLogger.msg(ERROR, "Failed to obtain OpenSSL identifier for %s", n); }

    X509V3_EXT_METHOD *vomsattribute_x509v3_ext_meth;


    OpenSSLInit();

    static std::mutex lock_;
    static bool done = false;

    std::unique_lock<std::mutex> lock(lock_);
    if (done) return;

    /* VOMS Attribute related objects*/
#if (OPENSSL_VERSION_NUMBER < 0x30400000L)
    OBJCREATE(idcenoRevAvailOID, "idcenoRevAvail");
    OBJCREATE(idceauthKeyIdentifierOID, "idceauthKeyIdentifier");
    OBJCREATE(idceTargetsOID, "idceTargets");

    vomsattribute_x509v3_ext_meth = VOMSAttribute_auth_x509v3_ext_meth();
    if (vomsattribute_x509v3_ext_meth) {
      OBJSETNID(vomsattribute_x509v3_ext_meth->ext_nid, idceauthKeyIdentifierOID);
      X509V3_EXT_add(vomsattribute_x509v3_ext_meth);
    }

    vomsattribute_x509v3_ext_meth = VOMSAttribute_avail_x509v3_ext_meth();
    if (vomsattribute_x509v3_ext_meth) {
      OBJSETNID(vomsattribute_x509v3_ext_meth->ext_nid, idcenoRevAvailOID);
      X509V3_EXT_add(vomsattribute_x509v3_ext_meth);
    }

    vomsattribute_x509v3_ext_meth = VOMSAttribute_targets_x509v3_ext_meth();
    if (vomsattribute_x509v3_ext_meth) {
      OBJSETNID(vomsattribute_x509v3_ext_meth->ext_nid, idceTargetsOID);
      X509V3_EXT_add(vomsattribute_x509v3_ext_meth);
    }
#endif

    OBJCREATE(idatcapOID,"idatcap");
    OBJCREATE(attributesOID,"attributes");
    OBJCREATE(acseqOID, "acseq");
    OBJCREATE(orderOID, "order");
    OBJCREATE(vomsOID, "voms");
    OBJCREATE(incfileOID, "incfile");
    OBJCREATE(voOID, "vo");
    OBJCREATE(certseqOID, "certseq");

    vomsattribute_x509v3_ext_meth = VOMSAttribute_acseq_x509v3_ext_meth();
    if (vomsattribute_x509v3_ext_meth) {
      OBJSETNID(vomsattribute_x509v3_ext_meth->ext_nid, acseqOID);
      X509V3_EXT_add(vomsattribute_x509v3_ext_meth);
    }

    vomsattribute_x509v3_ext_meth = VOMSAttribute_certseq_x509v3_ext_meth();
    if (vomsattribute_x509v3_ext_meth) {
      OBJSETNID(vomsattribute_x509v3_ext_meth->ext_nid, certseqOID);
      X509V3_EXT_add(vomsattribute_x509v3_ext_meth);
    }

    vomsattribute_x509v3_ext_meth = VOMSAttribute_attribs_x509v3_ext_meth();
    if (vomsattribute_x509v3_ext_meth) {
      OBJSETNID(vomsattribute_x509v3_ext_meth->ext_nid, attributesOID);
      X509V3_EXT_add(vomsattribute_x509v3_ext_meth);
    }

    if(!PersistentLibraryInit("modcredential")) {
      CredentialLogger.msg(WARNING, "Failed to lock arccredential library in memory");
    };
    done=true;

  }

  static int createVOMSAC(X509 *issuer, STACK_OF(X509) *issuerstack, X509 *holder,
                          EVP_PKEY *pkey, BIGNUM *serialnum,
                          std::vector<std::string> &fqan,
                          std::vector<std::string> &targets,
                          std::vector<std::string>& attrs,
                          AC *ac, std::string voname, std::string uri, int lifetime) {
    #define ERROR(e) do { err = (e); goto err; } while (0)
    AC *a = NULL;

    InitVOMSAttribute();

    if (!issuer || !holder || !serialnum || fqan.empty() || !ac || !pkey)
      return AC_ERR_PARAMETERS;


    int err = AC_ERR_UNKNOWN;
    time_t curtime;

    a = ac;
    Arc::Credential::X509_NAMERef subname(X509_NAME_dup(X509_get_issuer_name(holder))); //old or new version?
    Arc::Credential::X509_NAMERef issname(X509_NAME_dup(X509_get_subject_name(issuer)));

    time(&curtime);
    Arc::Credential::ASN1_GENERALIZEDTIMERef time1(ASN1_GENERALIZEDTIME_set(NULL, curtime));
    Arc::Credential::ASN1_GENERALIZEDTIMERef time2(ASN1_GENERALIZEDTIME_set(NULL, curtime+lifetime));

    AC_ATTRRef capabilities(AC_ATTR_new());
    AC_IETFATTRRef capnames(AC_IETFATTR_new());
    Arc::Credential::ASN1_OBJECTRef cobj(OBJ_txt2obj(idatcapOID,0));

    Arc::Credential::ASN1_INTEGERRef holdserial(ASN1_INTEGER_dup(X509_get_serialNumber(holder)));
    AC_FULL_ATTRIBUTESRef ac_full_attrs(AC_FULL_ATTRIBUTES_new());
    AC_ATT_HOLDERRef ac_att_holder(AC_ATT_HOLDER_new());

    std::string buffer, complete;

    Arc::Credential::X509_ALGORRef alg1;
    Arc::Credential::X509_ALGORRef alg2;
    Arc::Credential::ASN1_BIT_STRINGRef uid;

#if (OPENSSL_VERSION_NUMBER < 0x30400000L)
    Arc::Credential::X509_EXTENSIONRef norevavail;
    Arc::Credential::X509_EXTENSIONRef targetsext;
    Arc::Credential::X509_EXTENSIONRef auth;
    Arc::Credential::X509_EXTENSIONRef certstack;

    X509V3_CTX extctx;
    X509V3_set_ctx(&extctx, issuer, NULL, NULL, NULL, 0);

    Arc::Credential::ASN1_INTEGERRef serial(BN_to_ASN1_INTEGER(serialnum, NULL));
    Arc::Credential::ASN1_INTEGERRef version(BN_to_ASN1_INTEGER((BIGNUM *)(BN_value_one()), NULL));

    if (!subname || !issuer || !holdserial || !serial ||
      !capabilities || !cobj || !capnames || !time1 || !time2 || !ac_full_attrs || !ac_att_holder)
      ERROR(AC_ERR_MEMORY);

    for (std::vector<std::string>::iterator i = targets.begin(); i != targets.end(); i++) {
      if (i == targets.begin()) complete = (*i);
      else complete.append(",").append(*i);
    }

    // prepare AC_IETFATTR
    for (std::vector<std::string>::iterator i = fqan.begin(); i != fqan.end(); i++) {
      AC_IETFATTRVALRef tmpc(AC_IETFATTRVAL_new());
      if (!tmpc) {
        ERROR(AC_ERR_MEMORY);
      }
      tmpc->value.octet_string = ASN1_OCTET_STRING_new();
      if(!tmpc->value.octet_string)
        ERROR(AC_ERR_MEMORY);
      
      tmpc->type = V_ASN1_OCTET_STRING;

      CredentialLogger.msg(DEBUG,"VOMS: create FQAN: %s",*i);

      ASN1_OCTET_STRING_set(tmpc->value.octet_string, (const unsigned char*)((*i).c_str()), (*i).length());

      if(!capnames->values) capnames->values = sk_AC_IETFATTRVAL_new_null();
      if(!capnames->values)
        ERROR(AC_ERR_MEMORY);

      sk_AC_IETFATTRVAL_push(capnames->values, tmpc.release());
    }

    buffer.append(voname);
    buffer.append("://");
    buffer.append(uri);
    {
      Arc::Credential::GENERAL_NAMERef g(GENERAL_NAME_new());
      Arc::Credential::ASN1_IA5STRINGRef tmpr(ASN1_IA5STRING_new());
      if (!tmpr || !g)
        ERROR(AC_ERR_MEMORY);

      ASN1_STRING_set(tmpr, buffer.c_str(), buffer.size());
      g->type  = GEN_URI;
      g->d.ia5 = tmpr.release();

      if(!capnames->names) capnames->names = sk_GENERAL_NAME_new_null();
      if(!capnames->names)
        ERROR(AC_ERR_MEMORY);

      sk_GENERAL_NAME_push(capnames->names, g.release());
    }

    // stuff the created AC_IETFATTR in ietfattr (values) and define its object
    if(!capabilities->ietfattr) capabilities->ietfattr = sk_AC_IETFATTR_new_null();
    if(!capabilities->ietfattr)
        ERROR(AC_ERR_MEMORY);
    sk_AC_IETFATTR_push(capabilities->ietfattr, capnames.release());
    ASN1_OBJECT_free(capabilities->type);
    capabilities->type = cobj.release();

    // prepare AC_FULL_ATTRIBUTES
    for (std::vector<std::string>::iterator i = attrs.begin(); i != attrs.end(); i++) {
      std::string qual, name, value;

      CredentialLogger.msg(DEBUG,"VOMS: create attribute: %s",*i);

      AC_ATTRIBUTERef ac_attr(AC_ATTRIBUTE_new());
      if (!ac_attr)
        ERROR(AC_ERR_MEMORY);

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
      } else {
        name = (*i).substr(pos, pos1 - pos);
        value = (*i).substr(pos1 + 1);
      }

      if (!qual.empty()) {
        ASN1_OCTET_STRING_set(ac_attr->qualifier, (const unsigned char*)(qual.c_str()), qual.length());
      } else {
        ASN1_OCTET_STRING_set(ac_attr->qualifier, (const unsigned char*)(voname.c_str()), voname.length());
      }

      ASN1_OCTET_STRING_set(ac_attr->name, (const unsigned char*)(name.c_str()), name.length());
      ASN1_OCTET_STRING_set(ac_attr->value, (const unsigned char*)(value.c_str()), value.length());

      if(!ac_att_holder->attributes) ac_att_holder->attributes = sk_AC_ATTRIBUTE_new_null();
      if(!ac_att_holder->attributes)
        ERROR(AC_ERR_MEMORY);
      sk_AC_ATTRIBUTE_push(ac_att_holder->attributes, ac_attr.release());
    }

    if (attrs.empty()) {
      ac_att_holder.reset();
    } else {
      Arc::Credential::GENERAL_NAMERef g(GENERAL_NAME_new());
      Arc::Credential::ASN1_IA5STRINGRef tmpr(ASN1_IA5STRING_new());
      if (!tmpr || !g)
        ERROR(AC_ERR_MEMORY);

      std::string buffer(voname);
      buffer.append("://");
      buffer.append(uri);

      ASN1_STRING_set(tmpr, buffer.c_str(), buffer.length());
      g->type  = GEN_URI;
      g->d.ia5 = tmpr.release();
      if(!ac_att_holder->grantor) ac_att_holder->grantor = sk_GENERAL_NAME_new_null();
      if(!ac_att_holder->grantor)
        ERROR(AC_ERR_MEMORY);
      sk_GENERAL_NAME_push(ac_att_holder->grantor, g.release());
      if(!ac_full_attrs->providers) ac_full_attrs->providers = sk_AC_ATT_HOLDER_new_null();
      if(!ac_full_attrs->providers)
        ERROR(AC_ERR_MEMORY);
      sk_AC_ATT_HOLDER_push(ac_full_attrs->providers, ac_att_holder.release());
    }

    // push both AC_ATTR into STACK_OF(AC_ATTR)
    if(!a->acinfo->attrib) a->acinfo->attrib = sk_AC_ATTR_new_null();
    if(!a->acinfo->attrib) ERROR(AC_ERR_MEMORY);
    sk_AC_ATTR_push(a->acinfo->attrib, capabilities.release());

    if (ac_full_attrs) {
      Arc::Credential::X509_EXTENSIONRef ext(X509V3_EXT_conf_nid(NULL, &extctx, OBJ_txt2nid(attributesOID), (char *)(ac_full_attrs->providers)));
      ac_full_attrs.reset();
      if (!ext)
        ERROR(AC_ERR_NO_EXTENSION);

      if(!a->acinfo->exts) a->acinfo->exts = sk_X509_EXTENSION_new_null();
      if(!a->acinfo->exts) ERROR(AC_ERR_MEMORY);
      sk_X509_EXTENSION_push(a->acinfo->exts, ext.release());
    }

    {
      Arc::Credential::X509StackRef stk(sk_X509_new_null());
      sk_X509_push(stk, X509_dup(issuer));

      if (issuerstack) {
        for (int j =0; j < sk_X509_num(issuerstack); j++) {
          sk_X509_push(stk, X509_dup(sk_X509_value(issuerstack, j)));
        }
      }

      //for(int i=0; i<sk_X509_num(stk); i++)
      //  fprintf(stderr, "stk[%i] = %d  %s\n", i , sk_X509_value(stk, i),
      //  X509_NAME_oneline(X509_get_subject_name((X509 *)sk_X509_value(stk, i)), NULL, 0));

      certstack = X509V3_EXT_conf_nid(NULL, &extctx, OBJ_txt2nid(certseqOID), (char*)stk.get());
    }

    // Create extensions
    norevavail = X509V3_EXT_conf_nid(NULL, &extctx, OBJ_txt2nid(idcenoRevAvailOID), (char*)"loc");
    if (!norevavail)
      ERROR(AC_ERR_NO_EXTENSION);
    X509_EXTENSION_set_critical(norevavail, 0);

    auth = X509V3_EXT_conf_nid(NULL, &extctx, OBJ_txt2nid(idceauthKeyIdentifierOID), (char *)"keyid,issuer");
    if (!auth)
      ERROR(AC_ERR_NO_EXTENSION);
    X509_EXTENSION_set_critical(auth, 0);

    if (!complete.empty()) {
      targetsext = X509V3_EXT_conf_nid(NULL, &extctx, OBJ_txt2nid(idceTargetsOID), (char*)(complete.c_str()));
      if (!targetsext)
        ERROR(AC_ERR_NO_EXTENSION);
      X509_EXTENSION_set_critical(targetsext,1);
    }

    if(a->acinfo->exts == NULL) {
      a->acinfo->exts = sk_X509_EXTENSION_new_null();
      if(a->acinfo->exts == NULL) ERROR(AC_ERR_NO_EXTENSION);
    }
    if(sk_X509_EXTENSION_push(a->acinfo->exts, norevavail)) norevavail.release();
    if(sk_X509_EXTENSION_push(a->acinfo->exts, auth)) auth.release();
    if(certstack) {
      if(sk_X509_EXTENSION_push(a->acinfo->exts, certstack)) certstack.release();
    }
    if(targetsext) {
      if(sk_X509_EXTENSION_push(a->acinfo->exts, targetsext)) targetsext.release();
    }

    {
    X509_ALGOR* alg = (X509_ALGOR*)X509_get0_tbs_sigalg(issuer);
    if(alg) alg1 = X509_ALGOR_dup(alg);
    alg = nullptr;
    X509_get0_signature(NULL, (X509_ALGOR const**)&alg, issuer);
    if(alg) alg2 = X509_ALGOR_dup(alg);
    }

    {
      const ASN1_BIT_STRING* issuerUID = NULL;
      X509_get0_uids(issuer, &issuerUID, NULL);
      if (issuerUID)
        if (!(uid = ASN1_STRING_dup(issuerUID)))
          ERROR(AC_ERR_MEMORY);
    }

    if(a->acinfo->holder->baseid == NULL) a->acinfo->holder->baseid = AC_IS_new(); // optional
    if(a->acinfo->form == NULL) a->acinfo->form = AC_FORM_new(); // optional

    if(subname) {
      Arc::Credential::GENERAL_NAMERef dirn1(GENERAL_NAME_new());
      if(!dirn1) ERROR(AC_ERR_MEMORY);
      dirn1->d.dirn = subname.release();
      dirn1->type = GEN_DIRNAME;
      if(!a->acinfo->holder->baseid->issuer) a->acinfo->holder->baseid->issuer = sk_GENERAL_NAME_new_null();
      if(!a->acinfo->holder->baseid->issuer) ERROR(AC_ERR_MEMORY);
      sk_GENERAL_NAME_push(a->acinfo->holder->baseid->issuer, dirn1.release());
    }

    if(issname) {
      Arc::Credential::GENERAL_NAMERef dirn2(GENERAL_NAME_new());
      if(!dirn2) ERROR(AC_ERR_MEMORY);
      dirn2->d.dirn = issname.release();
      dirn2->type = GEN_DIRNAME;
      if(!a->acinfo->form->names) a->acinfo->form->names = sk_GENERAL_NAME_new_null();
      if(!a->acinfo->form->names) ERROR(AC_ERR_MEMORY);
      sk_GENERAL_NAME_push(a->acinfo->form->names, dirn2.release());
    }

    if(holdserial) {
      ASN1_INTEGER_free(a->acinfo->holder->baseid->serial);
      a->acinfo->holder->baseid->serial = holdserial.release();
    }

    if(serial) {
      ASN1_INTEGER_free(a->acinfo->serial);
      a->acinfo->serial = serial.release();
    }

    if(version) {
      ASN1_INTEGER_free(a->acinfo->version);
      a->acinfo->version = version.release();
    }

    if(time1) {
      ASN1_GENERALIZEDTIME_free(a->acinfo->validity->notBefore);
      a->acinfo->validity->notBefore = time1.release();
    }

    if(time2) {
      ASN1_GENERALIZEDTIME_free(a->acinfo->validity->notAfter);
      a->acinfo->validity->notAfter  = time2.release();
    }

    if (uid) {
      ASN1_BIT_STRING_free(a->acinfo->id);
      a->acinfo->id = uid.release();
    }

    if(alg1) {
      X509_ALGOR_free(a->acinfo->alg);
      a->acinfo->alg = alg1.release();
    }

    if(alg2) {
      X509_ALGOR_free(a->sig_alg);
      a->sig_alg = alg2.release();
    }

    ASN1_sign((int (*)(OPENSSL_CONST void*, unsigned char**))i2d_AC_INFO, a->acinfo->alg, a->sig_alg, a->signature,
            (char *)a->acinfo, pkey, EVP_sha256());

    err = 0;

err:


#else // (OPENSSL_VERSION_NUMBER < 0x30400000L)


    Arc::Credential::ASN1_STRINGRef asnitem(ASN1_STRING_new());

    if (!subname || !issuer || !holdserial ||
      !capabilities || !cobj || !asnitem ||  !capnames || !time1 || !time2 || !ac_full_attrs || !ac_att_holder)
      ERROR(AC_ERR_MEMORY);

    for (std::vector<std::string>::iterator i = targets.begin(); i != targets.end(); i++) {
      if (i == targets.begin()) complete = (*i);
      else complete.append(",").append(*i);
    }

    // prepare AC_IETFATTR
    for (std::vector<std::string>::iterator i = fqan.begin(); i != fqan.end(); i++) {
      Arc::Credential::ASN1_OCTET_STRINGRef octet_string(ASN1_OCTET_STRING_new());
      if(!octet_string)
        ERROR(AC_ERR_MEMORY);

      CredentialLogger.msg(DEBUG,"VOMS: create FQAN: %s",*i);

      ASN1_OCTET_STRING_set(octet_string, (const unsigned char*)((*i).c_str()), (*i).length());

      // The name if this function is misleading. Despite named add1 no copy of octet_string is made.
      if(!OSSL_IETF_ATTR_SYNTAX_add1_value(capnames, OSSL_IETFAS_OCTETS, octet_string))
        ERROR(AC_ERR_MEMORY);

      octet_string.release();
    }

    buffer.append(voname);
    buffer.append("://");
    buffer.append(uri);
    {
      Arc::Credential::GENERAL_NAMESRef names(GENERAL_NAMES_new());
      Arc::Credential::GENERAL_NAMERef g(GENERAL_NAME_new());
      Arc::Credential::ASN1_IA5STRINGRef tmpr(ASN1_IA5STRING_new());
      if (!names || !tmpr || !g)
        ERROR(AC_ERR_MEMORY);
 
      ASN1_STRING_set(tmpr, buffer.c_str(), buffer.size());
      GENERAL_NAME_set0_value(g, GEN_URI, tmpr.release());
      sk_GENERAL_NAME_push(names, g.release());
      OSSL_IETF_ATTR_SYNTAX_set0_policyAuthority(capnames, names.release());
    }

    if(!X509_ATTRIBUTE_set1_object(capabilities, cobj))
      ERROR(AC_ERR_NO_EXTENSION);

    {
      unsigned char * asndata = NULL;
      int asnlen = i2d_OSSL_IETF_ATTR_SYNTAX(capnames, &asndata);
      if(asnlen < 0)
        ERROR(AC_ERR_MEMORY);
      ASN1_STRING_set0(asnitem, asndata, asnlen);
    }

    if(!X509_ATTRIBUTE_set1_data(capabilities, V_ASN1_SEQUENCE, asnitem, -1))
      ERROR(AC_ERR_NO_EXTENSION);

    // prepare AC_FULL_ATTRIBUTES
    for (std::vector<std::string>::iterator i = attrs.begin(); i != attrs.end(); i++) {
      std::string qual, name, value;

      CredentialLogger.msg(DEBUG,"VOMS: create attribute: %s",*i);

      AC_ATTRIBUTERef ac_attr(AC_ATTRIBUTE_new());
      if (!ac_attr)
        ERROR(AC_ERR_MEMORY);

      //Accoding to the definition of voms, the attributes will be like "qualifier::name=value" or "::name=value"
      size_t pos =(*i).find_first_of("::");
      if (pos != std::string::npos) {
        qual = (*i).substr(0, pos);
        pos += 2;
      }
      else { pos = 2; }

      size_t pos1 = (*i).find_first_of("=");
      if (pos1 == std::string::npos)
        ERROR(AC_ERR_PARAMETERS);
 
      name = (*i).substr(pos, pos1 - pos);
      value = (*i).substr(pos1 + 1);

      if (!qual.empty()) {
        ASN1_OCTET_STRING_set(ac_attr->qualifier, (const unsigned char*)(qual.c_str()), qual.length());
      } else {
        ASN1_OCTET_STRING_set(ac_attr->qualifier, (const unsigned char*)(voname.c_str()), voname.length());
      }

      ASN1_OCTET_STRING_set(ac_attr->name, (const unsigned char*)(name.c_str()), name.length());
      ASN1_OCTET_STRING_set(ac_attr->value, (const unsigned char*)(value.c_str()), value.length());

      if(!ac_att_holder->attributes) ac_att_holder->attributes = sk_AC_ATTRIBUTE_new_null();
      if(!ac_att_holder->attributes) ERROR(AC_ERR_MEMORY);
      sk_AC_ATTRIBUTE_push(ac_att_holder->attributes, ac_attr.release());
    }

    if (attrs.empty()) {
      ac_att_holder.reset();
    } else {
      Arc::Credential::GENERAL_NAMERef g(GENERAL_NAME_new());
      Arc::Credential::ASN1_IA5STRINGRef tmpr(ASN1_IA5STRING_new());
      if (!tmpr || !g)
        ERROR(AC_ERR_MEMORY);

      std::string buffer(voname);
      buffer.append("://");
      buffer.append(uri);

      ASN1_STRING_set(tmpr, buffer.c_str(), buffer.length());
      g->type  = GEN_URI;
      g->d.ia5 = tmpr.release();
      if(!ac_att_holder->grantor) ac_att_holder->grantor = sk_GENERAL_NAME_new_null();
      if(!ac_att_holder->grantor) ERROR(AC_ERR_MEMORY);
      sk_GENERAL_NAME_push(ac_att_holder->grantor, g.release());
      if(!ac_full_attrs->providers) ac_full_attrs->providers = sk_AC_ATT_HOLDER_new_null();
      if(!ac_full_attrs->providers) ERROR(AC_ERR_MEMORY);
      sk_AC_ATT_HOLDER_push(ac_full_attrs->providers, ac_att_holder.release());
    }

    // push both AC_ATTR into STACK_OF(AC_ATTR)
    if(!X509_ACERT_add1_attr(a, capabilities))
      ERROR(AC_ERR_NO_EXTENSION);

    if (ac_full_attrs) {
      if(!X509_ACERT_add1_ext_i2d(a, OBJ_txt2nid(attributesOID), ac_full_attrs, 0, X509V3_ADD_APPEND))
        ERROR(AC_ERR_NO_EXTENSION);
    }

    if (!complete.empty()) {
      Arc::Credential::ASN1_IA5STRINGRef ia5(ASN1_IA5STRING_new());
      if (!ia5)
        ERROR(AC_ERR_MEMORY);
      ASN1_STRING_set(ia5, complete.c_str(), -1);
      Arc::Credential::GENERAL_NAMERef name(GENERAL_NAME_new());
      if (!name)
        ERROR(AC_ERR_MEMORY);
      GENERAL_NAME_set0_value(name, GEN_URI, ia5.release());
      OSSL_TARGETRef target(OSSL_TARGET_new());
      if (!target)
        ERROR(AC_ERR_MEMORY);
      target->type = OSSL_TGT_TARGET_NAME;
      target->choice.targetName = name.release();
      OSSL_TARGETSRef targets(OSSL_TARGETS_new());
      if (!targets)
        ERROR(AC_ERR_MEMORY);
      sk_OSSL_TARGET_push(targets, target.release());
      OSSL_TARGETING_INFORMATIONRef targetInfo(OSSL_TARGETING_INFORMATION_new());
      if (!targetInfo)
        ERROR(AC_ERR_MEMORY);
      sk_OSSL_TARGETS_push(targetInfo, targets.release());
      (void)X509_ACERT_add1_ext_i2d(a, NID_target_information, targetInfo, 1, X509V3_ADD_APPEND);
    }

    {
      Arc::Credential::ASN1_NULLRef norevav(ASN1_NULL_new());
      if (!norevav)
        ERROR(AC_ERR_MEMORY);
      (void)X509_ACERT_add1_ext_i2d(a, NID_no_rev_avail, norevav, 0, X509V3_ADD_APPEND);
    }

    {
      AUTHORITY_KEYIDRef key(AUTHORITY_KEYID_new());
      Arc::Credential::GENERAL_NAMERef name;
      (void)GENERAL_NAME_set1_X509_NAME(&name.put(),X509_get_subject_name(issuer));
      if (!key || !name)
        ERROR(AC_ERR_MEMORY);
      key->issuer = GENERAL_NAMES_new();
      if(!key->issuer)
        ERROR(AC_ERR_MEMORY);
      sk_GENERAL_NAME_push(key->issuer, name.release());
      key->serial = ASN1_INTEGER_dup(X509_get_serialNumber(issuer));
      (void)X509_ACERT_add1_ext_i2d(a, NID_authority_key_identifier, key, 0, X509V3_ADD_APPEND);
    }

    {
      Arc::Credential::X509StackRef stk(sk_X509_new_null());
      if (!stk)
        ERROR(AC_ERR_MEMORY);
      sk_X509_push(stk, X509_dup(issuer));
      if (issuerstack) {
        for (int j =0; j < sk_X509_num(issuerstack); j++) {
          sk_X509_push(stk, X509_dup(sk_X509_value(issuerstack, j)));
        }
      }
      AC_CERTSRef accerts(AC_CERTS_new());
      if (!accerts)
        ERROR(AC_ERR_MEMORY);
      accerts->stackcert = stk.release();
      (void)X509_ACERT_add1_ext_i2d(a, OBJ_txt2nid(certseqOID), accerts, 0, X509V3_ADD_APPEND);
    }

    {
    X509_ALGOR* alg = (X509_ALGOR*)X509_get0_tbs_sigalg(issuer);
    if(alg) alg1 = X509_ALGOR_dup(alg);
    alg = nullptr;
    X509_get0_signature(NULL, (X509_ALGOR const**)&alg, issuer);
    if(alg) alg2 = X509_ALGOR_dup(alg);
    }

    {
      const ASN1_BIT_STRING* issuerUID = NULL;
      X509_get0_uids(issuer, &issuerUID, NULL);
      if (issuerUID)
        if (!(uid = ASN1_STRING_dup(issuerUID)))
          ERROR(AC_ERR_MEMORY);
    }

    if(subname || holdserial || uid) {
      OSSL_ISSUER_SERIALRef issuerSerial(OSSL_ISSUER_SERIAL_new());
      if (!issuerSerial)
        ERROR(AC_ERR_MEMORY);
      if(subname)
        OSSL_ISSUER_SERIAL_set1_issuer(issuerSerial, subname);
      if(holdserial)
        OSSL_ISSUER_SERIAL_set1_serial(issuerSerial, holdserial);
      if(uid)
        OSSL_ISSUER_SERIAL_set1_issuerUID(issuerSerial, uid); // wrong uid, must be X509_ACERT_set1_issuerUID
      X509_ACERT_set0_holder_baseCertId(a, issuerSerial.release());
    }

    if(issname) {
      X509_ACERT_set1_issuerName(a, issname);
    }

    if(serialnum) {
      Arc::Credential::ASN1_INTEGERRef num(BN_to_ASN1_INTEGER(serialnum, NULL));
      if (!num)
        ERROR(AC_ERR_MEMORY);
      X509_ACERT_set1_serialNumber(a, num);
    }

    if(time1) {
      X509_ACERT_set1_notBefore(a, time1);
    }

    if(time2) {
      X509_ACERT_set1_notAfter(a, time2);
    }

    if (uid) {
      // const_cast hack due to missing set method
      const ASN1_BIT_STRING* bstr = X509_ACERT_get0_issuerUID(a);
      if(bstr)
        ASN1_BIT_STRING_set(const_cast<ASN1_BIT_STRING*>(bstr), const_cast<unsigned char*>(ASN1_STRING_get0_data(uid)), ASN1_STRING_length(uid));
    }

    if(alg1) {
      // const_cast hack due to missing set method
      const X509_ALGOR* sig_alg = X509_ACERT_get0_info_sigalg(a);
      if (sig_alg)
        X509_ALGOR_copy(const_cast<X509_ALGOR*>(sig_alg), alg1);
    }

    if(alg2) {
      // const_cast hack due to missing set method
      const X509_ALGOR* sig_alg = NULL;
      X509_ACERT_get0_signature(a, NULL, &sig_alg);
      if(sig_alg)
        X509_ALGOR_copy(const_cast<X509_ALGOR*>(sig_alg), alg2);
    }

    X509_ACERT_sign(a, pkey, EVP_sha256());

    err = 0;

err:


#endif // (OPENSSL_VERSION_NUMBER < 0x30400000L)


    return err;
  }

  bool createVOMSAC(std::string &codedac, Credential &issuer_cred, Credential &holder_cred,
             std::vector<std::string> &fqan, std::vector<std::string> &targets,
             std::vector<std::string>& attributes, std::string &voname, std::string &uri, int lifetime) {

    Arc::Credential::X509Ref issuer(issuer_cred.GetCert());
    Arc::Credential::X509StackRef issuerchain(issuer_cred.GetCertChain());
    Arc::Credential::EVP_PKEYRef issuerkey(issuer_cred.GetPrivKey());
    Arc::Credential::X509Ref holder(holder_cred.GetCert());

    ACRef ac(AC_new());

    if(createVOMSAC(issuer, issuerchain, holder, issuerkey, (BIGNUM *)(BN_value_one()),
             fqan, targets, attributes, ac, voname, uri, lifetime) != 0){
      return false;
    }

    unsigned int len = i2d_AC(ac, NULL);
    unsigned char *tmp = (unsigned char *)OPENSSL_malloc(len);

    if (tmp) {
      unsigned char *ttmp = tmp;
      i2d_AC(ac, &ttmp);
      //codedac = std::string((char *)tmp, len);
      codedac.append((const char*)tmp, len);
    }
    OPENSSL_free(tmp);

    return true;
  }


  bool addVOMSAC(AC** &aclist, std::string &acorder, std::string &codedac) {
    InitVOMSAttribute();

    if(codedac.empty()) return true;
    int l = codedac.size();

    std::unique_ptr<unsigned char,void (&)(void*)> pp((unsigned char *)malloc(codedac.size()),free);
    if(!pp) {
      CredentialLogger.msg(ERROR,"VOMS: Can not allocate memory for parsing AC");
      return false;
    }
    memcpy(pp.get(), codedac.data(), l);

    Arc::Credential::BIGNUMRef dataorder(BN_new());
    if (!dataorder) {
      CredentialLogger.msg(ERROR,"VOMS: Can not allocate memory for storing the order of AC");
      return false;
    }
    BN_one(dataorder);

    //Parse the AC, and insert it into an AC list
    unsigned char const* p = pp.get();
    AC* received_ac = NULL;
    if((received_ac = d2i_AC(NULL, &p, l))) {
      AC** actmplist = (AC **)listadd((char **)aclist, (char *)received_ac, sizeof(AC *));
      if (actmplist) {
        aclist = actmplist;
        (void)BN_lshift1(dataorder, dataorder);
        (void)BN_set_bit(dataorder, 0);
        char *buffer = BN_bn2hex(dataorder);
        if(buffer) acorder = std::string(buffer);
        OPENSSL_free(buffer);
      } else {
        listfree((char **)aclist, (freefn)AC_free);
        return false;
      }
    } else {
      CredentialLogger.msg(ERROR,"VOMS: Can not parse AC");
      return false;
    }
    return true;
  }

  static int cb(int ok, X509_STORE_CTX *ctx) {
    if (!ok) {
      if (X509_STORE_CTX_get_error(ctx) == X509_V_ERR_CERT_HAS_EXPIRED) ok=1;
      /* since we are just checking the certificates, it is
       * ok if they are self signed. But we should still warn
       * the user.
       */
      if (X509_STORE_CTX_get_error(ctx) == X509_V_ERR_DEPTH_ZERO_SELF_SIGNED_CERT) ok=1;
      /* Continue after extension errors too */
      if (X509_STORE_CTX_get_error(ctx) == X509_V_ERR_INVALID_CA) ok=1;
      if (X509_STORE_CTX_get_error(ctx) == X509_V_ERR_PATH_LENGTH_EXCEEDED) ok=1;
      if (X509_STORE_CTX_get_error(ctx) == X509_V_ERR_CERT_CHAIN_TOO_LONG) ok=1;
      if (X509_STORE_CTX_get_error(ctx) == X509_V_ERR_DEPTH_ZERO_SELF_SIGNED_CERT) ok=1;
    }
    return(ok);
  }

  static bool checkCert(STACK_OF(X509) *stack, const std::string& ca_cert_dir, const std::string& ca_cert_file, bool system_ca_dir, bool system_ca_file) {
    int index = 0;

    if(ca_cert_dir.empty() && ca_cert_file.empty() && !system_ca_dir && !system_ca_file) {
      CredentialLogger.msg(ERROR,"VOMS: CA directory or CA file must be provided or default setting enabled");
      return false;
    }

    Arc::Credential::X509_STORERef ctx(X509_STORE_new());
    if (ctx) {
      X509_STORE_set_verify_cb_func(ctx,cb);
//#ifdef SIGPIPE
//      signal(SIGPIPE,SIG_IGN);
//#endif
//      CRYPTO_malloc_init();

      if(system_ca_dir) {
        X509_LOOKUP* lookup = X509_STORE_add_lookup(ctx, X509_LOOKUP_hash_dir());
        if (lookup) X509_LOOKUP_add_dir(lookup, NULL, X509_FILETYPE_DEFAULT);
      }
      if(system_ca_file) {




      }
      X509_LOOKUP *lookup = NULL;
      if (!(ca_cert_dir.empty()) && (lookup = X509_STORE_add_lookup(ctx,X509_LOOKUP_hash_dir()))) {
        X509_LOOKUP_add_dir(lookup, ca_cert_dir.c_str(), X509_FILETYPE_PEM);
      }
      if (!(ca_cert_file.empty()) && (lookup = X509_STORE_add_lookup(ctx, X509_LOOKUP_file()))) {
        X509_LOOKUP_load_file(lookup, ca_cert_file.c_str(), X509_FILETYPE_PEM);
      }
      //Check the AC issuer certificate's chain
      for (int i = sk_X509_num(stack)-1; i >=0; i--) {
        Arc::Credential::X509_STORE_CTXRef csc(X509_STORE_CTX_new());
        if (csc) {
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
          if(X509_STORE_CTX_init(csc, ctx, sk_X509_value(stack, i), NULL)) {
            index = X509_verify_cert(csc);
          }
          if(!index) break;
          //If the 'i'th certificate is verified, then add it as trusted certificate,
          //then 'i'th certificate will be used as 'trusted certificate' to check
          //the 'i-1'th certificate
          X509_STORE_add_cert(ctx,sk_X509_value(stack, i));
        }
      }
    }

    return (index != 0);
  }

  static bool checkSigAC(X509* cert, AC* ac){
    if (!cert || !ac) return false;

    Arc::Credential::EVP_PKEYRef key(X509_extract_key(cert));
    if (!key) return false;

    int res = 0;
#if (OPENSSL_VERSION_NUMBER < 0x30400000L)
    res = ASN1_verify((int (*)(OPENSSL_CONST void*, unsigned char**))i2d_AC_INFO, ac->sig_alg, ac->signature,
                        (char *)ac->acinfo, key);
#else
    res = X509_ACERT_verify(ac, key);
#endif

    if (!res) CredentialLogger.msg(ERROR,"VOMS: failed to verify AC signature");

    return (res == 1);
  }

  static std::string x509name2ascii(X509_NAME const * name) {
    std::string str;
    if(name!=NULL) {
      char* buf = X509_NAME_oneline(name, NULL, 0);
      if(buf) {
        str.append(buf);
        OPENSSL_free(buf);
      }
    }
    return str;
  }

  static bool checkTrust(const VOMSTrustChain& chain,STACK_OF(X509)* certstack) {
    int n = 0;
    X509 *current = NULL;
    //A trusted chain is like following:
    // /O=Grid/O=NorduGrid/CN=host/arthur.hep.lu.se
    // /O=Grid/O=NorduGrid/CN=NorduGrid Certification Authority
    // ----NEXT CHAIN----
    if(chain.size()-1 > (sk_X509_num(certstack)+1)) return false;

    for(int i = 0; i< chain.size(); i++)
      CredentialLogger.msg(VERBOSE, "VOMS: trust chain to check: %s ", chain[i]);

    for(;n < sk_X509_num(certstack);++n) {
      if((n+1) >= chain.size()) return true;
      current = sk_X509_value(certstack,n);
      if(!current) return false;
      std::string sub_name = x509name2ascii(X509_get_subject_name(current));
      if(chain[n] != sub_name) {
        CredentialLogger.msg(VERBOSE,"VOMS: the DN in certificate: %s does not match that in trusted DN list: %s",
          sub_name, chain[n]);
        return false;
      }
      std::string iss_name = x509name2ascii(X509_get_issuer_name(current));
      if(chain[n+1] != iss_name) {
        CredentialLogger.msg(VERBOSE,"VOMS: the Issuer identity in certificate: %s does not match that in trusted DN list: %s",
          iss_name, chain[n+1]);
        return false;
      }
    }

    return true;
  }

  static bool checkTrust(const RegularExpression& reg,STACK_OF(X509)* certstack) {
    if(sk_X509_num(certstack) <= 0) return false;
    X509 *current = sk_X509_value(certstack,0);
    std::string subject = x509name2ascii(X509_get_subject_name(current));
    std::string issuer = x509name2ascii(X509_get_issuer_name(current));
    std::list<std::string> unmatched, matched;
    return (reg.match(subject,unmatched,matched) && reg.match(issuer,unmatched,matched));

  }

  /* Get the DNs chain from relative *.lsc file.
   * The location of .lsc file is path: $vomsdir/<VO>/<hostname>.lsc
   */
  static bool getLSC(const std::string& vomsdir, const std::string& voname, const std::string& hostname, std::vector<std::string>& vomscert_trust_dn) {
    std::string lsc_loc = vomsdir + G_DIR_SEPARATOR_S + voname + G_DIR_SEPARATOR_S + hostname + ".lsc";
    if (!Glib::file_test(lsc_loc, Glib::FILE_TEST_IS_REGULAR)) {
      CredentialLogger.msg(INFO, "VOMS: The lsc file %s does not exist", lsc_loc);
      return false;
    }
    std::string trustdn_str;
    std::ifstream in(lsc_loc.c_str(), std::ios::in);
    if (!in) {
      CredentialLogger.msg(ERROR, "VOMS: The lsc file %s can not be open", lsc_loc);
      return false;
    }
    std::getline<char>(in, trustdn_str, 0);
    in.close();
    tokenize(trustdn_str, vomscert_trust_dn, "\n");
    return true;
  }

  static bool checkSignature(AC* ac,
    const std::string vomsdir, const std::string& voname, const std::string& hostname, 
    const std::string& ca_cert_dir, const std::string& ca_cert_file, bool system_ca_dir, bool system_ca_file,
    VOMSTrustList& vomscert_trust_dn, 
    X509*& issuer_cert, unsigned int& status, bool verify) {

    bool res = true;
    issuer_cert = NULL;
    Arc::Credential::X509Ref issuer;

    int nid = OBJ_txt2nid(certseqOID);
#if (OPENSSL_VERSION_NUMBER < 0x30400000L)
    const STACK_OF(X509_EXTENSION) *exts = ac->acinfo->exts;
#else
    const STACK_OF(X509_EXTENSION) *exts = X509_ACERT_get0_extensions(ac);
#endif
    int pos = X509v3_get_ext_by_NID(exts, nid, -1);
    if (pos >= 0) {
      //Check if the DN/CA file is installed for a given VO.
      X509_EXTENSION* ext = sk_X509_EXTENSION_value(exts, pos);
      if(!ext) {
        // X509 parsing error
        status |= VOMSACInfo::X509ParsingFailed;
        return false;
      }
      AC_CERTSRef certs((AC_CERTS *)X509V3_EXT_d2i(ext));
      if(!certs) {
        // X509 parsing error
        status |= VOMSACInfo::X509ParsingFailed;
        return false;
      }
      //The relatively new version of VOMS server is supposed to
      //create AC which includes the certificate stack:
      //the certificate of voms server; the non-CA certificate/s
      //(if there are) that signs the voms server' certificate.
      STACK_OF(X509)* certstack = certs->stackcert;

      if(verify) {
        bool trust_success = false;
        bool lsc_check = false;
        if((vomscert_trust_dn.SizeChains()==0) && (vomscert_trust_dn.SizeRegexs()==0)) {
          std::vector<std::string> voms_trustdn;
          if(!getLSC(vomsdir, voname, hostname, voms_trustdn)) {
            CredentialLogger.msg(WARNING,"VOMS: there is no constraints of trusted voms DNs, the certificates stack in AC will not be checked.");
            trust_success = true;
            status |= VOMSACInfo::TrustFailed;
            status |= VOMSACInfo::LSCFailed;
          }
          else {
            vomscert_trust_dn.AddElement(voms_trustdn);
            lsc_check = true;
            //lsc checking only happens if the VOMSTrustList argument is empty.
          }
        }

        //Check if the DN of those certificates in the certificate stack
        //corresponds to the trusted DN chain in the configuration
        if(certstack && !trust_success) {
          for(int n = 0;n < vomscert_trust_dn.SizeChains();++n) {
            const VOMSTrustChain& chain = vomscert_trust_dn.GetChain(n);
            if(checkTrust(chain,certstack)) {
              trust_success = true;
              break;
            }
          }
          if(!trust_success) for(int n = 0;n < vomscert_trust_dn.SizeRegexs();++n) {
            const RegularExpression& reg = vomscert_trust_dn.GetRegex(n);
            if(checkTrust(reg,certstack)) {
              trust_success = true;
              break;
            }
          }
        }

        if (!trust_success) {
          CredentialLogger.msg(ERROR,"VOMS: unable to match certificate chain against VOMS trusted DNs");
          if(!lsc_check) status |= VOMSACInfo::TrustFailed;
          else status |= VOMSACInfo::LSCFailed;
          //return false;
        }
      }

      bool sig_valid = false;
      if(certstack) {
        //If the certificate stack does correspond to some of the trusted DN chain,
        //then check if the AC signature is valid by using the voms server
        //certificate (voms server certificate is supposed to be the first
        //in the certificate stack).
        issuer = X509_dup(sk_X509_value(certstack, 0));
      }

      if(issuer) {
        if (checkSigAC(issuer, ac)) {
          sig_valid = true;
        } else {
          CredentialLogger.msg(ERROR,"VOMS: AC signature verification failed");
        }
      }

      if(verify) {
        //Check if those certificate in the certificate stack are trusted.
        if (sig_valid) { // Note - sig_valid=true never happens with certstack=NULL
          if (!checkCert(certstack, ca_cert_dir, ca_cert_file, system_ca_dir, system_ca_file)) {
            issuer.reset();
            CredentialLogger.msg(ERROR,"VOMS: unable to verify certificate chain");
            status |= VOMSACInfo::CAUnknown;
            res = false;
          }
        }
        else {
          CredentialLogger.msg(ERROR,"VOMS: cannot validate AC issuer for VO %s",voname);
          status |= VOMSACInfo::ACParsingFailed;
          res = false;
        }
      }
    }

    issuer_cert = issuer.release();
    return res;
  }


  static bool checkAttributes(AC const * ac, std::vector<std::string>& attributes, unsigned int& status) {

#if (OPENSSL_VERSION_NUMBER < 0x30400000L)

    STACK_OF(AC_ATTR) *atts = ac->acinfo->attrib;

    /* find AC_ATTR with IETFATTR type */
    int  nid = OBJ_txt2nid(idatcapOID);
    int pos = X509at_get_attr_by_NID((STACK_OF(X509_ATTRIBUTE)*)atts, nid, -1);
    if (!(pos >=0)) {
      CredentialLogger.msg(ERROR,"VOMS: Can not find AC_ATTR with IETFATTR type");
      return false;
    }
    AC_ATTR *caps = sk_AC_ATTR_value(atts, pos);

    /* check there's exactly one IETFATTR attribute */
    if (sk_AC_IETFATTR_num(caps->ietfattr) != 1) {
      CredentialLogger.msg(ERROR,"VOMS: case of multiple IETFATTR attributes not supported");
      return false;
    }

    /* retrieve the only AC_IETFFATTR */
    AC_IETFATTR *capattr = sk_AC_IETFATTR_value(caps->ietfattr, 0);
    if(!capattr) {
      CredentialLogger.msg(ERROR,"VOMS: missing IETFATTR");
      return false;
    }
    STACK_OF(AC_IETFATTRVAL) *values = capattr->values;

    /* check it has exactly one policyAuthority */
    if (sk_GENERAL_NAME_num(capattr->names) != 1) {
      CredentialLogger.msg(ERROR,"VOMS: case of multiple policyAuthority not supported");
      return false;
    }

    /* store policyAuthority */
    GENERAL_NAME *data = sk_GENERAL_NAME_value(capattr->names, 0);
    if (data && data->type == GEN_URI) {
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
      AC_IETFATTRVAL *capname = sk_AC_IETFATTRVAL_value(values, i);

      if (capname->type != V_ASN1_OCTET_STRING) {
        CredentialLogger.msg(ERROR,"VOMS: the format of IETFATTRVAL is not supported - expecting OCTET STRING");
        return false;
      }

      std::string fqan((const char*)(capname->value.octet_string->data), capname->value.octet_string->length);

      // if the attribute is like: /knowarc.eu/Role=NULL/Capability=NULL
      // or /knowarc.eu/Role=tester/Capability=NULL
      // then remove the element with "=NULL" to be:
      // /knowarc.eu
      // /knowarc.eu/Role=tester

      std::string str = fqan;
      std::size_t pos = str.find("/Role=NULL");
      if(pos != std::string::npos) str.erase(pos, 10);
      pos = str.find("/Capability=NULL");
      if(pos != std::string::npos) str.erase(pos, 16);

      attributes.push_back(str);
    }

#else

    /* find AC_ATTR with IETFATTR type */
    int  nid = OBJ_txt2nid(idatcapOID);
    int pos = X509_ACERT_get_attr_by_NID(ac, nid, -1);
    if (!(pos >=0)) {
      CredentialLogger.msg(ERROR,"VOMS: Can not find AC_ATTR with IETFATTR type");
      return false;
    }
    AC_ATTR *caps = X509_ACERT_get_attr(ac, pos); // name should be get0

    /* check there's exactly one IETFATTR attribute */
    if (X509_ATTRIBUTE_count(caps) != 1) {
      CredentialLogger.msg(ERROR,"VOMS: case of multiple IETFATTR attributes not supported");
      return false;
    }

    /* retrieve the only AC_IETFFATTR */
    ASN1_STRING* asnitem = (ASN1_STRING*)X509_ATTRIBUTE_get0_data(caps, 0,  V_ASN1_SEQUENCE, NULL);
    unsigned char const * asndata = ASN1_STRING_get0_data(asnitem);
    AC_IETFATTRRef capattr(d2i_OSSL_IETF_ATTR_SYNTAX(NULL, &asndata, ASN1_STRING_length(asnitem)));
    if (!capattr) {
      CredentialLogger.msg(ERROR,"VOMS: failed to access IETFATTR attribute");
      return false;
    }

    /* check it has exactly one policyAuthority */
    if (sk_GENERAL_NAME_num(OSSL_IETF_ATTR_SYNTAX_get0_policyAuthority(capattr)) != 1) {
      CredentialLogger.msg(ERROR,"VOMS: case of multiple policyAuthority not supported");
      return false;
    }

    /* store policyAuthority */
    GENERAL_NAME *data = sk_GENERAL_NAME_value(OSSL_IETF_ATTR_SYNTAX_get0_policyAuthority(capattr), 0);
    if (data && data->type == GEN_URI) {
      std::string voname("/voname=");
      voname.append((const char*) ASN1_STRING_get0_data(data->d.ia5), ASN1_STRING_length(data->d.ia5));
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
    for (int i=0; i<OSSL_IETF_ATTR_SYNTAX_get_value_num(capattr); i++) {
      int captype = 0;
      ASN1_OCTET_STRING *capname = (ASN1_OCTET_STRING*)OSSL_IETF_ATTR_SYNTAX_get0_value(capattr, i, &captype);

      if (captype != OSSL_IETFAS_OCTETS) {
        CredentialLogger.msg(ERROR,"VOMS: the format of IETFATTRVAL is not supported - expecting OCTET STRING");
        return false;
      }

      std::string fqan((const char*) ASN1_STRING_get0_data(capname), ASN1_STRING_length(capname));

      // if the attribute is like: /knowarc.eu/Role=NULL/Capability=NULL
      // or /knowarc.eu/Role=tester/Capability=NULL
      // then remove the element with "=NULL" to be:
      // /knowarc.eu
      // /knowarc.eu/Role=tester

      std::string str = fqan;
      std::size_t pos = str.find("/Role=NULL");
      if(pos != std::string::npos) str.erase(pos, 10);
      pos = str.find("/Capability=NULL");
      if(pos != std::string::npos) str.erase(pos, 16);

      attributes.push_back(str);
    }

#endif

    return true;
  }

  std::string getHostFQDN(void) {
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

  static bool interpretAttributes(AC_FULL_ATTRIBUTES *full_attr, std::vector<std::string>& attributes, unsigned int& status) {
    std::string name, value, qualifier, grantor, voname, uri;
    GENERAL_NAME *gn = NULL;
    STACK_OF(AC_ATT_HOLDER) *providers = NULL;
    int i;

    providers = full_attr->providers;

    for (i = 0; i < sk_AC_ATT_HOLDER_num(providers); i++) {
      AC_ATT_HOLDER *holder = sk_AC_ATT_HOLDER_value(providers, i);
      STACK_OF(AC_ATTRIBUTE) *atts = holder->attributes;

      gn = sk_GENERAL_NAME_value(holder->grantor, 0);
      grantor.assign((const char*) ASN1_STRING_get0_data(gn->d.ia5), ASN1_STRING_length(gn->d.ia5));
      if(grantor.empty()) {
        CredentialLogger.msg(ERROR,"VOMS: the grantor attribute is empty");
        status |= VOMSACInfo::InternalParsingFailed;
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

        name.assign((const char*) ASN1_STRING_get0_data(at->name), ASN1_STRING_length(at->name));
        if(name.empty()) {
          CredentialLogger.msg(ERROR,"VOMS: the attribute name is empty");
          status |= VOMSACInfo::InternalParsingFailed;
          return false;
        }
        value.assign((const char*) ASN1_STRING_get0_data(at->value), ASN1_STRING_length(at->value));
        if(value.empty()) {
          CredentialLogger.msg(WARNING,"VOMS: the attribute value for %s is empty", name.c_str());
          //return false;
        }
        qualifier.assign((const char*) ASN1_STRING_get0_data(at->qualifier), ASN1_STRING_length(at->qualifier));
        if(qualifier.empty()) {
          CredentialLogger.msg(ERROR,"VOMS: the attribute qualifier is empty");
          status |= VOMSACInfo::InternalParsingFailed;
          return false;
        }
        //attribute.append("/grantor=").append(grantor).append("/").append(qualifier).append(":").append(name).append("=").append(value);
        std::string separator;
        if(qualifier.substr(0,1) != "/") separator = "/";
        attribute.append("/voname=").append(voname).
                  append("/hostname=").append(uri).
                  append(separator).append(qualifier).append(":").append(name).
                  append("=").append(value);
        attributes.push_back(attribute);
      }
      grantor.clear();
    }
    return true;
  }

  static bool checkExtensions(STACK_OF(X509_EXTENSION) const *exts, X509 *iss, std::string const & targetFQDN, std::vector<std::string>& output, unsigned int& status) {

#if (OPENSSL_VERSION_NUMBER < 0x30400000L)

    int nid1 = OBJ_txt2nid(idcenoRevAvailOID);
    int nid2 = OBJ_txt2nid(idceauthKeyIdentifierOID);
    int nid3 = OBJ_txt2nid(idceTargetsOID);
    int nid5 = OBJ_txt2nid(attributesOID);

    int pos1 = X509v3_get_ext_by_NID(exts, nid1, -1);
    int pos2 = X509v3_get_ext_by_NID(exts, nid2, -1);
    int pos3 = X509v3_get_ext_by_critical(exts, 1, -1);
    int pos4 = X509v3_get_ext_by_NID(exts, nid3, -1);
    int pos5 = X509v3_get_ext_by_NID(exts, nid5, -1);

    /* noRevAvail, Authkeyid MUST be present */
    if ((pos1 < 0) || (pos2 < 0)) {
      CredentialLogger.msg(ERROR,"VOMS: both idcenoRevAvail and authorityKeyIdentifier certificate extensions must be present");
      status = VOMSACInfo::InternalParsingFailed;
      return false;
    }

    //Check if the target fqan matches idceTargets
    while (pos3 >=0) {
      X509_EXTENSION *ex = sk_X509_EXTENSION_value(exts, pos3);
      if (pos3 == pos4) {     //The only critical extension allowed is idceTargets,
        std::string fqdn = targetFQDN;
        int ok = 0;
        int i;
        Arc::Credential::ASN1_IA5STRINGRef fqdns(ASN1_IA5STRING_new());
        if (fqdns) {
          ASN1_STRING_set(fqdns, fqdn.c_str(), fqdn.size());
          AC_TARGETSRef targets((AC_TARGETS *)X509V3_EXT_d2i(ex));
          if (targets) {
            for (i = 0; i < sk_AC_TARGET_num(targets->targets); i++) {
              AC_TARGET *name = sk_AC_TARGET_value(targets->targets, i);
              if (name && name->name && name->name->type == GEN_URI) {
                ok = !ASN1_STRING_cmp(name->name->d.ia5, fqdns);
                if (ok)
                  break;
              }
            }
          }
        }
        if (!ok) {
          CredentialLogger.msg(WARNING,"VOMS: FQDN of this host %s does not match any target in AC", fqdn);
          // return false; ???
        }
      }
      else {
        CredentialLogger.msg(ERROR,"VOMS: the only supported critical extension of the AC is idceTargets");
        status = VOMSACInfo::InternalParsingFailed;
        return false;
      }
      pos3 = X509v3_get_ext_by_critical(exts, 1, pos3);
    }

    //Parse the attributes
    if (pos5 >= 0) {
      X509_EXTENSION *ex = sk_X509_EXTENSION_value(exts, pos5);
      AC_FULL_ATTRIBUTESRef full_attr((AC_FULL_ATTRIBUTES *)X509V3_EXT_d2i(ex));
      if (full_attr) {
        if (!interpretAttributes(full_attr, output, status)) {
          CredentialLogger.msg(ERROR,"VOMS: failed to parse attributes from AC");
          AC_FULL_ATTRIBUTES_free(full_attr);
          status = VOMSACInfo::InternalParsingFailed;
          return false;
        }
      }
    }

    //Check the authorityKeyIdentifier
    if (pos2 >= 0) {
      X509_EXTENSION *ex;
      bool keyerr = false;
      ex = sk_X509_EXTENSION_value(exts, pos2);
      AUTHORITY_KEYIDRef key((AUTHORITY_KEYID *)X509V3_EXT_d2i(ex));
      if (key) {
        if (iss) {
          if (key->keyid) {
            unsigned char hashed[20];
            ASN1_BIT_STRING* pkeystr = X509_get0_pubkey_bitstr(iss);
            if (!SHA1(pkeystr->data,
                      pkeystr->length,
                      hashed))
              keyerr = true;

            if ((memcmp(key->keyid->data, hashed, 20) != 0) &&
                (key->keyid->length == 20))
              keyerr = true;
          }
          else {
            if (!(key->issuer && key->serial)) keyerr = true;
            if (ASN1_INTEGER_cmp((key->serial), X509_get_serialNumber(iss))) keyerr = true;
            if (key->serial->type != GEN_DIRNAME) keyerr = true;
            if (X509_NAME_cmp(sk_GENERAL_NAME_value((key->issuer), 0)->d.dirn, X509_get_subject_name(iss))) keyerr = true;
          }
        }
      }
      else {
        keyerr = true;
      }

      if(keyerr) {
        CredentialLogger.msg(ERROR,"VOMS: authorityKey is wrong");
        status = VOMSACInfo::InternalParsingFailed;
        return false;
      }
    }

#else

    int nid1 = NID_no_rev_avail;
    int nid2 = NID_authority_key_identifier;
    int nid3 = NID_target_information;
    int nid5 = OBJ_txt2nid(attributesOID);

    int pos1 = X509v3_get_ext_by_NID(exts, nid1, -1);
    int pos2 = X509v3_get_ext_by_NID(exts, nid2, -1);
    int pos3 = X509v3_get_ext_by_critical(exts, 1, -1);
    int pos4 = X509v3_get_ext_by_NID(exts, nid3, -1);
    int pos5 = X509v3_get_ext_by_NID(exts, nid5, -1);

    /* noRevAvail, Authkeyid MUST be present */
    if ((pos1 < 0) || (pos2 < 0)) {
      CredentialLogger.msg(ERROR,"VOMS: both idcenoRevAvail and authorityKeyIdentifier certificate extensions must be present");
      status = VOMSACInfo::InternalParsingFailed;
      return false;
    }

    //Check if the target fqan matches idceTargets
    while (pos3 >=0) {

      X509_EXTENSION *ex = sk_X509_EXTENSION_value(exts, pos3);
      if ((pos3 == pos4) && !targetFQDN.empty()) {     //The only critical extension allowed is idceTargets,
        int ok = 0;
        Arc::Credential::ASN1_IA5STRINGRef fqdns(ASN1_IA5STRING_new());
        if (fqdns) {
          ASN1_STRING_set(fqdns, targetFQDN.c_str(), targetFQDN.size());
          OSSL_TARGETING_INFORMATIONRef targetsInfo((OSSL_TARGETING_INFORMATION*)X509V3_EXT_d2i(ex));
          if (targetsInfo) {
            for (int n = 0; n < sk_OSSL_TARGETS_num(targetsInfo); n++) {
              OSSL_TARGETS* targets = sk_OSSL_TARGETS_value(targetsInfo, n);
              if (targets) {
                for (int i = 0; i < sk_OSSL_TARGET_num(targets); i++) {
                  OSSL_TARGET *name = sk_OSSL_TARGET_value(targets, i);
                  if (name->type == OSSL_TGT_TARGET_NAME && name->choice.targetName && name->choice.targetName->type == GEN_URI) {
                    ok = !ASN1_STRING_cmp(name->choice.targetName->d.ia5, fqdns);
                    if (ok)
                      break;
                  }
                }
              }
              if (ok)
                break;
            }
          }
        }
        if (!ok) {
          CredentialLogger.msg(WARNING,"VOMS: FQDN of this host %s does not match any target in AC", targetFQDN);
          // return false; ???
        }
      }
      else {
        CredentialLogger.msg(ERROR,"VOMS: the only supported critical extension of the AC is idceTargets");
        status = VOMSACInfo::InternalParsingFailed;
        return false;
      }
      pos3 = X509v3_get_ext_by_critical(exts, 1, pos3);
    }

    //Parse the attributes
    if (pos5 >= 0) {
      X509_EXTENSION *ex = sk_X509_EXTENSION_value(exts, pos5);
      AC_FULL_ATTRIBUTESRef full_attr((AC_FULL_ATTRIBUTES *)X509V3_EXT_d2i(ex));
      if (full_attr) {
        if (!interpretAttributes(full_attr, output, status)) {
          CredentialLogger.msg(ERROR,"VOMS: failed to parse attributes from AC");
          AC_FULL_ATTRIBUTES_free(full_attr);
          status = VOMSACInfo::InternalParsingFailed;
          return false;
        }
      }
    }

    //Check the authorityKeyIdentifier
    if (pos2 >= 0) {
      bool keyerr = false;
      X509_EXTENSION *ex = sk_X509_EXTENSION_value(exts, pos2);
      AUTHORITY_KEYIDRef key((AUTHORITY_KEYID *)X509V3_EXT_d2i(ex));
      if (key) {
        if (iss) {
          if (key->keyid) {
            unsigned char hashed[20];
            const ASN1_BIT_STRING* pkeystr = X509_get0_pubkey_bitstr(iss);
            if (!SHA1(ASN1_STRING_get0_data(pkeystr),
                      ASN1_STRING_length(pkeystr),
                      hashed))
              keyerr = true;

            if ((memcmp(ASN1_STRING_get0_data(key->keyid), hashed, 20) != 0) &&
                (ASN1_STRING_length(key->keyid) == 20))
              keyerr = true;
          }
          else {
            if (!key->issuer || !key->serial) keyerr = true;
            else {
              if (ASN1_INTEGER_cmp((key->serial), X509_get_serialNumber(iss))) keyerr = true;
              else {
                GENERAL_NAME* issuerName = sk_GENERAL_NAME_value((key->issuer), 0);
                if (!issuerName) keyerr = true;
                else {
                  if (issuerName->type != GEN_DIRNAME) keyerr = true;
                  else if (X509_NAME_cmp(issuerName->d.dirn, X509_get_subject_name(iss))) keyerr = true;
                }
              }
            }
          }
        }
      }
      else {
        keyerr = true;
      }

      if(keyerr) {
        CredentialLogger.msg(ERROR,"VOMS: authorityKey is wrong");
        status = VOMSACInfo::InternalParsingFailed;
        return false;
      }
    }

#endif

    return true;
  }

  static time_t ASN1_GENERALIZEDTIME_get(const ASN1_GENERALIZEDTIME* const s) {
    if ((s == NULL) || (ASN1_STRING_get0_data(s) == NULL) || (ASN1_STRING_length(s) == 0)) return Arc::Time::UNDEFINED;
    std::string str((char const *) ASN1_STRING_get0_data(s), ASN1_STRING_length(s));
    Arc::Time t(str);
    return t.GetTime();
  }

  static bool checkACInfo(X509* cert, X509* issuer, AC* ac, std::string const & targetFQDN,
    std::vector<std::string>& output, std::string& ac_holder_name,
    std::string& ac_issuer_name, Time& valid_from, Time& valid_till, unsigned int& status) {

    bool res = true;

#if (OPENSSL_VERSION_NUMBER < 0x30400000L)

    if(!ac || !cert || !(ac->acinfo) || !(ac->acinfo->version) || !(ac->acinfo->holder)
       || (ac->acinfo->holder->digest) || !(ac->acinfo->form) || !(ac->acinfo->form->names)
       || (ac->acinfo->form->is) || (ac->acinfo->form->digest) || !(ac->acinfo->serial)
       || !(ac->acinfo->alg) || !(ac->acinfo->validity)
       || !(ac->acinfo->validity->notBefore) || !(ac->acinfo->validity->notAfter)
       || !(ac->acinfo->attrib) || !(ac->sig_alg) || !(ac->signature)) {
      CredentialLogger.msg(ERROR,"VOMS: missing AC parts");
      status |= VOMSACInfo::ACParsingFailed;
      return false;
    }

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
      CredentialLogger.msg(ERROR,"VOMS: unsupported time format in AC - expecting GENERALIZED TIME");
      status |= VOMSACInfo::ACParsingFailed;
      return false; // ?
    }
    if ((X509_cmp_current_time(start) >= 0) &&
        (X509_cmp_time(start, &ctime) >= 0)) {
      CredentialLogger.msg(ERROR,"VOMS: AC is not yet valid");
      status |= VOMSACInfo::TimeValidFailed;
      res = false;
      //return false;
    }
    if ((X509_cmp_current_time(end) <= 0) &&
        (X509_cmp_time(end, &dtime) <= 0)) {
      CredentialLogger.msg(ERROR,"VOMS: AC has expired");
      status |= VOMSACInfo::TimeValidFailed;
      res = false;
      //return false;
    }
    valid_from = Time(ASN1_GENERALIZEDTIME_get(start));
    valid_till = Time(ASN1_GENERALIZEDTIME_get(end));


    STACK_OF(GENERAL_NAME) *names;
    GENERAL_NAME  *name = NULL;

    if (ac->acinfo->holder->baseid) {
      if(!(ac->acinfo->holder->baseid->serial) ||
         !(ac->acinfo->holder->baseid->issuer)) {
        CredentialLogger.msg(ERROR,"VOMS: AC is not complete - missing Serial or Issuer information");
        status |= VOMSACInfo::ACParsingFailed;
        return false;
      }

      CredentialLogger.msg(DEBUG,"VOMS: the holder serial number is:  %lx", ASN1_INTEGER_get(X509_get_serialNumber(cert)));
      CredentialLogger.msg(DEBUG,"VOMS: the serial number in AC is:  %lx", ASN1_INTEGER_get(ac->acinfo->holder->baseid->serial));

      if (ASN1_INTEGER_cmp(ac->acinfo->holder->baseid->serial, X509_get_serialNumber(cert))) {
        CredentialLogger.msg(VERBOSE,"VOMS: the holder serial number %lx is not the same as the serial number in AC %lx, the holder certificate that is used to create a voms proxy could be a proxy certificate with a different serial number as the original EEC cert",
          ASN1_INTEGER_get(X509_get_serialNumber(cert)),
          ASN1_INTEGER_get(ac->acinfo->holder->baseid->serial));
        // return false;
      }

      names = ac->acinfo->holder->baseid->issuer;
      if ((sk_GENERAL_NAME_num(names) != 1) || !(name = sk_GENERAL_NAME_value(names,0)) ||
        (name->type != GEN_DIRNAME)) {
        CredentialLogger.msg(ERROR,"VOMS: the holder information in AC is wrong");
        status |= VOMSACInfo::ACParsingFailed;
        return false;
      }

      char *ac_holder_name_chars = X509_NAME_oneline(name->d.dirn,NULL,0);
      if(ac_holder_name_chars) {
        ac_holder_name = ac_holder_name_chars;
        OPENSSL_free(ac_holder_name_chars);
      }
      std::string holder_name;
      char *holder_name_chars = X509_NAME_oneline(X509_get_subject_name(cert),NULL,0);
      if(holder_name_chars) {
        holder_name = holder_name_chars;
        OPENSSL_free(holder_name_chars);
      }
      std::string holder_issuer_name;
      char *holder_issuer_name_chars = X509_NAME_oneline(X509_get_issuer_name(cert),NULL,0);
      if(holder_issuer_name_chars) {
        holder_issuer_name = holder_issuer_name_chars;
        OPENSSL_free(holder_issuer_name_chars);
      }
      CredentialLogger.msg(DEBUG,"VOMS: DN of holder in AC: %s",ac_holder_name.c_str());
      CredentialLogger.msg(DEBUG,"VOMS: DN of holder: %s",holder_name.c_str());
      CredentialLogger.msg(DEBUG,"VOMS: DN of issuer: %s",holder_issuer_name.c_str());

      if ((ac_holder_name != holder_name) && (ac_holder_name != holder_issuer_name)) {
        std::size_t found1, found2;
        found1 = holder_name.find(ac_holder_name);
        found2 = holder_issuer_name.find(ac_holder_name);
        if((found1 == std::string::npos) && (found2 == std::string::npos)) {
          CredentialLogger.msg(ERROR,"VOMS: the holder name in AC is not related to the distinguished name in holder certificate");
          status |= VOMSACInfo::ACParsingFailed;
           return false;
        }
      }

      const ASN1_BIT_STRING* issuerUID = NULL;
      X509_get0_uids(cert, &issuerUID, NULL);
      if ((ac->acinfo->holder->baseid->uid && issuerUID) ||
          (!issuerUID && !ac->acinfo->holder->baseid->uid)) {
        if (ac->acinfo->holder->baseid->uid) {
          if (ASN1_STRING_cmp(ac->acinfo->holder->baseid->uid, issuerUID)) {
            CredentialLogger.msg(ERROR,"VOMS: the holder issuerUID is not the same as that in AC");
            status |= VOMSACInfo::ACParsingFailed;
            return false;
          }
        }
      }
       else {
         CredentialLogger.msg(ERROR,"VOMS: the holder issuerUID is not the same as that in AC");
         status |= VOMSACInfo::ACParsingFailed;
         return false;
      }
    }
    else if (ac->acinfo->holder->name) {
      names = ac->acinfo->holder->name;
      if ((sk_GENERAL_NAME_num(names) == 1) ||      //???
          ((name = sk_GENERAL_NAME_value(names,0))) ||
          (name->type != GEN_DIRNAME)) {
        if (X509_NAME_cmp(name->d.dirn, X509_get_issuer_name(cert))) {
          // CHECK ALT_NAMES
          // in VOMS ACs, checking into alt names is assumed to always fail.
          CredentialLogger.msg(ERROR,"VOMS: the holder issuer name is not the same as that in AC");
          status |= VOMSACInfo::ACParsingFailed;
          return false;
        }
      }
    }

    names = ac->acinfo->form->names;
    if ((sk_GENERAL_NAME_num(names) != 1) || !(name = sk_GENERAL_NAME_value(names,0)) ||
      (name->type != GEN_DIRNAME)) {
      CredentialLogger.msg(ERROR,"VOMS: the issuer information in AC is wrong");
      status |= VOMSACInfo::ACParsingFailed;
      return false;
    }
    ac_issuer_name = x509name2ascii(name->d.dirn);
    if(issuer) {
      if (X509_NAME_cmp(name->d.dirn, X509_get_subject_name(issuer))) {
        std::string issuer_name = x509name2ascii(X509_get_subject_name(issuer));
        CredentialLogger.msg(ERROR,"VOMS: the issuer name %s is not the same as that in AC - %s",
          issuer_name, ac_issuer_name);
        status |= VOMSACInfo::ACParsingFailed;
        return false;
      }
    }

    if (ac->acinfo->serial->length > 20) {
      CredentialLogger.msg(ERROR,"VOMS: the serial number of AC INFO is too long - expecting no more than 20 octets");
      status |= VOMSACInfo::InternalParsingFailed;
      return false;
    }

    //Check AC's attribute
    if(!checkAttributes(ac, output, status)) res = false; // ??

    //Check AC's extension
    if(!checkExtensions(ac->acinfo->exts, issuer, targetFQDN, output, status)) res = false;

#else // (OPENSSL_VERSION_NUMBER < 0x30400000L)

    if(!ac || !cert
       || !X509_ACERT_get0_issuerName(ac) || !X509_ACERT_get0_serialNumber(ac)
       || !X509_ACERT_get0_info_sigalg(ac) || !X509_ACERT_get0_notBefore(ac) || !X509_ACERT_get0_notAfter(ac)) {
      CredentialLogger.msg(ERROR,"VOMS: missing AC parts");
      status |= VOMSACInfo::ACParsingFailed;
      return false;
    }
    const ASN1_BIT_STRING * signature = NULL;
    const X509_ALGOR * sig_alg = NULL;
    X509_ACERT_get0_signature(ac, &signature, &sig_alg);
    if(!sig_alg || !signature) {
      CredentialLogger.msg(ERROR,"VOMS: missing AC parts");
      status |= VOMSACInfo::ACParsingFailed;
      return false;
    }

    //Check the validity time
    ASN1_TIME const *start;
    ASN1_TIME const *end;
    start = X509_ACERT_get0_notBefore(ac);
    end = X509_ACERT_get0_notAfter(ac);

    time_t ctime, dtime;
    time (&ctime);
    ctime += 300;
    dtime = ctime-600;

    if ((ASN1_STRING_type(start) != V_ASN1_GENERALIZEDTIME) || (ASN1_STRING_type(end) != V_ASN1_GENERALIZEDTIME)) {
      CredentialLogger.msg(ERROR,"VOMS: unsupported time format in AC - expecting GENERALIZED TIME");
      status |= VOMSACInfo::ACParsingFailed;
      return false; // ?
    }
    if ((X509_cmp_current_time(start) >= 0) &&
        (X509_cmp_time(start, &ctime) >= 0)) {
      CredentialLogger.msg(ERROR,"VOMS: AC is not yet valid");
      status |= VOMSACInfo::TimeValidFailed;
      res = false;
      //return false;
    }
    if ((X509_cmp_current_time(end) <= 0) &&
        (X509_cmp_time(end, &dtime) <= 0)) {
      CredentialLogger.msg(ERROR,"VOMS: AC has expired");
      status |= VOMSACInfo::TimeValidFailed;
      res = false;
      //return false;
    }
    valid_from = Time(ASN1_GENERALIZEDTIME_get(start));
    valid_till = Time(ASN1_GENERALIZEDTIME_get(end));



    if (X509_ACERT_get0_holder_baseCertId(ac)) {
      if(!OSSL_ISSUER_SERIAL_get0_serial(X509_ACERT_get0_holder_baseCertId(ac)) ||
         !OSSL_ISSUER_SERIAL_get0_issuer(X509_ACERT_get0_holder_baseCertId(ac))) {
        CredentialLogger.msg(ERROR,"VOMS: AC is not complete - missing Serial or Issuer information");
        status |= VOMSACInfo::ACParsingFailed;
        return false;
      }

      CredentialLogger.msg(DEBUG,"VOMS: the holder serial number is:  %lx", ASN1_INTEGER_get(X509_get_serialNumber(cert)));
      CredentialLogger.msg(DEBUG,"VOMS: the serial number in AC is:  %lx", ASN1_INTEGER_get(OSSL_ISSUER_SERIAL_get0_serial(X509_ACERT_get0_holder_baseCertId(ac))));

      if (ASN1_INTEGER_cmp(OSSL_ISSUER_SERIAL_get0_serial(X509_ACERT_get0_holder_baseCertId(ac)), X509_get_serialNumber(cert))) {
        CredentialLogger.msg(VERBOSE,"VOMS: the holder serial number %lx is not the same as the serial number in AC %lx, the holder certificate that is used to create a voms proxy could be a proxy certificate with a different serial number as the original EEC cert",
          ASN1_INTEGER_get(X509_get_serialNumber(cert)),
          ASN1_INTEGER_get(OSSL_ISSUER_SERIAL_get0_serial(X509_ACERT_get0_holder_baseCertId(ac))));
        // return false;
      }

      X509_NAME const *name = NULL;
      name = OSSL_ISSUER_SERIAL_get0_issuer(X509_ACERT_get0_holder_baseCertId(ac));
      if (!name) {
        CredentialLogger.msg(ERROR,"VOMS: the holder information in AC is wrong");
        status |= VOMSACInfo::ACParsingFailed;
        return false;
      }

      char *ac_holder_name_chars = X509_NAME_oneline(name,NULL,0);
      if(ac_holder_name_chars) {
        ac_holder_name = ac_holder_name_chars;
        OPENSSL_free(ac_holder_name_chars);
      }
      std::string holder_name;
      char *holder_name_chars = X509_NAME_oneline(X509_get_subject_name(cert),NULL,0);
      if(holder_name_chars) {
        holder_name = holder_name_chars;
        OPENSSL_free(holder_name_chars);
      }
      std::string holder_issuer_name;
      char *holder_issuer_name_chars = X509_NAME_oneline(X509_get_issuer_name(cert),NULL,0);
      if(holder_issuer_name_chars) {
        holder_issuer_name = holder_issuer_name_chars;
        OPENSSL_free(holder_issuer_name_chars);
      }
      CredentialLogger.msg(DEBUG,"VOMS: DN of holder in AC: %s",ac_holder_name.c_str());
      CredentialLogger.msg(DEBUG,"VOMS: DN of holder: %s",holder_name.c_str());
      CredentialLogger.msg(DEBUG,"VOMS: DN of issuer: %s",holder_issuer_name.c_str());

      if ((ac_holder_name != holder_name) && (ac_holder_name != holder_issuer_name)) {
        std::size_t found1, found2;
        found1 = holder_name.find(ac_holder_name);
        found2 = holder_issuer_name.find(ac_holder_name);
        if((found1 == std::string::npos) && (found2 == std::string::npos)) {
          CredentialLogger.msg(ERROR,"VOMS: the holder name in AC is not related to the distinguished name in holder certificate");
          status |= VOMSACInfo::ACParsingFailed;
          return false;
        }
      }

      const ASN1_BIT_STRING* issuerUID = NULL;
      X509_get0_uids(cert, &issuerUID, NULL);
      if ((OSSL_ISSUER_SERIAL_get0_issuerUID(X509_ACERT_get0_holder_baseCertId(ac)) && issuerUID) ||
          (!issuerUID && !OSSL_ISSUER_SERIAL_get0_issuerUID(X509_ACERT_get0_holder_baseCertId(ac)))) {
        if (OSSL_ISSUER_SERIAL_get0_issuerUID(X509_ACERT_get0_holder_baseCertId(ac))) {
          if (ASN1_STRING_cmp(OSSL_ISSUER_SERIAL_get0_issuerUID(X509_ACERT_get0_holder_baseCertId(ac)), issuerUID)) {
            CredentialLogger.msg(ERROR,"VOMS: the holder issuerUID is not the same as that in AC");
            status |= VOMSACInfo::ACParsingFailed;
            return false;
          }
        }
      }
      else {
        CredentialLogger.msg(ERROR,"VOMS: the holder issuerUID is not the same as that in AC");
        status |= VOMSACInfo::ACParsingFailed;
        return false;
      }
    }
    else if (X509_ACERT_get0_holder_entityName(ac)) {
      GENERAL_NAME const * name = NULL;
      GENERAL_NAMES const * names = X509_ACERT_get0_holder_entityName(ac);
      if ((sk_GENERAL_NAME_num(names) == 1) && (name = sk_GENERAL_NAME_value(names,0)) && (name->type != GEN_DIRNAME)) {
        if (X509_NAME_cmp(name->d.dirn, X509_get_issuer_name(cert))) {
          // CHECK ALT_NAMES
          // in VOMS ACs, checking into alt names is assumed to always fail.
          CredentialLogger.msg(ERROR,"VOMS: the holder issuer name is not the same as that in AC");
          status |= VOMSACInfo::ACParsingFailed;
          return false;
        }
      }
    }

    X509_NAME const *name = X509_ACERT_get0_issuerName(ac);
    if (!name) {
      CredentialLogger.msg(ERROR,"VOMS: the issuer information in AC is wrong");
      status |= VOMSACInfo::ACParsingFailed;
      return false;
    }
    ac_issuer_name = x509name2ascii(name);
    if(issuer) {
      if (X509_NAME_cmp(name, X509_get_subject_name(issuer))) {
        std::string issuer_name = x509name2ascii(X509_get_subject_name(issuer));
        CredentialLogger.msg(ERROR,"VOMS: the issuer name %s is not the same as that in AC - %s",
          issuer_name, ac_issuer_name);
        status |= VOMSACInfo::ACParsingFailed;
        return false;
      }
    }

    if (ASN1_STRING_length(X509_ACERT_get0_serialNumber(ac)) > 20) {
      CredentialLogger.msg(ERROR,"VOMS: the serial number of AC INFO is too long - expecting no more than 20 octets");
      status |= VOMSACInfo::InternalParsingFailed;
      return false;
    }

    //Check AC's attribute
    if(!checkAttributes(ac, output, status)) res = false; // ??

    //Check AC's extension
    if(!checkExtensions(X509_ACERT_get0_extensions(ac), issuer, targetFQDN, output, status)) res = false;

#endif // (OPENSSL_VERSION_NUMBER < 0x30400000L)

    return res;
  }

  // Returns false if any error happened.
  // Also always fills status with information about errors detected if any.
  static bool verifyVOMSAC(AC* ac,
        const std::string& ca_cert_dir, const std::string& ca_cert_file, bool system_ca_dir, bool system_ca_file, const std::string vomsdir,
        VOMSTrustList& vomscert_trust_dn, std::string const & targetFQDN, X509* holder,
        std::vector<std::string>& attr_output,
        std::string& vo_name, std::string& ac_holder_name, std::string& ac_issuer_name,
        Time& from, Time& till, unsigned int& status, bool verify) {
    bool res = true;
    //Extract name
    int nid = 0;
    int pos = 0;
    nid = OBJ_txt2nid(idatcapOID);
#if (OPENSSL_VERSION_NUMBER < 0x30400000L)
    STACK_OF(AC_ATTR) * atts = ac->acinfo->attrib;
    pos = X509at_get_attr_by_NID((STACK_OF(X509_ATTRIBUTE)*)atts, nid, -1);
#else
    pos = X509_ACERT_get_attr_by_NID(ac, nid, -1);
#endif
    if(!(pos >=0)) {
      CredentialLogger.msg(ERROR,"VOMS: unable to extract VO name from AC");
      status |= VOMSACInfo::ACParsingFailed; // ?
      return false;
    }

#if (OPENSSL_VERSION_NUMBER < 0x30400000L)
    AC_ATTR * caps = sk_AC_ATTR_value(atts, pos);
#else
    AC_ATTR * caps = X509_ACERT_get_attr(ac, pos);
#endif
    if(!caps) {
      // Must not happen. X509 parsing error
      CredentialLogger.msg(ERROR,"VOMS: unable to extract VO name from AC");
      status |= VOMSACInfo::ACParsingFailed; // ?
      return false;
    }

#if (OPENSSL_VERSION_NUMBER < 0x30400000L)
    AC_IETFATTR * capattr = sk_AC_IETFATTR_value(caps->ietfattr, 0);
#else
    ASN1_STRING* asnitem = (ASN1_STRING*)X509_ATTRIBUTE_get0_data(caps, 0,  V_ASN1_SEQUENCE, NULL);
    unsigned char const * asndata = ASN1_STRING_get0_data(asnitem);
    AC_IETFATTR * capattr = d2i_OSSL_IETF_ATTR_SYNTAX(NULL, &asndata, ASN1_STRING_length(asnitem));
#endif
    if(!capattr) {
      // Must not happen. AC parsing error
      CredentialLogger.msg(ERROR,"VOMS: unable to extract VO name from AC");
      status |= VOMSACInfo::ACParsingFailed; // ?
      return false;
    }

#if (OPENSSL_VERSION_NUMBER < 0x30400000L)
    GENERAL_NAME * name = sk_GENERAL_NAME_value(capattr->names, 0);
#else
    GENERAL_NAME * name = sk_GENERAL_NAME_value(OSSL_IETF_ATTR_SYNTAX_get0_policyAuthority(capattr), 0);
#endif
    if(!name) {
      // Must not happen. AC parsing error
      CredentialLogger.msg(ERROR,"VOMS: unable to extract VO name from AC");
      status |= VOMSACInfo::ACParsingFailed; // ?
      return false;
    }

    std::string voname((const char *) ASN1_STRING_get0_data(name->d.ia5), 0, ASN1_STRING_length(name->d.ia5));
    std::string::size_type cpos = voname.find("://");
    std::string hostname;
    if (cpos != std::string::npos) {
      std::string::size_type cpos2 = voname.find(":", cpos+1);
      if (cpos2 != std::string::npos)
        hostname = voname.substr(cpos+3, (cpos2 - cpos - 3));
      else {
        // Must not happen. VOMS parsing error
        CredentialLogger.msg(ERROR,"VOMS: unable to determine hostname of AC from VO name: %s",voname);
        status |= VOMSACInfo::InternalParsingFailed; // ?
        return false;
      }
      voname = voname.substr(0, cpos);
      vo_name = voname;
    }
    else {
      // Must not happen. VOMS parsing error
      CredentialLogger.msg(ERROR,"VOMS: unable to extract VO name from AC");
      status |= VOMSACInfo::InternalParsingFailed; // ?
      return false;
    }

    Credential::X509Ref issuer;

    if(!checkSignature(ac, vomsdir, voname, hostname,
                       ca_cert_dir, ca_cert_file, system_ca_dir, system_ca_file, vomscert_trust_dn,
                       issuer.put(), status, verify)) {
      CredentialLogger.msg(ERROR,"VOMS: can not verify the signature of the AC");
      res = false;
    }

    if(!checkACInfo(holder, issuer, ac, targetFQDN, attr_output, ac_holder_name, ac_issuer_name, from, till, status)) {
      // Not printing anything because checkACInfo prints a lot of information itself
      CredentialLogger.msg(ERROR,"VOMS: problems while parsing information in AC");
      res = false;
    }

    return res;
  }

  bool parseVOMSAC(X509* holder,
        const std::string& ca_cert_dir, const std::string& ca_cert_file, bool system_ca_dir, bool system_ca_file,
        const std::string& vomsdir, VOMSTrustList& vomscert_trust_dn,
        std::vector<VOMSACInfo>& output, bool verify, bool reportall, std::string const & targetFQDN) {

    InitVOMSAttribute();

    //Search the extension
    int nid = 0;
    int position = 0;
    bool critical = false;
#if (OPENSSL_VERSION_NUMBER < 0x40000000L)
    X509_EXTENSION * ext;
#else
    const X509_EXTENSION * ext;
#endif
    AC_SEQRef aclist;
    nid = OBJ_txt2nid(acseqOID);
    position = X509_get_ext_by_NID(holder, nid, -1);
    if(position >= 0) {
      ext = X509_get_ext(holder, position);
      if (ext){
        if(X509_EXTENSION_get_critical(ext)) critical = true;
        aclist = (AC_SEQ *)X509V3_EXT_d2i(ext);
      }
    }
    if(!aclist) {
      ERR_clear_error();
      //while(ERR_get_error() != 0);
      //std::cerr<<"No AC in the proxy certificate"<<std::endl; return false;
      return true;
    }

    bool verified = true;
    int num = sk_AC_num(aclist->acs);
    for (int i = 0; i < num; i++) {
      AC *ac = sk_AC_value(aclist->acs, i);
      VOMSACInfo ac_info;
      bool r = verifyVOMSAC(ac, ca_cert_dir, ca_cert_file, system_ca_dir, system_ca_file,
          vomsdir.empty()?default_vomsdir:vomsdir, vomscert_trust_dn, targetFQDN,  
          holder, ac_info.attributes, ac_info.voname, ac_info.holder, ac_info.issuer, 
          ac_info.from, ac_info.till, ac_info.status, verify);
      if(!r) verified = false;
      if(r || reportall) {
        if(critical) ac_info.status |= VOMSACInfo::IsCritical;
        output.push_back(ac_info);
      }
      ERR_clear_error();
    }

    return verified;
  }

  bool parseVOMSAC(const Credential& holder_cred,
         const std::string& ca_cert_dir, const std::string& ca_cert_file, bool system_ca_dir, bool system_ca_file,
         const std::string& vomsdir, VOMSTrustList& vomscert_trust_dn,
         std::vector<VOMSACInfo>& output, bool verify, bool reportall, std::string const & targetFQDN) {
    Arc::Credential::X509Ref holder(holder_cred.GetCert());
    if(!holder) return false;
    bool res = parseVOMSAC(holder, ca_cert_dir, ca_cert_file, system_ca_dir, system_ca_file, vomsdir,
                           vomscert_trust_dn, output, verify, reportall, targetFQDN);

    //Also parse the voms attributes inside the certificates on
    //the upstream of the holder certificate; in this case,
    //multiple level of delegation exists, and user(or intermediate
    //actor such as grid manager) could hold a voms proxy and use this
    //proxy to create a more level of proxy
    Arc::Credential::X509StackRef certchain(holder_cred.GetCertChain());
    if(certchain) {
      for(int idx = 0;;++idx) {
        if(idx >= sk_X509_num(certchain)) break;
        // TODO: stop at actual certificate, do not go to CAs and sub-CAs
        X509* cert = sk_X509_value(certchain,sk_X509_num(certchain)-idx-1);
        bool res2 = parseVOMSAC(cert, ca_cert_dir, ca_cert_file, system_ca_dir, system_ca_file, vomsdir,
                                vomscert_trust_dn, output, verify, reportall, targetFQDN);
        if (!res2) res = res2;
      };
    }

    return res;
  }

  bool parseVOMSAC(const std::string& cert_str,
         const std::string& ca_cert_dir, const std::string& ca_cert_file, bool system_ca_dir, bool system_ca_file,
         const std::string& vomsdir, VOMSTrustList& vomscert_trust_dn,
         std::vector<VOMSACInfo>& output, bool verify, bool reportall) {

    Arc::Credential::X509StackRef cert_chain(sk_X509_new_null());
    Arc::Credential::BIORef bio(BIO_new(BIO_s_mem()));
    BIO_write(bio, cert_str.c_str(), cert_str.size());

    bool res = true;
    bool found = false;
    while(!BIO_eof(bio)) {
      Arc::Credential::X509Ref tmp;
      if(!(PEM_read_bio_X509(bio, &tmp.put(), NULL, NULL))){
        ERR_clear_error();
        if(!found) res = false;
        break;
      }
      else { found = true; }
      if(sk_X509_push(cert_chain, tmp)) tmp.release();
    }

    for(int idx = 0;;++idx) {
      if(idx >= sk_X509_num(cert_chain)) break;
      X509* cert = sk_X509_value(cert_chain, idx);
      bool res2 = parseVOMSAC(cert, ca_cert_dir, ca_cert_file, system_ca_dir, system_ca_file, vomsdir,
                             vomscert_trust_dn, output, verify, reportall);
      if (!res2) res = res2;
    }

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

  static char *base64Encode(const char *data, int size, int *j) {
    BIO *in = NULL;
    BIO *b64 = NULL;
    int len = 0;
    char *buffer = NULL;

    in  = BIO_new(BIO_s_mem());
    b64 = BIO_new(BIO_f_base64());
    if (!in || !b64)
    goto err;

    b64 = BIO_push(b64, in);

    BIO_write(b64, data, size);
    BIO_flush(b64);

    *j = len = BIO_pending(in);

    buffer = (char *)malloc(len);
    if (!buffer)
      goto err;

    if (BIO_read(in, buffer, len) != len) {
      free(buffer);
      buffer = NULL;
      goto err;
    }

  err:
    BIO_free(b64);
    BIO_free(in);
    return buffer;
  }

  static char *base64Decode(const char *data, int size, int *j) {
    char *buffer = (char *)malloc(size);
    if (!buffer)
      return NULL;

    memset(buffer, 0, size);

    BIO *b64 = BIO_new(BIO_f_base64());
    BIO *in = BIO_new_mem_buf((void*)data, size);
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

  char *VOMSEncode(const char *data, int size, int *j) {
    // Only base64 encoding is used here
    return base64Encode(data, size, j);
  }

  char *VOMSDecode(const char *data, int size, int *j) {
    int i = 0;

    while (i < size)
      if (data[i++] == '\n')
        return base64Decode(data, size, j);

    return MyDecode(data, size, j);
  }

  std::string getCredentialProperty(const Arc::Credential& u, const std::string& property, const std::string& ca_cert_dir, const std::string& ca_cert_file, bool system_ca_dir, bool system_ca_file, const std::string& vomsdir, const std::vector<std::string>& voms_trust_list) {
    if (property == "dn"){
        return u.GetIdentityName();
    }
    // If it was not DN, then we have to deal with VOMS
    std::vector<VOMSACInfo> output;
    VOMSTrustList vomstrustlist(voms_trust_list);
    bool verify = false;
    if(vomstrustlist.SizeRegexs() || vomstrustlist.SizeChains())verify = true;
    parseVOMSAC(u,ca_cert_dir,ca_cert_file,system_ca_dir,system_ca_file,vomsdir,vomstrustlist,output,verify);
    if (property == "voms:vo"){
        if (output.empty()) {
                // if it's not possible to determine the VO -- such jobs will go into generic share
                return "";
        } else {
                // Using first vo in list
                return output[0].voname;
        }
    }
    else if (property == "voms:role"){
        size_t pos1, pos2;
        unsigned int i;
        unsigned int n;
        std::string vo_name;
        for (n=0;n<output.size();++n) {
                if(output[n].voname.empty()) continue;
                if(vo_name.empty()) vo_name = output[n].voname;
                for (i=0;i<output[n].attributes.size();++i){
                        std::string attribute = output[n].attributes[i];
                        pos1 = attribute.find("/Role=");
                        if(pos1 != std::string::npos){
                                pos2 = attribute.find("/",pos1+1);
                                std::string role;
                                if(pos2 == std::string::npos)
                                        role = attribute.substr(pos1+6,attribute.length()-pos1-6);
                                else
                                        role = attribute.substr(pos1+6,pos2-pos1-6);
                                return output[n].voname+":"+role;
                        }
                }
        }
        if(vo_name.empty()) return "";
        return vo_name+":null"; // Primary VO with no role
    }
    else if (property == "voms:group"){
        size_t pos1, pos2;
        unsigned int i;
        unsigned int n;
        std::string vo_name;
        for (n=0;n<output.size();++n) {
                if(output[n].voname.empty()) continue;
                if(vo_name.empty()) vo_name = output[n].voname;
                for (i=0;i<output[n].attributes.size();++i){
                        std::string attribute = output[n].attributes[i];
                        pos1 = attribute.find("/Group=");
                        if(pos1 != std::string::npos){
                                pos2 = attribute.find("/",pos1+1);
                                std::string group;
                                if(pos2 == std::string::npos)
                                        group = attribute.substr(pos1+7,attribute.length()-pos1-7);
                                else
                                        group = attribute.substr(pos1+7,pos2-pos1-7);
                                return "/"+output[n].voname+"/"+group;
                                //vo_name.insert(vo_name.end(),'/');
                                //vo_name.insert(vo_name.length(),group);
                                ///return vo_name;
                        }
                }
        }
        if(vo_name.empty()) return "";
        return "/"+vo_name; // Primary VO with no group
    }
    else if (property == "voms:nickname"){
        size_t pos1;
        unsigned int i;
        unsigned int n;
        std::string vo_name;
        for (n=0;n<output.size();++n) {
                if(output[n].voname.empty()) continue;
                if(vo_name.empty()) vo_name = output[n].voname;
                for (i=0;i<output[n].attributes.size();++i){
                        std::string attribute = output[n].attributes[i];
                        pos1 = attribute.find("nickname=");
                        if(pos1 != std::string::npos){
                                std::string nickname(attribute.substr(pos1+9));
                                return nickname;
                        }
                }
        }
    }

    return "";
  }

  std::string VOMSFQANToFull(const std::string& voname, const std::string& fqan) {
    std::list<std::string> elements;
    tokenize(fqan,elements,"/");
    if(elements.empty()) return fqan; // No idea how to handle
    std::list<std::string>::iterator element = elements.begin();
    if(element->find('=') != std::string::npos) return fqan; // Already full
    // Insert Group= into every group part
    std::string fqan_ = "/Group="+(*element);
    for(++element;element!=elements.end();++element) {
      if(element->find('=') != std::string::npos) break;
      fqan_ += "/Group="+(*element);
    }
    for(;element!=elements.end();++element) {
      fqan_ += "/"+(*element);
    }
    if(!voname.empty()) fqan_ = std::string("/VO=")+voname+fqan_;
    return fqan_;
  }

  bool VOMSACSeqEncode(const std::string& ac_seq, std::string& asn1) {
    bool ret = false;
    if(ac_seq.empty()) return false;
    Arc::Credential::X509_EXTENSIONRef ext(X509V3_EXT_conf_nid(NULL, NULL, OBJ_txt2nid(acseqOID), (char*)(ac_seq.c_str())));
    if(ext) {
      asn1.clear();
      asn1.assign((const char*) ASN1_STRING_get0_data(X509_EXTENSION_get_data(ext)), ASN1_STRING_length(X509_EXTENSION_get_data(ext)));
      ret = true;
    }
    return ret;
  }

  bool VOMSACSeqEncode(const std::list<std::string> acs, std::string& asn1) {
    std::string ac_seq;
    std::list<std::string>::const_iterator it;
    for(it = acs.begin(); it != acs.end(); it++) {
      std::string ac = (*it);
      ac_seq.append(VOMS_AC_HEADER).append("\n");
      ac_seq.append(ac).append("\n");
      ac_seq.append(VOMS_AC_TRAILER).append("\n");
    }
    return VOMSACSeqEncode(ac_seq, asn1);
  }


  // The attributes passed to this method are of "extended fqan" kind with every field
  // made of key=value pair. Also each attribute has /VO=voname prepended.
  // Special ARC attribute /voname=voname/hostname=hostname is used for assigning
  // server host name to VO.
  std::string VOMSFQANFromFull(const std::string& attribute) {
    std::list<std::string> elements;
    Arc::tokenize(attribute, elements, "/");
    // Handle first element which contains VO name
    std::list<std::string>::iterator i = elements.begin();
    if(i == elements.end()) return ""; // empty attribute?
    // Handle first element which contains VO name
    std::string fqan_voname;
    std::vector<std::string> keyvalue;
    Arc::tokenize(*i, keyvalue, "=");
    if (keyvalue.size() != 2) return ""; // improper record
    if (keyvalue[0] == "voname") { // VO to hostname association
      // does not map into FQAN directly
      return "";
    } else if(keyvalue[0] == "VO") {
      fqan_voname = keyvalue[1];
      if(fqan_voname.empty()) {
        return "";
      };
    } else {
      // Skip unknown record
      return "";
    }
    ++i;
    //voms_fqan_t fqan;
    std::string fqan_group;
    std::string fqan_role;
    std::string fqan_capability;
    for (; i != elements.end(); ++i) {
      std::vector<std::string> keyvalue;
      Arc::tokenize(*i, keyvalue, "=");
      // /Group=mygroup/Role=myrole
      // Ignoring unrecognized records
      if (keyvalue.size() == 2) {
        if (keyvalue[0] == "Group") {
          fqan_group += "/"+keyvalue[1];
        } else if (keyvalue[0] == "Role") {
          fqan_role = keyvalue[1];
        } else if (keyvalue[0] == "Capability") {
          fqan_capability = keyvalue[1];
        };
      };
    };
    std::string fqan = fqan_group;
    if(!fqan_role.empty()) fqan += "/Role="+fqan_role;
    if(!fqan_capability.empty()) fqan += "/Capability="+fqan_capability;
    return fqan;
  }


} // namespace Arc
