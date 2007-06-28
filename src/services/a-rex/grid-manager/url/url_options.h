#ifndef __URL_OPTIONS_H__
#define __URL_OPTIONS_H__

#include <string>
#include <vector>
#include <iostream>

#define PORT_DEFAULT_RC      389
#define PORT_DEFAULT_RLS     39281
#define PORT_DEFAULT_LDAP    389
#define PORT_DEFAULT_HTTP    80
#define PORT_DEFAULT_HTTPS   443
#define PORT_DEFAULT_HTTPG   8000
#define PORT_DEFAULT_FTP     21
#define PORT_DEFAULT_GSIFTP  2811

int remove_url_options(std::string& url);
int get_url_option(const std::string& url,const char* name,int num,std::string& value);
int get_url_option(const std::string& url,const char* name,std::string& value);
int add_url_option(std::string& url,const char* name,const char* value,int num);
int add_url_option(std::string& url,const std::string& option,int num,const char* name = NULL);
int del_url_option(std::string& url,const char* name,int num);
int get_url_options(const char* host,std::string& options);
int add_url_options(std::string& url,const char* options,int num);
int canonic_url(std::string &url);
const char* get_url_path(const char* url);
std::string get_url_host(const char* url);

class URL_ {
 friend std::ostream& operator<<(std::ostream& o,const URL_& u);
 protected:
  int port;
  std::string host;
  std::string proto;
  std::string path;
  bool valid;
  static std::string empty;
 public:
  URL_(const char*);
  ~URL_(void) { };
  int Port() const { if(!valid) return 0; return port; };
  const std::string& Proto() const { if(!valid) return empty; return proto; };
  const std::string& Host() const { if(!valid) return empty; return host; };
  const std::string& Path() const { if(!valid) return empty; return path; };
  operator bool(void) const { return valid; };
  void operator =(const URL_& u) { 
    proto=u.proto; port=u.port; host=u.host; path=u.path; valid=u.valid;
  };
};

std::ostream& operator<<(std::ostream& o,const URL_& u);
 
#endif // __URL_OPTIONS_H__
