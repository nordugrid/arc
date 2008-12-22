#include <iostream>
#include <cstring>

#include <openssl/err.h>

#include "CertUtil.h"

#define X509_CERT_DIR  "X509_CERT_DIR"

#ifndef WIN32
#define FILE_SEPERATOR "/"
#else
#define FILE_SEPERATOR "\\"
#endif
#define SIGNING_POLICY_FILE_EXTENSION   ".signing_policy"

namespace ArcCredential {

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

  if (X509_STORE_load_locations(cert_store, vctx->ca_file.empty() ? NULL:vctx->ca_file.c_str(), 
     vctx->ca_dir.empty() ? NULL:vctx->ca_dir.c_str())) {
    store_ctx = X509_STORE_CTX_new();
    X509_STORE_CTX_init(store_ctx, cert_store, user_cert,NULL); //Last parameter is "untrusted", probably related globus code is wrong.

#if SSLEAY_VERSION_NUMBER >=  0x0090600fL
    /* override the check_issued with our version */
    store_ctx->check_issued = check_issued;
#endif

    /*
     * If this is not set, OpenSSL-0.9.8 assumes the proxy cert 
     * as an EEC and the next level cert in the chain as a CA cert
     * and throws an invalid CA error. If we set this, the callback
     * (verify_callback) gets called with 
     * ok = 0 with an error "unhandled critical extension" 
     * and "path length exceeded".
     * verify_callback will check the critical extension later.
     */
#if (OPENSSL_VERSION_NUMBER >= 0x0090800fL)
    X509_STORE_CTX_set_flags(store_ctx, X509_V_FLAG_ALLOW_PROXY_CERTS);
#endif

    if (!X509_STORE_CTX_set_ex_data(store_ctx, VERIFY_CTX_STORE_EX_DATA_IDX, (void *)vctx)) {
      std::cerr<<"Can not set the STORE_CTX"<<std::endl; goto err;
    }
  
    //X509_STORE_CTX_set_depth(store_ctx, 10);   
               
    if(!X509_verify_cert(store_ctx)) { goto err; }
  } 

  //Replace the trusted certificate chain after verification passed, the
  //trusted ca certificate is added
  if(*certchain) { sk_X509_pop_free(*certchain, X509_free); }
  *certchain = sk_X509_new_null();
  for (i=0; i < sk_X509_num(store_ctx->chain); i++) {
    X509* tmp = NULL; tmp = X509_dup(sk_X509_value(store_ctx->chain,i));
    sk_X509_insert(*certchain, tmp, i);
  }

  retval = 1;

err:
  if(cert_store) { X509_STORE_free(cert_store); }
  if(store_ctx) { X509_STORE_CTX_free(store_ctx); }
   
  return retval;
}

