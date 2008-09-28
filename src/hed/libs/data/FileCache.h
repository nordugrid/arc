#ifndef FILECACHE_H_
#define FILECACHE_H_


#ifndef WIN32

#include <sstream>
#include <vector>

#include <arc/DateTime.h>

#include "FileCacheHash.h"

namespace Arc {

class Logger;

/**
 * Contains data on the parameters of a cache.
 */
struct CacheParameters {
  std::string cache_path;
  std::string cache_job_dir_path;
  std::string cache_link_path;
};

/**
 * FileCache provides an interface to all cache operations
 * to be used by external classes. An instance should be created
 * per job, and all files within the job are managed by that
 * instance. When it is decided a file should be downloaded to the
 * cache, Start() should be called, so that the cache file can be
 * prepared and locked. When a transfer has finished successfully,
 * Link() or Copy() should be called to create a hard link to
 * a per-job directory in the cache and then soft link, or copy
 * the file directly to the session directory so it can be accessed from
 * the user's job. Stop() must then be called to release any
 * locks on the cache file.
 * 
 * The cache directory, the directory for per-job hard links and the
 * optional directory to link to when the soft-links are made
 * are set in the global configuration file. The names of cache files
 * are formed from a hash of the URL specified as input to the job.
 * To ease the load on the file system, the cache files are split
 * into subdirectories based on the characters in the hash.
 * The length of each subdirectory and number of subdirectories
 * can be specified in the conf file. For example if these two
 * parameters are respectively 2 and 3 the file with hash
 * 76f11edda169848038efbd9fa3df5693 is stored in
 * 76/f1/1e/dda169848038efbd9fa3df5693. A cache filename can be found
 * by passing the URL to File().
 * 
 * A metadata file with the '.meta' suffix is stored next to each
 * cache file. This contains the URL corresponding to the cache
 * file and the expiry time, if it is available. For example
 * lfc://lfc1.ndgf.org//grid/atlas/test/test1 1208352933
 * 
 * While cache files are downloaded, they are locked by creating a
 * lock file with the '.lock' suffix next to the cache file. Calling
 * Start() creates this lock and Stop() releases it. All processes
 * calling Start() must wait until they successfully obtain the lock
 * before downloading can begin.
 * NB! This method of file locking only works for processes running on
 * the same host.
 */
class FileCache {
 private:
  /**
   * Vector of caches. Each entry defines a cache and specifies 
   * a cache directory, per-job directory and link path.
   */
  std::vector<struct CacheParameters> _caches;
  /**
   * path to directory with cache info files 
   */
  std::string _cache_path;
  /**
   * path to directory where per-job hard links are created
   */
  std::string _cache_job_dir_path;
  /**
   * path used to create link in case cache_directory is visible
   * under different name during actual usage 
   */
  std::string _cache_link_path;
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
   * Parameters for the cache directory structure
   */
  int _cache_dir_length;
  int _cache_dir_levels;
  /**
   * Our hostname (same as given by uname -n)
   */
  std::string _hostname;
  /**
   * Our pid
   */
  std::string _pid;
  /** 
   * The suffix to use for lock files
   */
  static const std::string CACHE_LOCK_SUFFIX;
  /** 
   * The suffix to use for meta files
   */
  static const std::string CACHE_META_SUFFIX;
  /**
   * Common code for constuctors
   */
  bool _init(std::vector<struct CacheParameters> caches,
             std::string id,
             uid_t job_uid,
             gid_t job_gid,
             int cache_dir_length,
             int cache_dir_levels);
  /**
   * Return the filename of the lock file associated to the given url
   */
  std::string _getLockFileName(std::string url);
  /**
   * Return the filename of the meta file associated to the given url
   */
  std::string _getMetaFileName(std::string url);
  /**
   * Return true if the lock on the cache file corresponding to
   * this url exists and is owned by this process
   */
  bool _checkLock(std::string url);
  /**
   * Generic method to make directories
   * @param dir directory to create
   * @param all_read if true, make the directory readable by all users,
   * if false, it is readable only by the user who created it.
   */
  bool _cacheMkDir(std::string dir, bool all_read);
  /**
   * Choose a cache to use from the list, based on the first character
   * of the url hash mod number of caches.
   * Returns the index of the cache to use in the list.
   */
  int _chooseCache(std::string hash);
  /**
   * Logger for messages
   */
  static Logger logger;
 public:
  /**
   * Create a new FileCache instance.
   * TODO: Take these parameters directly from the conf file?
   * @param cache_path path to directory with cache files
   * @param cache_job_dir_path path to where hard links should
   * be created in per-job directories. This path cannot be under
   * the main cache directory.
   * @param cache_link_path path used to create link in case 
   * cache directory is visible under different name during actual
   * usage. When linking from the session dir this path should be used
   * instead of cache_path.
   * @param id the job id. This is used to create the per-job dir
   * which the job's cache files will be hard linked from
   * @param job_uid owner of job. The per-job dir will only be
   * readable by this user
   * @param job_gid owner group of job
   * @param cache_dir_length the number of characters for each subdirectory
   * name in the cache hierarchy
   * @param cache_dir_levels the number of subdirectory levels in the cache
   */
  FileCache(std::string cache_path,
            std::string cache_job_dir_path,
            std::string cache_link_path,
            std::string id,
            uid_t job_uid,
            gid_t job_gid,
            int cache_dir_length,
            int cache_dir_levels);

  /**
   * Create a new FileCache instance with multiple cache dirs
   * @param caches a vector of CacheParameter structs describing
   * each cache.
   * @param id the job id. This is used to create the per-job dir
   * which the job's cache files will be hard linked from
   * @param job_uid owner of job. The per-job dir will only be
   * readable by this user
   * @param job_gid owner group of job
   * @param cache_dir_length the number of characters for each subdirectory
   * name in the cache hierarchy
   * @param cache_dir_levels the number of subdirectory levels in the cache
   */
  FileCache(std::vector<struct CacheParameters> caches,
            std::string id,
            uid_t job_uid,
            gid_t job_gid,
            int cache_dir_length,
            int cache_dir_levels);
  /**
   * Copy constructor
   */
  FileCache(const FileCache& cache);
  /**
   * Default constructor. Invalid cache.
   */
  FileCache() {_cache_path = "";};
  /**
   * Destructor
   */
  virtual ~FileCache(void);
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
   */
  bool Start(std::string url,bool &available, bool &is_locked);
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
   * @param link_path path to the session dir for soft-link or new file
   * @param url url of file to link to or copy
   */
  bool Link(std::string link_path, std::string url);
  /**
   * Copy the cache file corresponding to url to the dest_path 
   */
  bool Copy(std::string dest_path, std::string url);
  /**
   * Remove some amount of oldest information from cache. 
   * Returns true on success. Not implemented.
   * @param size amount to be removed (bytes)
   */
  bool Clean(unsigned long long int size = 1) {return false;};
  /**
   * Release claims on input files for the job specified by id.
   * For each cache directory the per-job directory with the
   * hard-links will be deleted.
   */
  bool Release();
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
  operator bool() { return (_caches.size() != 0); };
  /**
   * Return true if all attributes are equal
   */
  bool operator ==(const FileCache& a);

};

} // namespace Arc

#else

class FileCache {
 public:
  operator bool() { return false; };
};

#endif /*WIN32*/

#endif /*FILECACHE_H_*/
