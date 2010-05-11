#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef WIN32
#include <arc/win32.h>
#endif

#include <openssl/ssl.h>

#include "GlobusHack.h"

namespace Arc {

  static void fix_proxy_nid(X509_EXTENSION* ext,int nid,int gnid) {
    if(ext == NULL) return;
    if(ext->object == NULL) return;
    if(ext->object->nid != 0) return;
    int onid = OBJ_obj2nid(ext->object);
    if(onid == gnid) ext->object->nid = nid;
  }

  static void fix_proxy_nid(X509* cert,int nid,int gnid) {
    if(cert == NULL) return;
    STACK_OF(X509_EXTENSION) * extensions = cert->cert_info->extensions;
    if(extensions == NULL) return;
    int num = sk_X509_EXTENSION_num(extensions);
    for(int i=0;i<num;++i) {
      X509_EXTENSION* ext = sk_X509_EXTENSION_value(extensions,i);
      fix_proxy_nid(ext,nid,gnid);
    }
  }

  static void fix_proxy_nid(STACK_OF(X509)* chain,int nid,int gnid) {
    if(chain == NULL) return;
    int num = sk_X509_num(chain);
    for(int i=0;i<num;++i) {
      X509* cert = sk_X509_value(chain,i);
      fix_proxy_nid(cert,nid,gnid);
    }
  }

  static int verify_cert_callback(X509_STORE_CTX *sctx,void* arg) {
    const char* sn = "proxyCertInfo";
    const char* gsn = "PROXYCERTINFO";
    int nid = OBJ_sn2nid(sn);
    int gnid = OBJ_sn2nid(gsn);
    if((nid > 0) && (gnid > 0)) {
      fix_proxy_nid(sctx->cert,nid,gnid);
      fix_proxy_nid(sctx->chain,nid,gnid);
      fix_proxy_nid(sctx->untrusted,nid,gnid);
    }
    return X509_verify_cert(sctx);
  }

  bool GlobusSetVerifyCertCallback(SSL_CTX* sslctx) {
    SSL_CTX_set_cert_verify_callback(sslctx,&verify_cert_callback,NULL);
    return true;
  }

}

