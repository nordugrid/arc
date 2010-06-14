#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#include "proxy.h"

#include <arc/Utils.h>
#include <arc/StringConv.h>
#include <arc/URL.h>
#include <arc/FileUtils.h>
#include <arc/credential/Credential.h>
#include <arc/credentialstore/CredentialStore.h>

int prepare_proxy(void) {
  int h = -1;
  off_t len;
  char* buf = NULL;
  off_t l,ll;
  int res=-1;

  if(getuid() == 0) { /* create temporary proxy */
    std::string proxy_file=Arc::GetEnv("X509_USER_PROXY");
    if(proxy_file.empty()) goto exit;
    h=Arc::FileOpen(proxy_file,O_RDONLY);
    if(h==-1) goto exit;
    if((len=lseek(h,0,SEEK_END))==-1) goto exit;
    if(lseek(h,0,SEEK_SET) != 0) goto exit;
    buf=(char*)malloc(len);
    if(buf==NULL) goto exit;
    for(l=0;l<len;) {
      ll=read(h,buf+l,len-l);
      if(ll==-1) goto exit;
      if(ll==0) break;
      l+=ll;
    };
    close(h); h=-1; len=l;
    std::string proxy_file_tmp = proxy_file;
    proxy_file_tmp+=".tmp";
    h=Arc::FileOpen(proxy_file_tmp,O_WRONLY | O_CREAT,S_IRUSR | S_IWUSR);
    if(h==-1) goto exit;
    (void)chmod(proxy_file_tmp.c_str(),S_IRUSR | S_IWUSR);
    for(l=0;l<len;) {
      ll=write(h,buf+l,len-l);
      if(ll==1) goto exit;
      l+=ll;
    };
    close(h); h=-1; 
    Arc::SetEnv("X509_USER_PROXY",proxy_file_tmp);
  };
  res=0;
 exit:
  if(buf) free(buf);
  if(h!=-1) close(h);
  return res;
}

int remove_proxy(void) {
  if(getuid() == 0) {
    std::string proxy_file=Arc::GetEnv("X509_USER_PROXY");
    if(proxy_file.empty()) return 0;
    remove(proxy_file.c_str());
  };
  return 0;
}

int renew_proxy(const char* old_proxy,const char* new_proxy) {
  int h = -1;
  off_t len,l,ll;
  char* buf = NULL;
  std::string proxy_file_tmp;
  struct stat st;
  int res = -1;

  h=Arc::FileOpen(new_proxy,O_RDONLY);
  if(h==-1) {
    fprintf(stderr,"Can't open new proxy: %s\n",new_proxy);
    goto exit;
  };
  if((len=lseek(h,0,SEEK_END))==-1) goto exit;
  lseek(h,0,SEEK_SET);
  if((buf=(char*)(malloc(len))) == NULL) {
    fprintf(stderr,"Out of memory\n");
    goto exit;
  };
  for(l=0;l<len;) {
    ll=read(h,buf+l,len-l);
    if(ll==-1) {
      fprintf(stderr,"Can't read new proxy: %s\n",new_proxy);
      goto exit;
    };
    if(ll==0) break;
    l+=ll;
  };
  close(h); h=-1; len=l;
  proxy_file_tmp=old_proxy;
  proxy_file_tmp+=".renew";
  remove(proxy_file_tmp.c_str());
  h=Arc::FileOpen(proxy_file_tmp,O_WRONLY | O_CREAT | O_EXCL,S_IRUSR | S_IWUSR);
  if(h==-1) {
    fprintf(stderr,"Can't create temporary proxy: %s\n",proxy_file_tmp.c_str());
    goto exit;
  };
  (void)chmod(proxy_file_tmp.c_str(),S_IRUSR | S_IWUSR);
  for(l=0;l<len;) {
    ll=write(h,buf+l,len-l);
    if(ll==-1) {
      fprintf(stderr,"Can't write temporary proxy: %s\n",proxy_file_tmp.c_str());
      goto exit;
    };
    l+=ll;
  };
  if(stat(old_proxy,&st) == 0) {
    fchown(h,st.st_uid,st.st_gid);
    if(remove(old_proxy) != 0) {
      fprintf(stderr,"Can't remove proxy: %s\n",old_proxy);
      goto exit;
    };
  };
  close(h); h=-1;
  if(rename(proxy_file_tmp.c_str(),old_proxy) != 0) {
    fprintf(stderr,"Can't rename temporary proxy: %s\n",proxy_file_tmp.c_str());
    goto exit;
  };
  res=0;
 exit:
  if(h!=-1) close(h);
  if(buf) free(buf);
  if(!proxy_file_tmp.empty()) remove(proxy_file_tmp.c_str());
  return res;
}

bool myproxy_renew(const char* old_proxy_file,const char* new_proxy_file,const char* myproxy_server) {
  if(!myproxy_server) return false;
  if(!old_proxy_file) return false;
  if(!new_proxy_file) return false;

  Arc::URL url(myproxy_server);
  Arc::UserConfig usercfg(Arc::initializeCredentialsType(Arc::initializeCredentialsType::TryCredentials));
  //usercfg.ProxyPath(old_proxy_file);
  usercfg.ProxyPath("");
  usercfg.CertificatePath("");
  usercfg.KeyPath("");
  Arc::CredentialStore cstore(usercfg,url);

  std::map<std::string,std::string> storeopt;
  std::map<std::string,std::string>::const_iterator m;
  m=url.Options().find("username");
  if(m != url.Options().end()) {
    storeopt["username"]=m->second;
  } else {
    Arc::Credential proxy(std::string(old_proxy_file),"","","");
    storeopt["username"]=proxy.GetIdentityName();
  };
  m=url.Options().find("credname");
  if(m != url.Options().end()) {
    storeopt["credname"]=m->second;
  };
  storeopt["lifetime"] = Arc::tostring(60*60*12);
  m=url.Options().find("password");
  if(m != url.Options().end()) {
    storeopt["password"] = m->second;
  };

  /* Get new proxy */
  std::string new_proxy_str;
  if(!cstore.Retrieve(storeopt,new_proxy_str)) {
    fprintf(stderr, "Failed to retrieve a proxy from MyProxy server %s\n", myproxy_server);
    return false;
  };
  std::ofstream h(new_proxy_file,std::ios::trunc);
  h<<new_proxy_str;
  if(h.fail()) {
    fprintf(stderr,"Can't open proxy file: %s\n",new_proxy_file);
    return false;
  };
  h.close();
  if(h.fail()) {
    fprintf(stderr,"Can't write to proxy file: %s\n",new_proxy_file);
    ::unlink(new_proxy_file);
    return false;
  };
  return true;
}


