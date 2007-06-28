//@ #include "../std.h"
//@ #include "inttostring.h"
//@ 
#include <src/libs/common/StringConv.h>
#define inttostring Arc::tostring
//@ 
#include "url_options.h"

std::string URL_::empty("");
/*
  0 - found, -1 - error, 1 - meta-url without hosts
*/
static int find_hosts(const std::string &url,int &host_s,int &host_e) {
  std::string::size_type n = url.find("://");
  if(n == std::string::npos) return -1; /* not an url */
  host_s=n;
  n=url.find('/'); if(n < host_s) return -1; /* strange url */
  host_s+=3;
  host_e=(n=url.find('/',host_s));
  if(n == std::string::npos) host_e=url.length();
  n=url.find('@',host_s);
  if((strncmp("rc://",url.c_str(),5) == 0) ||
     (strncmp("rls://",url.c_str(),6) == 0)) { /* meta url is different */
    if((n == std::string::npos) || (n >= host_e)) { host_e=host_s; return 1; };
    host_e=n;
  } else {
    if((n != std::string::npos) && (n < host_e)) host_s=n+1; 
  };
  if(host_e < host_s) return -1; /* in case of error in code */
  return 0;
}

static int next_host(const std::string& url,int host_s,int host_e) {
  std::string::size_type n = url.find('|',host_s);
  if((n == std::string::npos) || (n > host_e)) n=host_e;
  if(n <= host_s) return -1;
  return n;
}

int remove_url_options(std::string& url) {
  int host_s;
  int host_e;
  int r = find_hosts(url,host_s,host_e);
  if(r == -1) return 1;
  if(r == 1) return 0; /* no hosts */
  int host_cur;
  int host_s_ = host_s;
  for(;;) {
    if(host_s >= host_e) break;
    host_cur=next_host(url,host_s,host_e);
    if(host_cur == -1) break;
    std::string::size_type n = url.find(';',host_s);
    if((n == std::string::npos) || (n>host_cur)) n=host_cur;
    url.erase(n,host_cur-n);
    host_e-=(host_cur-n);
    if(n==host_s) { /* global options without host name */
      if(n == host_s_) { /* first option */
        if(n != host_e) {
          url.erase(n,1); host_e--; n--;
        };
      } else {
        n--; url.erase(n,1); host_e--;
      };
    };
    host_s=n+1;
  };
  return 0; 
}

static int find_url_option(const std::string& url,const char* name,int num,int &opt_s,int &opt_e,int host_s,int host_e) {
  int host_cur;
  opt_s = -1;
  for(;;) {
    if(host_s >= host_e) break;
    host_cur=next_host(url,host_s,host_e);
    if(host_cur == -1) break;
    if(num <= 0) { /* look for option */
      opt_s = host_cur;
      if(name) {
        int name_l = strlen(name);
        std::string::size_type opt_c = url.find(';',host_s);
        if((opt_c != std::string::npos) && (opt_c < host_cur)) {
          opt_c++;
          for(;;) {
            if(opt_c>=host_cur) break;
            std::string::size_type n = url.find(';',opt_c);
            if((n == std::string::npos) || (n>host_cur)) n=host_cur;
            if(n == opt_c) { opt_c++; continue; }; /* empty option */
            int l = (n-opt_c);
            if(l < name_l) { opt_c=n+1; continue; };
            if(!strncmp(name,url.c_str()+opt_c,name_l)) { /* check name */
              if((l==name_l) || (url[opt_c+name_l] == '=')) {
                opt_s = opt_c; opt_e = n;
                return 0;
              };
            };
            opt_c=n+1;
          };
        };
      };
      return 1;
    };
    host_s=host_cur+1; num--;
  };
  return 1;
}

static int hosts_num(const std::string &url,int host_s,int host_e) {
  int n = 1;
  std::string::size_type host_c = host_s;
  for(;;) {
    host_c=url.find('|',host_c);
    if((host_c == std::string::npos) || (host_c >= host_e)) break;
    host_c++; n++;
  };
  return n;
}

int get_url_option(const std::string& url,const char* name,int num,std::string& value) {
  int host_s;
  int host_e;
  value = "";
  if(find_hosts(url,host_s,host_e) != 0) return 1;
  int opt_s;
  int opt_e;
  if(find_url_option(url,name,num,opt_s,opt_e,host_s,host_e) != 0) return 1;
  int name_l = strlen(name);
  value=url.substr(opt_s+name_l+1,(opt_e-opt_s)-(name_l+1));
  return 0;
}

