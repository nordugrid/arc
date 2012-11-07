#ifndef __ARC_GM_PROXY_H__
#define __ARC_GM_PROXY_H__

namespace ARex {

int prepare_proxy(void);
int remove_proxy(void);
int renew_proxy(const char* old_proxy,const char* new_proxy);
bool myproxy_renew(const char* old_proxy_file,const char* new_proxy_file,const char* myproxy_server);
 
} // namespace ARex

#endif // __ARC_GM_PROXY_H__
