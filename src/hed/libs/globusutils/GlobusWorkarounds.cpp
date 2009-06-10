#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <iostream>

#include <openssl/objects.h>
#include <openssl/x509v3.h>

#include <arc/Utils.h>

#include "GlobusWorkarounds.h"

namespace Arc {

  bool GlobusRecoverProxyOpenSSL(void) {
#ifdef HAVE_OPENSSL_PROXY
    // No harm even if not needed - shall trun proxies on for code 
    // which was written with no proxies in mind
    SetEnv("OPENSSL_ALLOW_PROXY_CERTS","1");
#if OPENSSL_VERSION_NUMBER > 0x0090804f
#  warning *********************************************************
#  warning ** Since OpenSSL 0.9.8e proxy extension is const.      **
#  warning ** Hence we can't manipulate it. That means combining  **
#  warning ** it with Globus Toolkit libraries may cause problems **
#  warning ** during runtime. Problematic behavior was observed   **
#  warning ** at least for Globus Toolkit version 2.0. But it was **
#  warning ** tested and worked for Globus Toolkit 2.4.1.         **
#  warning *********************************************************
    return true;
#else
    // OBJ_create(OBJ_proxyCertInfo,SN_proxyCertInfo,LN_proxyCertInfo);
    // Use OpenSSL hack to make proxies work with Globus disabled
    const char* sn = "proxyCertInfo";
    const char* gsn = "PROXYCERTINFO";
    int nid = OBJ_sn2nid(sn);
    int gnid = OBJ_sn2nid(gsn);
    // If Globus proxy extension is present
    // And if OpenSSL proxy extension is present
    // And if they are not equal
    if((gnid > 0) && (nid > 0) && (gnid != nid)) { 
      ASN1_OBJECT* obj = NULL;
      X509V3_EXT_METHOD* ext = X509V3_EXT_get_nid(nid);
      X509V3_EXT_METHOD* gext = X509V3_EXT_get_nid(gnid);
      // Globus object with OpenSSL NID
      unsigned char tmpbuf[512];
      int i = a2d_ASN1_OBJECT(tmpbuf,sizeof(tmpbuf),"1.3.6.1.5.5.7.1.14",-1);
      if(i > 0) {
        obj = ASN1_OBJECT_create(nid,tmpbuf,i,gsn,"Proxy Certificate Info Extension");
        if(obj != NULL) {
          gnid = OBJ_add_object(obj);
          // Merging Globus and OpenSSL extensions - probably dangerous
          if((ext != NULL) && (gext != NULL)) {
            gext->ext_nid = gnid;
            if(ext->d2i == NULL) ext->d2i=gext->d2i;
            if(ext->i2d == NULL) ext->i2d=gext->i2d;
            return true;
          }
        }
      }
    }
    return false;
#endif // OPENSSL_VERSION_NUMBER > 0x0090804f
#else  // HAVE_OPENSSL_PROXY
    return true;
#endif // HAVE_OPENSSL_PROXY
  }

}