int verify_callback(int ok, X509_STORE_CTX* store_ctx) {
  cert_verify_context*      vctx;
  vctx = (cert_verify_context *) X509_STORE_CTX_get_ex_data(store_ctx, VERIFY_CTX_STORE_EX_DATA_IDX);
  //TODO get SSL object here, special for GSSAPI
  if(!vctx) { return (0);} 
 
  /* Now check for some error conditions which can be disregarded. */
  if(!ok) {
    switch (store_ctx->error) {
    case X509_V_ERR_PATH_LENGTH_EXCEEDED:
      /*
      * Since OpenSSL does not know about proxies,
      * it will count them against the path length
      * So we will ignore the errors and do our
      * own checks later on, when we check the last
      * certificate in the chain we will check the chain.
      */
      std::cout<<"X509_V_ERR_PATH_LENGTH_EXCEEDED"<<std::endl;  

#if (OPENSSL_VERSION_NUMBER >= 0x0090800fL)
      /*
      * OpenSSL-0.9.8 (because of proxy support) has this error 
      *(0.9.7d did not have this, not proxy support still)
      * So we will ignore the errors now and do our checks later
      * on.
      */
    case X509_V_ERR_PROXY_PATH_LENGTH_EXCEEDED:
      std::cout<<"X509_V_ERR_PATH_LENGTH_EXCEEDED --- with proxy"<<std::endl;
      ok = 1;
      break;
#endif

#if (OPENSSL_VERSION_NUMBER > 0x0090706fL)
      /*
      * In the later version (097g+) OpenSSL does know about 
      * proxies, but not non-rfc compliant proxies, it will 
      * count them as unhandled critical extensions.
      * So we will ignore the errors and do our
      * own checks later on, when we check the last
      * certificate in the chain we will check the chain.
      * As OpenSSL does not recognize legacy proxies (pre-RFC, and older fasion proxies)
      */
    case X509_V_ERR_UNHANDLED_CRITICAL_EXTENSION:
      //std::cout<<"X509_V_ERR_UNHANDLED_CRITICAL_EXTENSION"<<std::endl;
      /*
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
      */
  #if (OPENSSL_VERSION_NUMBER >= 0x0090800fL)
      store_ctx->current_cert->ex_flags |= EXFLAG_PROXY;
  #endif
      ok = 1;
      break;
    case X509_V_ERR_INVALID_PURPOSE:
      /*
      * Invalid purpose if we init sec context with a server that does
      * not have the SSL Server Netscape extension (occurs with 0.9.7
      * servers)
      */
      ok = 1;
      break;
#endif

#if (OPENSSL_VERSION_NUMBER > 0x0090706fL)
    case X509_V_ERR_INVALID_CA:
    {
      /*
      * If the previous cert in the chain is a proxy cert then
      * we get this error just because openssl does not recognize 
      * our proxy and treats it as an EEC. And thus, it would
      * treat higher level proxies (if any) or EEC as CA cert 
      * (which are not actually CA certs) and would throw this
      * error. As long as the previous cert in the chain is a
      * proxy cert, we ignore this error.
      */
      X509* prev_cert = sk_X509_value(store_ctx->chain, store_ctx->error_depth-1);
      certType type;
      if(check_cert_type(prev_cert, type)) { if(CERT_IS_PROXY(type)) ok = 1; } 
      break;
    }
#endif	
    default:
      break;
    } 

    //if failed, show the error message.
    if(!ok) {
      char * subject_name = X509_NAME_oneline(X509_get_subject_name(store_ctx->current_cert), 0, 0);
      unsigned long issuer_hash = X509_issuer_name_hash(store_ctx->current_cert);

      std::cout<<"Error number in store context: "<<store_ctx->error<<std::endl;
      if(sk_X509_num(store_ctx->chain) ==1) {std::cout<<"Self-signed certificate"<<std::endl; }

      if (store_ctx->error == X509_V_ERR_CERT_NOT_YET_VALID) {
        std::cerr<<"The certificate with subject "<<subject_name<<" is not valid"<<std::endl;
      }
      else if(store_ctx->error == X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT_LOCALLY) {
        std::cerr<<"Can not find issuer certificate for the certificate with subject"<<subject_name<<"and hash: "<<issuer_hash<<std::endl;
      }
      else if(store_ctx->error == X509_V_ERR_CERT_HAS_EXPIRED) {
        std::cerr<<"Certificate with subject "<<subject_name<<" has expired"<<std::endl;
      }
      else if(store_ctx->error == X509_V_ERR_SELF_SIGNED_CERT_IN_CHAIN) {
        std::cerr<<"Untrusted self-signed certificate in chain with subject:  "<<subject_name<<"and hash: "<<issuer_hash<<std::endl;
      }
      else { std::cerr<<"Certificate verification error: "<< X509_verify_cert_error_string(store_ctx->error) <<std::endl; }

      if(subject_name) OPENSSL_free(subject_name);

      return ok;
    }
    store_ctx->error = 0;
    return ok;
  }

  /* All of the OpenSSL tests have passed and we now get to 
   * look at the certificate to verify the proxy rules, 
   * and ca-signing-policy rules. CRL checking will also be done.
   */

  /*
   * Test if the name ends in CN=proxy and if the issuer
   * name matches the subject without the final proxy. 
   */
  certType type;
  bool ret = check_cert_type(store_ctx->current_cert,type);  
  if(!ret) { std::cerr<<"Can not get the certificate type"<<std::endl; return (0);}
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
      std::cerr<<"The proxy to be signed should be compatible with the signing certificate "<<vctx->cert_type<<" "<<type<<std::endl;
      return (0);
    }

    if(CERT_IS_LIMITED_PROXY(vctx->cert_type) && 
         !(CERT_IS_LIMITED_PROXY(type) || CERT_IS_INDEPENDENT_PROXY(type))) {
      std::cerr<<"Can't sign a non-limited, non-independent proxy with a limited proxy"<<std::endl;
      store_ctx->error = X509_V_ERR_CERT_SIGNATURE_FAILURE;
      return (0);
    }
     
    vctx->proxy_depth++;  
    if(vctx->max_proxy_depth!=-1 && vctx->max_proxy_depth < vctx->proxy_depth) {
      std::cerr<<"The proxy depth is out of maxium limitation:"<<"max_proxy_depth is "<<vctx->max_proxy_depth<<"current proxy_depth is: "<<vctx->proxy_depth<<std::endl;
      return (0);  
    }
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
    X509_OBJECT     obj;
    X509_CRL *      crl = NULL;
    X509_CRL_INFO * crl_info = NULL;
    X509_REVOKED *  revoked = NULL;;
    EVP_PKEY *key = NULL;

    /**TODO: In globus code, it check the "issuer, not "subject", because it also includes the situation of proxy? 
     * (For proxy, the up-level/issuer need to be checked?) 
     */
    if (X509_STORE_get_by_subject(store_ctx, X509_LU_CRL, X509_get_subject_name(store_ctx->current_cert), &obj)) {
      crl =  obj.data.crl;
      crl_info = crl->crl;
      /* verify the signature on this CRL */
      key = X509_get_pubkey(store_ctx->current_cert);
      if (X509_CRL_verify(crl, key) <= 0) {
        store_ctx->error = X509_V_ERR_CRL_SIGNATURE_FAILURE;
        std::cerr<<"Couldn't verify that the available CRL is valid"<<std::endl;
        EVP_PKEY_free(key); X509_OBJECT_free_contents(&obj); return (0);
      }

      /* Check date see if expired */
      i = X509_cmp_current_time(crl_info->lastUpdate);
      if (i == 0) {
        store_ctx->error = X509_V_ERR_ERROR_IN_CRL_LAST_UPDATE_FIELD;
        std::cerr<<"In the available CRL, the lastUpdate field is not valid"<<std::endl;
        EVP_PKEY_free(key); X509_OBJECT_free_contents(&obj); return (0);
      }
      if(i>0) { 
        store_ctx->error = X509_V_ERR_CRL_NOT_YET_VALID;
        std::cerr<<"The available CRL is not yet valid"<<std::endl;
        EVP_PKEY_free(key); X509_OBJECT_free_contents(&obj); return (0);
      }

      i = (crl_info->nextUpdate != NULL) ? X509_cmp_current_time(crl_info->nextUpdate) : 1;
      if (i == 0) {
        store_ctx->error = X509_V_ERR_ERROR_IN_CRL_NEXT_UPDATE_FIELD;
        std::cerr<<"In the available CRL, the nextUpdate field is not valid"<<std::endl;
        EVP_PKEY_free(key); X509_OBJECT_free_contents(&obj); return (0);
      }
           
      if (i < 0) {
        store_ctx->error = X509_V_ERR_CRL_HAS_EXPIRED;
        std::cerr<<"The available CRL has expired"<<std::endl;
        EVP_PKEY_free(key); X509_OBJECT_free_contents(&obj); return (0);
      }
      EVP_PKEY_free(key);
      X509_OBJECT_free_contents(&obj);
    }

    /* now check if the issuer has a CRL, and we are revoked */
    if (X509_STORE_get_by_subject(store_ctx, X509_LU_CRL, X509_get_issuer_name(store_ctx->current_cert), &obj)) {
      crl = obj.data.crl;
      crl_info = crl->crl;
      /* check if this cert is revoked */
      n = sk_X509_REVOKED_num(crl_info->revoked);
      for (i=0; i<n; i++) {
        revoked = (X509_REVOKED *)sk_X509_REVOKED_value(crl_info->revoked,i);
        if(!ASN1_INTEGER_cmp(revoked->serialNumber, X509_get_serialNumber(store_ctx->current_cert))) {
          long serial;
          char buf[256];
          char* subject_string;
          serial = ASN1_INTEGER_get(revoked->serialNumber);
          sprintf(buf,"%ld (0x%lX)",serial,serial);
          subject_string = X509_NAME_oneline(X509_get_subject_name(store_ctx->current_cert),NULL,0);
          std::cerr<<"Serial number = "<<buf<<" Subject = "<<subject_string<<std::endl;

          store_ctx->error = X509_V_ERR_CERT_REVOKED;
          OPENSSL_free(subject_string);
          X509_OBJECT_free_contents(&obj); return (0);
        }
      }
      X509_OBJECT_free_contents(&obj);
    }
