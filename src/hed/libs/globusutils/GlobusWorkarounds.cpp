#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <openssl/objects.h>

#include "GlobusWorkarounds.h"

namespace Arc {

  bool GlobusRecoverProxyOpenSSL(void) {
    // OBJ_create(OBJ_proxyCertInfo,SN_proxyCertInfo,LN_proxyCertInfo);
    const char* sn = "proxyCertInfo";
    int nid = OBJ_sn2nid(sn);
    if(nid > 0) {
      ASN1_OBJECT* obj = OBJ_nid2obj(nid);
      if(obj != NULL) {
        nid = OBJ_add_object(obj);
        if(nid > 0) return true;
      }
    }
    return false;
  }

}

