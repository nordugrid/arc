#include "conf_cache.h"

CacheConfig::CacheConfig(std::string username): _cache_max(80),
                                                _cache_min(70),
                                                _clean_cache(true),
                                                _old_conf(false) {
  // open conf file
  std::ifstream cfile;
  ConfigSections* cf = NULL;

  if(!config_open(cfile)) throw CacheConfigException("Can't open configuration file");
  
  if(central_configuration) {
    cf=new ConfigSections(cfile);
    cf->AddSection("common");
    cf->AddSection("grid-manager");
  }
  
  // for backwards compatibility
  std::string cache_data_dir("");
  std::string cache_link_dir("");
  
  for(;;) {
    std::string rest;
    std::string command;
    if(central_configuration) {
      cf->ReadNext(command,rest);
    } else {
      command = config_read_line(cfile,rest);
    }

    if(command.length() == 0) break;
    
    else if(command == "cachedir") {
      std::string cache_dir = config_next_arg(rest);
      if(cache_dir.length() == 0) continue; // cache is disabled
      cache_link_dir = config_next_arg(rest);

      // take off leading slashes
      if (cache_dir.rfind("/") == cache_dir.length()-1) cache_dir = cache_dir.substr(0, cache_dir.length()-1);

      // add this cache to our list
      std::list<std::string> cache;
      cache.push_back(cache_dir+"/data");
      cache.push_back(cache_dir+"/joblinks");
      cache.push_back(cache_link_dir);
      _cache_dirs.push_back(cache);
    }
    else if(command == "cachedata") {
      cache_data_dir = config_next_arg(rest);
    }
    else if(command == "cachesize") {
      std::string max_s = config_next_arg(rest);
      if(max_s.length() == 0) {
        // cleaning disabled
        _clean_cache = false;
        continue; 
      }
      std::string min_s = config_next_arg(rest);
      if(min_s.length() == 0) {
        throw CacheConfigException("Not enough parameters in cachesize parameter");
      }
      int max_i;
      if(!Arc::stringto(max_s,max_i)) {
        throw CacheConfigException("bad number in cachesize parameter");
      }
      if (max_i > 100) {
        _old_conf = true;
        continue;
      }
      _cache_max = max_i;
      int min_i;
      if(!Arc::stringto(min_s,min_i)) {
        throw CacheConfigException("bad number in cachesize parameter");
      }
      if (min_i > 100) _old_conf = true;
      else _cache_min = min_i;
    }
    else if(command == "control") {
      // if the user specified here matches the one given, exit the loop
      config_next_arg(rest);
      bool usermatch = false;
      std::string user = config_next_arg(rest);
      while (user != "") {
        if(user == "*") {  /* add all gridmap users */
           if(!gridmap_user_list(rest)) throw CacheConfigException("Can't read users in gridmap file " + globus_gridmap);
        }
        else if (user == username || user == ".") {
          usermatch = true;
          break;
        }
        user = config_next_arg(rest);
      }
      if (usermatch) break;
      _cache_dirs.clear();
      _cache_max = 80;
      _cache_min = 70;
      _clean_cache = true;
      _old_conf = false;
    }
  }
  // check for old configuration, use cache_data_dir as cache dir
  if (_old_conf && !_cache_dirs.empty() && !cache_data_dir.empty()) {
    _cache_dirs.clear();
    std::list<std::string> cache;
    cache.push_back(cache_data_dir+"/data");
    cache.push_back(cache_data_dir+"/joblinks");
    cache.push_back(cache_link_dir);
    _cache_dirs.push_back(cache);
  }
  config_close(cfile);
  if(cf) delete cf;
}


