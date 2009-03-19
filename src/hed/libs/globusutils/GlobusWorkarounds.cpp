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
    // OBJ_create(OBJ_proxyCertInfo,SN_proxyCertInfo,LN_proxyCertInfo);
    // Use OpenSSL hack to make proxies work with Globus disabled
    SetEnv("OPENSSL_ALLOW_PROXY_CERTS","1");
    const char* sn = "proxyCertInfo";
    const char* gsn = "PROXYCERTINFO";
    int nid = OBJ_sn2nid(sn);
    int gnid = OBJ_sn2nid(gsn);
    // If Globus proxy extension is present
    // And if OpenSSL proxy extension is present
    // And if they are not equal
#if 0
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
            if((ext->d2i == NULL) && (gext->d2i != NULL)) ext->d2i=(X509V3_EXT_D2I)(gext->d2i);
            if((ext->i2d == NULL) && (gext->i2d != NULL)) ext->i2d=(X509V3_EXT_I2D)(gext->i2d);
            return true;
          }
        }
      }
    }
#endif
    if((gnid != NID_undef) && (nid != NID_undef) && (gnid != nid)) {
      ASN1_OBJECT* obj = NULL;
      X509V3_EXT_METHOD* ext = X509V3_EXT_get_nid(nid);
      X509V3_EXT_METHOD* gext = X509V3_EXT_get_nid(gnid);
      // Globus object with OpenSSL NID
//      gnid = OBJ_create("1.3.6.1.5.5.7.1.14", gsn, "Proxy Certificate Info Extension");
      // Merging Globus and OpenSSL extensions - probably dangerous
      if((ext != NULL) && (gext != NULL)) {
//        gext->ext_nid = gnid;
        std::cout<<"Two ssl pci!!!!!!!"<<std::endl;
        std::cout<<"GSN:++++  "<<OBJ_nid2sn(gnid)<<std::endl;
        std::cout<<"SN:++++   "<<OBJ_nid2sn(nid)<<std::endl;
        const char* ssn = "PROXYCERTINFO_V4";
        int snid = OBJ_sn2nid(ssn);
        std::cout<<"SSN:++++   "<<OBJ_nid2sn(snid)<<std::endl;
if(snid == gnid) std::cout<<"=========="<<std::endl;
else std::cout<<"??????????????"<<std::endl;
        if((ext->d2i == NULL) && (gext->d2i != NULL));//ext->d2i=(X509V3_EXT_D2I)(gext->d2i);
        if((ext->i2d == NULL) && (gext->i2d != NULL));//ext->i2d=(X509V3_EXT_I2D)(gext->i2d);
        return true;
      }
    }
  

    return false;
#else
    return true;
#endif
  }

}

