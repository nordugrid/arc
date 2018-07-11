#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <fstream>
#include <iostream>

#include <arc/Logger.h>
#include <arc/ArcConfigIni.h>

#include "../run/run_plugin.h"
#include "simplemap.h"

#include "unixmap.h"

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

AuthResult UnixMap::mapgroup(const char* line) {
  mapped_=false;
  if(!line) {
    logger.msg(Arc::ERROR,"User name mapping command is empty");
    return AAA_FAILURE;
  };
  for(;*line;line++) if(!isspace(*line)) break;
  if(*line == 0) {
    logger.msg(Arc::ERROR,"User name mapping command is empty");
    return AAA_FAILURE;
  };
  const char* groupname = line;
  for(;*line;line++) if(isspace(*line)) break;
  int groupname_len = line-groupname;
  if(groupname_len == 0) {
    logger.msg(Arc::ERROR,"User name mapping has empty group: %s", groupname);
    return AAA_FAILURE;
  };
  if(!user_.check_group(std::string(groupname,groupname_len))) return AAA_NO_MATCH;
  unix_user_.name.resize(0);
  unix_user_.group.resize(0);
  for(;*line;line++) if(!isspace(*line)) break;
  const char* command = line;
  for(;*line;line++) if(isspace(*line)) break;
  size_t command_len = line-command;
  if(command_len == 0) {
    logger.msg(Arc::ERROR,"User name mapping has empty command: %s", command);
    return AAA_FAILURE;
  };
  for(;*line;line++) if(!isspace(*line)) break;
  for(source_t* s = sources;s->cmd;s++) {
    if((strncmp(s->cmd,command,command_len) == 0) &&
       (strlen(s->cmd) == command_len)) {
      AuthResult res=(this->*(s->map))(user_,unix_user_,line);
      if(res == AAA_POSITIVE_MATCH) {
        mapped_=true;
        return AAA_POSITIVE_MATCH;
      };
      if(res == AAA_FAILURE) {
        // Processing failure cause immediate error
        return AAA_FAILURE;
      };
      // Paranoid about negative match
      return AAA_NO_MATCH;
    };
  };
  return AAA_FAILURE;
}

AuthResult UnixMap::mapvo(const char* line) {
  mapped_=false;
  if(!line) {
    logger.msg(Arc::ERROR,"User name mapping command is empty");
    return AAA_FAILURE;
  };
  for(;*line;line++) if(!isspace(*line)) break;
  if(*line == 0) {
    logger.msg(Arc::ERROR,"User name mapping command is empty");
    return AAA_FAILURE;
  };
  const char* voname = line;
  for(;*line;line++) if(isspace(*line)) break;
  int voname_len = line-voname;
  if(voname_len == 0) {
    logger.msg(Arc::ERROR,"User name mapping has empty VO: %s", voname);
    return AAA_FAILURE;
  };
  if(!user_.check_vo(std::string(voname,voname_len))) return AAA_NO_MATCH;
  unix_user_.name.resize(0);
  unix_user_.group.resize(0);
  for(;*line;line++) if(!isspace(*line)) break;
  const char* command = line;
  for(;*line;line++) if(isspace(*line)) break;
  size_t command_len = line-command;
  if(command_len == 0) {
    logger.msg(Arc::ERROR,"User name mapping has empty command: %s", command);
    return AAA_FAILURE;
  };
  for(;*line;line++) if(!isspace(*line)) break;
  for(source_t* s = sources;s->cmd;s++) {
    if((strncmp(s->cmd,command,command_len) == 0) &&
       (strlen(s->cmd) == command_len)) {
      AuthResult res=(this->*(s->map))(user_,unix_user_,line);
      if(res == AAA_POSITIVE_MATCH) {
        mapped_=true;
        return AAA_POSITIVE_MATCH;
      };
      if(res == AAA_FAILURE) {
        // Processing failure cause immediate error
        return AAA_FAILURE;
      };
      // Paranoid about negative match
      return AAA_NO_MATCH;
    };
  };
  return AAA_FAILURE;
}

AuthResult UnixMap::mapname(const char* line) {
  mapped_=false;
  if(!line) {
    logger.msg(Arc::ERROR,"User name mapping command is empty");
    return AAA_FAILURE;
  };
  for(;*line;line++) if(!isspace(*line)) break;
  if(*line == 0) {
    logger.msg(Arc::ERROR,"User name mapping command is empty");
    return AAA_FAILURE;
  };
  const char* unixname = line;
  for(;*line;line++) if(isspace(*line)) break;
  int unixname_len = line-unixname;
  if(unixname_len == 0) {
    logger.msg(Arc::ERROR,"User name mapping has empty name: %s", unixname);
    return AAA_FAILURE;
  };
  unix_user_.name.assign(unixname,unixname_len);
  split_unixname(unix_user_.name,unix_user_.group);
  for(;*line;line++) if(!isspace(*line)) break;
  const char* command = line;
  for(;*line;line++) if(isspace(*line)) break;
  size_t command_len = line-command;
  if(command_len == 0) {
    logger.msg(Arc::ERROR,"User name mapping has empty command: %s", command);
    return AAA_FAILURE;
  }
  for(;*line;line++) if(!isspace(*line)) break;
  for(source_t* s = sources;s->cmd;s++) {
    if((strncmp(s->cmd,command,command_len) == 0) &&
       (strlen(s->cmd) == command_len)) {
      AuthResult res=(this->*(s->map))(user_,unix_user_,line);
      if(res == AAA_POSITIVE_MATCH) {
        mapped_=true;
        return AAA_POSITIVE_MATCH;
      };
      if(res == AAA_FAILURE) {
        // Processing failure cause immediate error
        return AAA_FAILURE;
      };
      // Paranoid about negative match
      return AAA_NO_MATCH;
    };
  };
  if(unix_user_.name.length() != 0) {
    // Try authorization rules if username is predefined
    AuthResult decision = user_.evaluate(command);
    if(decision == AAA_POSITIVE_MATCH) {
      mapped_=true;
      return AAA_POSITIVE_MATCH;
    };
    return decision; // propagate failure information
  };
  // If user name is not defined then it was supposed to be 
  // mapping rule. And if not then we failed.
  return AAA_FAILURE;
}

