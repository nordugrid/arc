// -*- indent-tabs-mode: nil -*-

#ifndef FILECACHE_H_
#define FILECACHE_H_

#include <sstream>
#include <vector>
#include <map>
#include <set>
#include <arc/DateTime.h>
#include <arc/Logger.h>

#include "FileCacheHash.h"

namespace Arc {

  /// Contains data on the parameters of a cache.
  struct CacheParameters {
    std::string cache_path;
    std::string cache_link_path;
  };

#ifndef WIN32

  /// FileCache provides an interface to all cache operations.
  /**
   * When it is decided a file should be downloaded to the cache, Start()
   * should be called, so that the cache file can be prepared and locked if
   * necessary. If the file is already available it is not locked and Link()
   * can be called immediately to create a hard link to a per-job directory in
   * the cache and then soft link, or copy the file directly to the session
   * directory so it can be accessed from the user's job. If the file is not
   * available, Start() will lock it, then after downloading Link() can be
   * called. Stop() must then be called to release the lock. If the transfer
   * failed, StopAndDelete() can be called to clean up the cache file. After
   * a job has finished, Release() should be called to remove the hard links
   * created for that job.
   *
   * Cache files are locked for writing using the FileLock class, which
   * creates a lock file with the '.lock' suffix next to the cache file.
   * If Start() is called and the cache file is not already available, it
   * creates this lock and Stop() must be called to release it. All processes
   * calling Start() must wait until they successfully obtain the lock before
   * downloading can begin.
   *
   * The cache directory(ies) and the optional directory to link to when the
   * soft-links are made are set in the constructor. The names of cache files
   * are formed from an SHA-1 hash of the URL to cache. To ease the load on
   * the file system, the cache files are split into subdirectories based on
   * the first two characters in the hash. For example the file with hash
   * 76f11edda169848038efbd9fa3df5693 is stored in
   * 76/f11edda169848038efbd9fa3df5693. A cache filename can be found by
   * passing the URL to Find().  For more information on the structure of the
   * cache, see the ARC Computing Element System Administrator Guide
   * (NORDUGRID-MANUAL-20).
   */
  class FileCache {
   private:
    /// Map of urls and the cache they are mapped to/exist in
    std::map <std::string, struct CacheParameters> _cache_map;
    /// Vector of caches. Each entry defines a cache and specifies
    /// a cache directory and optional link path.
    std::vector<struct CacheParameters> _caches;
    /// Vector of remote caches. Each entry defines a cache and specifies
    /// a cache directory, per-job directory and link/copy information.
    std::vector<struct CacheParameters> _remote_caches;
    /// Vector of caches to be drained.
    std::vector<struct CacheParameters> _draining_caches;
    /// A list of URLs that have already been unlocked in Link(). URLs in
    /// this set will not be unlocked in Stop().
    std::set<std::string> _urls_unlocked;
    /// Identifier used to claim files, ie the job id
    std::string _id;
    /// uid corresponding to the user running the job.
    /// The directory with hard links to cached files will be searchable only by this user
    uid_t _uid;
    /// gid corresponding to the user running the job.
    gid_t _gid;
    /// The max used space for caches, as a percentage of the file system total
    int _max_used;
    /// The min used space for caches, as a percentage of the file system total
    int _min_used;

    /// The sub-dir of the cache for data
    static const std::string CACHE_DATA_DIR;
    /// The sub-dir of the cache for per-job links
    static const std::string CACHE_JOB_DIR;
    /// The length of each cache subdirectory
    static const int CACHE_DIR_LENGTH;
    /// The number of levels of cache subdirectories
    static const int CACHE_DIR_LEVELS;
    /// The suffix to use for meta files
    static const std::string CACHE_META_SUFFIX;
    /// Default validity time of cached DNs
    static const int CACHE_DEFAULT_AUTH_VALIDITY;
    /// Timeout on cache lock. The lock file is continually updated during the
    /// transfer so 15 mins with no transfer update should mean stale lock.
    static const int CACHE_LOCK_TIMEOUT;
    /// Timeout on lock on meta file
    static const int CACHE_META_LOCK_TIMEOUT;

