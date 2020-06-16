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
    _cleaning_enabled(false),
    _log_file("/var/log/arc/cache-clean.log"),
    _log_level("INFO") ,
    _lifetime("0"),
    _cache_shared(false),
    _clean_timeout(0) {
  // Load conf file
  Arc::ConfigFile cfile;
  if(!cfile.open(config.ConfigFile())) throw CacheConfigException("Can't open configuration file");
  
  /* detect type of file */
  if (cfile.detect() != Arc::ConfigFile::file_INI) {
    cfile.close();
    throw CacheConfigException("Can't recognize type of configuration file");
  }

  Arc::ConfigIni cf(cfile);
  try {
    parseINIConf(cf);
  } catch (CacheConfigException& e) {
    cfile.close();
    throw;
  }

  cfile.close();
}

void CacheConfig::parseINIConf(Arc::ConfigIni& cf) {
  
  cf.SetSectionIndicator(".");
  cf.AddSection("arex/cache/cleaner"); // 0
  cf.AddSection("arex/cache");         // 1
  cf.AddSection("arex/ws/cache");      // 2
  
  for(;;) {
    std::string rest;
    std::string command;
    cf.ReadNext(command,rest);

    if(command.length() == 0) break; // EOF

    if (cf.SectionNum() == 0) { // arex/cache/cleaner
      if (cf.SubSection()[0] == '\0') {
        // The presence of this sub-block enables cleaning
        _cleaning_enabled = true;
        if(command == "cachesize") {
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
        else if(command == "logfile") {
          std::string logfile = rest;
          if (logfile.length() < 2 || logfile[0] != '/' || logfile[logfile.length()-1] == '/')
            throw CacheConfigException("Bad filename in cachelogfile parameter");
          _log_file = logfile;
        }
        else if(command == "loglevel") {
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
        else if(command == "calculatesize") {
          std::string cache_shared = Arc::ConfigIni::NextArg(rest);
          if (cache_shared == "cachedir") {
            _cache_shared = true;
          }
          else if (cache_shared != "filesystem") {
            throw CacheConfigException("Bad value in cacheshared parameter: only 'cachedir' or 'filesystem' allowed");
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
      }
    } else if (cf.SectionNum() == 1) { // arex/cache
      if (cf.SubSection()[0] == '\0') {
        if(command == "cachedir") {
          std::string cache_dir = Arc::ConfigIni::NextArg(rest);
          if(cache_dir.length() == 0) continue; // cache is disabled
          std::string cache_link_dir = Arc::ConfigIni::NextArg(rest);

          // validation of paths
          while (cache_dir.length() > 1 && cache_dir.rfind("/") == cache_dir.length()-1) cache_dir = cache_dir.substr(0, cache_dir.length()-1);
          if (cache_dir[0] != '/') throw CacheConfigException("Cache path must start with '/'");
          if (cache_dir.find("..") != std::string::npos) throw CacheConfigException("Cache path cannot contain '..'");
          if (!cache_link_dir.empty() && cache_link_dir != "." && cache_link_dir != "drain" && cache_link_dir != "readonly") {
            while (cache_link_dir.rfind("/") == cache_link_dir.length()-1) cache_link_dir = cache_link_dir.substr(0, cache_link_dir.length()-1);
            if (cache_link_dir[0] != '/') throw CacheConfigException("Cache link path must start with '/'");
            if (cache_link_dir.find("..") != std::string::npos) throw CacheConfigException("Cache link path cannot contain '..'");
          }
          // check if the cache dir needs to be drained or is read-only
          if (cache_link_dir == "drain") {
            _draining_cache_dirs.push_back(cache_dir);
          }
          else if (cache_link_dir == "readonly") {
            _readonly_cache_dirs.push_back(cache_dir);
          }
          else {
            if (!cache_link_dir.empty()) {
              cache_dir += " "+cache_link_dir;
            }
            _cache_dirs.push_back(cache_dir);
          }
        }
      }
    } else if (cf.SectionNum() == 2) { // arex/ws/cache
      if (cf.SubSection()[0] == '\0') {
        if (command == "cacheaccess") {
          Arc::RegularExpression regexp(Arc::ConfigIni::NextArg(rest));
          if (!regexp.isOk()) throw CacheConfigException("Bad regexp " + regexp.getPattern() + " in cacheaccess");

          std::string cred_type(Arc::ConfigIni::NextArg(rest));
          if (cred_type.empty()) throw CacheConfigException("Missing credential type in cacheaccess");

          Arc::RegularExpression cred_value(rest);
          if (!cred_value.isOk()) throw CacheConfigException("Missing credential value in cacheaccess");

          struct CacheAccess ca;
          ca.regexp = regexp;
          ca.cred_type = cred_type;
          ca.cred_value = cred_value;
          _cache_access.push_back(ca);
        }
      }
    }

  }
}

void CacheConfig::substitute(const GMConfig& config, const Arc::User& user) {
  for (std::vector<std::string>::iterator i = _cache_dirs.begin(); i != _cache_dirs.end(); ++i) {
    config.Substitute(*i, user);
  }
  for (std::vector<std::string>::iterator i = _draining_cache_dirs.begin(); i != _draining_cache_dirs.end(); ++i) {
    config.Substitute(*i, user);
  }
  for (std::vector<std::string>::iterator i = _readonly_cache_dirs.begin(); i != _readonly_cache_dirs.end(); ++i) {
    config.Substitute(*i, user);
  }
}

} // namespace ARex
