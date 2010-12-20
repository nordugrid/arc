// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_RLS_H__
#define __ARC_RLS_H__

#include <list>
#include <string>

#include <arc/UserConfig.h>

extern "C" {
#include <globus_rls_client.h>
}

namespace Arc {

  class URL;

  typedef bool (*rls_lrc_callback_t)(globus_rls_handle_t *h,
                                     const URL& url, void *arg);

  bool rls_find_lrcs(const URL& url, const UserConfig& usercfg, rls_lrc_callback_t callback, void *arg);
  bool rls_find_lrcs(const URL& url, std::list<URL> lrcs, const UserConfig& usercfg);
  bool rls_find_lrcs(std::list<URL> rlis, std::list<URL> lrcs, const UserConfig& usercfg,
                     rls_lrc_callback_t callback, void *arg);
  bool rls_find_lrcs(std::list<URL> rlis, std::list<URL> lrcs, bool down,
                     bool up, const UserConfig& usercfg,
                     rls_lrc_callback_t callback, void *arg);

} // namespace Arc

#endif //__ARC_RLS_H__