    /// Common code for constructors
    bool _init(const std::vector<std::string>& caches,
               const std::vector<std::string>& remote_caches,
               const std::vector<std::string>& draining_caches,
               const std::string& id,
               uid_t job_uid,
               gid_t job_gid,
               int cache_max = 100,
               int cache_min = 100);
    /// Check the meta file corresponding to cache file filename is valid,
    /// and create one if it doesn't exist. Returns false if creation fails,
    /// and if it was due to being locked, is_locked is set to true.
    bool _checkMetaFile(const std::string& filename, const std::string& url, bool& is_locked);
    /// Return the filename of the meta file associated to the given url
    std::string _getMetaFileName(const std::string& url);
    /// Get the hashed path corresponding to the given url
    std::string _getHash(const std::string& url) const;
    /// Choose a cache directory to use for this url, based on the free
    /// size of the cache directories and cache_size limitation of the arc.conf
    /// Returns the index of the cache to use in the list.
    int _chooseCache(const std::string& url) const;
    /// Return the cache info < total space in KB, used space in KB>
    std::pair <unsigned long long , unsigned long long> _getCacheInfo(const std::string& path) const;
    /// For cleaning up after a cache file was locked during Link()
    bool _cleanFilesAndReturnFalse(const std::string& hard_link_file, bool& locked);

    /// Logger for messages
    static Logger logger;

   public:
    /**
     * Create a new FileCache instance.
     * @param cache_path The format is "cache_dir[ link_path]".
     * path is the path to the cache directory and the optional
     * link_path is used to create a link in case the
     * cache directory is visible under a different name during actual
     * usage. When linking from the session dir this path is used
     * instead of cache_path.
     * @param id the job id. This is used to create the per-job dir
     * which the job's cache files will be hard linked from
     * @param job_uid owner of job. The per-job dir will only be
     * readable by this user
     * @param job_gid owner group of job
     */
    FileCache(const std::string& cache_path,
              const std::string& id,
              uid_t job_uid,
              gid_t job_gid);

    /**
     * Create a new FileCache instance with multiple cache dirs
     * @param caches a vector of strings describing caches. The format
     * of each string is "cache_dir[ link_path]".
     * @param id the job id. This is used to create the per-job dir
     * which the job's cache files will be hard linked from
     * @param job_uid owner of job. The per-job dir will only be
     * readable by this user
     * @param job_gid owner group of job
     */
    FileCache(const std::vector<std::string>& caches,
              const std::string& id,
              uid_t job_uid,
              gid_t job_gid);       
    /**
     * Create a new FileCache instance with multiple cache dirs,
     * remote caches and draining cache directories.
     * @param caches a vector of strings describing caches. The format
     * of each string is "cache_dir[ link_path]".
     * @param remote_caches Same format as caches. These are the
     * paths to caches which are under the control of other Grid
     * Managers and are read-only for this process.
     * @param draining_caches Same format as caches. These are the
     * paths to caches which are to be drained.
     * @param id the job id. This is used to create the per-job dir
     * which the job's cache files will be hard linked from
     * @param job_uid owner of job. The per-job dir will only be
     * readable by this user
     * @param job_gid owner group of job
     * @param cache_max maximum used space by cache, as percentage
     * of the file system
     * @param cache_min minimum used space by cache, as percentage
     * of the file system
     */
    FileCache(const std::vector<std::string>& caches,
              const std::vector<std::string>& remote_caches,
              const std::vector<std::string>& draining_caches,
              const std::string& id,
              uid_t job_uid,
              gid_t job_gid,
              int cache_max = 100,
              int cache_min = 100);