int get_url_option(const std::string& url,const char* name,std::string& value) {
  value = "";
  int host_s;
  int host_e;
  std::string::size_type n = url.find("://");
  if(n == std::string::npos) return -1; /* not an url */
  host_s=n;
  n=url.find('/'); if(n < host_s) return -1; /* strange url */
  host_s+=3;
  host_e=(n=url.find('/',host_s)); if(n == std::string::npos) host_e=url.length();
  if(host_e < host_s) return -1; /* in case of error in code */
  int opt_s;
  int opt_e;
  if(find_url_option(url,name,0,opt_s,opt_e,host_s,host_e) != 0) return 1;
  int name_l = strlen(name);
  if((opt_e-opt_s)<=name_l) return 0;
  value=url.substr(opt_s+name_l+1,(opt_e-opt_s)-(name_l+1));
  return 0;
}

int get_url_options(const char* host,std::string& options) {
  options.erase();
  char* opt_s = strchr(host,';');
  if(opt_s == NULL) return 0;
  options=opt_s; 
  std::string::size_type opt_e = options.find('/');
  if(opt_e != std::string::npos) options.resize(opt_e);
  return 0;
}

int add_url_options(std::string& url,const char* options,int num) {
  int host_s;
  int host_e;
  if(options == NULL) return 0;
  if(options[0] == 0) return 0;
  int r = find_hosts(url,host_s,host_e);
  if(r == -1) return 1;
  if(r == 1) { /* meta url without hosts - insert global options */
    url.insert(host_s,"@");
    url.insert(host_s,options);
    url.insert(host_s,";");
    return 0;
  };
  if(num==-1) {
    int n = hosts_num(url,host_s,host_e);
    int res = 0;
    for(int i=0;i<n;i++) res|=add_url_options(url,options,i);
    return res;
  };
  int opt_s;
  int opt_e;
  find_url_option(url,NULL,num,opt_s,opt_e,host_s,host_e);
  if(opt_s == -1) return 1;
  url.insert(opt_s,options);
  url.insert(opt_s,";");
  return 0;
}

int add_url_option(std::string& url,const char* name,const char* value,int num) {
  std::string option = name;
  if(value != NULL) option = option + "=" + value;
  return add_url_option(url,option,num,name);
}

int add_url_option(std::string& url,const std::string& option,int num,const char* name) {
  std::string name_;
  if(name == NULL) {
    std::string::size_type n = option.find('=');
    if(n == std::string::npos) { name=option.c_str(); }
    else { name_=option.substr(0,n); name=name_.c_str(); };
  };
  int host_s;
  int host_e;
  int r = find_hosts(url,host_s,host_e);
  if(r == -1) return 1; /* bad url or other problems */
  if(r == 1) { /* meta url without hosts - insert global options */
    url.insert(host_s,"@");
    url.insert(host_s,option);
    url.insert(host_s,";");
    return 0;
  };
  if(num==-1) {
    int n = hosts_num(url,host_s,host_e);
    int res = 0;
    for(int i=0;i<n;i++) res|=add_url_option(url,option,i,name);
    return res;
  };
  int opt_s;
  int opt_e;
  if(find_url_option(url,name,num,opt_s,opt_e,host_s,host_e) != 0) {
    if(opt_s == -1) return 1;
    url.insert(opt_s,option);
    url.insert(opt_s,";");
    return 0;
  };
  url.replace(opt_s,opt_e-opt_s,option);
  return 0;
}

int del_url_option(std::string& url,const char* name,int num) {
  int host_s;
  int host_e;
  if(find_hosts(url,host_s,host_e) != 0) return 1;
  if(num==-1) {
    int n = hosts_num(url,host_s,host_e);
    int res = 0;
    for(int i=0;i<n;i++) res|=del_url_option(url,name,i);
    return res;
  };
  int opt_s;
  int opt_e;
  if(find_url_option(url,name,num,opt_s,opt_e,host_s,host_e) != 0) return 1;
  url.erase(opt_s,opt_e-opt_s);
  return 0;
}

