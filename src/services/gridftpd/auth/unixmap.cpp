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
  { "map_with_file", &UnixMap::map_mapfile },
  { "map_to_pool", &UnixMap::map_simplepool },
  { "map_to_user", &UnixMap::map_unixuser },
  { "lcmaps", &UnixMap::map_lcmaps },
  { "map_with_plugin", &UnixMap::map_mapplugin },
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

static inline void skip_spaces(char const * & line) {
  for(;*line;line++) if(!isspace(*line)) break; // skip spaces
}

static inline void skip_till_space(char const * & line) {
  for(;*line;line++) if(isspace(*line)) break;
}

enum opt_action {
  action_continue,
  action_stop
};

enum opt_event {
  event_nogroup,
  event_nomap,
  event_map
};

static char const option_none_str[] = { "*" };

static char const action_continue_str[] = { "continue" };
static char const action_stop_str[] = { "stop" };

static char const event_nogroup_str[] = { "onnogroup" };
static char const event_nomap_str[]   = { "onnomap" };
static char const event_map_str[]     = { "onmap" };

#define match_string(tag,start,end) (((end-start+1) == sizeof(tag)) && (strncmp(start,tag,end-start) == 0))

static bool parse_option(const char* option, int option_len, opt_event& event, opt_action& action) {
  const char* option_end = option+option_len;
  if(match_string(option_none_str,option,option_end)) return true;
  const char* event_start = option;
  for(;option<option_end;option++) if(*option == '=') break;
  if(option >= option_end) return false;
  const char* event_end = option;
  ++option;
  const char* action_start = option;
  const char* action_end = option_end;
  if(match_string(event_nogroup_str,event_start,event_end)) {
    event = event_nogroup;
  } else if(match_string(event_nomap_str,event_start,event_end)) {
    event = event_nomap;
  } else if(match_string(event_map_str,event_start,event_end)) {
    event = event_map;
  } else {
    return false;
  }
  if(match_string(action_continue_str,action_start,action_end)) {
    action = action_continue;
  } else if(match_string(action_stop_str,action_start,action_end)) {
    action = action_stop;
  } else {
    return false;
  }
  return true;
}

static bool parse_options(const char* options, int options_len, opt_action& onnogroup, opt_action& onnomap, opt_action& onmap) {
  // default actions
  onnogroup = action_continue;
  onnomap = action_continue;
  onmap = action_stop;
  const char* options_end = options+options_len;
  while(options<options_end) {
    const char* option = options;
    for(;options<options_end;options++) if(*options == ',') break;
    opt_event event;
    opt_action action;
    if(!parse_option(option, options-option, event, action))
      return false;
    switch(event) {
      case event_nogroup:
        onnogroup = action;
        break;
      case event_nomap:
        onnomap = action;
        break;
      case event_map:
        onmap = action;
        break;
    };
    ++options;
  }
  return true;
}

AuthResult UnixMap::mapgroup(const char* rule, const char* line) {
  // now: rule = options authgroup args
  mapped_=false;
  if(!line) {
    logger.msg(Arc::ERROR,"User name mapping command is empty");
    return AAA_FAILURE;
  };
  skip_spaces(line);
  if(*line == 0) {
    logger.msg(Arc::ERROR,"User name mapping command is empty");
    return AAA_FAILURE;
  };
  // identify common parts of mapping commands
  const char* options = line;
  skip_till_space(line);
  int options_len = line-options;
  skip_spaces(line);
  const char* groupname = line;
  skip_till_space(line);
  int groupname_len = line-groupname;
  if(groupname_len == 0) {
    logger.msg(Arc::ERROR,"User name mapping has empty group: %s", groupname);
    return AAA_FAILURE;
  };
  // parse options
  opt_action onnogroup;
  opt_action onnomap;
  opt_action onmap;
  if(!parse_options(options, options_len, onnogroup, onnomap, onmap)) {
    logger.msg(Arc::ERROR,"User name mapping options parsing error");
    return AAA_FAILURE;
  };
  // match specified authgroup
  if(!user_.check_group(std::string(groupname,groupname_len))) {
    // event onnogroup
    mapped_= (onnogroup == action_stop);
    return AAA_NO_MATCH;
  };
  unix_user_.name.resize(0);
  unix_user_.group.resize(0);
  skip_spaces(line);
  if(!rule || (*rule == 0)) {
    logger.msg(Arc::ERROR,"User name mapping has empty command");
    return AAA_FAILURE;
  };
  for(source_t* s = sources;s->cmd;s++) {
    if(strcmp(s->cmd,rule) == 0) {
      AuthResult res=(this->*(s->map))(user_,unix_user_,line);
      if(res == AAA_POSITIVE_MATCH) {
        // event onmap
        mapped_= (onmap == action_stop);
        return AAA_POSITIVE_MATCH;
      };
      if(res == AAA_FAILURE) {
        // Processing failure, cause immediate error
        return AAA_FAILURE;
      };
      // Paranoid about negative match
      // event onnomap
      mapped_= (onnomap == action_stop);
      return AAA_NO_MATCH;
    };
  };
  logger.msg(Arc::ERROR,"Unknown user name mapping rule %s",rule);
  return AAA_FAILURE;
}

AuthResult UnixMap::setunixuser(const char* unixname, const char* unixgroup) {
  mapped_=false;
  if((!unixname) || (*unixname == 0)) {
    logger.msg(Arc::ERROR,"User name mapping has empty name: %s", unixname);
    return AAA_FAILURE;
  };
  unix_user_.name.assign(unixname);
  if(unixgroup)
    unix_user_.group.assign(unixgroup);
  mapped_=true;
  return AAA_POSITIVE_MATCH;
}

// -----------------------------------------------------------

static void subst_arg(std::string& str,void* arg) {
  AuthUser* it = (AuthUser*)arg;
  if(!it) return;
  AuthUserSubst(str,*it);
}

AuthResult UnixMap::map_mapplugin(const AuthUser& /* user */ ,unix_user_t& unix_user,const char* line) {
  // ... timeout path arg ...
  if(!line) {
    logger.msg(Arc::ERROR,"Plugin (user mapping) command is empty");
    return AAA_FAILURE;
  };
  skip_spaces(line);
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
  skip_spaces(line);
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
  // ... file
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
  // ... dir
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
  // ... unixname[:unixgroup]
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

