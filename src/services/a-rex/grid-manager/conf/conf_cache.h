#ifndef __GM_CONFIG_CACHE_H__
#define __GM_CONFIG_CACHE_H__

#include <string>
#include <fstream>

#include <arc/StringConv.h>

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
  virtual ~CacheConfigException() throw() {};
  std::string what() {return _desc;};
};

/**
 * Reads conf file and provides methods to obtain cache info from it.
 */
class CacheConfig {
 private:
   /**
    * List of (cache dir [link dir])
    */
   std::list<std::string> _cache_dirs;
   int _cache_max;
   int _cache_min;
   /**
    * true if cleaning is enabled
    */
   bool _clean_cache;
   /**
    * true if old configuration is detected
    */
   bool _old_conf;
 public:
   /**
    * Create a new CacheConfig instance. Read the config file and fill in
    * private member variables with cache parameters. If different users are
    * defined in the conf file, use the cache parameters for the given username.
    */
  CacheConfig(std::string username = "");
  ~CacheConfig(void) {};
  std::list<std::string> getCacheDirs() { return _cache_dirs; };
  /**
   * To allow for substitutions done during configuration
   */
  void setCacheDirs(std::list<std::string> cache_dirs) { _cache_dirs = cache_dirs; };
  int getCacheMax() { return _cache_max; };
  int getCacheMin() { return _cache_min; };
  bool cleanCache() { return _clean_cache; };
  bool oldConf() { return _old_conf; };
};

#endif /*__GM_CONFIG_CACHE_H__*/
