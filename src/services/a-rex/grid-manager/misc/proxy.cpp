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

int prepare_proxy(void) {
  char* proxy_file = NULL;
  char* proxy_file_tmp = NULL;
  struct stat stx;
  int h = -1;
  off_t len;
  char* buf = NULL;
  off_t l,ll;
  int res=-1;

  if(getuid() == 0) { /* create temporary proxy */
    proxy_file=getenv("X509_USER_PROXY");
    if(proxy_file==NULL) goto exit;
    h=open(proxy_file,O_RDONLY);
    if(h==-1) goto exit;
    if((len=lseek(h,0,SEEK_END))==-1) goto exit;
    lseek(h,0,SEEK_SET);
    buf=(char*)malloc(len);
    if(buf==NULL) goto exit;
    for(l=0;l<len;) {
      ll=read(h,buf+l,len-l);
      if(ll==-1) goto exit;
      if(ll==0) break;
      l+=ll;
    };
    close(h); h=-1; len=l;
    proxy_file_tmp=(char*)malloc(strlen(proxy_file)+5);
    if(proxy_file_tmp==NULL) goto exit;
    strcpy(proxy_file_tmp,proxy_file); strcat(proxy_file_tmp,".tmp");
    h=open(proxy_file_tmp,O_WRONLY | O_CREAT,S_IRUSR | S_IWUSR);
    if(h==-1) goto exit;
    (void)chmod(proxy_file_tmp,S_IRUSR | S_IWUSR);
    for(l=0;l<len;) {
      ll=write(h,buf+l,len-l);
      if(ll==1) goto exit;
      l+=ll;
    };
    close(h); h=-1; 
    setenv("X509_USER_PROXY",proxy_file_tmp,1);
  };
  res=0;
 exit:
  if(proxy_file_tmp) free (proxy_file_tmp);
  if(buf) free(buf);
  if(h!=-1) close(h);
  return res;
}

int remove_proxy(void) {
  char* proxy_file = NULL;
  if(getuid() == 0) {
    proxy_file=getenv("X509_USER_PROXY");
    if(proxy_file == NULL) return 0;
    remove(proxy_file);
  };
  return 0;
}

