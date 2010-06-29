#ifndef GRID_SERVER_PROXY_H
#define GRID_SERVER_PROXY_H

#include <gssapi.h>

namespace gridftpd {

  int prepare_proxy(void);
  int remove_proxy(void);
  int renew_proxy(const char* old_proxy,const char* new_proxy);
  bool myproxy_renew(const char* old_proxy_file,const char* new_proxy_file,const char* myproxy_server);
  char* write_proxy(gss_cred_id_t cred);
  char* write_cert_chain(const gss_ctx_id_t gss_context);
  void free_proxy(gss_cred_id_t cred);
  gss_cred_id_t read_proxy(const char* filename);

} // namespace gridftpd

#endif
