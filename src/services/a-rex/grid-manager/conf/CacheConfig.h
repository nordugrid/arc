#ifndef __GM_CONFIG_CACHE_H__
#define __GM_CONFIG_CACHE_H__

#include <arc/User.h>
#include <arc/XMLNode.h>

namespace ARex {

class ConfigSections;
class GMConfig;

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
 * Methods of this class may throw CacheConfigException.
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
  /**
   * Parsers for the two different conf styles
   */
  void parseINIConf(ConfigSections& cf);
  void parseXMLConf(const Arc::XMLNode& cfg);
 public:
   /**
    * Create a new CacheConfig instance. Read the config file and fill in
    * private member variables with cache parameters.
    */
  CacheConfig(const GMConfig& config);
   /**
    * Create a new CacheConfig instance. Read the XML config tree and fill in
    * private member variables with cache parameters.
    */
  CacheConfig(const Arc::XMLNode& cfg);
  /**
   * Empty CacheConfig
   */
  CacheConfig(): _cache_max(0), _cache_min(0), _clean_timeout(0) {};
  std::vector<std::string> getCacheDirs() const { return _cache_dirs; };
  std::vector<std::string> getRemoteCacheDirs() const { return _remote_cache_dirs; };
  std::vector<std::string> getDrainingCacheDirs() const { return _draining_cache_dirs; };
  /// Substitute all cache paths, with information given in user if necessary
  void substitute(const GMConfig& config, const Arc::User& user);
  int getCacheMax() const { return _cache_max; };
  int getCacheMin() const { return _cache_min; };
  bool cleanCache() const { return (_cache_max > 0 && _cache_max < 100); };
  std::string getLogFile() const { return _log_file; };
  std::string getLogLevel() const { return _log_level; };
  std::string getLifeTime() const { return _lifetime; };
  int getCleanTimeout() const { return _clean_timeout; };
};

} // namespace ARex

#endif /*__GM_CONFIG_CACHE_H__*/
