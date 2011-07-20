#include <arc/XMLNode.h>

#include "conf_cache.h"

CacheConfig::CacheConfig(const GMEnvironment& env,std::string username):
                                                _cache_max(100),
                                                _cache_min(100),
                                                _log_file("/var/log/arc/cache-clean.log"),
                                                _log_level("INFO") ,
                                                _lifetime("0") {
  // open conf file
  std::ifstream cfile;
  //if(nordugrid_config_loc().empty()) read_env_vars(true);
  if(!config_open(cfile,env)) throw CacheConfigException("Can't open configuration file");
  
  /* detect type of file */
  switch(config_detect(cfile)) {
    case config_file_XML: {
      Arc::XMLNode cfg;
      if(!cfg.ReadFromStream(cfile)) {
        config_close(cfile);
        throw CacheConfigException("Can't interpret configuration file as XML");
      };
      config_close(cfile);
      try {
        parseXMLConf(username, cfg);
      } catch (CacheConfigException& e) {
        config_close(cfile);
        throw;
      }
    }; break;
    case config_file_INI: {
      ConfigSections* cf = new ConfigSections(cfile);
      try {
        parseINIConf(username, cf);
      } catch (CacheConfigException& e) {
        delete cf;
        config_close(cfile);
        throw;
      }
      delete cf;
    }; break;
    default: {
      config_close(cfile);
      throw CacheConfigException("Can't recognize type of configuration file");
    }; break;
  };
  config_close(cfile);
}

void CacheConfig::parseINIConf(std::string username, ConfigSections* cf) {
  
  cf->AddSection("common");
  cf->AddSection("grid-manager");
  
  for(;;) {
    std::string rest;
    std::string command;
    cf->ReadNext(command,rest);

    if(command.length() == 0) break;
    
    else if(command == "remotecachedir") {
      std::string cache_dir = config_next_arg(rest);
      if(cache_dir.length() == 0) continue; // cache is disabled
      std::string cache_link_dir = config_next_arg(rest);
       
      // take off leading slashes
      if (cache_dir.rfind("/") == cache_dir.length()-1) cache_dir = cache_dir.substr(0, cache_dir.length()-1);

      // if there are substitutions, check username is defined
      if (username.empty() &&
          (cache_dir.find("%U") != std::string::npos ||
              cache_dir.find("%u") != std::string::npos ||
              cache_dir.find("%g") != std::string::npos ||
              cache_dir.find("%H") != std::string::npos ||
              cache_link_dir.find("%U") != std::string::npos ||  
              cache_link_dir.find("%u") != std::string::npos ||
              cache_link_dir.find("%g") != std::string::npos ||
              cache_link_dir.find("%H") != std::string::npos )) continue; 
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
      std::string cache_dir = config_next_arg(rest);
      if(cache_dir.length() == 0) continue; // cache is disabled
      std::string cache_link_dir = config_next_arg(rest);

      // validation of paths
      while (cache_dir.length() > 1 && cache_dir.rfind("/") == cache_dir.length()-1) cache_dir = cache_dir.substr(0, cache_dir.length()-1);
      if (cache_dir[0] != '/') throw CacheConfigException("Cache path must start with '/'");
      if (cache_dir.find("..") != std::string::npos) throw CacheConfigException("Cache path cannot contain '..'");
      if (!cache_link_dir.empty() && cache_link_dir != "." && cache_link_dir != "drain") {
        while (cache_link_dir.rfind("/") == cache_link_dir.length()-1) cache_link_dir = cache_link_dir.substr(0, cache_link_dir.length()-1);
        if (cache_link_dir[0] != '/') throw CacheConfigException("Cache link path must start with '/'");
        if (cache_link_dir.find("..") != std::string::npos) throw CacheConfigException("Cache link path cannot contain '..'");
      }
      
      // if there are substitutions, check username is defined
      if (username.empty() &&
          (cache_dir.find("%U") != std::string::npos ||
              cache_dir.find("%u") != std::string::npos ||
              cache_dir.find("%g") != std::string::npos ||
              cache_dir.find("%H") != std::string::npos ||
              cache_link_dir.find("%U") != std::string::npos ||  
              cache_link_dir.find("%u") != std::string::npos ||
              cache_link_dir.find("%g") != std::string::npos ||
              cache_link_dir.find("%H") != std::string::npos )) continue;

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
      std::string max_s = config_next_arg(rest);
      if(max_s.length() == 0)
        continue; 

      std::string min_s = config_next_arg(rest);
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
        throw CacheConfigException("max cache size must be between 0 and 100");
      if (min_i >= max_i)
        throw CacheConfigException("max cache size must be greater than min size");
      _cache_min = min_i;
    }
    else if(command == "cachelogfile") {
      std::string logfile = config_next_arg(rest);
      if (logfile.length() < 2 || logfile[0] != '/' || logfile[logfile.length()-1] == '/')
        throw CacheConfigException("Bad filename in cachelogfile parameter");
      _log_file = logfile;
    }
    else if(command == "cacheloglevel") {
      std::string log_level = config_next_arg(rest);
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
      std::string lifetime = config_next_arg(rest);
      if(lifetime.length() != 0) {
        _lifetime = lifetime;
      }
    }
    else if(command == "control") {
      // if the user specified here matches the one given, exit the loop
      config_next_arg(rest);
      bool usermatch = false;
      std::string user = config_next_arg(rest);
      while (user != "") {
        if(user == "*") {  /* add all gridmap users */
           throw CacheConfigException("Gridmap user list feature is not supported anymore. Plase use @filename to specify user list.");
        }
        else if (user[0] == '@') {
           std::string filename = user.substr(1);
           if(!file_user_list(filename,rest)) throw CacheConfigException("Can't read users in specified file " + filename);
        }
        else if (user == username || user == ".") {
          usermatch = true;
          break;
        }
        user = config_next_arg(rest);
      }
      if (usermatch) break;
      _cache_dirs.clear();
      _cache_max = 100;
      _cache_min = 100;
      _lifetime = "0";
    }
  }
}

