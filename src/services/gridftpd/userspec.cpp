#include <cstdlib>
#include <string>
#include <fstream>

//#include <globus_gss_assist.h>
//#include <globus_io.h>
#include <globus_ftp_control.h>
#include <grp.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#include "userspec.h"
#include "misc/escaped.h"
//#include "../misc/log_time.h"
//#include "../misc/inttostring.h"
#include "conf.h"

#define olog std::cerr

void userspec_t::free(void) {
  // Keep authentication info to preserve proxy (just in case)
}

//userspec_t::userspec_t(void):user(),map(user),default_map(user),name(NULL),group(NULL),home(NULL),gridmap(false) {
userspec_t::userspec_t(void):user(),uid(-1),gid(-1),map(user),default_map(user),gridmap(false) {
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
    if((tmp == NULL) || (tmp[0] == 0)) {
      globus_gridmap="/etc/grid-security/grid-mapfile";
    }
    else { globus_gridmap=tmp; };
  };
  std::ifstream f(globus_gridmap.c_str());
  if(!f.is_open() ) {
    olog<<"Mapfile is missing at "<<globus_gridmap<<std::endl;
    return false;
  };
  for(;!f.eof();) {
    std::string buf;//char buf[512]; // must be enough for DN + name
    getline(f,buf);
    //buf[511]=0;
    char* p = &buf[0];
    for(;*p;p++) if(((*p) != ' ') && ((*p) != '\t')) break;
    if((*p) == '#') continue;
    if((*p) == 0) continue;
    std::string val;
    int n = gridftpd::input_escaped_string(p,val);
    if(strcmp(val.c_str(),dn) != 0) continue;
    p+=n;
    if(user) {
      n=gridftpd::input_escaped_string(p,val);
      *user=strdup(val.c_str());
    };
    f.close();
    return true;
  };
  f.close();
  return false;
}

/*
int fill_user_spec(userspec_t *spec,globus_ftp_control_auth_info_t *auth,globus_ftp_control_handle_t *handle) {
  if(spec == NULL) return 1;
  if(auth == NULL) return 1;
  if(!(spec->fill(auth,handle)
    )) return 1;
  return 0;
}
*/

