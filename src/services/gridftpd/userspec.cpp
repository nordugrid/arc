#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <cstdlib>
#include <string>
#include <fstream>

#include <globus_ftp_control.h>
#include <grp.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#include <arc/ArcConfigIni.h>

#include "userspec.h"
#include "conf.h"

static Arc::Logger logger(Arc::Logger::getRootLogger(),"userspec_t");

void userspec_t::free(void) const {
  // Keep authentication info to preserve proxy (just in case)
}

userspec_t::userspec_t(void):user(),uid(-1),gid(-1),port(0),map(user),default_map(user) {
  host[0] = 0;
}

userspec_t::~userspec_t(void) {
  userspec_t::free();
}

bool check_gridmap(const char* dn,char** user,const char* mapfile) {
  std::string globus_gridmap;
  if(mapfile) {
    globus_gridmap=mapfile;
  }
  else {
    char* tmp=getenv("GRIDMAP");
    globus_gridmap=tmp?tmp:"";
  };
  if(!globus_gridmap.empty()) {
    std::ifstream f(globus_gridmap.c_str());
    if(!f.is_open() ) {
      logger.msg(Arc::ERROR, "Mapfile is missing at %s", globus_gridmap);
      return false;
    };
    for(;f.good();) {
      std::string buf;
      getline(f,buf);
      char* p = &buf[0];
      for(;*p;p++) if(((*p) != ' ') && ((*p) != '\t')) break;
      if((*p) == '#') continue;
      if((*p) == 0) continue;
      std::string val;
      int n = Arc::ConfigIni::NextArg(p,val,' ','"');
      if(strcmp(val.c_str(),dn) != 0) continue;
      p+=n;
      if(user) {
        n=Arc::ConfigIni::NextArg(p,val,' ','"');
        *user=strdup(val.c_str());
      };
      f.close();
      return true;
    };
    f.close();
  };
  return false;
}

bool userspec_t::fill(globus_ftp_control_auth_info_t *auth,globus_ftp_control_handle_t *handle, const char* cfg) {
  struct passwd pw_;
  struct group gr_;
  struct passwd* pw=NULL;
  struct group* gr=NULL;
  char bufp[BUFSIZ];
  char bufg[BUFSIZ];
  if(cfg) config_file = cfg;
  if(auth == NULL) return false;
  if(auth->auth_gssapi_subject == NULL) return false;
  std::string subject;
  Arc::ConfigIni::NextArg(auth->auth_gssapi_subject,subject,'\0','\0');
  char* name=NULL;
  char* gname=NULL;
  if(!check_gridmap(subject.c_str(),&name)) {
    logger.msg(Arc::INFO, "There is no local initial mapping for user");
  } else {
    if((name == NULL) || (name[0] == 0)) {
      logger.msg(Arc::INFO, "There is no local initial name for user");
      if(name) { std::free(name); name=NULL; };
    } else {
      gname = strchr(name,':');
      if(gname) {
        *gname = 0;
        ++gname;
        if(gname[0] == 0) gname = NULL;
      };
    };
  };
  // fill host info
  if(handle) {
    //int host[4] = {0,0,0,0};
    //unsigned short port = 0;
    if(globus_io_tcp_get_remote_address(&(handle->cc_handle.io_handle),
           host,&(port)) != GLOBUS_SUCCESS) {
      port=0;
      user.set(auth->auth_gssapi_subject,auth->auth_gssapi_context,
               auth->delegated_credential_handle);
    } else {
      char abuf[1024]; abuf[sizeof(abuf)-1]=0;
      struct hostent he;
      struct hostent* he_p;
      struct in_addr a;
      snprintf(abuf,sizeof(abuf)-1,"%u.%u.%u.%u",
               (unsigned int)host[0],(unsigned int)host[1],(unsigned int)host[2],(unsigned int)host[3]);
      if(inet_aton(abuf,&a) != 0) {
        int h_errnop;
        char buf[1024];
        he_p=globus_libc_gethostbyaddr_r((char*)&a,strlen(abuf),AF_INET,
                                               &he,buf,sizeof(buf),&h_errnop);
        if(he_p) {
          if(strcmp(he_p->h_name,"localhost") == 0) {
            abuf[sizeof(abuf)-1]=0;
            if(globus_libc_gethostname(abuf,sizeof(abuf)-1) != 0) {
              strcpy(abuf,"localhost");
            };
          };
        };
      };
      user.set(auth->auth_gssapi_subject,auth->auth_gssapi_context,
               auth->delegated_credential_handle,abuf);
    };
  } else {
    user.set(auth->auth_gssapi_subject,auth->auth_gssapi_context,
             auth->delegated_credential_handle);
  };
  if((!user.is_proxy()) || (user.proxy() == NULL) || (user.proxy()[0] == 0)) {
    logger.msg(Arc::INFO, "No proxy provided");
  } else {
    logger.msg(Arc::VERBOSE, "Proxy/credentials stored at %s", user.proxy());
  };
  if((getuid() == 0) && name) {
    logger.msg(Arc::INFO, "Initially mapped to local user: %s", name);
    getpwnam_r(name,&pw_,bufp,BUFSIZ,&pw);
    if(pw == NULL) {
      logger.msg(Arc::ERROR, "Local user %s does not exist",name);
      std::free(name); name=NULL;
      return false;
    };
    if(gname) {
      logger.msg(Arc::INFO, "Initially mapped to local group: %s", gname);
      getgrnam_r(gname,&gr_,bufg,BUFSIZ,&gr);
      if(gr == NULL) {
        logger.msg(Arc::ERROR, "Local group %s does not exist",gname);
        std::free(name); name=NULL;
        return false;
      };
    };
  } else {
    if(name) std::free(name); name=NULL; gname=NULL;
    getpwuid_r(getuid(),&pw_,bufp,BUFSIZ,&pw);
    if(pw == NULL) {
      logger.msg(Arc::WARNING, "Running user has no name");
    } else {
      name=strdup(pw->pw_name);
      logger.msg(Arc::INFO, "Mapped to running user: %s", name);
    };
  };
  if(pw) {
    uid=pw->pw_uid;
    if(gr) {
      gid=gr->gr_gid;
    } else {
      gid=pw->pw_gid;
    };
    logger.msg(Arc::INFO, "Mapped to local id: %i", uid);
    home=pw->pw_dir;
    if(!gr) {
      getgrgid_r(gid,&gr_,bufg,BUFSIZ,&gr);
      if(gr == NULL) {
        logger.msg(Arc::ERROR, "No group %i for mapped user", gid);
      };
    };
    std::string mapstr;
    if(name) mapstr+=name;
    mapstr+=":";
    if(gr) mapstr+=gr->gr_name;
    mapstr+=" all";
    default_map.mapname(mapstr.c_str());
    logger.msg(Arc::INFO, "Mapped to local group id: %i", gid);
    if(gr) logger.msg(Arc::INFO, "Mapped to local group name: %s", gr->gr_name);
    logger.msg(Arc::VERBOSE, "Mapped user's home: %s", home);
  };
  if(name) std::free(name);
  if(!user) return false;
  return true;
}

