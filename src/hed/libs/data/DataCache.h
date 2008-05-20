#ifndef __ARC_DATACACHE_H__
#define __ARC_DATACACHE_H__

#include <string>

#include "Cache.h"
#include "DataCallback.h"
#include <arc/DateTime.h>
#include <arc/URL.h>

namespace Arc {

  class Logger;

  /// High level interface to cache operations (same functionality :) )
  /// and additional functionality to integrate into grid-manager environment.
  class DataCache
    : public DataCallback {
  private:
    /* path to directory with cache info files */
    std::string cache_path;
    /* path to directory with cache data files.
       usually it is same as cache_path */
    std::string cache_data_path;
    /* path used to create link in case cache_directory is visible
       under different name during actual usage */
    std::string cache_link_path;
    /* identifier used to claim files */
    std::string id;
    /* needed for low-level interface */
    cache_download_handler cdh;
    /* url was provided - active object */
    bool have_url;
    /* url given */
    URL cache_url;
    /* file storing content of url */
    std::string cache_file;
    /* owner:group of cache */
    Arc::User cache_user;
    Time creation_time;
    Time expiration_time;
    static Logger logger;
    bool link_file(const std::string& link_path, const Arc::User& user);
    bool copy_file(const std::string& link_path, const Arc::User& user);
  public:
    typedef enum {
      file_no_error = 0,
      file_download_failed = 1,
      file_not_valid = 2,
      file_keep = 4
    } file_state_t;
    /// Default constructor (non-functional cache)
    DataCache();
    /// Constructor
    /// \param cache_path path to directory with cache info files
    /// \param cache_data_path path to directory with cache data files
    /// \param cache_link_path path used to create link in case
    /// cache_directory is visible under different name during actual usage
    /// \param id identifier used to claim files in cache
    /// \param cache_user owner of cahce (0 for public cache)
    DataCache(const std::string& cache_path,
	      const std::string& cache_data_path,
	      const std::string& cache_link_path,
	      const std::string& id, const Arc::User& cache_user);
    /// Copy constructor
    DataCache(const DataCache& cache);
    /// and destructor
    virtual ~DataCache();
    /// Prepare cache for downloading file. On success returns true.
    /// This function can block for long time if there is another process
    /// downloading same url.
    /// \param base_url url to assign to file in cache (file's identifier)
    /// \param available contains true on exit if file is already in cache
    bool start(const URL& base_url, bool& available);
    /// Returns path to file which contains/will contain content of
    /// assigned url.
    const std::string& file() const {
      return cache_file;
    }
    /// This method must be called after file was downloaded or download
    /// failed.
    /// \param failure true if download failed
    //bool stop(bool failure,bool invalidate);
    bool stop(int file_state = file_no_error);
    /// Must be called to create soft-link to cache file or to copy it.
    /// It's behavior depends on configuration. All necessary
    /// directories will be created. Returns false on error (usually
    /// that means soft-link already exists).
    /// \param link_path path for soft-link or new file.
    bool link(const std::string& link_path);
    /// \param user set owner of soft-link
    bool link(const std::string& link_path, const Arc::User& user);
    /// Do same as link() but always create copy
    bool copy(const std::string& link_path);
    bool copy(const std::string& link_path, const Arc::User& user);
    /// Remove some amount of oldest information from cache.
    /// Returns true on success.
    /// \param size amount to be removed (bytes)
    bool clean(unsigned long long int size = 1);
    /// Callback implementation to clean at least 1 byte.
    virtual bool cb(unsigned long long int size);
    /// Returns true if object is useable.
    operator bool() {
      return (cache_path.length() != 0);
    }
    /// Check if there is an information about creation time.
    bool CheckCreated() {
      return (creation_time != -1);
    }
    /// Set creation time.
    /// \param val creation time
    void SetCreated(Time val) {
      creation_time = val;
    }
    /// Get creation time.
    Time GetCreated() {
      return creation_time;
    }
    /// Check if there is an information about invalidation time.
    bool CheckValid() {
      return (expiration_time != -1);
    }
    /// Set invalidation time.
    /// \param val validity time
    void SetValid(Time val) {
      expiration_time = val;
    }
    /// Get invalidation time.
    Time GetValid() {
      return expiration_time;
    }
  };

} // namespace Arc

#endif // __ARC_DATACACHE_H__