    /// Default constructor. Invalid cache.
    FileCache() : _max_used(0), _min_used(0) {
      _caches.clear();
    }

    /// Start preparing to cache the file specified by url.
    /**
     * Start() returns true if the file was successfully prepared. The
     * available parameter is set to true if the file already exists and in
     * this case Link() can be called immediately. If available is false the
     * caller should write the file and then call Link() followed by Stop().
     * It returns false if it was unable to prepare the cache file for any
     * reason. In this case the is_locked parameter should be checked and if
     * it is true the file is locked by another process and the caller should
     * try again later.
     *
     * @param url url that is being downloaded
     * @param available true on exit if the file is already in cache
     * @param is_locked true on exit if the file is already locked, ie
     * cannot be used by this process
     * @param use_remote Whether to look to see if the file exists in a
     * remote cache. Can be set to false if for example a forced download
     * to cache is desired.
     * @param delete_first If true then any existing cache file is deleted.
     */
    bool Start(const std::string& url,
               bool& available,
               bool& is_locked,
               bool use_remote = true,
               bool delete_first = false);

    /// Stop the cache after a file was downloaded.
    /**
     * This method (or stopAndDelete) must be called after file was
     * downloaded or download failed, to release the lock on the
     * cache file. Stop() does not delete the cache file. It returns
     * false if the lock file does not exist, or another pid was found
     * inside the lock file (this means another process took over the
     * lock so this process must go back to Start()), or if it fails
     * to delete the lock file. It must only be called if the caller
     * holds the writing lock.
     * @param url the url of the file that was downloaded
     */
    bool Stop(const std::string& url);

    /// Stop the cache after a file was downloaded and delete the cache file.
    /**
     * Release the cache file and delete it, because for example a
     * failed download left an incomplete copy. This method also deletes
     * the meta file which contains the url corresponding to the cache file.
     * The logic of the return value is the same as Stop(). It must only be
     * called if the caller holds the writing lock.
     * @param url the url corresponding to the cache file that has
     * to be released and deleted
     */
    bool StopAndDelete(const std::string& url);

    /// Get the cache filename for the given URL.
    /**
     * Returns the full pathname of the file in the cache which
     * corresponds to the given url.
     * @param url the URL to look for in the cache
     */
    std::string File(const std::string& url);

    /// Link a cache file to the place it will be used.
    /**
     * Create a hard-link to the per-job dir from  the cache dir, and then a
     * soft-link from here to the session directory. This is effectively
     * 'claiming' the file for the job, so even if the original cache file is
     * deleted, eg by some external process, the hard link still exists until
     * it is explicitly released by calling Release().
     *
     * If cache_link_path is set to "." or copy or executable is true then
     * files will be copied directly to the session directory rather than
     * linked.
     *
     * After linking or copying, the cache file is checked for the presence of
     * a write lock, and whether the modification time has changed since
     * linking started (in case the file was locked, modified then released
     * during linking). If either of these are true the links created during
     * Link() are deleted and try_again is set to true. The caller should then
     * go back to Start(). If the caller has obtained a write lock from Start()
     * and then downloaded the file, it should set holding_lock to true, in
     * which case none of the above checks are performed.
     *
     * The session directory is accessed under the uid and gid passed in
     * the constructor.
     *
     * @param link_path path to the session dir for soft-link or new file
     * @param url url of file to link to or copy
     * @param copy If true the file is copied rather than soft-linked
     * to the session dir
     * @param executable If true then file is copied and given execute
     * permissions in the session dir
     * @param holding_lock Should be set to true if the caller already holds
     * the lock
     * @param try_again If after linking the cache file was found to be locked,
     * deleted or modified, then try_again is set to true
     */
    bool Link(const std::string& link_path,
              const std::string& url,
              bool copy,
              bool executable,
              bool holding_lock,
              bool& try_again);

    /// Release cache files used in this cache.
    /**
     * Release claims on input files for the job specified by id.
     * For each cache directory the per-job directory with the
     * hard-links will be deleted.
     */
    bool Release() const;