#endif /* X509_V_ERR_CERT_REVOKED */


    /** Only need to check signing policy file for no-proxy certificate*/
    char* cadir = NULL;
    char* ca_policy_file_path = NULL;
    if (X509_NAME_cmp(X509_get_subject_name(store_ctx->current_cert), X509_get_issuer_name(store_ctx->current_cert))) {
      //cadir = vctx->ca_dir.empty() ? getenv(X509_CERT_DIR) : vctx->ca_dir.c_str();
      cadir = (char*)(vctx->ca_dir.c_str());
      if(!cadir) { std::cerr<<"Can not find the directory of trusted CAs"<<std::endl; return (0);}
   
      unsigned int buffer_len; 
      unsigned long hash;
      hash = X509_NAME_hash(X509_get_issuer_name(store_ctx->current_cert));
       
      buffer_len = strlen(cadir) + strlen(FILE_SEPERATOR) + 8 /* hash */
        + strlen(SIGNING_POLICY_FILE_EXTENSION) + 1 /* NULL */;
      ca_policy_file_path = (char*) malloc(buffer_len);
      if(ca_policy_file_path == NULL) {store_ctx->error = X509_V_ERR_APPLICATION_VERIFICATION; return (0); }
      sprintf(ca_policy_file_path,"%s%s%08lx%s", cadir, FILE_SEPERATOR, hash, SIGNING_POLICY_FILE_EXTENSION);

      //TODO check the certificate against policy

      free(ca_policy_file_path);
    }
  } 

  /**Add the current certificate into cert chain*/
  if(vctx->cert_chain == NULL) { vctx->cert_chain = sk_X509_new_null(); }
  sk_X509_push(vctx->cert_chain, X509_dup(store_ctx->current_cert));
  vctx->cert_depth++;
 
  /**Check the proxy certificate infomation extension*/ 
  STACK_OF(X509_EXTENSION)* extensions;
  X509_EXTENSION* ext;
  ASN1_OBJECT* extension_obj;
  extensions = store_ctx->current_cert->cert_info->extensions;
  int i;
  for (i=0;i<sk_X509_EXTENSION_num(extensions);i++) {
    ext = (X509_EXTENSION *) sk_X509_EXTENSION_value(extensions,i);
    if(X509_EXTENSION_get_critical(ext)) {
      extension_obj = X509_EXTENSION_get_object(ext);
      int nid = OBJ_obj2nid(extension_obj); 
      if(nid != NID_basic_constraints &&
         nid != NID_key_usage &&
         nid != NID_ext_key_usage &&
         nid != NID_netscape_cert_type &&
         nid != NID_subject_key_identifier &&
         nid != NID_authority_key_identifier &&
         nid != OBJ_sn2nid("PROXYCERTINFO_V3") &&
         nid != OBJ_sn2nid("PROXYCERTINFO_V4")
#if (OPENSSL_VERSION_NUMBER > 0x0090706fL)
         && nid != NID_proxyCertInfo
#endif
        ) {
        store_ctx->error = X509_V_ERR_CERT_REJECTED;
        std::cerr<<"Certificate has unknown extension with numeric ID: "<<nid<<std::endl;
        return (0);
      }

#if (OPENSSL_VERSION_NUMBER > 0x0090706fL) && (nid == NID_proxyCertInfo)
    /* If the openssl version >=097g (which means proxy cert info is supported), and
     * NID_proxyCertInfo can be got from the extension, then we use 
     * the proxy cert info support from openssl itself. Otherwise we need
     * use globus-customized proxy cert info support.
     */
      PROXY_CERT_INFO_EXTENSION*  proxycertinfo = NULL;
      proxycertinfo = (PROXY_CERT_INFO_EXTENSION*) X509V3_EXT_d2i(ext);
        if (proxycertinfo == NULL) std::cerr<<"Can not convert DER encoded PROXY_CERT_INFO_EXTENSION extension to internal format"<<std::endl;
        int path_length = ASN1_INTEGER_get(proxycertinfo->pcPathLengthConstraint);
        /* ignore negative values */
        if(path_length > -1) {
          if(vctx->max_proxy_depth == -1 || vctx->max_proxy_depth > vctx->proxy_depth + path_length) {
            vctx->max_proxy_depth = vctx->proxy_depth + path_length;
            std::cout<<"proxy_depth: "<<vctx->proxy_depth<<"path_length: "<<path_length<<std::endl;
          }
        }
      }
      
      /**Parse the policy*/
      if(proxycertinfo != NULL) {
        if(store_ctx->current_cert->ex_flags & EXFLAG_PROXY) {
          switch (OBJ_obj2nid(proxycertinfo->proxyPolicy->policyLanguage)) 
          {
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
              {
              /* Here get the proxy policy */
              vctx->proxy_policy.clear();
              vctx->proxy_policy.append(proxycertinfo->proxyPolicy->policy->data, proxycertinfo->proxyPolicy->policy->length);
              /* Use : as seperator for policies parsed from different proxy certificate*/
              vctx->proxy_policy.append(":");
              }
              break;
          }
        } 
      }
      if(proxycertinfo != NULL) { PROXY_CERT_INFO_EXTENSION_free(proxycertinfo); proxycertinfo = NULL; }
#else
      PROXYCERTINFO*  proxycertinfo = NULL;
      if(nid == OBJ_sn2nid("PROXYCERTINFO_V3") || nid == OBJ_sn2nid("PROXYCERTINFO_V4")) {
        proxycertinfo = (PROXYCERTINFO*) X509V3_EXT_d2i(ext);
        if (proxycertinfo == NULL) std::cerr<<"Can not convert DER encoded PROXYCERTINFO extension to internal format"<<std::endl;
        int path_length = PROXYCERTINFO_get_path_length(proxycertinfo);
        /* ignore negative values */    
        if(path_length > -1) {
          if(vctx->max_proxy_depth == -1 || vctx->max_proxy_depth > vctx->proxy_depth + path_length) {
            vctx->max_proxy_depth = vctx->proxy_depth + path_length;
            std::cout<<"proxy_depth: "<<vctx->proxy_depth<<"path_length: "<<path_length<<std::endl;
          }
        }
      }

      /**Parse the policy*/
      if(proxycertinfo != NULL) {
        int policynid = OBJ_obj2nid(PROXYPOLICY_get_policy_language(proxycertinfo->proxypolicy));
        if(policynid == OBJ_sn2nid(INDEPENDENT_PROXY_SN)) {
               /* Put whatever explicit policy here to this particular proxy certificate, usually by
                * pulling them from some database. If there is none policy which need to be explicitly
                * inserted here, clear all the policy storage (make this and any subsequent proxy certificate
                * be void of any policy, because here the policylanguage is independent)
                */
            vctx->proxy_policy.clear();
        }
        else if(policynid == OBJ_sn2nid(IMPERSONATION_PROXY_SN)) {
               /* This is basically a NOP */
        }
        else {
            /* Here get the proxy policy */
            vctx->proxy_policy.clear();
            int length;
            char* policy_string = NULL;
            policy_string = (char*)PROXYPOLICY_get_policy(proxycertinfo->proxypolicy, &length);
            vctx->proxy_policy.append(policy_string, length);
            /* Use : as seperator for policies parsed from different proxy certificate*/
            vctx->proxy_policy.append(":");
            if(policy_string != NULL) free(policy_string);
        }
      }
      if(proxycertinfo != NULL) { PROXYCERTINFO_free(proxycertinfo); proxycertinfo = NULL; }
#endif
    }
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
  if(store_ctx->current_cert == store_ctx->cert) {
    for (i=0; i < sk_X509_num(store_ctx->chain); i++) {
      X509* cert = sk_X509_value(store_ctx->chain,i);
      if (((i - vctx->proxy_depth) > 1) && (cert->ex_pathlen != -1)
               && ((i - vctx->proxy_depth) > (cert->ex_pathlen + 1))
               && (cert->ex_flags & EXFLAG_BCONS)) {
        store_ctx->current_cert = cert; /* point at failing cert */
        store_ctx->error = X509_V_ERR_PATH_LENGTH_EXCEEDED;
        return (0);
      }
    }
  }

  return (1);
}