bool userspec_t::fill(globus_ftp_control_auth_info_t *auth,globus_ftp_control_handle_t *handle) {
  struct passwd pw_;
  struct group gr_;
  struct passwd *pw;
  struct group *gr;
  char buf[BUFSIZ];
  if(auth == NULL) return false;
  if(auth->auth_gssapi_subject == NULL) return false;
  std::string subject = auth->auth_gssapi_subject;
  gridftpd::make_unescaped_string(subject);
  char* name=NULL;
  if(!check_gridmap(subject.c_str(),&name)) {
    olog<<"Warning: there is no local mapping for user"<<std::endl;
    name=NULL;
  } else {
    if((name == NULL) || (name[0] == 0)) {
      olog<<"Warning: there is no local name for user"<<std::endl;
      if(name) { std::free(name); name=NULL; };
    } else {
      gridmap=true;
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
      snprintf(abuf,sizeof(abuf)-1,"%u.%u.%u.%u",host[0],host[1],host[2],host[3]);
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
  };
  if((!user.is_proxy()) || (user.proxy() == NULL) || (user.proxy()[0] == 0)) {
    olog<<"No proxy provided"<<std::endl;
  } else {
    olog<<"Proxy stored at "<<user.proxy()<<std::endl;
  };
  if((getuid() == 0) && name) {
    olog<<"Initially mapped to local user: "<<name<<std::endl;
    getpwnam_r(name,&pw_,buf,BUFSIZ,&pw);
    if(pw == NULL) {
      olog<<"Local user does not exist"<<std::endl;
      std::free(name); name=NULL;
      return false;
    };
  } else {
    if(name) std::free(name); name=NULL;
    getpwuid_r(getuid(),&pw_,buf,BUFSIZ,&pw);
    if(pw == NULL) {
      olog<<"Warning: running user has no name"<<std::endl;
    } else {
      name=strdup(pw->pw_name);
      olog<<"Mapped to running user: "<<name<<std::endl;
    };
  };
  if(pw) {
    uid=pw->pw_uid;
    gid=pw->pw_gid;
    olog<<"Mapped to local id: "<<pw->pw_uid<<std::endl;
    home=pw->pw_dir;
    getgrgid_r(pw->pw_gid,&gr_,buf,BUFSIZ,&gr);
    if(gr == NULL) {
      olog<<"No group "<<gid<<" for mapped user"<<std::endl;
    };
    std::string mapstr;
    if(name) mapstr+=name;
    mapstr+=":";
    if(gr) mapstr+=gr->gr_name;
    mapstr+=" all";
    default_map.mapname(mapstr.c_str());
    olog<<"Mapped to local group id: "<<pw->pw_gid<<std::endl;
    if(gr) olog<<"Mapped to local group name: "<<gr->gr_name<<std::endl;
    olog<<"Mapped user's home: "<<home<<std::endl;
  };
  if(name) std::free(name);
  return true;
}

bool userspec_t::fill(AuthUser& u) {
  struct passwd pw_;
  struct group gr_;
  struct passwd *pw;
  struct group *gr;
  char buf[BUFSIZ];
  std::string subject = u.DN();
  char* name=NULL;
  if(!check_gridmap(subject.c_str(),&name)) {
    olog<<"Warning: there is no local mapping for user"<<std::endl;
    name=NULL;
  } else {
    if((name == NULL) || (name[0] == 0)) {
      olog<<"Warning: there is no local name for user"<<std::endl;
      if(name) { std::free(name); name=NULL; };
    } else {
      gridmap=true;
    };
  };
  user=u;
  if((!user.is_proxy()) || (user.proxy() == NULL) || (user.proxy()[0] == 0)) {
    olog<<"No proxy provided"<<std::endl;
  } else {
    olog<<"Proxy stored at "<<user.proxy()<<std::endl;
  };
  if((getuid() == 0) && name) {
    olog<<"Initially mapped to local user: "<<name<<std::endl;
    getpwnam_r(name,&pw_,buf,BUFSIZ,&pw);
    if(pw == NULL) {
      olog<<"Local user does not exist"<<std::endl;
      std::free(name); name=NULL;
      return false;
    };
  } else {
    if(name) std::free(name); name=NULL;
    getpwuid_r(getuid(),&pw_,buf,BUFSIZ,&pw);
    if(pw == NULL) {
      olog<<"Warning: running user has no name"<<std::endl;
    } else {
      name=strdup(pw->pw_name);
      olog<<"Mapped to running user: "<<name<<std::endl;
    };
  };
  if(pw) {
    uid=pw->pw_uid;
    gid=pw->pw_gid;
    olog<<"Mapped to local id: "<<pw->pw_uid<<std::endl;
    home=pw->pw_dir;
    getgrgid_r(pw->pw_gid,&gr_,buf,BUFSIZ,&gr);
    if(gr == NULL) {
      olog<<"No group "<<gid<<" for mapped user"<<std::endl;
    };
    std::string mapstr;
    if(name) mapstr+=name;
    mapstr+=":";
    if(gr) mapstr+=gr->gr_name;
    mapstr+=" all";
    default_map.mapname(mapstr.c_str());
    olog<<"Mapped to local group id: "<<pw->pw_gid<<std::endl;
    if(gr) olog<<"Mapped to local group name: "<<gr->gr_name<<std::endl;
    olog<<"Mapped user's home: "<<home<<std::endl;
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
          olog << "Warning: undefined control sequence: %" << in[i] << std::endl;
        };
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
    olog<<"Local user "<<name<<" does not exist"<<std::endl;
    return false;
  };
  uid=pw->pw_uid;
  home=pw->pw_dir;
  gid=pw->pw_gid;
  if(group && group[0]) {
    getgrnam_r(group,&gr_,buf,BUFSIZ,&gr);
    if(gr == NULL) {
      olog<<"Warning: local group "<<group<<" does not exist"<<std::endl;
    } else {
      gid=gr->gr_gid;
    };
  };
  olog<<"Remapped to local user: "<<name<<std::endl;
  olog<<"Remapped to local id: "<<uid<<std::endl;
  olog<<"Remapped to local group id: "<<gid<<std::endl;
  if(group && group[0]) olog<<"Remapped to local group name: "<<group<<std::endl;
  olog<<"Remapped user's home: "<<home<<std::endl;
  return true;
}

bool userspec_t::mapname(const char* line) {
  if(!map.mapname(line)) return false;
  refresh();
  return true;
}

bool userspec_t::mapgroup(const char* line) {
  if(!map.mapgroup(line)) return false;
  refresh();
  return true;
}

bool userspec_t::mapvo(const char* line) {
  if(!map.mapvo(line)) return false;
  refresh();
  return true;
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