    /// Store a DN in the permissions cache for the given url.
    /**
     * Add the given DN to the list of cached DNs with the given expiry time.
     * @param url the url corresponding to the cache file to which we
     * want to add a cached DN
     * @param DN the DN of the user
     * @param expiry_time the expiry time of this DN in the DN cache
     */
    bool AddDN(const std::string& url, const std::string& DN, const Time& expiry_time);

    /// Check if a DN exists in the permission cache for the given url.
    /**
     * Check if the given DN is cached for authorisation.
     * @param url the url corresponding to the cache file for which we
     * want to check the cached DN
     * @param DN the DN of the user
     */
    bool CheckDN(const std::string& url, const std::string& DN);

    /// Check if it is possible to obtain the creation time of a cache file.
    /**
     * Returns true if the file exists in the cache, since the creation time
     * is the creation time of the cache file.
     * @param url the url corresponding to the cache file for which we
     * want to know if the creation date exists
     */
    bool CheckCreated(const std::string& url);

    /// Get the creation time of a cached file.
    /**
     * If the cache file does not exist, 0 is returned.
     * @param url the url corresponding to the cache file for which we
     * want to know the creation date
     */
    Time GetCreated(const std::string& url);

    /// Check if there is an expiry time of the given url in the cache.
    /**
     * @param url the url corresponding to the cache file for which we
     * want to know if the expiration time exists
     */
    bool CheckValid(const std::string& url);

    /// Get expiry time of a cached file.
    /**
     * If the time is not available, a time equivalent to 0 is returned.
     * @param url the url corresponding to the cache file for which we
     * want to know the expiry time
     */
    Time GetValid(const std::string& url);

    /// Set expiry time of a cache file.
    /**
     * @param url the url corresponding to the cache file for which we
     * want to set the expiry time
     * @param val expiry time
     */
    bool SetValid(const std::string& url, const Time& val);

    /// Returns true if object is useable.
    operator bool() {
      return (!_caches.empty());
    };

    /// Return true if all attributes are equal
    bool operator==(const FileCache& a);

  };

#else

  class FileCache {
  public:
    FileCache(const std::string& cache_path,
              const std::string& id,
              int job_uid,
              int job_gid) {}
    FileCache(const std::vector<std::string>& caches,
              const std::string& id,
              int job_uid,
              int job_gid) {}
    FileCache(const std::vector<std::string>& caches,
              const std::vector<std::string>& remote_caches,
              const std::vector<std::string>& draining_caches,
              const std::string& id,
              int job_uid,
              int job_gid,
              int cache_max=100,
              int cache_min=100) {}
    FileCache(const FileCache& cache) {}
    FileCache() {}
    bool Start(const std::string& url, bool& available, bool& is_locked, bool use_remote=true, bool delete_first=false) { return false; }
    bool Stop(const std::string& url) { return false; }
    bool StopAndDelete(const std::string& url) {return false; }
    std::string File(const std::string& url) { return url; }
    bool Link(const std::string& link_path, const std::string& url, bool copy, bool executable, bool holding_lock, bool& try_again)  { return false; }
    bool Release() const { return false;}
    bool AddDN(const std::string& url, const std::string& DN, const Time& expiry_time) { return false;}
    bool CheckDN(const std::string& url, const std::string& DN) { return false; }
    bool CheckCreated(const std::string& url){ return false; }
    Time GetCreated(const std::string& url) { return Time(); }
    bool CheckValid(const std::string& url) { return false; }
    Time GetValid(const std::string& url)  { return Time(); }
    bool SetValid(const std::string& url, const Time& val) { return false; }
    operator bool() {
      return false;
    };
    bool operator==(const FileCache& a)  { return false; }
  };
#endif /*WIN32*/


} // namespace Arc

#endif /*FILECACHE_H_*/