/* add port, strip options, remove locations from rc url */
int canonic_url(std::string &url) {
  /* find place for port */
  std::string::size_type n = url.find("://");
  if(n == std::string::npos) return 1; /* not an url */
  std::string::size_type host_s=n;
  n=url.find('/'); if(n < host_s) return 1; /* strange url */
  host_s+=3;
  std::string::size_type host_e = url.find('/',host_s);
  if(host_e == std::string::npos) host_e=url.length();
  /* remove username or locations */
  std::string::size_type host_h = url.find('@',host_s);
  if(strncasecmp(url.c_str(),"rls://",6) == 0) {
    // RLS is tricky because can contain '/' in locations
    // Assuming there is no '@' in lfn.
    if((host_h != std::string::npos) && (host_h > host_e)) {
      host_e=url.find('/',host_h);
      if(host_e == std::string::npos) host_e=url.length();
    };
  };
  if((host_h != std::string::npos) && (host_h < host_e)) {
    host_h++;
    url.erase(host_s,host_h-host_s);
    host_e-=(host_h-host_s);
  };
  /* remove options */
  std::string::size_type host_o = url.find(';',host_s);
  if((host_o != std::string::npos) && (host_o < host_e)) {
    url.erase(host_o,host_e-host_o);
    host_e=host_o;
  };
  /* add port if missing */
  std::string::size_type host_p = url.find(':',host_s);
  if((host_p == std::string::npos) || (host_p > host_e)) {
    int port = 0;
    if(strncasecmp(url.c_str(),"rc://",5) == 0) {
      port=PORT_DEFAULT_RC;
    } else if(strncasecmp(url.c_str(),"rls://",6) == 0) {
      port=PORT_DEFAULT_RLS;
    } else if(strncasecmp(url.c_str(),"http://",7) == 0) {
      port=PORT_DEFAULT_HTTP;
    } else if(strncasecmp(url.c_str(),"https://",8) == 0) {
      port=PORT_DEFAULT_HTTPS;
    } else if(strncasecmp(url.c_str(),"httpg://",8) == 0) {
      port=PORT_DEFAULT_HTTPG;
    } else if(strncasecmp(url.c_str(),"ftp://",6) == 0) {
      port=PORT_DEFAULT_FTP;
    } else if(strncasecmp(url.c_str(),"gsiftp://",9) == 0) {
      port=PORT_DEFAULT_GSIFTP;
    };
    if(port) {
      std::string port_s = ":" + inttostring(port);
      url.insert(host_e,port_s);
      host_e+=port_s.length();
    };
  };
  return 0;
}

const char* get_url_path(const char* url) {
  if(url == NULL) return NULL;
  const char* proto_end = strchr(url,':');
  if(proto_end == NULL) return NULL;
  const char* proto_slash = strchr(url,'/');
  if(proto_end == NULL) return NULL;
  if(proto_slash < proto_end) return NULL;
  proto_end++;
  if((*proto_end) != '/') return NULL;
  proto_end++;
  if((*proto_end) != '/') { return (proto_end-1); };
  proto_end++;
  if((*proto_end) == '/') { return proto_end; };
  proto_slash = strchr(proto_end,'/');
  return proto_slash;
}

std::string get_url_host(const char* url) {
  std::string host("");
  int host_s,host_e;
  if(find_hosts(url,host_s,host_e)!=0) return host;
  host=url+host_s; host.resize(host_e-host_s);
  return host;
}

URL_::URL_(const char* url) {
  valid=false;
  if(url==NULL) return;
  int l = strlen(url);
  const char* p = strstr(url,"://");
  if(!p) return;
  const char* host_s = p;
  p=strchr(url,'/'); if(p<host_s) return;
  host_s+=3;
  const char* host_e = strchr(host_s,'/');
  if(!host_e) host_e=url+l;
  proto.assign(url,host_s-url-3);
  if(*host_e) path.assign(host_e+1);
  const char* host_p = strchr(host_s,':');
  port = 0;
  if((!host_p) || ((host_p+1)>=host_e)) {
    host_p=host_e;
    if(proto == "rc" ) { port=PORT_DEFAULT_RC; }
    else if(proto == "rls") { port=PORT_DEFAULT_RLS; }
    else if(proto == "ldap") { port=PORT_DEFAULT_LDAP; }
    else if(proto == "http") { port=PORT_DEFAULT_HTTP; }
    else if(proto == "https") { port=PORT_DEFAULT_HTTPS; }
    else if(proto == "httpg") { port=PORT_DEFAULT_HTTPG; }
    else if(proto == "ftp") { port=PORT_DEFAULT_FTP; }
    else if(proto == "gsiftp") { port=PORT_DEFAULT_GSIFTP; }
    else { };
  } else {
    char* e;
    port=strtol(host_p+1,&e,10);
    if(e != host_e) return;
  };
  host.assign(host_s,host_p-host_s);
  valid=true;
  return;
}

std::ostream& operator<<(std::ostream& o,const URL_& u) {
  if(!u.valid) {
    o<<"<invalid>";
  } else {
    o<<u.proto<<"://"<<u.host<<":"<<u.port<<"/"<<u.path;
  };
  return o;
}

