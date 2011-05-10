#include <fstream>
#include <iostream>

#include <arc/Logger.h>
#include <arc/StringConv.h>
#include <arc/Run.h>

#include "simplemap.h"

#include "unixmap.h"

namespace ArcSHCLegacy {

static Arc::Logger logger(Arc::Logger::getRootLogger(),"UnixMap");

UnixMap::source_t UnixMap::sources[] = {
  { "mapfile", &UnixMap::map_mapfile },
  { "simplepool", &UnixMap::map_simplepool },
  { "unixuser", &UnixMap::map_unixuser },
  { "lcmaps", &UnixMap::map_lcmaps },
  { "mapplugin", &UnixMap::map_mapplugin },
  { NULL, NULL }
};

UnixMap::UnixMap(AuthUser& user,const std::string& id):
  user_(user),map_id_(id),mapped_(false) {
}

UnixMap::~UnixMap(void) {
}

void split_unixname(std::string& unixname,std::string& unixgroup) {
  std::string::size_type p = unixname.find(':');
  if(p != std::string::npos) {
    unixgroup=unixname.c_str()+p+1;
    unixname.resize(p);
  };
  if(unixname[0] == '*') unixname.resize(0);
  if(unixgroup[0] == '*') unixgroup.resize(0);
}

bool UnixMap::mapgroup(const char* line) {
  mapped_=false;
  if(!line) return false;
  for(;*line;line++) if(!isspace(*line)) break;
  if(*line == 0) return false;
  const char* groupname = line;
  for(;*line;line++) if(isspace(*line)) break;
  int groupname_len = line-groupname;
  if(groupname_len == 0) return false;
  if(!user_.check_group(std::string(groupname,groupname_len))) return false;
  unix_user_.name.resize(0);
  unix_user_.group.resize(0);
  for(;*line;line++) if(!isspace(*line)) break;
  const char* command = line;
  for(;*line;line++) if(isspace(*line)) break;
  size_t command_len = line-command;
  if(command_len == 0) return false;
  for(;*line;line++) if(!isspace(*line)) break;
  for(source_t* s = sources;s->cmd;s++) {
    if((strncmp(s->cmd,command,command_len) == 0) &&
       (strlen(s->cmd) == command_len)) {
      bool res=(this->*(s->map))(user_,unix_user_,line);
      if(res) {
        mapped_=true;
        return true;
      };
    };
  };
  return false;
}

bool UnixMap::mapvo(const char* line) {
  mapped_=false;
  if(!line) return false;
  for(;*line;line++) if(!isspace(*line)) break;
  if(*line == 0) return false;
  const char* voname = line;
  for(;*line;line++) if(isspace(*line)) break;
  int voname_len = line-voname;
  if(voname_len == 0) return false;
  if(!user_.check_vo(std::string(voname,voname_len))) return false;
  unix_user_.name.resize(0);
  unix_user_.group.resize(0);
  for(;*line;line++) if(!isspace(*line)) break;
  const char* command = line;
  for(;*line;line++) if(isspace(*line)) break;
  size_t command_len = line-command;
  if(command_len == 0) return false;
  for(;*line;line++) if(!isspace(*line)) break;
  for(source_t* s = sources;s->cmd;s++) {
    if((strncmp(s->cmd,command,command_len) == 0) &&
       (strlen(s->cmd) == command_len)) {
      bool res=(this->*(s->map))(user_,unix_user_,line);
      if(res) {
        mapped_=true;
        return true;
      };
    };
  };
  return false;
}

bool UnixMap::mapname(const char* line) {
  mapped_=false;
  if(!line) return false;
  for(;*line;line++) if(!isspace(*line)) break;
  if(*line == 0) return false;
  const char* unixname = line;
  for(;*line;line++) if(isspace(*line)) break;
  int unixname_len = line-unixname;
  if(unixname_len == 0) return false;
  unix_user_.name.assign(unixname,unixname_len);
  split_unixname(unix_user_.name,unix_user_.group);
  for(;*line;line++) if(!isspace(*line)) break;
  const char* command = line;
  for(;*line;line++) if(isspace(*line)) break;
  size_t command_len = line-command;
  if(command_len == 0) return false;
  for(;*line;line++) if(!isspace(*line)) break;
  for(source_t* s = sources;s->cmd;s++) {
    if((strncmp(s->cmd,command,command_len) == 0) &&
       (strlen(s->cmd) == command_len)) {
      bool res=(this->*(s->map))(user_,unix_user_,line);
      if(res) {
        mapped_=true;
        return true;
      };
    };
  };
  if(unix_user_.name.length() != 0) {
    // Try authorization rules if username is predefined
    int decision = user_.evaluate(command);
    if(decision == AAA_POSITIVE_MATCH) {
      mapped_=true;
      return true;
    };
  };
  return false;
}

// -----------------------------------------------------------

bool UnixMap::map_mapplugin(const AuthUser& /* user */ ,unix_user_t& unix_user,const char* line) {
  // timeout path arg ...
  if(!line) return false;
  for(;*line;line++) if(!isspace(*line)) break;
  if(*line == 0) return false;
  char* p;
  long int to = strtol(line,&p,0);
  if(p == line) return false;
  if(to < 0) return false;
  line=p;
  for(;*line;line++) if(!isspace(*line)) break;
  if(*line == 0) return false;
  std::list<std::string> args;
  Arc::tokenize(line,args," ","\"","\"");
  if(args.size() <= 0) return false;
  for(std::list<std::string>::iterator arg = args.begin();
          arg != args.end();++arg) user_.subst(*arg);
  std::string stdout_channel;
  Arc::Run run(args);
  run.AssignStdout(stdout_channel);
  if(!run.Start()) return false;
  if(!run.Wait(to)) return false;
  if(run.Result() != 0) return false;
  // Plugin should print user[:group] at stdout
  if(stdout_channel.length() > 512) return false; // really strange name
  unix_user.name = stdout_channel;
  split_unixname(unix_user.name,unix_user.group);
  return true;
}

bool UnixMap::map_mapfile(const AuthUser& user,unix_user_t& unix_user,const char* line) {
  // This is just grid-mapfile
  std::ifstream f(line);
  if(user.DN()[0] == 0) return false;
  if(!f.is_open() ) {
    logger.msg(Arc::ERROR, "Mapfile at %s can't be opened.", line);
    return false;
  };
  for(;!f.eof();) {
    std::string buf;
    getline(f,buf);
    std::string::size_type p = 0;
    for(;p<buf.length();++p) if(!isspace(buf[p])) break;
    if(buf[p] == '#') continue;
    if(p>=buf.length()) continue;
    std::string val;
    p = Arc::get_token(val,buf,p," ","\"","\"");
    if(val != user.DN()) continue;
    p = Arc::get_token(unix_user.name,buf,p," ","\"","\"");
    f.close();
    return true;
  };
  f.close();
  return false;
}

bool UnixMap::map_simplepool(const AuthUser& user,unix_user_t& unix_user,const char* line) {
  if(user.DN()[0] == 0) return false;
  SimpleMap pool(line);
  if(!pool) {
    logger.msg(Arc::ERROR, "User pool at %s can't be opened.", line);
    return false;
  };
  unix_user.name=pool.map(user.DN());
  if(unix_user.name.empty()) return false;
  split_unixname(unix_user.name,unix_user.group);
  return true;
}

bool UnixMap::map_unixuser(const AuthUser& /* user */,unix_user_t& unix_user,const char* line) {
  // Maping is always positive - just fill specified username
  std::string unixname(line);
  std::string unixgroup;
  std::string::size_type p = unixname.find(':');
  if(p != std::string::npos) {
    unixgroup=unixname.c_str()+p+1;
    unixname.resize(p);
  };
  if(unixname.empty()) return false;
  unix_user.name=unixname;
  unix_user.group=unixgroup;
  return true;
}

} // namespace ArcSHCLegacy

