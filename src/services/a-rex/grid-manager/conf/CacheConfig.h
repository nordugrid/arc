#ifndef __GM_CONFIG_CACHE_H__
#define __GM_CONFIG_CACHE_H__

#include <arc/ArcRegex.h>
#include <arc/User.h>
#include <arc/ArcConfigIni.h>

namespace ARex {

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
 public:
  /// A struct defining a URL pattern and credentials which can access it
  struct CacheAccess {
    Arc::RegularExpression regexp;
    std::string cred_type;
    Arc::RegularExpression cred_value;
  };
 private:
   /**
    * List of (cache dir [link dir])
    */
   std::vector<std::string> _cache_dirs;
   float _cache_max;
   float _cache_min;
   /**
    * Whether automatic cleaning is enabled
    */
   bool _cleaning_enabled;
   /**
    * Cache directories that are needed to be drained
    **/
   std::vector<std::string> _draining_cache_dirs;
   /**
    * Cache directories that are read-only
    **/
   std::vector<std::string> _readonly_cache_dirs;
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
    * Whether the cache is shared with other data on the file system
    */
   bool _cache_shared;
   /**
    * User-specified tool for getting space information for cleaning tool
    */
   std::string _cache_space_tool;
   /**
    * Timeout for cleaning process
    */
   int _clean_timeout;
   /**
    * List of CacheAccess structs describing who can access what URLs in cache
    */
   std::list<struct CacheAccess> _cache_access;
  /**
   * Parsers for the two different conf styles
   */
  void parseINIConf(Arc::ConfigIni& cf);
 public:
   /**
    * Create a new CacheConfig instance. Read the config file and fill in
    * private member variables with cache parameters.
    */
  CacheConfig(const GMConfig& config);
  /**
   * Empty CacheConfig
   */
  CacheConfig(): _cache_max(0), _cache_min(0), _cleaning_enabled(false), _cache_shared(false), _clean_timeout(0) {};
  std::vector<std::string> getCacheDirs() const { return _cache_dirs; };
  std::vector<std::string> getDrainingCacheDirs() const { return _draining_cache_dirs; };
  std::vector<std::string> getReadOnlyCacheDirs() const { return _readonly_cache_dirs; };
  /// Substitute all cache paths, with information given in user if necessary
  void substitute(const GMConfig& config, const Arc::User& user);
  float getCacheMax() const { return _cache_max; };
  float getCacheMin() const { return _cache_min; };
  bool cleanCache() const { return _cleaning_enabled; };
  std::string getLogFile() const { return _log_file; };
  std::string getLogLevel() const { return _log_level; };
  std::string getLifeTime() const { return _lifetime; };
  bool getCacheShared() const { return _cache_shared; };
  std::string getCacheSpaceTool() const { return _cache_space_tool; };
  int getCleanTimeout() const { return _clean_timeout; };
  const std::list<struct CacheAccess>& getCacheAccess() const { return _cache_access; };
};

} // namespace ARex

#endif /*__GM_CONFIG_CACHE_H__*/
