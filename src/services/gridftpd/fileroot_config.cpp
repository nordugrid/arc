#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef HAVE_SSTREAM
#include <sstream>
#else
#include <strstream>
#endif

#include <arc/ArcLocation.h>
#include <arc/FileUtils.h>
#include <arc/ArcConfigIni.h>

#include "conf.h"
#include "names.h"
#include "misc.h"
#include "auth/auth.h"
#include "conf/conf_vo.h"
#include "conf/environment.h"

#include "fileroot.h"

static Arc::Logger logger(Arc::Logger::getRootLogger(),"FileRoot");

int FileRoot::config(gridftpd::Daemon &daemon,ServerParams* params) {
  /* open and read configuration file */
  Arc::ConfigFile cfile;
  Arc::ConfigIni* cf = NULL;
  config_open_gridftp(cfile);
  if(!cfile.is_open()) {
    logger.msg(Arc::ERROR, "configuration file not found");
    return 1;
  };
  cf=new Arc::ConfigIni(cfile);
  cf->AddSection("common");    // 0 
  cf->AddSection("gridftpd");  // 1
  cf->SetSectionIndicator(".");
  for(;;) {
    std::string rest;
    std::string command;
    cf->ReadNext(command,rest);
    if(command.length() == 0) break; /* EOF */
    int r;
    if(daemon.config(cf->Section(),command,rest) == -1) { cfile.close(); delete cf; return 1; };

    if(cf->SubSection()[0] != 0) continue;

    if(cf->SectionNum() == 0) { // [common]

    } else if(cf->SectionNum() == 1) { // [gridftpd]
      if(command == "port") {
        if(params) {
          if(sscanf(rest.c_str(),"%u",&(params->port)) != 1) {
            logger.msg(Arc::ERROR, "Wrong port number in configuration");
            cfile.close();
            delete cf;
            return 1;
          };
        };
      } else if(command == "maxconnections") {
        if(params) {
          if(sscanf(rest.c_str(),"%u",&(params->max_connections)) != 1) {
            logger.msg(Arc::ERROR, "Wrong maxconnections number in configuration");
            cfile.close();
            delete cf;
            return 1;
          };
        };
      } else if(command == "defaultbuffer") {
        if(params) {
          if(sscanf(rest.c_str(),"%u",&(params->default_buffer)) != 1) {
            logger.msg(Arc::ERROR, "Wrong defaultbuffer number in configuration");
            cfile.close();
            delete cf;
            return 1;
          };
        };
      } else if(command == "maxbuffer") {
        if(params) {
          if(sscanf(rest.c_str(),"%u",&(params->max_buffer)) != 1) {
            logger.msg(Arc::ERROR, "Wrong maxbuffer number in configuration");
            cfile.close();
            delete cf;
            return 1;
          };
        };
      } else if(command == "firewall") {
        if(params) {
          std::string value=Arc::ConfigIni::NextArg(rest);
          int    errcode;
          struct hostent* host;
          struct hostent  hostbuf;
    #ifdef _MACOSX
          char   buf[BUFSIZ];
          if((host=gethostbyname2(value.c_str(),AF_INET)) == NULL) { //TODO: Deal with IPv6
    #else
    #ifndef _AIX
    #ifndef sun
          char   buf[BUFSIZ];
          if(gethostbyname_r(value.c_str(),
                  &hostbuf,buf,sizeof(buf),&host,&errcode)) {
    #else
          char   buf[BUFSIZ];
          if((host=gethostbyname_r(value.c_str(),
                  &hostbuf,buf,sizeof(buf),&errcode)) == NULL) {
    #endif
    #else
          struct hostent_data buf[BUFSIZ];
          if((errcode=gethostbyname_r(value.c_str(),
                  (host=&hostbuf),buf))) {
    #endif
    #endif
            logger.msg(Arc::ERROR, "Can't resolve host %s", value);
            cfile.close();
            delete cf;
            return 1;
          };
          if( (host == NULL) ||
              (host->h_length < sizeof(struct in_addr)) ||
              (host->h_addr_list[0] == NULL) ) {
            logger.msg(Arc::ERROR, "Can't resolve host %s", value);
            cfile.close();
            delete cf;
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
  };
  cfile.close();
  delete cf;
  return 0;
}


static int const cfgsec_common_mapping_n = 0;
static int const cfgsec_common_n = 1;
static int const cfgsec_group_n = 2;
static int const cfgsec_gridftpd_n = 3;
static int const cfgsec_vo_n = 4;

// Second level configuration method used when forking for new connection.
int FileRoot::config(Arc::ConfigIni &cf,std::string &pluginpath) {
  typedef enum {
    conf_state_none,
    conf_state_single,
    conf_state_group,  // inside [group] configuration
    conf_state_plugin  // inside plugin configuration
  } config_state_t;
  config_state_t st = conf_state_none;
  bool right_group = true;
  user.user.select_group(NULL);
  std::string group_name; // =config_next_arg(rest);
  int group_decision = AAA_NO_MATCH;
  std::string plugin_config;
  std::string plugin_name;
  std::string plugin_path;
  // for telling plugin its own endpoint
  // hostname can be overridden in configuration
  std::string hostname;
  char hostn[256];
  if (gethostname(hostn, sizeof(hostn)) != 0) logger.msg(Arc::WARNING, "Could not determine hostname from gethostname()");
  else hostname = hostn;
  std::string port = "2811"; // Hard-coded, but it is standard default
  for(;;) {
    std::string rest;
    std::string command;
    cf.ReadNext(command,rest);
    if(!right_group) { // skip configuration for wrong auth group
      if(!cf.SectionNew()) continue;
    };
    int r = gridftpd::config_vo(user.user,cf,command,rest,&logger); // [userlist] processing
    if(r==0) continue; // processed
    if(cf.SectionNew()) {
      if(right_group) switch(st) {
        case conf_state_group: { // authgroup processing ended
          if(group_name.length() == 0) {
            logger.msg(Arc::ERROR, "unnamed group");
            return 1;
          };
          if(group_decision == AAA_POSITIVE_MATCH) {
            user.user.add_group(group_name);
          };
        }; break;
        case conf_state_plugin: { // plugin processing ended
          if(plugin_name.length() == 0) {
            logger.msg(Arc::WARNING, "undefined plugin name");
            break;
          };
          if(plugin_path.length() == 0) {
            logger.msg(Arc::WARNING, "undefined virtual plugin path");
            break;
          };
          plugin_path=subst_user_spec(plugin_path,&user);
          if(!Arc::CanonicalDir(plugin_path,false)) {
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
          plugin_config+="endpoint gsiftp://"+hostname+":"+port+"/"+plugin_path+"\n";
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
      user.user.select_group(NULL);
      group_decision = AAA_NO_MATCH;
      if(cf.SubSection()[0] == 0) { // no subsection
        if(cf.SectionNum() == cfgsec_group_n) { // auth group definition
          st=conf_state_group;
          group_name=cf.SectionIdentifier();
        } else if(cf.SubSection()[0] == 0) { // no subsection
          st=conf_state_single;
        };
      } else if(cf.SectionNum() == cfgsec_gridftpd_n) { // subsection of gridftpd
        st=conf_state_plugin;
        plugin_name=cf.SubSection();
        plugin_path=cf.SubSection();
        // TODO: support for windows
        plugin_name+="plugin.so"; // library name is postfixed with plugin.so
      } else {
        st=conf_state_none;
      };
    };
    // processing command
    if((command.length() == 0) && (rest.length() == 0)) break; /* EOF */
    switch(st) {
      case conf_state_none: // not our sub-section
        break;
      case conf_state_single: { // ordinary section
        if((command == ".") && (rest.empty())) {
          // separator
        } else if(cf.SectionNum() == cfgsec_gridftpd_n) { // [gridftpd]
          if(command == "encryption") {  /* is encryption allowed ? */
            std::string value=Arc::ConfigIni::NextArg(rest);
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
            std::string value=Arc::ConfigIni::NextArg(rest);
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
              logger.msg(Arc::ERROR, "improper attribute for allowunknown command: %s", value);
              return 1;
            };
          } else if(command == "port") {
            port=Arc::ConfigIni::NextArg(rest);
          } else if(command == "allowactivedata") {
            std::string value=Arc::ConfigIni::NextArg(rest);
            if(value == "yes") {
              active_data=true;
            } else if(value == "no") {
              active_data=false;
            } else {
              user.user.clear_groups();
              nodes.clear();
              logger.msg(Arc::ERROR, "improper attribute for allowactvedata command: %s", value);
              return 1;
            };
          };
        } else if(cf.SectionNum() == cfgsec_common_n) { // [common]
          if(command == "hostname") { // should be in [common]
            hostname=Arc::ConfigIni::NextArg(rest);
          };
        } else if(cf.SectionNum() == cfgsec_common_mapping_n) {
          if(command == "unixmap") {  /* map to local unix user */
            if(!user.mapped()) {
              if(user.mapname(rest.c_str()) == AAA_FAILURE) {
                user.user.clear_groups(); nodes.clear();
                logger.msg(Arc::ERROR, "failed while processing configuration command: %s %s", command, rest);
                return 1;
              };
            };
          } else if(command == "unixgroupmap") {  /* map to local unix user */
            if(!user.mapped()) {
              if(user.mapgroup(rest.c_str()) == AAA_FAILURE) {
                user.user.clear_groups(); nodes.clear();
                logger.msg(Arc::ERROR, "failed while processing configuration command: %s %s", command, rest);
                return 1;
              };
            };
          } else if(command == "unixlistmap") {  /* map to local unix user */
            if(!user.mapped()) {
              if(user.mapvo(rest.c_str()) == AAA_FAILURE) {
                user.user.clear_groups(); nodes.clear();
                logger.msg(Arc::ERROR, "failed while processing configuration command: %s %s", command, rest);
                return 1;
              };
            };
          };
        };
      }; break;
      case conf_state_group: {
        /* definition of authorization group */
        if((command == ".") && (rest.empty())) {
          // separator
        } else if(command == "name") {
          group_name=rest;
        } else {
          if(group_decision == AAA_NO_MATCH) { // not decided yet
            rest=command+" "+rest;
            group_decision = user.user.evaluate(rest.c_str());
            if(group_decision == AAA_FAILURE) {
              //group_decision=AAA_NO_MATCH;
              logger.msg(Arc::ERROR, "Failed processing authorization group %s",group_name);
              return 1;
            };
          };
        };
      }; break;
      case conf_state_plugin: {
        // plugin configuration processing
        if((command == ".") && (rest.empty())) {
          // separator
        } else if(command == "allowaccess") {
          user.user.select_group(NULL);
          if(rest.find_first_not_of(" \t") == std::string::npos) {
            logger.msg(Arc::ERROR, "Missing authgroup name in allowaccess");
            return 1;
          } else {
            right_group=false; // switch default to not allowed
            for(;;) {
              std::string group_name=Arc::ConfigIni::NextArg(rest);
              if(group_name.length() == 0) break;
              right_group=user.user.select_group(group_name);
              if(right_group) {
                break;
              };
            };
          };
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

// Main configuration method applied after forking for new connection.
int FileRoot::config(globus_ftp_control_auth_info_t *auth,
                     globus_ftp_control_handle_t *handle) {
  gridftpd::GMEnvironment env;
  if(!env) {
    logger.msg(Arc::ERROR, "failed to initialize environment variables");
    return 1;
  };
  
  /* open and read configuration file */
  Arc::ConfigFile cfile;
  Arc::ConfigIni* cf = NULL;
  std::string config_file = config_open_gridftp(cfile);
  if(!cfile.is_open()) {
    logger.msg(Arc::ERROR, "configuration file not found");
    delete cf;
    return 1;
  };
  cf=new Arc::ConfigIni(cfile);
  // keep in sync with cfgsec_* constants
  cf->AddSection("common/mapping");
  cf->AddSection("common");
  cf->AddSection("authgroup");
  cf->AddSection("gridftpd");
  cf->AddSection("userlist");
  cf->SetSectionIndicator(".");
  /* keep information about user */
  if(!user.fill(auth,handle,config_file.c_str())) {
    logger.msg(Arc::ERROR, "failed to process client identification");
    delete cf;
    return 1;
  };
  std::string pluginpath;
  std::list<std::string> pluginpaths = Arc::ArcLocation::GetPlugins();
  if(pluginpaths.empty()) {
    logger.msg(Arc::ERROR, "failed to identify plugins path");
    delete cf;
    return 1;
  };
  pluginpath=pluginpaths.front();
  int r;
  r = config(*cf,pluginpath);
  cfile.close();
  delete cf;
  cf = NULL;
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

