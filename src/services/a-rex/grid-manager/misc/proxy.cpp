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

int prepare_proxy(void) {
  int h = -1;
  off_t len;
  char* buf = NULL;
  off_t l,ll;
  int res=-1;

  if(getuid() == 0) { /* create temporary proxy */
    std::string proxy_file=Arc::GetEnv("X509_USER_PROXY");
    if(proxy_file.empty()) goto exit;
    h=open(proxy_file.c_str(),O_RDONLY);
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
    h=open(proxy_file_tmp.c_str(),O_WRONLY | O_CREAT,S_IRUSR | S_IWUSR);
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

  h=open(new_proxy,O_RDONLY);
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
  h=open(proxy_file_tmp.c_str(),O_WRONLY | O_CREAT | O_EXCL,S_IRUSR | S_IWUSR);
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