// -----------------------------------------------------------

static void subst_arg(std::string& str,void* arg) {
  AuthUser* it = (AuthUser*)arg;
  if(!it) return;
  AuthUserSubst(str,*it);
}

AuthResult UnixMap::map_mapplugin(const AuthUser& /* user */ ,unix_user_t& unix_user,const char* line) {
  // timeout path arg ...
  if(!line) {
    logger.msg(Arc::ERROR,"Plugin (user mapping) command is empty");
    return AAA_FAILURE;
  };
  for(;*line;line++) if(!isspace(*line)) break;
  if(*line == 0) {
    logger.msg(Arc::ERROR,"Plugin (user mapping) command is empty");
    return AAA_FAILURE;
  };
  char* p;
  long int to = strtol(line,&p,0);
  if(p == line) {
    logger.msg(Arc::ERROR,"Plugin (user mapping) timeout is not a number: %s", line);
    return AAA_FAILURE;
  };
  if(to < 0) {
    logger.msg(Arc::ERROR,"Plugin (user mapping) timeout is wrong number: %s", line);
    return AAA_FAILURE;
  };
  line=p;
  for(;*line;line++) if(!isspace(*line)) break;
  if(*line == 0) {
    logger.msg(Arc::ERROR,"Plugin (user mapping) command is empty");
    return AAA_FAILURE;
  };
  std::string s = line;
  gridftpd::RunPlugin run(line);
  run.timeout(to);
  if(run.run(subst_arg,&user_)) {
    if(run.result() == 0) {
      if(run.stdout_channel().length() <= 512) { // sane name
        // Plugin should print user[:group] at stdout or nothing if no suitable mapping found
        unix_user.name = run.stdout_channel();
        split_unixname(unix_user.name,unix_user.group);
        if(unix_user.name.empty()) return AAA_NO_MATCH; // success but no match
        return AAA_POSITIVE_MATCH;
      } else {
        logger.msg(Arc::ERROR,"Plugin %s returned too much: %s",run.cmd(),run.stdout_channel());
      };
    } else {
      logger.msg(Arc::ERROR,"Plugin %s returned: %u",run.cmd(),(unsigned int)run.result());
    };
  } else {
    logger.msg(Arc::ERROR,"Plugin %s failed to run",run.cmd());
  };
  logger.msg(Arc::INFO,"Plugin %s printed: %u",run.cmd(),run.stdout_channel());
  logger.msg(Arc::ERROR,"Plugin %s error: %u",run.cmd(),run.stderr_channel());
  return AAA_FAILURE;
}

AuthResult UnixMap::map_mapfile(const AuthUser& user,unix_user_t& unix_user,const char* line) {
  // This is just grid-mapfile
  std::ifstream f(line);
  if(user.DN()[0] == 0) return AAA_FAILURE;
  if(!f.is_open() ) {
    logger.msg(Arc::ERROR, "Mapfile at %s can't be opened.", line);
    return AAA_FAILURE;
  };
  for(;f.good();) {
    std::string buf; //char buf[512]; // must be enough for DN + name
    getline(f,buf);
    char* p = &buf[0];
    for(;*p;p++) if(((*p) != ' ') && ((*p) != '\t')) break;
    if((*p) == '#') continue;
    if((*p) == 0) continue;
    std::string val;
    int n = Arc::ConfigIni::NextArg(p,val,' ','"');
    if(strcmp(val.c_str(),user.DN()) != 0) continue;
    p+=n;
    Arc::ConfigIni::NextArg(p,unix_user.name,' ','"');
    f.close();
    return AAA_POSITIVE_MATCH;
  };
  f.close();
  return AAA_NO_MATCH;
}

AuthResult UnixMap::map_simplepool(const AuthUser& user,unix_user_t& unix_user,const char* line) {
  if(user.DN()[0] == 0) {
    logger.msg(Arc::ERROR, "User pool call is missing user subject.");
    return AAA_NO_MATCH;
  };
  SimpleMap pool(line);
  if(!pool) {
    logger.msg(Arc::ERROR, "User pool at %s can't be opened.", line);
    return AAA_FAILURE;
  };
  unix_user.name=pool.map(user.DN());
  if(unix_user.name.empty()) {
    logger.msg(Arc::ERROR, "User pool at %s failed to perform user mapping.", line);
    return AAA_FAILURE;
  };
  split_unixname(unix_user.name,unix_user.group);
  return AAA_POSITIVE_MATCH;
}

AuthResult UnixMap::map_unixuser(const AuthUser& /* user */,unix_user_t& unix_user,const char* line) {
  // Maping is always positive - just fill specified username
  std::string unixname(line);
  std::string unixgroup;
  std::string::size_type p = unixname.find(':');
  if(p != std::string::npos) {
    unixgroup=unixname.c_str()+p+1;
    unixname.resize(p);
  };
  if(unixname.empty()) {
    logger.msg(Arc::ERROR, "User name direct mapping is missing user name: %s.", line);
    return AAA_FAILURE;
  };
  unix_user.name=unixname;
  unix_user.group=unixgroup;
  return AAA_POSITIVE_MATCH;
}

