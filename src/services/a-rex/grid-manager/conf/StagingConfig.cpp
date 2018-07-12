#include <arc/StringConv.h>

#include <arc/ArcConfig.h>
#include <arc/ArcConfigIni.h>

#include "StagingConfig.h"

namespace ARex {

Arc::Logger StagingConfig::logger(Arc::Logger::getRootLogger(), "StagingConfig");


StagingConfig::StagingConfig(const GMConfig& config):
  max_delivery(10),
  max_processor(10),
  max_emergency(1),
  max_prepared(200),
  min_speed(0),
  min_speed_time(300),
  min_average_speed(0),
  max_inactivity_time(300),
  max_retries(10),
  passive(true),
  secure(false),
  httpgetpartial(false),
  remote_size_limit(0),
  use_host_cert_for_remote_delivery(false),
  log_level(Arc::Logger::getRootLogger().getThreshold()),
  dtr_log(config.ControlDir()+"/dtr.state"),
  valid(true)
{
  perf_log.SetOutput("/var/log/arc/perfdata/data.perflog");

  Arc::ConfigFile cfile;
  if (!cfile.open(config.ConfigFile())) {
    logger.msg(Arc::ERROR, "Can't read configuration file");
    valid = false;
    return;
  }
  // check type of file
  if (cfile.detect() != Arc::ConfigFile::file_INI) {
    logger.msg(Arc::ERROR, "Can't recognize type of configuration file");
    valid = false;
    cfile.close();
    return;
  }
  if (!readStagingConf(cfile)) {
    logger.msg(Arc::ERROR, "Configuration error");
    valid = false;
  }
  cfile.close();
}

bool StagingConfig::readStagingConf(Arc::ConfigFile& cfile) {

  Arc::ConfigIni cf(cfile);
  static const int common_perflog_secnum = 0;
  cf.AddSection("common/perflog");
  static const int data_staging_secnum = 1;
  cf.AddSection("arex/data-staging");
  for(;;) {
    std::string rest;
    std::string command;
    cf.ReadNext(command,rest);
    if (command.empty()) break; // eof

    if (cf.SectionNum() == common_perflog_secnum) { // common/perflog
      if (cf.SubSection()[0] == '\0') {
        perf_log.SetEnabled(true);
        if (command == "perflogdir") {
          perf_log.SetOutput(rest + "/data.perflog");
        }
      }
      continue;
    }

    if (command == "maxdelivery") {
      if (!paramToInt(Arc::ConfigIni::NextArg(rest), max_delivery)) {
        logger.msg(Arc::ERROR, "Bad number in maxdelivery");
        return false;
      }
    }
    else if (command == "maxemergency") {
      if (!paramToInt(Arc::ConfigIni::NextArg(rest), max_emergency)) {
        logger.msg(Arc::ERROR, "Bad number in maxemergency");
        return false;
      }
    }
    else if (command == "maxprocessor") {
      if (!paramToInt(Arc::ConfigIni::NextArg(rest), max_processor)) {
        logger.msg(Arc::ERROR, "Bad number in maxprocessor");
        return false;
      }
    }
    else if (command == "maxprepared") {
      if (!paramToInt(Arc::ConfigIni::NextArg(rest), max_prepared) || max_prepared <= 0) {
        logger.msg(Arc::ERROR, "Bad number in maxprepared");
        return false;
      }
    }
    else if (command == "maxtransfertries") {
      if (!paramToInt(Arc::ConfigIni::NextArg(rest), max_retries)) {
        logger.msg(Arc::ERROR, "Bad number in maxtransfertries");
        return false;
      }
    }
    else if (command == "speedcontrol") {
      if (!Arc::stringto(Arc::ConfigIni::NextArg(rest), min_speed)) {
        min_speed = min_speed_time = min_average_speed = max_inactivity_time = 0;
      } else if (!Arc::stringto(Arc::ConfigIni::NextArg(rest), min_speed_time) ||
                 !Arc::stringto(Arc::ConfigIni::NextArg(rest), min_average_speed) ||
                 !Arc::stringto(Arc::ConfigIni::NextArg(rest), max_inactivity_time)) {
        logger.msg(Arc::ERROR, "Bad number in speedcontrol");
        return false;
      }
    }
    else if (command == "sharepolicy") {
      share_type = Arc::ConfigIni::NextArg(rest);
    }
    else if (command == "sharepriority") {
      std::string share = Arc::ConfigIni::NextArg(rest);
      int priority = 0;
      if (!paramToInt(Arc::ConfigIni::NextArg(rest), priority) || priority <= 0) {
        logger.msg(Arc::ERROR, "Bad number in definedshare %s", share);
        return false;
      }
      defined_shares[share] = priority;
    }
    else if (command == "deliveryservice") {
      std::string url = rest;
      Arc::URL u(url);
      if (!u) {
        logger.msg(Arc::ERROR, "Bad URL in deliveryservice: %s", url);
        return false;
      }
      delivery_services.push_back(u);
    }
    else if (command == "localdelivery") {
      std::string use_local = Arc::ConfigIni::NextArg(rest);
      if (use_local == "yes") delivery_services.push_back(Arc::URL("file:/local"));
    }
    else if (command == "remotesizelimit") {
      if (!Arc::stringto(Arc::ConfigIni::NextArg(rest), remote_size_limit)) {
        logger.msg(Arc::ERROR, "Bad number in remotesizelimit");
        return false;
      }
    }
    else if (command == "passivetransfer") {
      std::string pasv = Arc::ConfigIni::NextArg(rest);
      if (pasv == "yes") passive = true;
    }
    else if (command == "securetransfer") {
      std::string sec = Arc::ConfigIni::NextArg(rest);
      if (sec == "yes") secure = true;
    }
    else if (command == "httpgetpartial") {
      std::string partial = Arc::ConfigIni::NextArg(rest);
      if (partial == "no") httpgetpartial = false;
    }
    else if (command == "preferredpattern") {
      preferred_pattern = rest;
    }
    else if (command == "usehostcert") {
      std::string use_host_cert = Arc::ConfigIni::NextArg(rest);
      if (use_host_cert == "yes") use_host_cert_for_remote_delivery = true;
    }
    else if (command == "loglevel") {
      unsigned int level;
      if (!Arc::strtoint(Arc::ConfigIni::NextArg(rest), level)) {
        logger.msg(Arc::ERROR, "Bad value for loglevel");
        return false;
      }
      log_level = Arc::old_level_to_level(level);
    }
    else if (command == "statefile") {
      dtr_log = rest;
    }
    else if (command == "central_logfile") {
      dtr_central_log = rest;
    }
    else if (command == "use_remote_acix")  {
      std::string endpoint(rest);
      if (!Arc::URL(endpoint) || endpoint.find("://") == std::string::npos) {
        logger.msg(Arc::ERROR, "Bad URL in acix_endpoint");
        return false;
      }
      endpoint.replace(0, endpoint.find("://"), "acix");
      acix_endpoint = endpoint;
    }
  }
  return true;
}


bool StagingConfig::paramToInt(const std::string& param, int& value) {

  int i;
  if (!Arc::stringto(param, i)) return false;
  if (i < 0) i=-1;
  value = i;
  return true;
}

} // namespace ARex