bool userspec_t::fill(AuthUser& u, const char* cfg) {
  struct passwd pw_;
  struct group gr_;
  struct passwd *pw;
  struct group *gr;
  char bufp[BUFSIZ];
  char bufg[BUFSIZ];
  std::string subject = u.DN();
  char* name=NULL;
  char* gname=NULL;
  if(cfg) config_file = cfg;
  if(!check_gridmap(subject.c_str(),&name)) {
    logger.msg(Arc::INFO, "There is no local initial mapping for user");
    name=NULL;
  } else {
    if((name == NULL) || (name[0] == 0)) {
      logger.msg(Arc::INFO, "There is no local initial name for user");
      if(name) { std::free(name); name=NULL; };
    } else {
      gname = strchr(name,':');
      if(gname) {
        *gname = 0;
        ++gname;
        if(gname[0] == 0) gname = NULL;
      };
    };
  };
  user=u;
  if((!user.is_proxy()) || (user.proxy() == NULL) || (user.proxy()[0] == 0)) {
    logger.msg(Arc::INFO, "No proxy provided");
  } else {
    logger.msg(Arc::INFO, "Proxy stored at %s", user.proxy());
  };
  if((getuid() == 0) && name) {
    logger.msg(Arc::INFO, "Initially mapped to local user: %s", name);
    getpwnam_r(name,&pw_,bufp,BUFSIZ,&pw);
    if(pw == NULL) {
      logger.msg(Arc::ERROR, "Local user does not exist");
      std::free(name); name=NULL;
      return false;
    };
    if(gname) {
      logger.msg(Arc::INFO, "Initially mapped to local group: %s", gname);
      getgrnam_r(gname,&gr_,bufg,BUFSIZ,&gr);
      if(gr == NULL) {
        logger.msg(Arc::ERROR, "Local group %s does not exist",gname);
        std::free(name); name=NULL;
        return false;
      };
    };
  } else {
    if(name) std::free(name); name=NULL; gname=NULL;
    getpwuid_r(getuid(),&pw_,bufp,BUFSIZ,&pw);
    if(pw == NULL) {
      logger.msg(Arc::WARNING, "Running user has no name");
    } else {
      name=strdup(pw->pw_name);
      logger.msg(Arc::INFO, "Mapped to running user: %s", name);
    };
  };
  if(pw) {
    uid=pw->pw_uid;
    if(gr) {
      gid=gr->gr_gid;
    } else {
      gid=pw->pw_gid;
    };
    logger.msg(Arc::INFO, "Mapped to local id: %i", uid);
    home=pw->pw_dir;
    if(!gr) {
      getgrgid_r(gid,&gr_,bufg,BUFSIZ,&gr);
      if(gr == NULL) {
        logger.msg(Arc::INFO, "No group %i for mapped user", gid);
      };
    };
    std::string mapstr;
    if(name) mapstr+=name;
    mapstr+=":";
    if(gr) mapstr+=gr->gr_name;
    mapstr+=" all";
    default_map.mapname(mapstr.c_str());
    logger.msg(Arc::INFO, "Mapped to local group id: %i", pw->pw_gid);
    if(gr) logger.msg(Arc::INFO, "Mapped to local group name: %s", gr->gr_name);
    logger.msg(Arc::INFO, "Mapped user's home: %s", home);
  };
  if(name) std::free(name);
  return true;
}

