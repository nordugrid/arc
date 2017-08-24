#include <arc/StringConv.h>

#include <arc/ArcConfig.h>
#include <arc/ArcConfigIni.h>

#include "StagingConfig.h"

namespace ARex {

Arc::Logger StagingConfig::logger(Arc::Logger::getRootLogger(), "StagingConfig");

static bool elementtoboollogged(Arc::XMLNode pnode,const char* ename,bool& val,Arc::Logger& logger) {
  if(Arc::Config::elementtobool(pnode, ename, val)) return true;
  logger.msg(Arc::ERROR,"wrong boolean in %s",ename);
  return false;
}

template<typename T> static bool elementtointlogged(Arc::XMLNode pnode,const char* ename,T& val,Arc::Logger& logger) {
  if(Arc::Config::elementtoint(pnode, ename, val)) return true;
  logger.msg(Arc::ERROR,"wrong number in %s",ename);
  return false;
}

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
  passive(false),
  secure(false),
  httpgetpartial(true),
  remote_size_limit(0),
  use_host_cert_for_remote_delivery(false),
  log_level(Arc::Logger::getRootLogger().getThreshold()),
  valid(true)
{
  perf_log.SetOutput("/var/log/arc/perfdata/data.perflog");

  // For ini-style, use [data-staging] section, for xml use <dataTransfer> node

  Arc::ConfigFile cfile;
  if (!cfile.open(config.ConfigFile())) {
    logger.msg(Arc::ERROR, "Can't read configuration file");
    valid = false;
    return;
  }
  // detect type of file
  switch(cfile.detect()) {
    case Arc::ConfigFile::file_XML: {
      Arc::XMLNode cfg;
      if (!cfg.ReadFromStream(cfile)) {
        logger.msg(Arc::ERROR, "Can't interpret configuration file as XML");
        valid = false;
      }
      else if (!readStagingConf(cfg)) {
        logger.msg(Arc::ERROR, "Configuration error");
        valid = false;
      }
    } break;

    case Arc::ConfigFile::file_INI: {
      if (!readStagingConf(cfile)) {
        logger.msg(Arc::ERROR, "Configuration error");
        valid = false;
      }
    } break;

    default: {
      logger.msg(Arc::ERROR, "Can't recognize type of configuration file");
      valid = false;
    } break;
  }
  cfile.close();
}

bool StagingConfig::readStagingConf(Arc::ConfigFile& cfile) {

  Arc::ConfigIni cf(cfile);
  cf.AddSection("data-staging");
  cf.AddSection("common");
  for(;;) {
    std::string rest;
    std::string command;
    cf.ReadNext(command,rest);
    if (command.empty()) break; // eof

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
      if (!Arc::stringto(Arc::ConfigIni::NextArg(rest), min_speed) ||
          !Arc::stringto(Arc::ConfigIni::NextArg(rest), min_speed_time) ||
          !Arc::stringto(Arc::ConfigIni::NextArg(rest), min_average_speed) ||
          !Arc::stringto(Arc::ConfigIni::NextArg(rest), max_inactivity_time)) {
        logger.msg(Arc::ERROR, "Bad number in speedcontrol");
        return false;
      }
    }
    else if (command == "sharetype") {
      share_type = Arc::ConfigIni::NextArg(rest);
    }
    else if (command == "definedshare") {
      std::string share = Arc::ConfigIni::NextArg(rest);
      int priority = 0;
      if (!paramToInt(Arc::ConfigIni::NextArg(rest), priority) || priority <= 0) {
        logger.msg(Arc::ERROR, "Bad number in definedshare %s", share);
        return false;
      }
      defined_shares[share] = priority;
    }
    else if (command == "deliveryservice") {
      std::string url = Arc::ConfigIni::NextArg(rest);
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
      preferred_pattern = Arc::ConfigIni::NextArg(rest);
    }
    else if (command == "usehostcert") {
      std::string use_host_cert = Arc::ConfigIni::NextArg(rest);
      if (use_host_cert == "yes") use_host_cert_for_remote_delivery = true;
    }
    else if (command == "debug") {
      unsigned int level;
      if (!Arc::strtoint(Arc::ConfigIni::NextArg(rest), level)) {
        logger.msg(Arc::ERROR, "Bad value for debug");
        return false;
      }
      log_level = Arc::old_level_to_level(level);
    }
    else if (command == "dtrlog") {
      dtr_log = Arc::ConfigIni::NextArg(rest);
    }
    else if (command == "central_logfile") {
      dtr_central_log = Arc::ConfigIni::NextArg(rest);
    }
    else if (command == "acix_endpoint")  {
      std::string endpoint(Arc::ConfigIni::NextArg(rest));
      if (!Arc::URL(endpoint) || endpoint.find("://") == std::string::npos) {
        logger.msg(Arc::ERROR, "Bad URL in acix_endpoint");
        return false;
      }
      endpoint.replace(0, endpoint.find("://"), "acix");
      acix_endpoint = endpoint;
    }
    else if (command == "perflogdir") {
      perf_log.SetOutput(Arc::ConfigIni::NextArg(rest) + "/data.perflog");
    }
    else if (command == "enable_perflog_reporting") {
      std::string enableperflog = Arc::ConfigIni::NextArg(rest);
      if (enableperflog == "yes") perf_log.SetEnabled(true);
    }
  }
  return true;
}

