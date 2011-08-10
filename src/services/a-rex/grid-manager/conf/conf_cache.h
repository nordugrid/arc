#ifndef __GM_CONFIG_CACHE_H__
#define __GM_CONFIG_CACHE_H__

#include <string>
#include <fstream>

#include <arc/StringConv.h>
#include <arc/XMLNode.h>

#include "conf.h"
#include "environment.h"
#include "gridmap.h"
#include "conf_sections.h"

/**
 * Exception thrown by constructor caused by bad cache params in conf file
 */
class CacheConfigException : public std::exception {
private:
  std::string _desc;
  
public:
  CacheConfigException(std::string desc = ""): _desc(desc) {};
  ~CacheConfigException() throw() {};
  virtual const char* what() const throw() {return _desc.c_str();};
};

/**
 * Reads conf file and provides methods to obtain cache info from it.
 */
class CacheConfig {
 private:
   /**
    * List of (cache dir [link dir])
    */
   std::vector<std::string> _cache_dirs;
   /**
    * List of (cache dir [link dir]) for remote caches
    */
   std::vector<std::string> _remote_cache_dirs;
   int _cache_max;
   int _cache_min;
   /**
    * Cache directories that are needed to be drained
    **/
   std::vector<std::string> _draining_cache_dirs;
   /**
    * Logfile for cache cleaning messages
    */
   std::string _log_file;
   /**
    * cache-clean log level
    */
   std::string _log_level;
   /**
    * Lifetime of files in cache
    */
   std::string _lifetime;
   /**
    * Timeout for cleaning process
    */
   int _clean_timeout;
 public:
   /**
    * Create a new CacheConfig instance. Read the config file and fill in
    * private member variables with cache parameters. If different users are
    * defined in the conf file, use the cache parameters for the given username.
    */
  CacheConfig(const GMEnvironment& env,std::string username = "");
  /**
   * Empty CacheConfig
   */
  CacheConfig(): _cache_max(0), _cache_min(0), _clean_timeout(0) {};
  /**
   * Parsers for the two different conf styles
   */
  void parseINIConf(std::string username, ConfigSections* cf);
  void parseXMLConf(std::string username, Arc::XMLNode cfg);
  std::vector<std::string> getCacheDirs() const { return _cache_dirs; };
  std::vector<std::string> getRemoteCacheDirs() const { return _remote_cache_dirs; };
  std::vector<std::string> getDrainingCacheDirs() const { return _draining_cache_dirs; };
  /**
   * To allow for substitutions done during configuration
   */
  void setCacheDirs(std::vector<std::string> cache_dirs) { _cache_dirs = cache_dirs; };
  void setRemoteCacheDirs(std::vector<std::string> remote_cache_dirs) { _remote_cache_dirs = remote_cache_dirs; };
  void setDrainingCacheDirs(std::vector<std::string> draining_cache_dirs) { _draining_cache_dirs = draining_cache_dirs; }; 
  int getCacheMax() const { return _cache_max; };
  int getCacheMin() const { return _cache_min; };
  bool cleanCache() const { return (_cache_max > 0 && _cache_max < 100); };
  std::string getLogFile() const { return _log_file; };
  std::string getLogLevel() const { return _log_level; };
  std::string getLifeTime() const { return _lifetime; };
  int getCleanTimeout() const { return _clean_timeout; };
};

#endif /*__GM_CONFIG_CACHE_H__*/
