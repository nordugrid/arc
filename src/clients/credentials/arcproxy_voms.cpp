// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <arc/ArcLocation.h>
#include <arc/OptionParser.h>
#include <arc/StringConv.h>
#include <arc/Utils.h>
#include <arc/UserConfig.h>
#include <arc/FileUtils.h>
#include <arc/communication/ClientInterface.h>
#include <arc/credentialstore/ClientVOMS.h>
#include <arc/credentialstore/ClientVOMSRESTful.h>
#include <arc/credential/VOMSConfig.h>
#include <arc/credential/VOMSUtil.h>
#include <arc/credential/Credential.h>
#include <arc/credentialstore/CredentialStore.h>
#include <arc/crypto/OpenSSL.h>

bool contact_voms_servers(std::map<std::string,std::list<std::string> >& vomscmdlist,
      std::list<std::string>& orderlist, std::string& vomses_path,
      bool use_gsi_comm, bool use_http_comm, const std::string& voms_period, 
      Arc::UserConfig& usercfg, Arc::Logger& logger, const std::string& tmp_proxy_path, std::string& vomsacseq) {

  std::string ca_dir;
  ca_dir = usercfg.CACertificatesDirectory();

  class voms_match: public Arc::VOMSConfig::filter {
   private:
    const std::string& voms_;
   public:
    virtual bool match(const Arc::VOMSConfigLine& line) const {
      return ((line.Name() == voms_) || (line.Alias() == voms_));
    };
    voms_match(const std::string& voms):voms_(voms) { }; 
  };

  class vomses_match: public Arc::VOMSConfig::filter {
   private:
    const std::map<std::string,std::list<std::string> >& vomses_;
   public:
    virtual bool match(const Arc::VOMSConfigLine& line) const {
      // TODO: this will not scale for many voms servers specified at command line
      for(std::map<std::string,std::list<std::string> >::const_iterator voms = vomses_.begin();
                     voms != vomses_.end(); ++voms) {
        if((line.Name() == voms->first) || (line.Alias() == voms->first)) return true;
      };
      return false;
    };
    vomses_match(const std::map<std::string,std::list<std::string> >& vomses):vomses_(vomses) { }; 
  };

  Arc::VOMSConfig voms_config(vomses_path, vomses_match(vomscmdlist));
  if(!voms_config) {
    // logger
    logger.msg(Arc::ERROR, "Failed to process VOMS configuration or no suitable configuration lines found.");
    return false;
  }

  //Contact the voms servers to retrieve attribute certificate
  Arc::MCCConfig cfg;
  cfg.AddProxy(tmp_proxy_path);
  cfg.AddCADir(ca_dir);
  Arc::Period lifetime;
  if(!voms_period.empty()) {
    time_t voms_period_sec;
    if(!Arc::stringto(voms_period,voms_period_sec)) {
      logger.msg(Arc::ERROR, "Failed to parse requested VOMS lifetime: %s", voms_period);
      return false;
    }
    lifetime = voms_period_sec;
  }

  // TODO: group commands by server. Is it really useful? Can it really be used effectively.
  // Loop through commands
  for (std::map<std::string,std::list<std::string> >::iterator vomscmd = vomscmdlist.begin();
                                vomscmd != vomscmdlist.end(); ++vomscmd) {
    const std::string& voms_server = vomscmd->first; // server name
    const std::list<std::string>& voms_commands = vomscmd->second; // command to send
    bool succeeded = false; 
    int voms_lines_num = 0;
    // Loop through suitable voms configuration lines
    for (Arc::VOMSConfig::iterator vomsline = voms_config.First(voms_match(voms_server));
                               (bool)vomsline; vomsline = vomsline.Next(voms_match(voms_server))) {
      if(vomsline->Host().empty()) {
        logger.msg(Arc::ERROR, "Cannot get VOMS server address information from vomses line: \"%s\"", vomsline->Str());
        throw std::runtime_error("Cannot get VOMS server address information from vomses line: \"" + vomsline->Str() + "\"");
      }
      ++voms_lines_num;
      logger.msg(Arc::INFO, "Contacting VOMS server (named %s): %s on port: %s",
                vomsline->Name(), vomsline->Host(), vomsline->Port());
      std::cout << Arc::IString("Contacting VOMS server (named %s): %s on port: %s",
                vomsline->Name(), vomsline->Host(), vomsline->Port()) << std::endl;
      unsigned int port_num = 0;
      if(!vomsline->Port().empty()) {
        if(!Arc::stringto(vomsline->Port(),port_num)) {
          // Failed to parse port
          logger.msg(Arc::ERROR, "Failed to parse requested VOMS server port number: %s", vomsline->Port());
          continue;
        }
      } else {
        port_num = 8443; // default VOMS port ? 
      }
      if(use_http_comm) {
        // RESTful interface
        std::list<std::string> fqans;
        for(std::list<std::string>::const_iterator c_it = voms_commands.begin(); c_it != voms_commands.end(); ++c_it) {
          if (c_it->empty()) {
            // ?? fqans.push_back("/"+voms_name);
          } else if (Arc::lower(*c_it) == "all") {
            // ?? fqans.push_back("/"+voms_name);
          } else if (Arc::lower(*c_it) == "list") {
            // N
            // not supported
            logger.msg(Arc::ERROR, "List functionality is not supported for RESTful VOMS interface");
            continue;
          } else {
            fqans.push_back(*c_it); // ??
          }
        }
        Arc::ClientVOMSRESTful client(cfg, vomsline->Host(), port_num, Arc::TLSSec, usercfg.Timeout()/* todo: , proxy_host, proxy_port*/);
        std::string response;
        Arc::MCC_Status status = client.process(fqans, lifetime, response);
        if (!status) {
          std::cout << Arc::IString("The VOMS server with the information:\n\t%s\ncan not be reached, please make sure it is available.", vomsline->Str()) << std::endl;
          std::cout << Arc::IString("Collected error is:\n\t%s",(std::string)status) << std::endl;
          continue; //There could be another voms replicated server with the same name exists
        }
        if (response.empty()) {
          logger.msg(Arc::ERROR, "No valid response from VOMS server: %s", vomsline->Name());
          std::cout << Arc::IString("Collected error is:\n\t%s",(std::string)status) << std::endl;
          continue;
        }
        vomsacseq.append(response);
        succeeded = true;
        break; // ??
      } else {
        // old interface
        std::list<Arc::VOMSCommand> commands;
        for(std::list<std::string>::const_iterator c_it = voms_commands.begin(); c_it != voms_commands.end(); ++c_it) {
          if (c_it->empty()) {
            commands.push_back(Arc::VOMSCommand().GetGroup("/"+vomsline->Name()));
          } else if (Arc::lower(*c_it) == "all") {
            commands.push_back(Arc::VOMSCommand().GetEverything());
          } else if (Arc::lower(*c_it) == "list") {
            // N
            // not supported
            logger.msg(Arc::ERROR, "List functionality is not supported for legacy VOMS interface");
            continue;
          } else {
            std::string::size_type pos = c_it->find("/Role=");
            if (pos == 0) {
              commands.push_back(Arc::VOMSCommand().GetRole(c_it->substr(pos+6)));
            } else if((pos != std::string::npos) && (pos > 0)) {
              commands.push_back(Arc::VOMSCommand().GetRoleInGroup(c_it->substr(0, pos),c_it->substr(pos + 6)));
            } else if((*c_it)[0] == '/') {
              commands.push_back(Arc::VOMSCommand().GetGroup(*c_it));
            } else {
              // unexpected
              logger.msg(Arc::ERROR, "Failed to parse VOMS command: %s",*c_it);
              continue;
            }
          }
        }
        std::list<std::pair<std::string,std::string> > ordering;
        for(std::list<std::string>::iterator o_it = orderlist.begin(); o_it != orderlist.end(); ++o_it) {
          std::string::size_type pos = o_it->find(':');
          if(pos == std::string::npos) {
            ordering.push_back(std::pair<std::string,std::string>(*o_it,""));
          } else {
            ordering.push_back(std::pair<std::string,std::string>(o_it->substr(0,pos),o_it->substr(pos+1)));
          }
        }
        //logger.msg(Arc::VERBOSE, "Try to get attribute from VOMS server with order: %s", ordering);
        //logger.msg(Arc::VERBOSE, "Message sent to VOMS server %s is: %s", voms_name, send_msg);

        Arc::ClientVOMS client(cfg, vomsline->Host(), port_num, use_gsi_comm ? Arc::GSISec : Arc::TLSSec, usercfg.Timeout());
        std::string response;
        Arc::MCC_Status status = client.process(commands, ordering, lifetime, response);
        if (!status) {
          std::cout << Arc::IString("The VOMS server with the information:\n\t%s\ncan not be reached, please make sure it is available.", vomsline->Str()) << std::endl;
          std::cout << Arc::IString("Collected error is:\n\t%s",(std::string)status) << std::endl;
          continue; //There could be another voms replicated server with the same name exists
        }
        if (response.empty()) {
          logger.msg(Arc::ERROR, "No valid response from VOMS server: %s", vomsline->Name());
          std::cout << Arc::IString("Collected error is:\n\t%s",(std::string)status) << std::endl;
          continue;
        }
        vomsacseq.append(response);
        succeeded = true;
        break;
      }
    } // voms lines
    if(succeeded == false) {
      if(voms_lines_num > 1) {
        std::cout << Arc::IString("There are %d servers with the same name: %s in your vomses file, but none of them can be reached, or can return a valid message.", voms_lines_num, voms_server) << std::endl;
      }
      return false;
    }
  } // voms servers
  return true;
}
  
