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
  /**
   * \ingroup data
   * \headerfile FileCache.h arc/data/FileCache.h
   */
  struct CacheParameters {
    std::string cache_path;
    std::string cache_link_path;
  };

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
   * \ingroup data
   * \headerfile FileCache.h arc/data/FileCache.h
   */
  class FileCache {
   private:
    /// Map of urls and the cache they are mapped to/exist in
    std::map <std::string, struct CacheParameters> _cache_map;
    /// Vector of caches. Each entry defines a cache and specifies
    /// a cache directory and optional link path.
    std::vector<struct CacheParameters> _caches;
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
               const std::vector<std::string>& draining_caches,
               const std::string& id,
               uid_t job_uid,
               gid_t job_gid);
    /// Check the meta file corresponding to cache file filename is valid,
    /// and create one if it doesn't exist. Returns false if creation fails,
    /// and if it was due to being locked, is_locked is set to true.
    bool _checkMetaFile(const std::string& filename, const std::string& url, bool& is_locked);
    /// Create the meta file with the given content. Returns false and sets
    /// is_locked to true if the file is already locked.
    bool _createMetaFile(const std::string& meta_file, const std::string& content, bool& is_locked);
    /// Return the filename of the meta file associated to the given url
    std::string _getMetaFileName(const std::string& url);
    /// Get the hashed path corresponding to the given url
    std::string _getHash(const std::string& url) const;
    /// Choose a cache directory to use for this url, based on the free
    /// size of the cache directories. Returns the cache to use.
    struct CacheParameters _chooseCache(const std::string& url) const;
    /// Return the free space in GB at the given path
    float _getCacheInfo(const std::string& path) const;
    /// For cleaning up after a cache file was locked during Link()
    bool _cleanFilesAndReturnFalse(const std::string& hard_link_file, bool& locked);

    /// Logger for messages
    static Logger logger;

   public:
    /// Create a new FileCache instance with one cache directory.
    /**
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

    /// Create a new FileCache instance with multiple cache dirs.
    /**
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

    /// Create a new FileCache instance with multiple cache dirs and draining cache directories.
    /**
     * @param caches a vector of strings describing caches. The format
     * of each string is "cache_dir[ link_path]".
     * @param draining_caches Same format as caches. These are the
     * paths to caches which are to be drained.
     * @param id the job id. This is used to create the per-job dir
     * which the job's cache files will be hard linked from
     * @param job_uid owner of job. The per-job dir will only be
     * readable by this user
     * @param job_gid owner group of job
     */
    FileCache(const std::vector<std::string>& caches,
              const std::vector<std::string>& draining_caches,
              const std::string& id,
              uid_t job_uid,
              gid_t job_gid);

    /// Default constructor. Invalid cache.
    FileCache(): _uid(0),_gid(0) {
      _caches.clear();
    }

    /// Start preparing to cache the file specified by url.
    /**
     * Start() returns true if the file was successfully prepared. The
     * available parameter is set to true if the file already exists and in
     * this case Link() can be called immediately. If available is false the
     * caller should write the file and then call Link() followed by Stop().
     * Start() returns false if it was unable to prepare the cache file for any
     * reason. In this case the is_locked parameter should be checked and if
     * it is true the file is locked by another process and the caller should
     * try again later.
     *
     * @param url url that is being downloaded
     * @param available true on exit if the file is already in cache
     * @param is_locked true on exit if the file is already locked, ie
     * cannot be used by this process
     * @param delete_first If true then any existing cache file is deleted.
     * @return true if file is available or ready to be downloaded, false if
     * the file is already locked or preparing the cache failed.
     */
    bool Start(const std::string& url,
               bool& available,
               bool& is_locked,
               bool delete_first = false);

    /// Stop the cache after a file was downloaded.
    /**
     * This method (or stopAndDelete()) must be called after file was
     * downloaded or download failed, to release the lock on the
     * cache file. Stop() does not delete the cache file. It returns
     * false if the lock file does not exist, or another pid was found
     * inside the lock file (this means another process took over the
     * lock so this process must go back to Start()), or if it fails
     * to delete the lock file. It must only be called if the caller
     * actually downloaded the file. It must not be called if the file was
     * already available.
     * @param url the url of the file that was downloaded
     * @return true if the lock was successfully released.
     */
    bool Stop(const std::string& url);

    /// Stop the cache after a file was downloaded and delete the cache file.
    /**
     * Release the cache file and delete it, because for example a
     * failed download left an incomplete copy. This method also deletes
     * the meta file which contains the url corresponding to the cache file.
     * The logic of the return value is the same as Stop(). It must only be
     * called if the caller downloaded the file.
     * @param url the url corresponding to the cache file that has
     * to be released and deleted
     * @return true if the cache file and lock were successfully removed.
     */
    bool StopAndDelete(const std::string& url);

    /// Get the cache filename for the given URL.
    /**
     * @param url the URL to look for in the cache
     * @return the full pathname of the file in the cache which corresponds to
     * the given url.
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
     * Link() are deleted, try_again is set to true and Link() returns false.
     * The caller should then go back to Start(). If the caller has obtained a
     * write lock from Start() and then downloaded the file, it should set
     * holding_lock to true, in which case none of the above checks are
     * performed.
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
     * @return true if linking succeeded, false if an error occurred or the
     * file was locked or modified by another process during linking
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
     * @return false if any directory fails to be deleted
     */
    bool Release() const;

    /// Store a DN in the permissions cache for the given url.
    /**
     * Add the given DN to the list of cached DNs with the given expiry time.
     * @param url the url corresponding to the cache file to which we
     * want to add a cached DN
     * @param DN the DN of the user
     * @param expiry_time the expiry time of this DN in the DN cache
     * @return true if the DN was successfully added
     */
    bool AddDN(const std::string& url, const std::string& DN, const Time& expiry_time);

    /// Check if a DN exists in the permission cache and is still valid for the given url.
    /**
     * Check if the given DN is cached for authorisation and it is still valid.
     * @param url the url corresponding to the cache file for which we
     * want to check the cached DN
     * @param DN the DN of the user
     * @return true if the DN exists and is still valid
     */
    bool CheckDN(const std::string& url, const std::string& DN);

    /// Check if it is possible to obtain the creation time of a cache file.
    /**
     * @param url the url corresponding to the cache file for which we
     * want to know if the creation date exists
     * @return true if the file exists in the cache, since the creation time
     * is the creation time of the cache file.
     */
    bool CheckCreated(const std::string& url);

    /// Get the creation time of a cached file.
    /**
     * @param url the url corresponding to the cache file for which we
     * want to know the creation date
     * @return creation time of the file or 0 if the cache file does not exist
     */
    Time GetCreated(const std::string& url);

    /// Returns true if object is useable.
    operator bool() {
      return (!_caches.empty());
    };

    /// Returns true if all attributes are equal
    bool operator==(const FileCache& a);

  };

} // namespace Arc

#endif /*FILECACHE_H_*/
