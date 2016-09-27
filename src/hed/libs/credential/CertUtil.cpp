#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <arc/Logger.h>

#include <openssl/x509v3.h>
#include <openssl/err.h>

#include "CertUtil.h"

#define X509_CERT_DIR  "X509_CERT_DIR"

#ifndef WIN32
#define FILE_SEPARATOR "/"
#else
#define FILE_SEPARATOR "\\"
#endif
#define SIGNING_POLICY_FILE_EXTENSION   ".signing_policy"


namespace ArcCredential {

#if (OPENSSL_VERSION_NUMBER < 0x10100000L)

static X509_OBJECT* X509_OBJECT_new(void) {
  X509_OBJECT* obj = (X509_OBJECT*)std::malloc(sizeof(X509_OBJECT));
  if(obj) {
    std::memset(obj, 0, sizeof(X509_OBJECT));
  }
}

static void X509_OBJECT_free(X509_OBJECT* obj) {
  if(obj) {
    X509_OBJECT_free_contents(obj);
    std::free(obj);
  }
}

static X509_CRL *X509_OBJECT_get0_X509_CRL(X509_OBJECT *obj)
{
    if(!obj) return NULL;
    if(obj->type != X509_LU_CRL) return NULL;
    return obj->data.crl;
}

#define X509_STORE_CTX_get0_chain X509_STORE_CTX_get_chain
#define X509_CRL_get0_lastUpdate X509_CRL_get_lastUpdate
#define X509_CRL_get0_nextUpdate X509_CRL_get_nextUpdate

static const ASN1_INTEGER *X509_REVOKED_get0_serialNumber(const X509_REVOKED *x)
{
    if(!x) return NULL;
    return x->serialNumber;
}

#endif

static Arc::Logger& logger = Arc::Logger::rootLogger;

//static int check_issued(X509_STORE_CTX*, X509* x, X509* issuer);
static int verify_callback(int ok, X509_STORE_CTX* store_ctx);
static bool collect_proxy_info(cert_verify_context* vctx, X509* cert);

int verify_cert_chain(X509* cert, STACK_OF(X509)** certchain, cert_verify_context* vctx) {
  int i;
  int j;
  int retval = 0;
  X509_STORE* cert_store = NULL;
  X509_STORE_CTX* store_ctx = NULL;
  X509* cert_in_chain = NULL;
  X509* user_cert = NULL;

  user_cert = cert;
  cert_store = X509_STORE_new();
  X509_STORE_set_verify_cb_func(cert_store, verify_callback);
  if (*certchain != NULL) {
    for (i=0;i<sk_X509_num(*certchain);i++) {
      cert_in_chain = sk_X509_value(*certchain,i);
      if (!user_cert) {
        //Assume the first cert in cert chain is the user cert.
        user_cert = cert_in_chain;
      }
      else {
        j = X509_STORE_add_cert(cert_store, cert_in_chain);
        if (!j) {
          if ((ERR_GET_REASON(ERR_peek_error()) == X509_R_CERT_ALREADY_IN_HASH_TABLE)) {
            ERR_clear_error();
            break;
          }
          else { goto err; }
        }
      }
    }
  }
  if(user_cert == NULL) goto err;

  if (X509_STORE_load_locations(cert_store,
           vctx->ca_file.empty() ? NULL:vctx->ca_file.c_str(),
           vctx->ca_dir.empty() ? NULL:vctx->ca_dir.c_str())) {
    store_ctx = X509_STORE_CTX_new();
    X509_STORE_CTX_init(store_ctx, cert_store, user_cert, NULL);
    //Last parameter is "untrusted", probably related globus code is wrong.

    /* override the check_issued with our version */
    //store_ctx->check_issued = check_issued; todo:

    /*
     * If this is not set, OpenSSL-0.9.8 assumes the proxy cert
     * as an EEC and the next level cert in the chain as a CA cert
     * and throws an invalid CA error. If we set this, the callback
     * (verify_callback) gets called with
     * ok = 0 with an error "unhandled critical extension"
     * and "path length exceeded".
     * verify_callback will check the critical extension later.
     */
    X509_STORE_CTX_set_flags(store_ctx, X509_V_FLAG_ALLOW_PROXY_CERTS);

    if (!X509_STORE_CTX_set_ex_data(store_ctx, VERIFY_CTX_STORE_EX_DATA_IDX, (void *)vctx)) {
      logger.msg(Arc::ERROR,"Can not set the STORE_CTX for chain verification");
      goto err;
    }

    //X509_STORE_CTX_set_depth(store_ctx, 10);

    if(!X509_verify_cert(store_ctx)) { goto err; }
  }

  //Replace the trusted certificate chain after verification passed, the
  //trusted ca certificate is added but top cert is excluded if already
  //stored separately.
  if(store_ctx != NULL) {
    if(*certchain) { sk_X509_pop_free(*certchain, X509_free); }
    *certchain = sk_X509_new_null();
    for (i=(cert)?1:0; i < sk_X509_num(X509_STORE_CTX_get0_chain(store_ctx)); i++) {
      X509* tmp = NULL; tmp = X509_dup(sk_X509_value(X509_STORE_CTX_get0_chain(store_ctx),i));
      sk_X509_insert(*certchain, tmp, i);
    }
  }

  retval = 1;

err:
  if(cert_store) { X509_STORE_free(cert_store); }
  if(store_ctx) { X509_STORE_CTX_free(store_ctx); }

  return retval;
}

int collect_cert_chain(X509* cert, STACK_OF(X509)** certchain, cert_verify_context* vctx) {
  
  if(cert) {
    if(!collect_proxy_info(vctx, cert)) return (0);
  }
  if (*certchain != NULL) {
    for (int i=sk_X509_num(*certchain)-1; i >= 0; --i) {
      X509* cert_in_chain = sk_X509_value(*certchain,i);
      if(cert_in_chain) {
        if(!collect_proxy_info(vctx, cert_in_chain)) return (0);
      }
    }
  }

  return (1);
}

static int verify_callback(int ok, X509_STORE_CTX* store_ctx) {
  cert_verify_context*      vctx;
  vctx = (cert_verify_context *) X509_STORE_CTX_get_ex_data(store_ctx, VERIFY_CTX_STORE_EX_DATA_IDX);
  //TODO get SSL object here, special for GSSAPI
  if(!vctx) { return (0);}

  /*
  * Now check for some error conditions which can be disregarded. *
  if(!ok) {
    switch (X509_STORE_CTX_get_error(store_ctx)) {
    case X509_V_ERR_PATH_LENGTH_EXCEEDED:
      *
      * Since OpenSSL does not know about proxies,
      * it will count them against the path length
      * So we will ignore the errors and do our
      * own checks later on, when we check the last
      * certificate in the chain we will check the chain.
      *
      logger.msg(Arc::DEBUG,"X509_V_ERR_PATH_LENGTH_EXCEEDED");
      ok = 1;
      break;

      *
      * OpenSSL-0.9.8 (because of proxy support) has this error
      *(0.9.7d did not have this, not proxy support still)
      * So we will ignore the errors now and do our checks later
      * on.
      *
    case X509_V_ERR_PROXY_PATH_LENGTH_EXCEEDED:
      logger.msg(Arc::DEBUG,"X509_V_ERR_PATH_LENGTH_EXCEEDED --- with proxy");
      ok = 1;
      break;

#ifdef X509_V_ERR_UNHANDLED_CRITICAL_EXTENSION
      *
      * In the later version (097g+) OpenSSL does know about
      * proxies, but not non-rfc compliant proxies, it will
      * count them as unhandled critical extensions.
      * Of cause before version 097g, any proxy (not only non-rfc proxy) 
      * extension will be count as unhandled critical extensions. 
      * So we will ignore the errors and do our
      * own checks later on, when we check the last
      * certificate in the chain we will check the chain.
      * Since the X509_V_ERR_UNHANDLED_CRITICAL_EXTENSION (with value 34) 
      * starts to be defined in version 097(first version of 097 series), 
      * the detection of this value is used to switch to the following case.
      * As OpenSSL does not recognize legacy proxies (pre-RFC, and older fasion proxies)
      *
    case X509_V_ERR_UNHANDLED_CRITICAL_EXTENSION:
      logger.msg(Arc::DEBUG,"X509_V_ERR_UNHANDLED_CRITICAL_EXTENSION");
      *
      * Setting this for 098 or later versions avoid the invalid
      * CA error but would result in proxy path len exceeded which
      * is handled above. For versions less than 098 and greater
      * than or equal to 097g causes a seg fault in
      * check_chain_extensions (line 498 in crypto/x509/x509_vfy.c)
      * If this flag is set, openssl assumes proxy extensions would
      * definitely be there and tries to access the extensions but
      * the extension is not there really, as it not recognized by
      * openssl. So openssl versions >= 097g and < 098 would
      * consider our proxy as an EEC and higher level proxy in the
      * cert chain (if any) or EEC as a CA cert and thus would throw
      * as invalid CA error. We handle that error below.
      *
      store_ctx->current_cert->ex_flags |= EXFLAG_PROXY;
      ok = 1;
      break;
#endif

#ifdef X509_V_ERR_INVALID_PURPOSE
    case X509_V_ERR_INVALID_PURPOSE:
      *
      * Invalid purpose if we init sec context with a server that does
      * not have the SSL Server Netscape extension (occurs with 0.9.7
      * servers)
      *
      ok = 1;
      break;
#endif

    case X509_V_ERR_INVALID_CA:
    {
      *
      * If the previous cert in the chain is a proxy cert then
      * we get this error just because openssl does not recognize
      * our proxy and treats it as an EEC. And thus, it would
      * treat higher level proxies (if any) or EEC as CA cert
      * (which are not actually CA certs) and would throw this
      * error. As long as the previous cert in the chain is a
      * proxy cert, we ignore this error.
      *
      X509* prev_cert = sk_X509_value(X509_STORE_CTX_get0_chain(store_ctx), X509_STORE_CTX_get_error_depth(store_ctx)-1);
      certType type;
      if(check_cert_type(prev_cert, type)) { if(CERT_IS_PROXY(type)) ok = 1; }
      break;
    }
    default:
      break;
    }
  */



    //if failed, show the error message.
    if(!ok) {
      char * subject_name = X509_NAME_oneline(X509_get_subject_name(X509_STORE_CTX_get_current_cert(store_ctx)), 0, 0);
      unsigned long issuer_hash = X509_issuer_name_hash(X509_STORE_CTX_get_current_cert(store_ctx));

      logger.msg(Arc::DEBUG,"Error number in store context: %i",(int)(X509_STORE_CTX_get_error(store_ctx)));
      if(sk_X509_num(X509_STORE_CTX_get0_chain(store_ctx)) == 1) { logger.msg(Arc::VERBOSE,"Self-signed certificate"); }

      if (X509_STORE_CTX_get_error(store_ctx) == X509_V_ERR_CERT_NOT_YET_VALID) {
        logger.msg(Arc::INFO,"The certificate with subject %s  is not valid",subject_name);
      }
      else if(X509_STORE_CTX_get_error(store_ctx) == X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT_LOCALLY) {
        logger.msg(Arc::INFO,"Can not find issuer certificate for the certificate with subject %s and hash: %lu",subject_name,issuer_hash);
      }
      else if(X509_STORE_CTX_get_error(store_ctx) == X509_V_ERR_CERT_HAS_EXPIRED) {
        logger.msg(Arc::INFO,"Certificate with subject %s has expired",subject_name);
      }
      else if(X509_STORE_CTX_get_error(store_ctx) == X509_V_ERR_SELF_SIGNED_CERT_IN_CHAIN) {
        logger.msg(Arc::INFO,"Untrusted self-signed certificate in chain with "
                   "subject %s and hash: %lu",subject_name,issuer_hash);
      }
      else
        logger.msg(Arc::INFO,"Certificate verification error: %s",X509_verify_cert_error_string(X509_STORE_CTX_get_error(store_ctx)));

      if(subject_name) OPENSSL_free(subject_name);

      return ok;
    }
/*
    X509_STORE_CTX_set_error(store_ctx,0);
    return ok;
  }
*/


  /* All of the OpenSSL tests have passed and we now get to
   * look at the certificate to verify the proxy rules,
   * and ca-signing-policy rules. CRL checking will also be done.
   */

  /*
   * Test if the name ends in CN=proxy and if the issuer
   * name matches the subject without the final proxy.
   */
  certType type;
  bool ret = check_cert_type(X509_STORE_CTX_get_current_cert(store_ctx),type);
  if(!ret) {
    logger.msg(Arc::ERROR,"Can not get the certificate type");
    return 0;
  }

  if(CERT_IS_PROXY(type)){
   /* it is a proxy */
        /* a legacy globus proxy may only be followed by another legacy globus
         * proxy or a limited legacy globus_proxy.
         * a limited legacy globus proxy may only be followed by another
         * limited legacy globus proxy
         * a draft compliant proxy may only be followed by another draft
         * compliant proxy
         * a draft compliant limited proxy may only be followed by another draft
         * compliant limited proxy or a draft compliant independent proxy
         */

    if((CERT_IS_GSI_2_PROXY(vctx->cert_type) && !CERT_IS_GSI_2_PROXY(type)) ||
         (CERT_IS_GSI_3_PROXY(vctx->cert_type) && !CERT_IS_GSI_3_PROXY(type)) ||
         (CERT_IS_RFC_PROXY(vctx->cert_type) && !CERT_IS_RFC_PROXY(type))) {
      logger.msg(Arc::ERROR,"The proxy to be signed should be compatible with the signing certificate: (%s) -> (%s)",certTypeToString(vctx->cert_type),certTypeToString(type));
      return (0);
    }

    /* Limited proxy does not mean it can't be followed by unlimited proxy
       because proxy chain is checked recusrsively according to RFC3820.
       Unless specific limitation put on proxy means it can't be used
       for creating another proxy. But for that purpose proxy depth
       is used.
       In general it is task of authorization to check ALL proxies in 
       the chain for valid policies. *
    *
    if(CERT_IS_LIMITED_PROXY(vctx->cert_type) &&
       !(CERT_IS_LIMITED_PROXY(type) || CERT_IS_INDEPENDENT_PROXY(type))) {
      logger.msg(Arc::ERROR,"Can't sign a non-limited, non-independent proxy with a limited proxy");
      X509_STORE_CTX_set_error(store_ctx,X509_V_ERR_CERT_SIGNATURE_FAILURE);
      return (0);
    }
    */

    vctx->proxy_depth++;
    if((vctx->max_proxy_depth!=-1) &&
       (vctx->max_proxy_depth < vctx->proxy_depth)) {
      logger.msg(Arc::ERROR,"The proxy depth %i is out of maximum limit %i",vctx->proxy_depth,vctx->max_proxy_depth);
      return (0);
    }
    // vctx stores previous certificate type
    vctx->cert_type=type;
  }
	
  /** We need to check whether the certificate is revoked if it is not a proxy;
   *for proxy, it does not ever get revoked
   */
  if(vctx->cert_type == CERT_TYPE_EEC || vctx->cert_type == CERT_TYPE_CA) {
#ifdef X509_V_ERR_CERT_REVOKED
        /*
         * SSLeay 0.9.0 handles CRLs but does not check them.
         * We will check the crl for this cert, if there
         * is a CRL in the store.
         * If we find the crl is not valid, we will fail,
         * as once the sysadmin indicates that CRLs are to
         * be checked, he best keep it upto date.
         *
         * When future versions of SSLeay support this better,
         * we can remove these tests.
         * we come through this code for each certificate,
         * starting with the CA's We will check for a CRL
         * each time, but only check the signature if the
         * subject name matches, and check for revoked
         * if the issuer name matches.
         * this allows the CA to revoke its own cert as well.
         */
    int i, n;
    X509_CRL *      crl = NULL;
    X509_REVOKED *  revoked = NULL;;
    EVP_PKEY *key = NULL;
    X509_OBJECT*    obj = X509_OBJECT_new();
    if(!obj) return (0);

    /**TODO: In globus code, it check the "issuer, not "subject", because it also includes the situation of proxy?
     * (For proxy, the up-level/issuer need to be checked?)
     */
    if (X509_STORE_get_by_subject(store_ctx, X509_LU_CRL, X509_get_subject_name(X509_STORE_CTX_get_current_cert(store_ctx)), obj)) {
      if(crl=X509_OBJECT_get0_X509_CRL(obj)) {
        /* verify the signature on this CRL */
        key = X509_get_pubkey(X509_STORE_CTX_get_current_cert(store_ctx));
        if (X509_CRL_verify(crl, key) <= 0) {
          X509_STORE_CTX_set_error(store_ctx,X509_V_ERR_CRL_SIGNATURE_FAILURE);
          // TODO: tell which crl failed
          logger.msg(Arc::ERROR,"Couldn't verify availability of CRL");
          EVP_PKEY_free(key);
          X509_OBJECT_free(obj);
          return (0);
        }

        /* Check date see if expired */
        i = X509_CRL_get0_lastUpdate(crl) ? X509_cmp_current_time(X509_CRL_get0_lastUpdate(crl)) : 0;
        if (i == 0) {
          X509_STORE_CTX_set_error(store_ctx,X509_V_ERR_ERROR_IN_CRL_LAST_UPDATE_FIELD);
          // TODO: tell which crl failed
          logger.msg(Arc::ERROR,"In the available CRL the lastUpdate field is not valid");
          EVP_PKEY_free(key);
          X509_OBJECT_free(obj);
          return (0);
        }
        if(i>0) {
          X509_STORE_CTX_set_error(store_ctx,X509_V_ERR_CRL_NOT_YET_VALID);
          // TODO: tell which crl failed
          logger.msg(Arc::ERROR,"The available CRL is not yet valid");
          EVP_PKEY_free(key);
          X509_OBJECT_free(obj);
          return (0);
        }

        i = X509_CRL_get0_nextUpdate(crl) ? X509_cmp_current_time(X509_CRL_get0_nextUpdate(crl)) : 1;
        if (i == 0) {
          X509_STORE_CTX_set_error(store_ctx,X509_V_ERR_ERROR_IN_CRL_NEXT_UPDATE_FIELD);
          // TODO: tell which crl failed
          logger.msg(Arc::ERROR,"In the available CRL, the nextUpdate field is not valid");
          EVP_PKEY_free(key);
          X509_OBJECT_free(obj);
          return (0);
        }

        if (i < 0) {
          X509_STORE_CTX_set_error(store_ctx,X509_V_ERR_CRL_HAS_EXPIRED);
          logger.msg(Arc::ERROR,"The available CRL has expired");
          EVP_PKEY_free(key);
          X509_OBJECT_free(obj);
          return (0);
        }
        EVP_PKEY_free(key);
      }
    }

    /* now check if the issuer has a CRL, and we are revoked */
    if (X509_STORE_get_by_subject(store_ctx, X509_LU_CRL, X509_get_issuer_name(X509_STORE_CTX_get_current_cert(store_ctx)), obj)) {
      if(crl=X509_OBJECT_get0_X509_CRL(obj)) {
        /* check if this cert is revoked */
        n = sk_X509_REVOKED_num(X509_CRL_get_REVOKED(crl));
        for (i=0; i<n; i++) {
          revoked = (X509_REVOKED *)sk_X509_REVOKED_value(X509_CRL_get_REVOKED(crl),i);
          if(!ASN1_INTEGER_cmp(X509_REVOKED_get0_serialNumber(revoked), X509_get_serialNumber(X509_STORE_CTX_get_current_cert(store_ctx)))) {
            long serial;
            char buf[256];
            char* subject_string;
            serial = ASN1_INTEGER_get(X509_REVOKED_get0_serialNumber(revoked));
            snprintf(buf, sizeof(buf), "%ld (0x%lX)",serial,serial);
            subject_string = X509_NAME_oneline(X509_get_subject_name(X509_STORE_CTX_get_current_cert(store_ctx)),NULL,0);
            logger.msg(Arc::ERROR,"Certificate with serial number %s and subject \"%s\" is revoked",buf,subject_string);
            X509_STORE_CTX_set_error(store_ctx,X509_V_ERR_CERT_REVOKED);
            OPENSSL_free(subject_string);
            X509_OBJECT_free(obj);
            return (0);
          }
        }
      }
    }
    X509_OBJECT_free(obj);
#endif /* X509_V_ERR_CERT_REVOKED */


    /** Only need to check signing policy file for no-proxy certificate*/
    std::string cadir;
    if (X509_NAME_cmp(X509_get_subject_name(X509_STORE_CTX_get_current_cert(store_ctx)), X509_get_issuer_name(X509_STORE_CTX_get_current_cert(store_ctx)))) {

      cadir = vctx->ca_dir;
      if(cadir.empty()) {
        logger.msg(Arc::INFO,"Directory of trusted CAs is not specified/found; Using current path as the CA direcroty");
        cadir = ".";
      }

      unsigned long hash = X509_NAME_hash(X509_get_issuer_name(X509_STORE_CTX_get_current_cert(store_ctx)));
      unsigned int buffer_len = cadir.length() + strlen(FILE_SEPARATOR) + 8 * hash *
        + strlen(SIGNING_POLICY_FILE_EXTENSION) + 1 /* zero termination */;
      char* ca_policy_file_path = (char*) malloc(buffer_len);
      if(ca_policy_file_path == NULL) {
        logger.msg(Arc::ERROR,"Can't allocate memory for CA policy path");
        X509_STORE_CTX_set_error(store_ctx,X509_V_ERR_APPLICATION_VERIFICATION);
        return (0);
      }
      snprintf(ca_policy_file_path,buffer_len,"%s%s%08lx%s", cadir.c_str(), FILE_SEPARATOR, hash, SIGNING_POLICY_FILE_EXTENSION);
      ca_policy_file_path[buffer_len-1]=0;

      //TODO check the certificate against policy

      free(ca_policy_file_path);

    }
    return (1); //Every thing that need to check for non-proxy has been passed
  }

  /**Add the current certificate into cert chain*/
  if(vctx->cert_chain == NULL) { vctx->cert_chain = sk_X509_new_null(); }
  sk_X509_push(vctx->cert_chain, X509_dup(X509_STORE_CTX_get_current_cert(store_ctx)));
  vctx->cert_depth++;

  if(!collect_proxy_info(vctx, X509_STORE_CTX_get_current_cert(store_ctx))) {
    X509_STORE_CTX_set_error(store_ctx,X509_V_ERR_CERT_REJECTED);
    return (0);
  }

  /*
  * We ignored any path length restrictions above because
  * OpenSSL was counting proxies against the limit.
  * If we are on the last cert in the chain, we
  * know how many are proxies, so we can do the
  * path length check now.
  * See x509_vfy.c check_chain_purpose
  * all we do is substract off the proxy_dpeth
  */
  /*
  if(X509_STORE_CTX_get_current_cert(store_ctx) == store_ctx->cert) {
    if(X509_STORE_CTX_get0_chain(store_ctx)) for (i=0; i < sk_X509_num(X509_STORE_CTX_get0_chain(store_ctx)); i++) {
      X509* cert = sk_X509_value(X509_STORE_CTX_get0_chain(store_ctx),i);
      if (((i - vctx->proxy_depth) > 1) && (cert->ex_pathlen != -1)
               && ((i - vctx->proxy_depth) > (cert->ex_pathlen + 1))
               && (cert->ex_flags & EXFLAG_BCONS)) {
        store_ctx->current_cert = cert; * point at failing cert *
        X509_STORE_CTX_set_error(store_ctx,X509_V_ERR_PATH_LENGTH_EXCEEDED);
        return (0);
      }
    }
  }
  */

  return (1);
}

static bool collect_proxy_info(cert_verify_context* vctx, X509* cert) {
  /**Check the proxy certificate infomation extension*/
  X509_EXTENSION* ext;
  ASN1_OBJECT* extension_obj;
  int i;
  for (i=0;i<X509_get_ext_count(cert);i++) {
    ext = (X509_EXTENSION *) X509_get_ext(cert,i);
    if(X509_EXTENSION_get_critical(ext)) {
      extension_obj = X509_EXTENSION_get_object(ext);
      int nid = OBJ_obj2nid(extension_obj);
      if(nid != NID_basic_constraints &&
         nid != NID_key_usage &&
         nid != NID_ext_key_usage &&
         nid != NID_netscape_cert_type &&
         nid != NID_subject_key_identifier &&
         nid != NID_authority_key_identifier // &&
         //nid != OBJ_sn2nid("PROXYCERTINFO_V3") &&
         //nid != OBJ_sn2nid("PROXYCERTINFO_V4") &&
         //nid != OBJ_sn2nid("OLD_PROXYCERTINFO") &&
         //nid != OBJ_sn2nid("PROXYCERTINFO")
         && nid != NID_proxyCertInfo // TODO: keep only this?
        ) {
        logger.msg(Arc::ERROR,"Certificate has unknown extension with numeric ID %u and SN %s",(unsigned int)nid,OBJ_nid2sn(nid));
        return false;
      }


      if(nid == NID_proxyCertInfo) {
// TODO: do not use openssl version - instead use result of check if
// proxy extension is supported
      /* If the openssl version >=097g (which means proxy cert info is
       * supported), and NID_proxyCertInfo can be got from the extension,
       * then we use the proxy cert info support from openssl itself.
       * Otherwise we have to use globus-customized proxy cert info support.
       */
      PROXY_CERT_INFO_EXTENSION*  proxycertinfo = NULL;
      proxycertinfo = (PROXY_CERT_INFO_EXTENSION*) X509V3_EXT_d2i(ext);
      if (proxycertinfo == NULL) {
        logger.msg(Arc::WARNING,"Can not convert DER encoded PROXY_CERT_INFO_EXTENSION extension to internal format");
      } else {
        int path_length = ASN1_INTEGER_get(proxycertinfo->pcPathLengthConstraint);
        /* ignore negative values */
        if(path_length > -1) {
          if(vctx->max_proxy_depth == -1 || vctx->max_proxy_depth > vctx->proxy_depth + path_length) {
            vctx->max_proxy_depth = vctx->proxy_depth + path_length;
            logger.msg(Arc::DEBUG,"proxy_depth: %i, path_length: %i",(int)(vctx->proxy_depth),(int)path_length);
          }
        }
        /**Parse the policy*/
        // Must be proxy because proxy extension is set - if(X509_STORE_CTX_get_current_cert(store_ctx)->ex_flags & EXFLAG_PROXY)
        {
          switch (OBJ_obj2nid(proxycertinfo->proxyPolicy->policyLanguage)) {
            case NID_Independent:
               /* Put whatever explicit policy here to this particular proxy certificate, usually by
                * pulling them from some database. If there is none policy which need to be explicitly
                * inserted here, clear all the policy storage (make this and any subsequent proxy certificate
                * be void of any policy, because here the policylanguage is independent)
                */
              vctx->proxy_policy.clear();
              break;
            case NID_id_ppl_inheritAll:
               /* This is basically a NOP */
              break;
            default:
              /* Here get the proxy policy */
              vctx->proxy_policy.clear();
              if((proxycertinfo->proxyPolicy) &&
                 (proxycertinfo->proxyPolicy->policy) &&
                 (proxycertinfo->proxyPolicy->policy->data)) {
                vctx->proxy_policy.append(
                   (char const*)(proxycertinfo->proxyPolicy->policy->data),
                   proxycertinfo->proxyPolicy->policy->length);
              }
              /* Use : as separator for policies parsed from different proxy certificate*/
              /* !!!! Taking int account previous proxy_policy.clear() !!!!
                 !!!! it seems to be impossible to have more than one    !!!!
                 !!!!  policy collected anyway !!!! */
              vctx->proxy_policy.append(":");
              break;
          }
        }
        PROXY_CERT_INFO_EXTENSION_free(proxycertinfo);
        proxycertinfo = NULL;
      }
      }
    }
  }
  return true;
}


bool check_cert_type(X509* cert, certType& type) {
  logger.msg(Arc::DEBUG, "Trying to check X509 cert with check_cert_type");

  bool ret = false;
  type = CERT_TYPE_EEC;

  ASN1_STRING* data;
  X509_EXTENSION* certinfo_ext;
  //PROXY_CERT_INFO_EXTENSION* certinfo = NULL;
  PROXY_POLICY* policy = NULL;
  ASN1_OBJECT* policylang = NULL;
  int policynid;

  int index;
  int critical;
  BASIC_CONSTRAINTS* x509v3_bc = NULL;
  if(!cert) return false;
  if((x509v3_bc = (BASIC_CONSTRAINTS*) X509_get_ext_d2i(cert,
    NID_basic_constraints, &critical, NULL)) && x509v3_bc->ca) {
    type = CERT_TYPE_CA;
    if(x509v3_bc) { BASIC_CONSTRAINTS_free(x509v3_bc); }
    return true;
  }

  X509_NAME* issuer = NULL;
  X509_NAME* subject = X509_get_subject_name(cert);
  X509_NAME_ENTRY * name_entry = NULL;
  if(!subject) goto err;
  name_entry = X509_NAME_get_entry(subject, X509_NAME_entry_count(subject)-1);
  if(!name_entry) goto err;
  if (!OBJ_cmp(X509_NAME_ENTRY_get_object(name_entry),OBJ_nid2obj(NID_commonName))) {
    /* the name entry is of the type: common name */
    data = X509_NAME_ENTRY_get_data(name_entry);
    if(!data) goto err;
    if (data->length == 5 && !memcmp(data->data,"proxy",5)) { type = CERT_TYPE_GSI_2_PROXY; }
    else if(data->length == 13 && !memcmp(data->data,"limited proxy",13)) { type = CERT_TYPE_GSI_2_LIMITED_PROXY; }
    else if((index = X509_get_ext_by_NID(cert, NID_proxyCertInfo, -1)) != -1) {
      certinfo_ext = X509_get_ext(cert,index);
      if(X509_EXTENSION_get_critical(certinfo_ext)) {
        PROXY_CERT_INFO_EXTENSION* certinfo_openssl = NULL;
        PROXY_POLICY* policy_openssl = NULL;
        ASN1_OBJECT* policylang_openssl = NULL;        
        if((certinfo_openssl = (PROXY_CERT_INFO_EXTENSION *)X509V3_EXT_d2i(certinfo_ext)) == NULL) {
          logger.msg(Arc::ERROR,"Can't convert DER encoded PROXYCERTINFO extension to internal form");
          goto err;
        }
        if((policy_openssl = certinfo_openssl->proxyPolicy) == NULL) {
          logger.msg(Arc::ERROR,"Can't get policy from PROXYCERTINFO extension");
          goto err;
        }
        if((policylang_openssl = policy_openssl->policyLanguage) == NULL) {
          logger.msg(Arc::ERROR,"Can't get policy language from PROXYCERTINFO extension");
          goto err;
        }
        policynid = OBJ_obj2nid(policylang_openssl);
        if(policynid == NID_id_ppl_inheritAll) { type = CERT_TYPE_RFC_IMPERSONATION_PROXY; }
        else if(policynid == NID_Independent) { type = CERT_TYPE_RFC_INDEPENDENT_PROXY; }
        else if(policynid == NID_id_ppl_anyLanguage) { type = CERT_TYPE_RFC_ANYLANGUAGE_PROXY; }
        else { type = CERT_TYPE_RFC_RESTRICTED_PROXY; }

        //if((index = X509_get_ext_by_NID(cert, OBJ_txt2nid(PROXYCERTINFO_V3), -1)) != -1) {
        //  logger.msg(Arc::ERROR,"Found more than one PCI extension");
        //  goto err;
        //}
      }
    }

    /*Duplicate the issuer, and add the CN=proxy, or CN=limitedproxy, etc.
     * This should match the subject. i.e. proxy can only be signed by
     * the owner.  We do it this way, to double check all the ANS1 bits
     * as well.
     */
    X509_NAME_ENTRY* new_name_entry = NULL;
    if(type != CERT_TYPE_EEC && type != CERT_TYPE_CA) {
      issuer = X509_NAME_dup(X509_get_issuer_name(cert));
      new_name_entry = X509_NAME_ENTRY_create_by_NID(NULL, NID_commonName, V_ASN1_APP_CHOOSE, data->data, -1);
      if(!new_name_entry) goto err;
      X509_NAME_add_entry(issuer,new_name_entry,X509_NAME_entry_count(issuer),0);
      X509_NAME_ENTRY_free(new_name_entry);
      new_name_entry = NULL;

      if (X509_NAME_cmp(issuer, subject)) {
        /* Reject this certificate, only the user may sign the proxy */
        logger.msg(Arc::ERROR,"The subject does not match the issuer name + proxy CN entry");
        goto err;
      }
      X509_NAME_free(issuer);
      issuer = NULL;
    }
  }
  ret = true;

err:
  if(issuer) { X509_NAME_free(issuer); }
  //if(certinfo) {PROXY_CERT_INFO_EXTENSION_free(certinfo);}
  if(x509v3_bc) { BASIC_CONSTRAINTS_free(x509v3_bc); }

  return ret;
}

/**Replace the OpenSSL check_issued in x509_vfy.c with our own,
 *so we can override the key usage checks if it's a proxy.
 *We are only looking for X509_V_ERR_KEYUSAGE_NO_CERTSIGN
*/
/*
static int check_issued(X509_STORE_CTX*, X509* x, X509* issuer) {
  int  ret;
  int  ret_code = 1;

  ret = X509_check_issued(issuer, x);
  if (ret != X509_V_OK) {
    ret_code = 0;
    switch (ret) {
      case X509_V_ERR_AKID_SKID_MISMATCH:
            *
             * If the proxy was created with a previous version of Globus
             * where the extensions where copied from the user certificate
             * This error could arise, as the akid will be the wrong key
             * So if its a proxy, we will ignore this error.
             * We should remove this in 12/2001
             * At which time we may want to add the akid extension to the proxy.
             *
      case X509_V_ERR_KEYUSAGE_NO_CERTSIGN:
            *
             * If this is a proxy certificate then the issuer
             * does not need to have the key_usage set.
             * So check if its a proxy, and ignore
             * the error if so.
             *
        certType type;
        check_cert_type(x, type);
        if (CERT_IS_PROXY(type)) { ret_code = 1; }
        break;
      default:
        break;
      }
    }
    return ret_code;
}
*/

const char* certTypeToString(certType type) {
  switch(type) {
    case CERT_TYPE_EEC:
    case CERT_TYPE_CA:
      return "CA certificate";
    case CERT_TYPE_GSI_3_IMPERSONATION_PROXY:
      return "X.509 Proxy Certificate Profile (pre-RFC) compliant impersonation proxy";
    case CERT_TYPE_GSI_3_INDEPENDENT_PROXY:
      return "X.509 Proxy Certificate Profile (pre-RFC) compliant independent proxy";
    case CERT_TYPE_GSI_3_LIMITED_PROXY:
      return "X.509 Proxy Certificate Profile (pre-RFC) compliant limited proxy";
    case CERT_TYPE_GSI_3_RESTRICTED_PROXY:
      return "X.509 Proxy Certificate Profile (pre-RFC) compliant restricted proxy";
    case CERT_TYPE_GSI_2_PROXY:
      return "Legacy Globus impersonation proxy";
    case CERT_TYPE_GSI_2_LIMITED_PROXY:
      return "Legacy Globus limited impersonation proxy";
    case CERT_TYPE_RFC_IMPERSONATION_PROXY:
      return "X.509 Proxy Certificate Profile RFC compliant impersonation proxy - RFC inheritAll proxy";
    case CERT_TYPE_RFC_INDEPENDENT_PROXY:
      return "X.509 Proxy Certificate Profile RFC compliant independent proxy - RFC independent proxy";
    case CERT_TYPE_RFC_LIMITED_PROXY:
      return "X.509 Proxy Certificate Profile RFC compliant limited proxy";
    case CERT_TYPE_RFC_RESTRICTED_PROXY:
      return "X.509 Proxy Certificate Profile RFC compliant restricted proxy";
    case CERT_TYPE_RFC_ANYLANGUAGE_PROXY:
      return "RFC anyLanguage proxy";
    default:
      return "Unknown certificate type";
  }
}

} // namespace ArcCredential

