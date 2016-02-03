#ifndef GRID_SERVER_PROXY_H
#define GRID_SERVER_PROXY_H

#include <gssapi.h>

namespace gridftpd {

  char* write_proxy(gss_cred_id_t cred);
  char* write_cert_chain(const gss_ctx_id_t gss_context);

} // namespace gridftpd

#endif