void CacheConfig::parseXMLConf(std::string username, Arc::XMLNode cfg) { 
  /*
  control
    username
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
    defaultTTL
    defaultTTR
    maxReruns
    noRootPower
  */
  Arc::XMLNode control_node = cfg["control"];
  while (control_node) {
    // does this match our username?
    std::string user = control_node["username"];
    if (user != username && user != ".") {
      ++control_node;
      continue;
    }
    Arc::XMLNode cache_node = control_node["cache"];
    if(cache_node) {
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
      
        // if there are user substitutions, check username is defined
        if (username.empty() &&
            (cache_dir.find("%U") != std::string::npos ||
                cache_dir.find("%u") != std::string::npos ||
                cache_dir.find("%g") != std::string::npos ||
                cache_dir.find("%H") != std::string::npos ||
                cache_link_dir.find("%U") != std::string::npos ||  
                cache_link_dir.find("%u") != std::string::npos ||
                cache_link_dir.find("%g") != std::string::npos ||
                cache_link_dir.find("%H") != std::string::npos )) continue;
        
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
      
        // if there are user substitutions, check username is defined
        if (username.empty() &&
            (cache_dir.find("%U") != std::string::npos ||
                cache_dir.find("%u") != std::string::npos ||
                cache_dir.find("%g") != std::string::npos ||
                cache_dir.find("%H") != std::string::npos ||
                cache_link_dir.find("%U") != std::string::npos ||  
                cache_link_dir.find("%u") != std::string::npos ||
                cache_link_dir.find("%g") != std::string::npos ||
                cache_link_dir.find("%H") != std::string::npos )) continue;
        
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
    } else {
      // cache is disabled
    }
    ++control_node;
  }
}

