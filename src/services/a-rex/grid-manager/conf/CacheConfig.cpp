#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string>
#include <fstream>

#include <arc/StringConv.h>

#include "GMConfig.h"

#include "CacheConfig.h"

namespace ARex {

CacheConfig::CacheConfig(const GMConfig& config):
    _cache_max(100),
    _cache_min(100),
    _log_file("/var/log/arc/cache-clean.log"),
    _log_level("INFO") ,
    _lifetime("0"),
    _cache_shared(false),
    _clean_timeout(0) {
  // Load conf file
  Arc::ConfigFile cfile;
  if(!cfile.open(config.ConfigFile())) throw CacheConfigException("Can't open configuration file");
  
  /* detect type of file */
  switch(cfile.detect()) {
    case Arc::ConfigFile::file_XML: {
      Arc::XMLNode cfg;
      if(!cfg.ReadFromStream(cfile)) {
        cfile.close();
        throw CacheConfigException("Can't interpret configuration file as XML");
      };
      cfile.close();
      try {
        parseXMLConf(cfg);
      } catch (CacheConfigException& e) {
        cfile.close();
        throw;
      }
    }; break;
    case Arc::ConfigFile::file_INI: {
      Arc::ConfigIni cf(cfile);
      try {
        parseINIConf(cf);
      } catch (CacheConfigException& e) {
        cfile.close();
        throw;
      }
    }; break;
    default: {
      cfile.close();
      throw CacheConfigException("Can't recognize type of configuration file");
    }; break;
  };
  cfile.close();
}

CacheConfig::CacheConfig(const Arc::XMLNode& cfg):
    _cache_max(100),
    _cache_min(100),
    _log_file("/var/log/arc/cache-clean.log"),
    _log_level("INFO") ,
    _lifetime("0"),
    _cache_shared(false),
    _clean_timeout(0) {
  parseXMLConf(cfg);
}

void CacheConfig::parseINIConf(Arc::ConfigIni& cf) {
  
  cf.AddSection("common");
  cf.AddSection("grid-manager");
  
  for(;;) {
    std::string rest;
    std::string command;
    cf.ReadNext(command,rest);

    if(command.length() == 0) break; // EOF
    
    else if(command == "remotecachedir") {
      std::string cache_dir = Arc::ConfigIni::NextArg(rest);
      if(cache_dir.length() == 0) continue; // cache is disabled
      std::string cache_link_dir = Arc::ConfigIni::NextArg(rest);
       
      // take off leading slashes
      if (cache_dir.rfind("/") == cache_dir.length()-1) cache_dir = cache_dir.substr(0, cache_dir.length()-1);
      // add this cache to our list
      std::string cache = cache_dir;
      // check if the cache dir needs to be drained 
      bool isDrainingCache = false;
      if (cache_link_dir == "drain") {
        cache = cache_dir.substr(0, cache_dir.find(" "));
        cache_link_dir = "";
        isDrainingCache = true;
      }
      if (!cache_link_dir.empty()) cache += " "+cache_link_dir;
   
      if(isDrainingCache)
        _draining_cache_dirs.push_back(cache);  
      else
        _remote_cache_dirs.push_back(cache);
    }
    else if(command == "cachedir") {
      std::string cache_dir = Arc::ConfigIni::NextArg(rest);
      if(cache_dir.length() == 0) continue; // cache is disabled
      std::string cache_link_dir = Arc::ConfigIni::NextArg(rest);

      // validation of paths
      while (cache_dir.length() > 1 && cache_dir.rfind("/") == cache_dir.length()-1) cache_dir = cache_dir.substr(0, cache_dir.length()-1);
      if (cache_dir[0] != '/') throw CacheConfigException("Cache path must start with '/'");
      if (cache_dir.find("..") != std::string::npos) throw CacheConfigException("Cache path cannot contain '..'");
      if (!cache_link_dir.empty() && cache_link_dir != "." && cache_link_dir != "drain") {
        while (cache_link_dir.rfind("/") == cache_link_dir.length()-1) cache_link_dir = cache_link_dir.substr(0, cache_link_dir.length()-1);
        if (cache_link_dir[0] != '/') throw CacheConfigException("Cache link path must start with '/'");
        if (cache_link_dir.find("..") != std::string::npos) throw CacheConfigException("Cache link path cannot contain '..'");
      }
      // add this cache to our list
      std::string cache = cache_dir;
      bool isDrainingCache = false;
      // check if the cache dir needs to be drained 
      if (cache_link_dir == "drain") {
        cache = cache_dir.substr(0, cache_dir.find(' '));
        cache_link_dir = "";
        isDrainingCache = true;
      }
      if (!cache_link_dir.empty())
        cache += " "+cache_link_dir;

      if (isDrainingCache)
        _draining_cache_dirs.push_back(cache); 
      else
        _cache_dirs.push_back(cache);
    }
    else if(command == "cachesize") {
      std::string max_s = Arc::ConfigIni::NextArg(rest);
      if(max_s.length() == 0)
        continue; 

      std::string min_s = Arc::ConfigIni::NextArg(rest);
      if(min_s.length() == 0)
        throw CacheConfigException("Not enough parameters in cachesize parameter");

      off_t max_i;
      if(!Arc::stringto(max_s,max_i))
        throw CacheConfigException("bad number in cachesize parameter");
      if (max_i > 100 || max_i < 0)
        throw CacheConfigException("max cache size must be between 0 and 100");
      _cache_max = max_i;
      
      off_t min_i;
      if(!Arc::stringto(min_s,min_i))
        throw CacheConfigException("bad number in cachesize parameter");
      if (min_i > 100 || min_i < 0)
        throw CacheConfigException("min cache size must be between 0 and 100");
      if (min_i >= max_i)
        throw CacheConfigException("max cache size must be greater than min size");
      _cache_min = min_i;
    }
    else if(command == "cachelogfile") {
      std::string logfile = Arc::ConfigIni::NextArg(rest);
      if (logfile.length() < 2 || logfile[0] != '/' || logfile[logfile.length()-1] == '/')
        throw CacheConfigException("Bad filename in cachelogfile parameter");
      _log_file = logfile;
    }
    else if(command == "cacheloglevel") {
      std::string log_level = Arc::ConfigIni::NextArg(rest);
      if(log_level.length() == 0)
        throw CacheConfigException("No value specified in cacheloglevel");
      off_t level_i;
      if(!Arc::stringto(log_level, level_i))
        throw CacheConfigException("bad number in cacheloglevel parameter");
      // manual conversion from int to log level
      switch (level_i) {
        case 0: { _log_level = "FATAL"; };
          break;
        case 1: { _log_level = "ERROR"; };
          break;
        case 2: { _log_level = "WARNING"; };
          break;
        case 3: { _log_level = "INFO"; };
          break;
        case 4: { _log_level = "VERBOSE"; };
          break;
        case 5: { _log_level = "DEBUG"; };
          break;
        default: { _log_level = "INFO"; };
          break;
      } 
    }
    else if(command == "cachelifetime") {
      std::string lifetime = Arc::ConfigIni::NextArg(rest);
      if(lifetime.length() != 0) {
        _lifetime = lifetime;
      }
    }
    else if(command == "cacheshared") {
      std::string cache_shared = Arc::ConfigIni::NextArg(rest);
      if (cache_shared == "yes") {
        _cache_shared = true;
      }
      else if (cache_shared != "no") {
        throw CacheConfigException("Bad value in cacheshared parameter");
      }
    }
    else if (command == "cachespacetool") {
      _cache_space_tool = rest;
    }
    else if (command == "cachecleantimeout") {
      std::string timeout = Arc::ConfigIni::NextArg(rest);
      if(timeout.length() == 0)
        continue;

      if(!Arc::stringto(timeout, _clean_timeout))
        throw CacheConfigException("bad number in cachecleantimeout parameter");
    }
    else if (command == "cacheaccess") {
      Arc::RegularExpression regexp(Arc::ConfigIni::NextArg(rest));
      if (!regexp.isOk()) throw CacheConfigException("Bad regexp " + regexp.getPattern() + " in cacheaccess");

      std::string cred_type(Arc::ConfigIni::NextArg(rest));
      if (cred_type.empty()) throw CacheConfigException("Missing credential type in cacheaccess");

      std::string cred_value(rest);
      if (cred_value.empty()) throw CacheConfigException("Missing credential value in cacheaccess");

      struct CacheAccess ca;
      ca.regexp = regexp;
      ca.cred_type = cred_type;
      ca.cred_value = cred_value;
      _cache_access.push_back(ca);
    }
  }
}

void CacheConfig::parseXMLConf(const Arc::XMLNode& cfg) {
  /*
  control
    controlDir
    sessionRootDir
    cache
      location
        path
        link
      remotelocation
        path
        link
      highWatermark
      lowWatermark
      cacheLifetime
      cacheLogFile
      cacheLogLevel
      cacheCleanTimeout
      cacheShared
      cacheSpaceTool
    defaultTTL
    defaultTTR
    maxReruns
    noRootPower
  */
  Arc::XMLNode control_node = cfg;
  if(control_node.Name() != "control") {
    control_node = cfg["control"];
  }
  if (!control_node) throw CacheConfigException("No control element found in configuration");

  Arc::XMLNode cache_node = control_node["cache"];
  if (!cache_node) return; // no cache configured

  Arc::XMLNode location_node = cache_node["location"];
  for(;location_node;++location_node) {
    std::string cache_dir = location_node["path"];
    std::string cache_link_dir = location_node["link"];
    if(cache_dir.length() == 0)
      throw CacheConfigException("Missing path in cache location element");

    // validation of paths
    while (cache_dir.length() > 1 && cache_dir.rfind("/") == cache_dir.length()-1) cache_dir = cache_dir.substr(0, cache_dir.length()-1);
    if (cache_dir[0] != '/') throw CacheConfigException("Cache path must start with '/'");
    if (cache_dir.find("..") != std::string::npos) throw CacheConfigException("Cache path cannot contain '..'");
    if (!cache_link_dir.empty() && cache_link_dir != "." && cache_link_dir != "drain") {
      while (cache_link_dir.rfind("/") == cache_link_dir.length()-1) cache_link_dir = cache_link_dir.substr(0, cache_link_dir.length()-1);
      if (cache_link_dir[0] != '/') throw CacheConfigException("Cache link path must start with '/'");
      if (cache_link_dir.find("..") != std::string::npos) throw CacheConfigException("Cache link path cannot contain '..'");
    }

    // add this cache to our list
    std::string cache = cache_dir;
    bool isDrainingCache = false;
    // check if the cache dir needs to be drained
    if (cache_link_dir == "drain") {
      cache = cache_dir.substr(0, cache_dir.find (" "));
      cache_link_dir = "";
      isDrainingCache = true;
    }

    if (!cache_link_dir.empty())
      cache += " "+cache_link_dir;

    // TODO: handle paths with spaces
    if(isDrainingCache)
      _draining_cache_dirs.push_back(cache);
    else
      _cache_dirs.push_back(cache);
  }
  Arc::XMLNode high_node = cache_node["highWatermark"];
  Arc::XMLNode low_node = cache_node["lowWatermark"];
  if (high_node && !low_node) {
    throw CacheConfigException("missing lowWatermark parameter");
  } else if (low_node && !high_node) {
    throw CacheConfigException("missing highWatermark parameter");
  } else if (low_node && high_node) {
    off_t max_i;
    if(!Arc::stringto((std::string)high_node,max_i))
      throw CacheConfigException("bad number in highWatermark parameter");
    if (max_i > 100)
      throw CacheConfigException("number is too high in highWatermark parameter");
    _cache_max = max_i;

    off_t min_i;
    if(!Arc::stringto((std::string)low_node,min_i))
      throw CacheConfigException("bad number in lowWatermark parameter");
    if (min_i > 100)
      throw CacheConfigException("number is too high in lowWatermark parameter");
    if (min_i >= max_i)
      throw CacheConfigException("highWatermark must be greater than lowWatermark");
    _cache_min = min_i;
  }
  std::string cache_log_file = cache_node["cacheLogFile"];
  if (!cache_log_file.empty()) {
    if (cache_log_file.length() < 2 || cache_log_file[0] != '/' || cache_log_file[cache_log_file.length()-1] == '/')
      throw CacheConfigException("Bad filename in cachelogfile parameter");
    _log_file = cache_log_file;
  }
  std::string cache_log_level = cache_node["cacheLogLevel"];
  if (!cache_log_level.empty())
    _log_level = cache_log_level;
  std::string cache_lifetime = cache_node["cacheLifetime"];
  if (!cache_lifetime.empty())
    _lifetime = cache_lifetime;
  std::string cache_shared = cache_node["cacheShared"];
  if (cache_shared == "yes") {
    _cache_shared = true;
  }
  std::string cache_space_tool = cache_node["cacheSpaceTool"];
  if (!cache_space_tool.empty()) {
    _cache_space_tool = cache_space_tool;
  }
  std::string clean_timeout = cache_node["cacheCleanTimeout"];
  if (!clean_timeout.empty()) {
    if(!Arc::stringto(clean_timeout, _clean_timeout))
      throw CacheConfigException("bad number in cacheCleanTimeout parameter");
  }
  Arc::XMLNode remote_location_node = cache_node["remotelocation"];
  for(;remote_location_node;++remote_location_node) {
    std::string cache_dir = remote_location_node["path"];
    std::string cache_link_dir = remote_location_node["link"];
    if(cache_dir.length() == 0)
      throw CacheConfigException("Missing path in remote cache location element");

    // validation of paths
    while (cache_dir.length() > 1 && cache_dir.rfind("/") == cache_dir.length()-1) cache_dir = cache_dir.substr(0, cache_dir.length()-1);
    if (cache_dir[0] != '/') throw CacheConfigException("Remote cache path must start with '/'");
    if (cache_dir.find("..") != std::string::npos) throw CacheConfigException("Remote cache path cannot contain '..'");
    if (!cache_link_dir.empty() && cache_link_dir != "." && cache_link_dir != "drain" && cache_link_dir != "replicate") {
      while (cache_link_dir.rfind("/") == cache_link_dir.length()-1) cache_link_dir = cache_link_dir.substr(0, cache_link_dir.length()-1);
      if (cache_link_dir[0] != '/') throw CacheConfigException("Remote cache link path must start with '/'");
      if (cache_link_dir.find("..") != std::string::npos) throw CacheConfigException("Remote cache link path cannot contain '..'");
    }

    // add this cache to our list
    std::string cache = cache_dir;
    bool isDrainingCache = false;
    // check if the cache dir needs to be drained
    if (cache_link_dir == "drain") {
      cache = cache_dir.substr(0, cache_dir.find (" "));
      cache_link_dir = "";
      isDrainingCache = true;
    }

    if (!cache_link_dir.empty())
      cache += " "+cache_link_dir;

    // TODO: handle paths with spaces
    if(isDrainingCache)
      _draining_cache_dirs.push_back(cache);
    else
      _remote_cache_dirs.push_back(cache);
  }
}

void CacheConfig::substitute(const GMConfig& config, const Arc::User& user) {
  for (std::vector<std::string>::iterator i = _cache_dirs.begin(); i != _cache_dirs.end(); ++i) {
    config.Substitute(*i, user);
  }
  for (std::vector<std::string>::iterator i = _remote_cache_dirs.begin(); i != _remote_cache_dirs.end(); ++i) {
    config.Substitute(*i, user);
  }
  for (std::vector<std::string>::iterator i = _draining_cache_dirs.begin(); i != _draining_cache_dirs.end(); ++i) {
    config.Substitute(*i, user);
  }
}

} // namespace ARex