bool StagingConfig::readStagingConf(const Arc::XMLNode& cfg) {
  /*
  DTR
    maxDelivery
    maxProcessor
    maxEmergency
    maxPrepared
    shareType
    definedShare
      name
      priority
    deliveryService
    localDelivery
    useHostCert
    logLevel
    dtrLog
    centralDTRLog
  */
  Arc::XMLNode tmp_node = cfg["dataTransfer"]["DTR"];
  if (tmp_node) {
    if (!elementtointlogged(tmp_node, "maxDelivery", max_delivery, logger)) return false;
    if (!elementtointlogged(tmp_node, "maxProcessor", max_processor, logger)) return false;
    if (!elementtointlogged(tmp_node, "maxEmergency", max_emergency, logger)) return false;
    if (!elementtointlogged(tmp_node, "maxPrepared", max_prepared, logger)) return false;
    if (tmp_node["shareType"]) share_type = (std::string)tmp_node["shareType"];
    Arc::XMLNode defined_share = tmp_node["definedShare"];
    for(;defined_share;++defined_share) {
      std::string share_name = defined_share["name"];
      int share_priority = -1;
      if (elementtointlogged(defined_share, "priority", share_priority, logger) &&
          (share_priority > 0) && !share_name.empty()) {
        defined_shares[share_name] = share_priority;
      }
    }
    Arc::XMLNode delivery_service = tmp_node["deliveryService"];
    for(;delivery_service;++delivery_service) {
      Arc::URL u((std::string)delivery_service);
      if (!u) {
        logger.msg(Arc::ERROR, "Bad URL in deliveryService: %s", std::string(delivery_service));
        return false;
      }
      delivery_services.push_back(u);
    }
    bool use_local_delivery = false;
    if (!elementtoboollogged(tmp_node,"localDelivery",use_local_delivery,logger)) return false;
    if (use_local_delivery) delivery_services.push_back(Arc::URL("file:/local"));
    if (tmp_node["remoteSizeLimit"]) {
      if (!Arc::stringto((std::string)tmp_node["remoteSizeLimit"], remote_size_limit)) return false;
    }
    if (!elementtoboollogged(tmp_node, "localDelivery", use_host_cert_for_remote_delivery, logger)) return false;
    if (tmp_node["logLevel"]) {
      if (!Arc::istring_to_level((std::string)tmp_node["logLevel"], log_level)) {
        logger.msg(Arc::ERROR, "Bad value for logLevel");
        return false;
      }
    }
    if (tmp_node["dtrLog"]) dtr_log = (std::string)tmp_node["dtrLog"];
    if (tmp_node["centralDTRLog"]) dtr_central_log = (std::string)tmp_node["centraDTRLog"];
  }
  /*
  dataTransfer
    secureTransfer
    passiveTransfer
    localTransfer
    httpGetPartial
    preferredPattern
    acixEndpoint
    timeouts
      minSpeed
      minSpeedTime
      minAverageSpeed
      maxInactivityTime
    maxRetries
    mapURL (link)
      from
      to
  */
  tmp_node = cfg["dataTransfer"];
  if (tmp_node) {
    Arc::XMLNode to_node = tmp_node["timeouts"];
    if (to_node) {
      if (!elementtointlogged(tmp_node, "minSpeed", min_speed, logger)) return false;
      if (!elementtointlogged(tmp_node, "minAverageSpeed", min_average_speed, logger)) return false;
      if (!elementtointlogged(tmp_node, "minSpeedTime", min_speed_time, logger)) return false;
      if (!elementtointlogged(tmp_node, "maxInactivityTime", max_inactivity_time, logger)) return false;
    }
    if (!elementtoboollogged(tmp_node, "passiveTransfer", passive, logger)) return false;
    if (!elementtoboollogged(tmp_node, "secureTransfer", secure, logger)) return false;
    if (!elementtoboollogged(tmp_node, "httpGetPartial", httpgetpartial, logger)) return false;
    if (!elementtointlogged(tmp_node, "maxRetries", max_retries, logger)) return false;
    if (tmp_node["preferredPattern"]) preferred_pattern = (std::string)(tmp_node["preferredPattern"]);
    if (tmp_node["acixEndpoint"]) {
      std::string endpoint((std::string)(tmp_node["acixEndPoint"]));
      if (!Arc::URL(endpoint) || endpoint.find("://") == std::string::npos) {
        logger.msg(Arc::ERROR, "Bad URL in acix_endpoint"); return false;
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