bool check_cert_type(X509* cert, certType& type) {
  bool ret = false;
  type = CERT_TYPE_EEC;
  
  ASN1_STRING* data;
  X509_EXTENSION* certinfo_ext;
  PROXYCERTINFO* certinfo = NULL;
  PROXYPOLICY* policy = NULL;
  ASN1_OBJECT* policylang = NULL;
  int policynid;

  int index = -1;
  int critical;
  BASIC_CONSTRAINTS* x509v3_bc = NULL;
  if((x509v3_bc = (BASIC_CONSTRAINTS*) X509_get_ext_d2i(cert, NID_basic_constraints, &critical, &index)) && x509v3_bc->ca) {
    type = CERT_TYPE_CA;
    if(x509v3_bc) { BASIC_CONSTRAINTS_free(x509v3_bc); }
    return true;
  }

  X509_NAME* issuer = NULL;
  X509_NAME* subject = X509_get_subject_name(cert);
  X509_NAME_ENTRY * name_entry = X509_NAME_get_entry(subject, X509_NAME_entry_count(subject)-1);
  if (!OBJ_cmp(name_entry->object,OBJ_nid2obj(NID_commonName))) {
    /* the name entry is of the type: common name */
    data = X509_NAME_ENTRY_get_data(name_entry);
    if (data->length == 5 && !memcmp(data->data,"proxy",5)) { type = CERT_TYPE_GSI_2_PROXY; }
    else if(data->length == 13 && !memcmp(data->data,"limited proxy",13)) { type = CERT_TYPE_GSI_2_LIMITED_PROXY; }
    else if((index = X509_get_ext_by_NID(cert, OBJ_txt2nid("PROXYCERTINFO_V4"), -1)) != -1) {
      certinfo_ext = X509_get_ext(cert,index);
      if(X509_EXTENSION_get_critical(certinfo_ext)) {
        if((certinfo = (PROXYCERTINFO *)X509V3_EXT_d2i(certinfo_ext)) == NULL) {
          std::cerr<<"Can't convert DER encoded PROXYCERTINFO extension to internal form"<<std::endl;
          goto err;
        } 
        if((policy = PROXYCERTINFO_get_proxypolicy(certinfo)) == NULL) { 
          std::cerr<<"Can't get policy from PROXYCERTINFO extension" <<std::endl; 
          goto err;
        }
        if((policylang = PROXYPOLICY_get_policy_language(policy)) == NULL) {
          std::cerr<<"Can't get policy language from PROXYCERTINFO extension" <<std::endl;
          goto err;
        }
        policynid = OBJ_obj2nid(policylang);
        if(policynid == OBJ_sn2nid(IMPERSONATION_PROXY_SN)) { type = CERT_TYPE_RFC_IMPERSONATION_PROXY; }
        else if(policynid == OBJ_sn2nid(INDEPENDENT_PROXY_SN)){ type = CERT_TYPE_RFC_INDEPENDENT_PROXY; }
        else if(policynid == OBJ_sn2nid(ANYLANGUAGE_PROXY_SN)){ type = CERT_TYPE_RFC_ANYLANGUAGE_PROXY; }
        else if(policynid == OBJ_sn2nid(LIMITED_PROXY_SN)) { type = CERT_TYPE_RFC_LIMITED_PROXY; }
        else {type = CERT_TYPE_RFC_RESTRICTED_PROXY; }

        if((index = X509_get_ext_by_NID(cert, OBJ_txt2nid("PROXYCERTINFO_V3"), -1)) != -1) {
          std::cerr<<"Found more than one PCI extension"<<std::endl;
          goto err;
        } 
      }
    }
    else if((index = X509_get_ext_by_NID(cert, OBJ_txt2nid("PROXYCERTINFO_V3"), -1)) != -1) {
      certinfo_ext = X509_get_ext(cert,index);
      if(X509_EXTENSION_get_critical(certinfo_ext)) {
        if((certinfo = (PROXYCERTINFO *)X509V3_EXT_d2i(certinfo_ext)) == NULL) {
          std::cerr<<"Can't convert DER encoded PROXYCERTINFO extension to internal form"<<std::endl;
          goto err;
        }
        if((policy = PROXYCERTINFO_get_proxypolicy(certinfo)) == NULL) {
          std::cerr<<"Can't get policy from PROXYCERTINFO extension" <<std::endl;
          goto err;
        }
        if((policylang = PROXYPOLICY_get_policy_language(policy)) == NULL) {
          std::cerr<<"Can't get policy language from PROXYCERTINFO extension" <<std::endl;
          goto err;
        }
        policynid = OBJ_obj2nid(policylang);
        if(policynid == OBJ_sn2nid(IMPERSONATION_PROXY_SN)) { type = CERT_TYPE_GSI_3_IMPERSONATION_PROXY; }
        else if(policynid == OBJ_sn2nid(INDEPENDENT_PROXY_SN)){ type = CERT_TYPE_GSI_3_INDEPENDENT_PROXY; }
        else if(policynid == OBJ_sn2nid(LIMITED_PROXY_SN)) { type = CERT_TYPE_GSI_3_LIMITED_PROXY; }
        else {type = CERT_TYPE_GSI_3_RESTRICTED_PROXY; }
        
        if((index = X509_get_ext_by_NID(cert, OBJ_txt2nid("PROXYCERTINFO_V4"), -1)) != -1) {
          std::cerr<<"Found more than one PCI extension"<<std::endl;
          goto err;
        }
      }
    }
    
    /*Duplicate the issuer, and add the CN=proxy, or CN=limitedproxy, etc. This should
     * match the subject. i.e. proxy can only be signed by
     * the owner.  We do it this way, to double check
     * all the ANS1 bits as well.
     */
    X509_NAME_ENTRY* new_name_entry = NULL;
    if(ret != CERT_TYPE_EEC && ret != CERT_TYPE_CA) {
      issuer = X509_NAME_dup(X509_get_issuer_name(cert));
      new_name_entry = X509_NAME_ENTRY_create_by_NID(NULL, NID_commonName, V_ASN1_APP_CHOOSE, data->data, -1);
      if(!new_name_entry) {goto err;}
      X509_NAME_add_entry(issuer,new_name_entry,X509_NAME_entry_count(issuer),0);
      X509_NAME_ENTRY_free(new_name_entry);
      new_name_entry = NULL;

      if (X509_NAME_cmp(issuer, subject)) {
        /* Reject this certificate, only the user may sign the proxy */
        std::cerr<<"The subject does not match the issuer name + proxy CN entry"<<std::endl;
        goto err;
      }
      X509_NAME_free(issuer);
      issuer = NULL;
    }
  }
  ret = true;  

err:
  if(issuer) { X509_NAME_free(issuer); }
  if(certinfo) {PROXYCERTINFO_free(certinfo);}

  return ret;
} 

#if SSLEAY_VERSION_NUMBER >=  0x0090600fL
/**Replace the OpenSSL check_issued in x509_vfy.c with our own,
 *so we can override the key usage checks if its a proxy. 
 *We are only looking for X509_V_ERR_KEYUSAGE_NO_CERTSIGN
*/
int check_issued(X509_STORE_CTX*, X509* x, X509* issuer) {
  int  ret;
  int  ret_code = 1;

  ret = X509_check_issued(issuer, x);
  if (ret != X509_V_OK) {
    ret_code = 0;
    switch (ret) {
      case X509_V_ERR_AKID_SKID_MISMATCH:
            /* 
             * If the proxy was created with a previous version of Globus
             * where the extensions where copied from the user certificate
             * This error could arise, as the akid will be the wrong key
             * So if its a proxy, we will ignore this error.
             * We should remove this in 12/2001 
             * At which time we may want to add the akid extension to the proxy.
             */
      case X509_V_ERR_KEYUSAGE_NO_CERTSIGN:
            /*
             * If this is a proxy certificate then the issuer
             * does not need to have the key_usage set.
             * So check if its a proxy, and ignore
             * the error if so. 
             */
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
#endif

}