std::string subst_user_spec(std::string &in,userspec_t *spec) {
  std::string out = "";
  unsigned int i;
  unsigned int last;
  last=0;
  for(i=0;i<in.length();i++) {
    if(in[i] == '%') {
      if(i>last) out+=in.substr(last,i-last);
      i++; if(i>=in.length()) {         };
      switch(in[i]) {
        case 'u': {
          char buf[10];
          snprintf(buf,9,"%i",spec->uid); out+=buf; last=i+1; 
        }; break;
        case 'U': { out+=spec->get_uname(); last=i+1; }; break;
        case 'g': {
          char buf[10];
          snprintf(buf,9,"%i",spec->gid); out+=buf; last=i+1; 
        }; break;
        case 'G': { out+=spec->get_gname(); last=i+1; }; break;
        case 'D': { out+=spec->user.DN(); last=i+1; }; break;
        case 'H': { out+=spec->home; last=i+1; }; break;
        case '%': { out+='%'; last=i+1; }; break;
        default: {
          logger.msg(Arc::WARNING, "Undefined control sequence: %%%s", in[i]);
        }; break;
      };
    };
  };
  if(i>last) out+=in.substr(last);
  return out;
}

bool userspec_t::refresh(void) {
  if(!map) return false;
  home=""; uid=-1; gid=-1; 
  const char* name = map.unix_name();
  const char* group = map.unix_group();
  if(name == NULL) return false;
  if(name[0] == 0) return false;
  struct passwd pw_;
  struct group gr_;
  struct passwd *pw;
  struct group *gr;
  char buf[BUFSIZ];
  getpwnam_r(name,&pw_,buf,BUFSIZ,&pw);
  if(pw == NULL) {
    logger.msg(Arc::ERROR, "Local user %s does not exist", name);
    return false;
  };
  uid=pw->pw_uid;
  home=pw->pw_dir;
  gid=pw->pw_gid;
  if(group && group[0]) {
    getgrnam_r(group,&gr_,buf,BUFSIZ,&gr);
    if(gr == NULL) {
      logger.msg(Arc::WARNING, "Local group %s does not exist", group);
    } else {
      gid=gr->gr_gid;
    };
  };
  logger.msg(Arc::INFO, "Remapped to local user: %s", name);
  logger.msg(Arc::INFO, "Remapped to local id: %i", uid);
  logger.msg(Arc::INFO, "Remapped to local group id: %i", gid);
  if(group && group[0]) logger.msg(Arc::INFO, "Remapped to local group name: %s", group);
  logger.msg(Arc::INFO, "Remapped user's home: %s", home);
  return true;
}

AuthResult userspec_t::mapname(const char* line) {
  AuthResult res = map.mapname(line);
  if(res == AAA_POSITIVE_MATCH) refresh();
  return res;
}

AuthResult userspec_t::mapgroup(const char* line) {
  AuthResult res = map.mapgroup(line);
  if(res == AAA_POSITIVE_MATCH) refresh();
  return res;
}

AuthResult userspec_t::mapvo(const char* line) {
  AuthResult res = map.mapvo(line);
  if(res == AAA_POSITIVE_MATCH) refresh();
  return res;
}

const char* userspec_t::get_uname(void) {
  const char* name = NULL;
  if((bool)map) { name=map.unix_name(); }
  else if((bool)default_map) { name=default_map.unix_name(); };
  if(!name) name="";
  return name;
}

const char* userspec_t::get_gname(void) {
  const char* group = NULL;
  if((bool)map) { group=map.unix_group(); }
  else if((bool)default_map) { group=default_map.unix_group(); };
  if(!group) group="";
  return group;
}

