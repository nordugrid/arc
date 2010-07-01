#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef HAVE_SSTREAM
#include <sstream>
#else
#include <strstream>
#endif

#include "conf.h"
#include "names.h"
#include "misc.h"
#include "auth/auth.h"
#include "misc/canonical_dir.h"
#include "conf/conf_vo.h"
#include "conf/environment.h"

#include "fileroot.h"

static Arc::Logger logger(Arc::Logger::getRootLogger(),"FileRoot");

int FileRoot::config(gridftpd::Daemon &daemon,ServerParams* params) {
  /* open and read configuration file */
  std::ifstream cfile;
  gridftpd::ConfigSections* cf = NULL;
  config_open_gridftp(cfile);
  if(!cfile.is_open()) {
    logger.msg(Arc::ERROR, "configuration file not found");
    return 1;
  };
  cf=new gridftpd::ConfigSections(cfile);
  cf->AddSection("common");
  cf->AddSection("gridftpd");
  for(;;) {
    std::string rest;
    std::string command;
    cf->ReadNext(command,rest);
    if(command.length() == 0) break; /* EOF */
    if(cf->SubSection()[0] != 0) continue;
    int r;
    r=daemon.config(command,rest);
    if(r == 0) continue;
    if(r == -1) { gridftpd::config_close(cfile); if(cf) delete cf; return 1; };
    if(command == "port") {
      if(params) {
        if(sscanf(rest.c_str(),"%u",&(params->port)) != 1) {
          logger.msg(Arc::ERROR, "Wrong port number in configuration");
          gridftpd::config_close(cfile);
          if(cf) delete cf;
          return 1;
        };
      };
    } else if(command == "maxconnections") {
      if(params) {
        if(sscanf(rest.c_str(),"%u",&(params->max_connections)) != 1) {
          logger.msg(Arc::ERROR, "Wrong maxconnections number in configuration");
          gridftpd::config_close(cfile);
          if(cf) delete cf;
          return 1;
        };
      };
    } else if(command == "defaultbuffer") {
      if(params) {
        if(sscanf(rest.c_str(),"%u",&(params->default_buffer)) != 1) {
          logger.msg(Arc::ERROR, "Wrong defaultbuffer number in configuration");
          gridftpd::config_close(cfile);
          if(cf) delete cf;
          return 1;
        };
      };
    } else if(command == "maxbuffer") {
      if(params) {
        if(sscanf(rest.c_str(),"%u",&(params->max_buffer)) != 1) {
          logger.msg(Arc::ERROR, "Wrong maxbuffer number in configuration");
          gridftpd::config_close(cfile);
          if(cf) delete cf;
          return 1;
        };
      };
    } else if(command == "firewall") {
      if(params) {
        std::string value=gridftpd::config_next_arg(rest);
        int    errcode;
        struct hostent* host;
        struct hostent  hostbuf;
#ifndef _AIX
        char   buf[BUFSIZ];
        if(gethostbyname_r(value.c_str(),
                &hostbuf,buf,sizeof(buf),&host,&errcode)) {
#else
        struct hostent_data buf[BUFSIZ];
        if((errcode=gethostbyname_r(value.c_str(),
                (host=&hostbuf),buf))) {
#endif
          logger.msg(Arc::ERROR, "Can't resolve host %s", value);
          gridftpd::config_close(cfile);
          if(cf) delete cf;
          return 1;
        };
        if( (host == NULL) ||
            (host->h_length < sizeof(struct in_addr)) ||
            (host->h_addr_list[0] == NULL) ) {
          logger.msg(Arc::ERROR, "Can't resolve host %s", value);
          gridftpd::config_close(cfile);
          if(cf) delete cf;
          return 1;
        };
        unsigned char* addr = (unsigned char*)(&(((struct in_addr*)(host->h_addr_list[0]))->s_addr));
        params->firewall[0]=addr[0];
        params->firewall[1]=addr[1];
        params->firewall[2]=addr[2];
        params->firewall[3]=addr[3];
      };
    };
  };
    gridftpd::config_close(cfile);
  if(cf) delete cf;
  return 0;
}



int FileRoot::config(std::ifstream &cfile,std::string &pluginpath) {
  bool right_group = true;
  for(;;) {
    std::string rest;
    std::string command=gridftpd::config_read_line(cfile,rest);
    if(command.length() == 0) break; /* EOF */
    if(gridftpd::Daemon::skip_config(command) == 0) {
    }
    else if(command == "include") {  /* include content of another file */
      std::string name=gridftpd::config_next_arg(rest);
      std::ifstream cfile_;
      cfile_.open(name.c_str(),std::ifstream::in);
      if(!cfile_.is_open()) {
        logger.msg(Arc::ERROR, "couldn't open file %s", name);
        return 1;
      };
      config(cfile_,pluginpath);
      cfile_.close();
    }
    else if(command == "encryption") {  /* is encryption allowed ? */
      std::string value=gridftpd::config_next_arg(rest);
      if(value == "yes") {
        heavy_encryption=true;
      } else if(value == "no") {
        heavy_encryption=false;
      } else {
        user.user.clear_groups();
        nodes.clear();
        logger.msg(Arc::ERROR, "improper attribute for encryption command: %s", value);
        return 1;
      };
    }
    else if(command == "allowunknown") { /* should user be present in grid-mapfile ? */
      std::string value=gridftpd::config_next_arg(rest);
      if(value == "yes") {
        user.gridmap=true;
      } else if(value == "no") {
        if(!user.gridmap) {
          logger.msg(Arc::ERROR, "unknown (non-gridmap) user is not allowed");
          return 1;
        };
      } else {
        user.user.clear_groups();
        nodes.clear();
        logger.msg(Arc::ERROR, "improper attribute for encryption command: %s", value);
        return 1;
      };
    }
    else if(command == "group") { /* definition of group of users based on DN */
      if(!right_group) {
        for(;;) {
          command=gridftpd::config_read_line(cfile,rest);
          if(command.length() == 0) break; /* eof - bad */
          if(command == "end") break;
        };
        continue;
      };
      std::string group_name=gridftpd::config_next_arg(rest);
      int decision = AAA_NO_MATCH;
      for(;;) {
        std::string rest=gridftpd::config_read_line(cfile);
        if(rest.length() == 0) break; /* eof - bad */
        if(rest == "end") break;
        if(decision == AAA_NO_MATCH) {
          decision = user.user.evaluate(rest.c_str());
          if(decision == AAA_FAILURE) decision=AAA_NO_MATCH;
        };
      };
      if(decision == AAA_POSITIVE_MATCH) user.user.add_group(group_name);
    }
    else if(command == "vo") {
      if(gridftpd::config_vo(user.user,command,rest) != 0) {
        logger.msg(Arc::ERROR, "couldn't process VO configuration");
        return 1;
      };
    }
    else if(command == "unixmap") {  /* map to local unix user */
      if(!user.mapped()) user.mapname(rest.c_str());
    }
    else if(command == "unixgroup") {  /* map to local unix user */
      if(!user.mapped()) user.mapgroup(rest.c_str());
    }
    else if(command == "unixvo") {  /* map to local unix user */
      if(!user.mapped()) user.mapvo(rest.c_str());
    }
    else if(command == "groupcfg") {  /* next commands only for these groups */
      if(rest.find_first_not_of(" \t") == std::string::npos) {
        right_group=true; continue;
      };
      right_group=false;
      for(;;) {
        std::string group_name=gridftpd::config_next_arg(rest);
        if(group_name.length() == 0) break;
        right_group=user.user.check_group(group_name);
        if(right_group) break;
      };
    }
    else if(command == "pluginpath") {
      if(!right_group) continue;
      pluginpath=gridftpd::config_next_arg(rest);
      if(pluginpath.length() == 0) pluginpath="/";
    }
    else if(command == "port") {

    }
    else if(command == "plugin") {
      if(!right_group) {
        for(;;) {
          command=gridftpd::config_read_line(cfile,rest);
          if(command.length() == 0) break; /* eof - bad */
          if(command == "end") break;
        };
        continue;
      };
      std::string dir = gridftpd::config_next_arg(rest);
      std::string plugin = gridftpd::config_next_arg(rest);
      if(plugin.length() == 0) {
        logger.msg(Arc::WARNING, "can't parse configuration line: %s %s %s %s", command, dir, plugin, rest);
        continue;
      };
      dir=subst_user_spec(dir,&user);
      if(gridftpd::canonical_dir(dir,false) != 0) {
        logger.msg(Arc::WARNING, "bad directory in plugin command: %s", dir);
        continue;
      };
      /* look if path is not already registered */
      bool already_have_this_path=false;
      for(std::list<FileNode>::iterator i=nodes.begin();i!=nodes.end();++i) {
        if((*i) == dir) {
          already_have_this_path=true;
          break;
        };
      };
      if(already_have_this_path) {
        logger.msg(Arc::WARNING, "Already have directory: %s", dir);
        for(;;) {
          command=gridftpd::config_read_line(cfile,rest);
          if(command.length() == 0) break; /* eof - bad */
          if(command == "end") break;
        };
      }
      else {
        logger.msg(Arc::INFO, "Registering directory: %s with plugin: %s", dir, plugin);
        plugin=pluginpath+'/'+plugin;
        FileNode node((char*)(dir.c_str()),(char*)(plugin.c_str)(),cfile,user);
        if(node.has_plugin()) { nodes.push_back(node); }
        else {
          logger.msg(Arc::ERROR, "file node creation failed: %s", dir);
          for(;;) {
            command=gridftpd::config_read_line(cfile,rest);
            if(command.length() == 0) break; /* eof - bad */
            if(command == "end") break;
          };
        };
      };
    }
    else {
      logger.msg(Arc::WARNING, "unsupported configuration command: %s", command);
    };
  };
  return 0;
}

int FileRoot::config(gridftpd::ConfigSections &cf,std::string &pluginpath) {
  typedef enum {
    conf_state_single,
    conf_state_group,
    conf_state_plugin
  } config_state_t;
  config_state_t st = conf_state_single;
  bool right_group = true;
  std::string group_name; // =config_next_arg(rest);
  int group_decision = AAA_NO_MATCH;
  std::string plugin_config;
  std::string plugin_name;
  std::string plugin_path;
  for(;;) {
    std::string rest;
    std::string command;
    cf.ReadNext(command,rest);
    if(!right_group) {
      if(!cf.SectionNew()) continue;
    };
    int r = config_vo(user.user,cf,command,rest);
    if(r==0) continue;
    if(cf.SectionNew()) {
      if(right_group) switch(st) {
        case conf_state_group: {
          if(group_name.length() == 0) {
            logger.msg(Arc::ERROR, "unnamed group");
            return 1;
          };
          if(group_decision == AAA_POSITIVE_MATCH) {
            user.user.add_group(group_name);
          };
        }; break;
        case conf_state_plugin: {
          if(plugin_name.length() == 0) {
            logger.msg(Arc::WARNING, "undefined plugin");
            break;
          };
          if(plugin_path.length() == 0) {
            logger.msg(Arc::WARNING, "undefined virtual plugin path");
            break;
          };
          plugin_path=subst_user_spec(plugin_path,&user);
          if(gridftpd::canonical_dir(plugin_path,false) != 0) {
            logger.msg(Arc::WARNING, "bad directory for plugin: %s", plugin_path);
            break;
          };
          /* look if path is not already registered */
          bool already_have_this_path=false;
          for(std::list<FileNode>::iterator i=nodes.begin();i!=nodes.end();++i) {
            if((*i) == plugin_path) {
              already_have_this_path=true;
              break;
            };
          };
          if(already_have_this_path) {
            logger.msg(Arc::WARNING, "Already have directory: %s", plugin_path);
            break;
          };
          logger.msg(Arc::INFO, "Registering directory: %s with plugin: %s", plugin_path, plugin_name);
          plugin_name=pluginpath+'/'+plugin_name;
          plugin_config+="end\n";
#ifdef HAVE_SSTREAM
          std::stringstream fake_cfile(plugin_config);
#else
          std::strstream fake_cfile;
          fake_cfile<<plugin_config;
#endif
          FileNode node((char*)(plugin_path.c_str()),(char*)(plugin_name.c_str)(),fake_cfile,user);
          if(node.has_plugin()) { nodes.push_back(node); }
          else {
            logger.msg(Arc::ERROR, "file node creation failed: %s", plugin_path);
          };
        }; break;
        default: {
          break;
        }
      };
      // prepare for new section
      plugin_config="";
      plugin_name="";
      plugin_path="";
      group_name="";
      right_group = true;
      group_decision = AAA_NO_MATCH;
      // 0 - common, 1 - group, 2 - gridftpd, 3 - vo
      if(cf.SectionNum() == 1) {
        st=conf_state_group;
        group_name=cf.SubSection();
      } else if(cf.SubSection()[0] == 0) {
        st=conf_state_single;
      } else {
        st=conf_state_plugin;
      };
    };
    if((command.length() == 0) && (rest.length() == 0)) break; /* EOF */
    switch(st) {
      case conf_state_single: {
        if(gridftpd::Daemon::skip_config(command) == 0) {
        } else if(command == "include") {  /* include content of another file */
          std::string name=gridftpd::config_next_arg(rest);
          std::ifstream cfile_;
          cfile_.open(name.c_str(),std::ifstream::in);
          if(!cfile_.is_open()) {
            logger.msg(Arc::ERROR, "couldn't open file %s", name);
            return 1;
          };
          config(cfile_,pluginpath); // included are in old format
          cfile_.close();
        } else if(command == "encryption") {  /* is encryption allowed ? */
          std::string value=gridftpd::config_next_arg(rest);
          if(value == "yes") {
            heavy_encryption=true;
          } else if(value == "no") {
            heavy_encryption=false;
          } else {
            user.user.clear_groups();
            nodes.clear();
            logger.msg(Arc::ERROR, "improper attribute for encryption command: %s", value);
            return 1;
          };
        } else if(command == "allowunknown") {
          /* should user be present in grid-mapfile ? */
          std::string value=gridftpd::config_next_arg(rest);
          if(value == "yes") {
            user.gridmap=true;
          } else if(value == "no") {
            if(!user.gridmap) {
              logger.msg(Arc::ERROR, "unknown (non-gridmap) user is not allowed");
              return 1;
            };
          } else {
            user.user.clear_groups();
            nodes.clear();
            logger.msg(Arc::ERROR, "improper attribute for encryption command: %s", value);
            return 1;
          };
        } else if(command == "pluginpath") {
          if(!right_group) continue;
          pluginpath=gridftpd::config_next_arg(rest);
          if(pluginpath.length() == 0) pluginpath="/";
        } else if(command == "port") {
        } else if(command == "unixmap") {  /* map to local unix user */
          if(!user.mapped()) user.mapname(rest.c_str());
        } else if(command == "unixgroup") {  /* map to local unix user */
          if(!user.mapped()) user.mapgroup(rest.c_str());
        } else if(command == "unixvo") {  /* map to local unix user */
          if(!user.mapped()) user.mapvo(rest.c_str());
        };
      }; break;
      case conf_state_group: {
        /* definition of group of users based on DN */
        if(command == "name") {
          group_name=rest;
        } else if(command == "groupcfg") {
          if(rest.find_first_not_of(" \t") == std::string::npos) {
            right_group=true;
          } else {
            right_group=false;
            for(;;) {
              std::string group_name=gridftpd::config_next_arg(rest);
              if(group_name.length() == 0) break;
              right_group=user.user.check_group(group_name);
              if(right_group) break;
            };
          };
        } else {
          if(group_decision == AAA_NO_MATCH) {
            rest=command+" "+rest;
            group_decision = user.user.evaluate(rest.c_str());
            if(group_decision == AAA_FAILURE) group_decision=AAA_NO_MATCH;
          };
        };
      }; break;
      case conf_state_plugin: {
        if(command == "groupcfg") {
          if(rest.find_first_not_of(" \t") == std::string::npos) {
            right_group=true;
          } else {
            right_group=false;
            for(;;) {
              std::string group_name=gridftpd::config_next_arg(rest);
              if(group_name.length() == 0) break;
              right_group=user.user.check_group(group_name);
              if(right_group) break;
            };
          };
        } else if(command == "plugin") {
          plugin_name=rest;
        } else if(command == "path") {
          plugin_path=rest;
        } else {
          plugin_config+=command;
          plugin_config+=" ";
          plugin_config+=rest;
          plugin_config+="\n";
        };
      }; break;
    };
  };
  return 0;
}

int FileRoot::config(globus_ftp_control_auth_info_t *auth,
                     globus_ftp_control_handle_t *handle) {
  /* open and read configuration file */
  std::ifstream cfile;
  gridftpd::ConfigSections* cf = NULL;
  config_open_gridftp(cfile);
  if(!cfile.is_open()) {
    logger.msg(Arc::ERROR, "configuration file not found");
    delete cf;
    return 1;
  };
  cf=new gridftpd::ConfigSections(cfile);
  cf->AddSection("common");
  cf->AddSection("group");
  cf->AddSection("gridftpd");
  cf->AddSection("vo");
  /* keep information about user */
  if(!user.fill(auth,handle)) {
    delete cf;
    return 1;
  };
  gridftpd::GMEnvironment env;
  if(!env)
    return 1;

  std::string pluginpath=env.nordugrid_lib_loc();
  if(pluginpath.length() == 0) pluginpath="/";
  int r;
  r = config(*cf,pluginpath);
  gridftpd::config_close(cfile);
  if(cf) delete cf;
  if(r != 0) return r;
  if(!user.gridmap) {
    logger.msg(Arc::ERROR, "unknown (non-gridmap) user is not allowed");
    return 1;
  };
  /* must be sorted to make sure we can always find right directory */
  nodes.sort(FileNode::compare);
  /* create dummy directories */
  int nn=nodes.size();
  for(std::list<FileNode>::iterator i=nodes.begin();i!=nodes.end();++i) {
    if(nn==0) break;
    std::string name = i->point;
    for(;remove_last_name(name);) {
      if(name.length() == 0) break;
      bool found = false;
      std::list<FileNode>::iterator ii=i;
      for(;;) {
        ++ii; if(ii == nodes.end()) break;
        if(name == ii->point) { found=true; break; };
      };
      if(!found) { /* add dummy dir */
        logger.msg(Arc::ERROR, "Registering dummy directory: %s", name);
        nodes.push_back(FileNode(name.c_str()));
      }
      else {
        break;
      };
    };
    nn--;
  };
  opened_node=nodes.end();
  user.free();
  return 0;
}

