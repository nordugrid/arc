// -*- indent-tabs-mode: nil -*-

#ifndef FILECACHE_H_
#define FILECACHE_H_

#include <sstream>
#include <vector>
#include <map>
#include <arc/DateTime.h>
// Independent implementaion of std roundf() 
#define roundf(num) ((fmod(num,1) < 0.5) ? floor(num):ceil(num))

#include "FileCacheHash.h"

namespace Arc {

  class Logger;

  /**
   * Contains data on the parameters of a cache.
   */
  struct CacheParameters {
    std::string cache_path;
    std::string cache_link_path;
  };

#ifndef WIN32

  /**
   * FileCache provides an interface to all cache operations
   * to be used by external classes. An instance should be created
   * per job, and all files within the job are managed by that
   * instance. When it is decided a file should be downloaded to the
   * cache, Start() should be called, so that the cache file can be
   * prepared and locked. When a transfer has finished successfully,
   * Link() should be called to create a hard link to
   * a per-job directory in the cache and then soft link, or copy
   * the file directly to the session directory so it can be accessed from
   * the user's job. Stop() must then be called to release any
   * locks on the cache file.
   *
   *
   * The cache directory(ies) and the optional directory to link to
   * when the soft-links are made are set in the global configuration
   * file. The names of cache files are formed from a hash of the URL
   * specified as input to the job. To ease the load on the file system,
   * the cache files are split into subdirectories based on the first
   * two characters in the hash. For example the file with hash
   * 76f11edda169848038efbd9fa3df5693 is stored in
   * 76/f11edda169848038efbd9fa3df5693. A cache filename can be found
   * by passing the URL to Find(). For more information on the
   * structure of the cache, see the Grid Manager Administration Guide.
   *
   * A metadata file with the '.meta' suffix is stored next to each
   * cache file. This contains the URL corresponding to the cache
   * file and the expiry time, if it is available. For example
   * lfc://lfc1.ndgf.org//grid/atlas/test/test1 20081007151045Z
   *
   * While cache files are downloaded, they are locked using the
   * FileLock class, which creates a lock file with the '.lock' suffix
   * next to the cache file. Calling Start() creates this lock and Stop()
   * releases it. All processes calling Start() must wait until they
   * successfully obtain the lock before downloading can begin. Once
   * a process obtains a lock it must later release it by calling
   * Stop() or StopAndDelete().
   */
  class FileCache {
  private:
    /**
     * Map of the cache files and the cache directories.
     */
    std::map <std::string, int> _cache_map;
    /**
     * Vector of caches. Each entry defines a cache and specifies
     * a cache directory and optional link path.
     */
    std::vector<struct CacheParameters> _caches;
    /**
     * Vector of remote caches. Each entry defines a cache and specifies 
     * a cache directory, per-job directory and link/copy information.
     */
    std::vector<struct CacheParameters> _remote_caches;
    /**
     * Vector of caches to be drained. 
     */
    std::vector<struct CacheParameters> _draining_caches;
    /**
     * identifier used to claim files, ie the job id
     */
    std::string _id;
    /**
     * owner:group corresponding to the user running the job.
     * The directory with hard links to cached files will be searchable only by this user
     */
    uid_t _uid;
    gid_t _gid;
    /**
     * Our hostname (same as given by uname -n)
     */
    std::string _hostname;
    /**
     * Our pid
     */
    std::string _pid;
    /**
     * The max and min used space for caches, as a percentage of the
     * file system
     */
    int _max_used;
    int _min_used;
    /**
     * The sub-dir of the cache for data
     */
    static const std::string CACHE_DATA_DIR;
    /**
     * The sub-dir of the cache for per-job links
     */
    static const std::string CACHE_JOB_DIR;
    /**
     * The length of each cache subdirectory
     */
    static const int CACHE_DIR_LENGTH;
    /**
     * The number of levels of cache subdirectories
     */
    static const int CACHE_DIR_LEVELS;
    /**
     * The suffix to use for meta files
     */
    static const std::string CACHE_META_SUFFIX;
    /**
     * Default validity time of cached DNs
     */
    static const int CACHE_DEFAULT_AUTH_VALIDITY;
    /**
     * Common code for constuctors
     */
    bool _init(std::vector<std::string> caches,
               std::vector<std::string> remote_caches,
               std::vector<std::string> draining_caches,
               std::string id,
               uid_t job_uid,
               gid_t job_gid,
               int cache_max = 100,
               int cache_min = 100);
    /**
     * Return the filename of the meta file associated to the given url
     */
    std::string _getMetaFileName(std::string url);
    /**
     * Generic method to make directories
     * @param dir directory to create
     * @param all_read if true, make the directory readable by all users,
     * if false, it is readable only by the user who created it.
     */
    bool _cacheMkDir(std::string dir, bool all_read);
   /**
     * Choose a cache directory to use for this url, based on the free 
     * size of the cache directories and cache_size limitation of the arc.conf
     * Returns the index of the cache to use in the list.
     */ 
    int _chooseCache(std::string url);
    /**
     * Retun the cache info < total space in KB, used space in KB>   
     */
    std::pair <unsigned long long , unsigned long long> _getCacheInfo(std::string path);
    /**
     * Logger for messages
     */
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
    FileCache(std::string cache_path,
              std::string id,
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
    FileCache(std::vector<std::string> caches,
              std::string id,
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
    FileCache(std::vector<std::string> caches,
              std::vector<std::string> remote_caches,
              std::vector<std::string> draining_caches, 
              std::string id,
              uid_t job_uid,
              gid_t job_gid,
              int cache_max = 100,
              int cache_min = 100);
    /**
     * Default constructor. Invalid cache.
     */
    FileCache() {
      _caches.clear();
    }

    /**
     * Prepare cache for downloading file, and lock the cached file.
     * On success returns true. If there is another process downloading
     * the same url, false is returned and is_locked is set to true.
     * In this case the client should wait and retry later. If the lock has
     * expired this process will take over the lock and the method will
     * return as if no lock was present, ie available and is_locked are
     * false.
     *
     * @param url url that is being downloaded
     * @param available true on exit if the file is already in cache
     * @param is_locked true on exit if the file is already locked, ie
     * cannot be used by this process
     * @param remote_caches Same format as caches. These are the
     * paths to caches which are under the control of other Grid
     * Managers and are read-only for this process.
     */
    bool Start(std::string url, bool& available, bool& is_locked, bool use_remote = true);
    /**
     * This method (or stopAndDelete) must be called after file was
     * downloaded or download failed, to release the lock on the
     * cache file. Stop() does not delete the cache file. It returns
     * false if the lock file does not exist, or another pid was found
     * inside the lock file (this means another process took over the
     * lock so this process must go back to Start()), or if it fails
     * to delete the lock file.
     * @param url the url of the file that was downloaded
     */
    bool Stop(std::string url);
    /**
     * Release the cache file and delete it, because for example a
     * failed download left an incomplete copy, or it has expired.
     * This method also deletes the meta file which contains the url
     * corresponding to the cache file. The logic of the return
     * value is the same as Stop().
     * @param url the url corresponding to the cache file that has
     * to be released and deleted
     */
    bool StopAndDelete(std::string url);
    /**
     * Returns the full pathname of the file in the cache which
     * corresponds to the given url.
     */
    std::string File(std::string url);
    /**
     * Create a hard-link to the per-job dir from
     * the cache dir, and then a soft-link from here to the
     * session directory. This is effectively 'claiming' the file
     * for the job, so even if the original cache file is deleted,
     * eg by some external process, the hard link still exists
     * until it is explicitly released by calling Release().
     *
     * If cache_link_path is set to "." then files will be
     * copied directly to the session directory rather than via
     * the hard link.
     *
     * The session directory is accessed under the uid passed in
     * the constructor if switch_user is true. Switching uid involves
     * holding a global lock, therefore care must be taken in a
     * multi-threaded environment.
     * @param link_path path to the session dir for soft-link or new file
     * @param url url of file to link to or copy
     * @param copy If true the file is copied rather than soft-linked
     * to the session dir
     * @param executable If true then file is copied and given execute
     * permissions in the session dir
     * @param switch_user If true then the session dir is accessed
     * under the uid passed in the constructor. Should be set to
     * false in DataMover.
     */
    bool Link(std::string link_path, std::string url, bool copy, bool executable, bool switch_user);
    /**
     * Copy the cache file corresponding to url to the dest_path.
     * The session directory is accessed under the uid passed in
     * the constructor, and switching uid involves holding a global
     * lock. Therefore care must be taken in a multi-threaded environment.
     *
     * This method is deprecated - Link() should be used instead with
     * copy set to true.
     */
    bool Copy(std::string dest_path, std::string url, bool executable = false);
    /**
     * Release claims on input files for the job specified by id.
     * For each cache directory the per-job directory with the
     * hard-links will be deleted.
     */
    bool Release();
    /**
     * Add the given DN to the list of cached DNs with the given expiry time
     * @param url the url corresponding to the cache file to which we
     * want to add a cached DN
     * @param DN the DN of the user
     * @param expiry_time the expiry time of this DN in the DN cache
     */
    bool AddDN(std::string url, std::string DN, Time expiry_time);
    /**
     * Check if the given DN is cached for authorisation.
     * @param url the url corresponding to the cache file for which we
     * want to check the cached DN
     * @param DN the DN of the user
     */
    bool CheckDN(std::string url, std::string DN);
    /**
     * Check if there is an information about creation time. Returns
     * true if the file exists in the cache, since the creation time
     * is the creation time of the cache file.
     * @param url the url corresponding to the cache file for which we
     * want to know if the creation date exists
     */
    bool CheckCreated(std::string url);
    /**
     * Get the creation time of a cached file. If the cache file does
     * not exist, 0 is returned.
     * @param url the url corresponding to the cache file for which we
     * want to know the creation date
     */
    Time GetCreated(std::string url);
    /**
     * Check if there is an information about expiry time.
     * @param url the url corresponding to the cache file for which we
     * want to know if the expiration time exists
     */
    bool CheckValid(std::string url);
    /**
     * Get expiry time of a cached file. If the time is not available,
     * a time equivalent to 0 is returned.
     * @param url the url corresponding to the cache file for which we
     * want to know the expiry time
     */
    Time GetValid(std::string url);
    /**
     * Set expiry time.
     * @param url the url corresponding to the cache file for which we
     * want to set the expiry time
     * @param val expiry time
     */
    bool SetValid(std::string url, Time val);
    /**
     * Returns true if object is useable.
     */
    operator bool() {
      return (_caches.size() != 0);
    };
    /**
     * Return true if all attributes are equal
     */
    bool operator==(const FileCache& a);

  };

#else

  class FileCache {
  public:
    FileCache(std::string cache_path,
              std::string id,
              int job_uid,
              int job_gid) {}
    FileCache(std::vector<std::string> caches,
              std::string id,
              int job_uid,
              int job_gid) {}
    FileCache(std::vector<std::string> caches,
              std::vector<std::string> remote_caches,
              std::vector<std::string> draining_caches, 
              std::string id,
              int job_uid,
              int job_gid,
              int cache_max=100,
              int cache_min=100) {}
    FileCache(const FileCache& cache) {}
    FileCache() {}
    bool Start(std::string url, bool& available, bool& is_locked, bool use_remote=true) { return false; }
    bool Stop(std::string url) { return false; }
    bool StopAndDelete(std::string url) {return false; }
    std::string File(std::string url) { return url; }
    bool Link(std::string link_path, std::string url, bool copy, bool executable, bool switch_user)  { return false; }
    bool Copy(std::string dest_path, std::string url, bool executable = false) { return false; }
    bool Release() { return false;}
    bool AddDN(std::string url, std::string DN, Time expiry_time) { return false;}
    bool CheckDN(std::string url, std::string DN) { return false; }
    bool CheckCreated(std::string url){ return false; }
    Time GetCreated(std::string url) { return Time(); }
    bool CheckValid(std::string url) { return false; }
    Time GetValid(std::string url)  { return Time(); }
    bool SetValid(std::string url, Time val) { return false; }
    operator bool() {
      return false;
    };
    bool operator==(const FileCache& a)  { return false; }
  };
#endif /*WIN32*/


} // namespace Arc

#endif /*FILECACHE_H_*/
