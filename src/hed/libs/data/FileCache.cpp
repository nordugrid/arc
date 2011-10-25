// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifndef WIN32

#include <cerrno>
#include <cmath>
#include <algorithm>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/utsname.h>
#include <sys/statvfs.h>

#include <glibmm.h>

#include <arc/FileAccess.h>
#include <arc/FileUtils.h>
#include <arc/FileLock.h>
#include <arc/StringConv.h>
#include <arc/Utils.h>

#include "FileCache.h"

namespace Arc {

  const std::string FileCache::CACHE_DATA_DIR = "data";
  const std::string FileCache::CACHE_JOB_DIR = "joblinks";
  const int FileCache::CACHE_DIR_LENGTH = 2;
  const int FileCache::CACHE_DIR_LEVELS = 1;
  const std::string FileCache::CACHE_META_SUFFIX = ".meta";
  const int FileCache::CACHE_DEFAULT_AUTH_VALIDITY = 86400; // 24 h
  const int FileCache::CACHE_LOCK_TIMEOUT = 900; // 15 mins
  const int FileCache::CACHE_META_LOCK_TIMEOUT = 2;

  Logger FileCache::logger(Logger::getRootLogger(), "FileCache");

  FileCache::FileCache(const std::string& cache_path,
                       const std::string& id,
                       uid_t job_uid,
                       gid_t job_gid) {

    // make a vector of one item and call _init
    std::vector<std::string> caches;
    std::vector<std::string> remote_caches;
    std::vector<std::string> draining_caches;
    if (!cache_path.empty()) 
      caches.push_back(cache_path);

    // if problem in init, clear _caches so object is invalid
    if (!_init(caches, remote_caches, draining_caches, id, job_uid, job_gid))
      _caches.clear();
  }

  FileCache::FileCache(const std::vector<std::string>& caches,
                       const std::string& id,
                       uid_t job_uid,
                       gid_t job_gid) {

    std::vector<std::string> remote_caches;
    std::vector<std::string> draining_caches;

    // if problem in init, clear _caches so object is invalid
    if (!_init(caches, remote_caches, draining_caches, id, job_uid, job_gid))
      _caches.clear();
  }

  FileCache::FileCache(const std::vector<std::string>& caches,
                       const std::vector<std::string>& remote_caches,
                       const std::vector<std::string>& draining_caches,
                       const std::string& id,
                       uid_t job_uid,
                       gid_t job_gid,
                       int cache_max,
                       int cache_min) {
  
    // if problem in init, clear _caches so object is invalid
    if (!_init(caches, remote_caches, draining_caches, id, job_uid, job_gid, cache_max, cache_min))
      _caches.clear();
  }

  bool FileCache::_init(const std::vector<std::string>& caches,
                        const std::vector<std::string>& remote_caches,
                        const std::vector<std::string>& draining_caches,
                        const std::string& id,
                        uid_t job_uid,
                        gid_t job_gid,
                        int cache_max,
                        int cache_min) {

    _id = id;
    _uid = job_uid;
    _gid = job_gid;
    _max_used = cache_max;
    _min_used = cache_min;

    // for each cache
    for (int i = 0; i < (int)caches.size(); i++) {
      std::string cache = caches[i];
      std::string cache_path = cache.substr(0, cache.find(" "));
      if (cache_path.empty()) {
        logger.msg(ERROR, "No cache directory specified");
        return false;
      }
      std::string cache_link_path = "";
      if (cache.find(" ") != std::string::npos)
        cache_link_path = cache.substr(cache.find_last_of(" ") + 1, cache.length() - cache.find_last_of(" ") + 1);

      // tidy up paths - take off any trailing slashes
      if (cache_path.rfind("/") == cache_path.length() - 1)
        cache_path = cache_path.substr(0, cache_path.length() - 1);
      if (cache_link_path.rfind("/") == cache_link_path.length() - 1)
        cache_link_path = cache_link_path.substr(0, cache_link_path.length() - 1);

      // create cache dir and subdirs
      if (!DirCreate(std::string(cache_path + "/" + CACHE_DATA_DIR), S_IRWXU | S_IRGRP | S_IROTH | S_IXGRP | S_IXOTH, true)) {
        logger.msg(ERROR, "Cannot create directory \"%s\" for cache", cache_path + "/" + CACHE_DATA_DIR);
        return false;
      }
      if (!DirCreate(std::string(cache_path + "/" + CACHE_JOB_DIR), S_IRWXU | S_IRGRP | S_IROTH | S_IXGRP | S_IXOTH, true)) {
        logger.msg(ERROR, "Cannot create directory \"%s\" for cache", cache_path + "/" + CACHE_JOB_DIR);
        return false;
      }
      // add this cache to our list
      struct CacheParameters cache_params;
      cache_params.cache_path = cache_path;
      cache_params.cache_link_path = cache_link_path;
      _caches.push_back(cache_params);
    }
  
    // add remote caches
    for (int i = 0; i < (int)remote_caches.size(); i++) {
      std::string cache = remote_caches[i];
      std::string cache_path = cache.substr(0, cache.find(" "));
      if (cache_path.empty()) {
        logger.msg(ERROR, "No remote cache directory specified");
        return false;
      }
      std::string cache_link_path = "";
      if (cache.find(" ") != std::string::npos) cache_link_path = cache.substr(cache.find_last_of(" ")+1, cache.length()-cache.find_last_of(" ")+1);
      
      // tidy up paths - take off any trailing slashes
      if (cache_path.rfind("/") == cache_path.length()-1) cache_path = cache_path.substr(0, cache_path.length()-1);
      if (cache_link_path.rfind("/") == cache_link_path.length()-1) cache_link_path = cache_link_path.substr(0, cache_link_path.length()-1);
  
      // add this cache to our list
      struct CacheParameters cache_params;
      cache_params.cache_path = cache_path;
      cache_params.cache_link_path = cache_link_path;
      _remote_caches.push_back(cache_params);
    }
  
    // for each draining cache
    for (int i = 0; i < (int)draining_caches.size(); i++) {
      std::string cache = draining_caches[i];
      std::string cache_path = cache.substr(0, cache.find(" "));
      if (cache_path.empty()) {
        logger.msg(ERROR, "No cache directory specified");
        return false;
      }
      // tidy up paths - take off any trailing slashes
      if (cache_path.rfind("/") == cache_path.length()-1) cache_path = cache_path.substr(0, cache_path.length()-1);
  
      // add this cache to our list
      struct CacheParameters cache_params;
      cache_params.cache_path = cache_path;
      cache_params.cache_link_path = "";
      _draining_caches.push_back(cache_params);
    }
    return true;
  }

  bool FileCache::Start(const std::string& url, bool& available, bool& is_locked, bool use_remote, bool delete_first) {

    if (!(*this))
      return false;

    available = false;
    is_locked = false;
    _cache_map.erase(url);
    std::string filename = File(url);

    // create directory structure if required, only readable by GM user
    if (!DirCreate(filename.substr(0, filename.rfind("/")), S_IRWXU, true))
      return false;

    // check if a lock file exists and whether it is still valid
    struct stat lockStat;
    if (FileStat(std::string(filename+FileLock::getLockSuffix()).c_str(), &lockStat, false)) {
      FileLock lock(filename, CACHE_LOCK_TIMEOUT);
      bool lock_removed = false;
      if (lock.acquire(lock_removed)) {
        // if lock was invalid delete cache file
        if (lock_removed && !FileDelete(filename.c_str()) && errno != ENOENT) {
          logger.msg(ERROR, "Failed to delete stale cache file %s: %s", filename, StrError(errno));
        }
        if (!lock.release()) {
          logger.msg(WARNING, "Failed to release lock on file %s", filename);
          is_locked = true;
          return false;
        }
      }
      else {
        is_locked = true;
        return false;
      }
    }

    // now check if the cache file is there already
    struct stat fileStat;
    if (FileStat(filename, &fileStat, true)) {
      available = true;
    }
    // if the file is not there. check remote caches
    else if (errno == ENOENT) {
      if (use_remote && !_remote_caches.empty() && !delete_first) {

        // get the hash of the url
        std::string hash(_getHash(url));
        std::string remote_cache_file;
        std::string remote_cache_link;
        // store remote file modification time to compare if copy fails
        Arc::Time remote_mod_time;
        // go through remote caches and try to find file
        std::vector<struct CacheParameters>::iterator it = _remote_caches.begin();
        for (; it != _remote_caches.end(); it++) {
          std::string remote_file = it->cache_path+"/"+CACHE_DATA_DIR+"/"+hash;
          if (FileStat(remote_file, &fileStat, true)) {
            remote_cache_file = remote_file;
            remote_cache_link = it->cache_link_path;
            remote_mod_time = Arc::Time(fileStat.st_mtime);
            break;
          }
        }
        if (!remote_cache_file.empty()) {
        
          logger.msg(INFO, "Found file %s in remote cache at %s", url, remote_cache_file);

          // check meta file - if fails use local file
          if (_checkMetaFile(remote_cache_file, url, is_locked)) {

            // check if a lock file exists and whether it is still valid - if
            // it is valid then just download to local cache
            bool use_local = false;
            if (FileStat(std::string(remote_cache_file+FileLock::getLockSuffix()).c_str(), &lockStat, false)) {

              FileLock remote_lock(remote_cache_file, CACHE_LOCK_TIMEOUT);
              bool remote_lock_removed = false;
              if (remote_lock.acquire(remote_lock_removed)) {
                // if lock was invalid delete cache file
                if (remote_lock_removed) {
                  use_local = true;
                  if (!FileDelete(remote_cache_file.c_str()) && errno != ENOENT) {
                    logger.msg(WARNING, "Failed to delete stale remote cache file %s: %s", remote_cache_file, StrError(errno));
                  }
                }
                if (!remote_lock.release()) {
                  logger.msg(WARNING, "Failed to release lock on remote cache file %s", remote_cache_file);
                  use_local = true;
                }
              } else {
                use_local = true;
              }
            }
            if (!use_local) {
              // ok to use remote file - now decide whether to replicate to local cache
              if (remote_cache_link == "replicate") {

                // check local cache meta file
                if (!_checkMetaFile(filename, url, is_locked)) return false;

                // acquire lock on local cache
                FileLock lock(filename, CACHE_LOCK_TIMEOUT);
                bool lock_removed = false;
                if (!lock.acquire(lock_removed)) {
                  logger.msg(INFO, "Failed to obtain lock on cache file %s", filename);
                  is_locked = true;
                  return false;
                }

                // we have the lock, if there was a stale lock remove cache file to be safe
                if (lock_removed && !FileDelete(filename) && errno != ENOENT) {
                  logger.msg(ERROR, "Error removing cache file %s: %s", filename, StrError(errno));
                  if (!lock.release())
                    logger.msg(ERROR, "Failed to remove lock on %s. Some manual intervention may be required", filename);
                  return false;
                }

                // copy the file to the local cache, remove lock and exit with available=true
                logger.msg(VERBOSE, "Replicating file %s to local cache file %s", remote_cache_file, filename);

                if (!FileCopy(remote_cache_file, filename)) {
                  logger.msg(ERROR, "Failed to copy file %s to %s: %s", remote_cache_file, filename, StrError(errno));
                  // it could have failed because another process deleted the remote file
                  struct stat remoteStat;
                  if (!FileStat(remote_cache_file, &remoteStat, false) ||
                      Arc::Time(remoteStat.st_mtime) > remote_mod_time) {
                    logger.msg(WARNING, "Replicating file %s from remote cache failed due to source being deleted or modified", remote_cache_file);
                    if (!FileDelete(filename) && errno != ENOENT)
                      logger.msg(ERROR, "Failed to delete bad copy of remote cache file %s at %s: %s", remote_cache_file, filename, StrError(errno));
                  }
                  if (!lock.release())
                    logger.msg(ERROR, "Failed to remove lock on %s. Some manual intervention may be required", remote_cache_file);
                  return false;
                }
                if (!lock.release())
                  logger.msg(ERROR, "Failed to remove lock on %s. Some manual intervention may be required", remote_cache_file);
              }
              else {
                // set remote cache in the cache map
                _cache_map[url] = *it;
              }
              available = true;
              return true;
            }
          }
        }
      }
    }
    else {
      // this is ok, we will download again
      logger.msg(WARNING, "Failed looking up attributes of cached file: %s", StrError(errno));
    }
    if (!available || delete_first) { // lock in preparation for writing
      FileLock lock(filename, CACHE_LOCK_TIMEOUT);
      bool lock_removed;
      if (!lock.acquire(lock_removed)) {
        logger.msg(INFO, "Failed to obtain lock on cache file %s", filename);
        is_locked = true;
        return false;
      }

      // we have the lock, if there was a stale lock or the file was requested
      // to be deleted, remove cache file
      if (lock_removed || delete_first) {
        if (!FileDelete(filename) && errno != ENOENT) {
          logger.msg(ERROR, "Error removing cache file %s: %s", filename, StrError(errno));
          if (!lock.release())
            logger.msg(ERROR, "Failed to remove lock on %s. Some manual intervention may be required", filename);
          return false;
        }
        available = false;
      }
    }
    // create the meta file to store the URL, if it does not exist
    if (!_checkMetaFile(filename, url, is_locked)) {
      // release locks if acquired
      if (!available) {
        FileLock lock(filename, CACHE_LOCK_TIMEOUT);
        if (!lock.release()) logger.msg(ERROR, "Failed to remove lock on %s. Some manual intervention may be required", filename);
      }
      return false;
    }
    return true;
  }

  bool FileCache::Stop(const std::string& url) {

    if (!(*this))
      return false;

    // check if already unlocked in Link()
    if (_urls_unlocked.find(url) == _urls_unlocked.end()) {

      std::string filename(File(url));
      // delete the lock
      FileLock lock(filename);
      if (!lock.release()) {
        logger.msg(ERROR, "Failed to unlock file %s: %s. Manual intervention may be required", filename, StrError(errno));
        return false;
      }
    }
    return true;
  }

  bool FileCache::StopAndDelete(const std::string& url) {

    if (!(*this))
      return false;

    std::string filename = File(url);
    FileLock lock(filename, CACHE_LOCK_TIMEOUT);

    // first check that the lock is still valid before deleting anything
    if (lock.check() != 0) {
      logger.msg(ERROR, "Invalid lock on file %s", filename);
      return false;
    }

    // delete the meta file - not critical so don't fail on error
    if (!FileDelete(_getMetaFileName(url)))
      logger.msg(ERROR, "Failed to remove .meta file %s: %s", _getMetaFileName(url), StrError(errno));

    // delete the cache file
    if (!FileDelete(filename) && errno != ENOENT) {
      // leave the lock file so that a bad cache file is not used next time
      logger.msg(ERROR, "Error removing cache file %s: %s", filename, StrError(errno));
      return false;
    }

    // delete the lock file last
    if (!lock.release()) {
      logger.msg(ERROR, "Failed to unlock file %s: %s. Manual intervention may be required", filename, StrError(errno));
      return false;
    }

    return true;
  }

  std::string FileCache::File(const std::string& url) {

    if (!(*this))
      return "";

    // get the hash of the url
    std::string hash(_getHash(url));

    // look up the cache map to see if the file is already in
    std::map <std::string, struct CacheParameters>::iterator iter = _cache_map.find(url) ;
    if (iter != _cache_map.end()) {
      return _cache_map[url].cache_path + "/" + CACHE_DATA_DIR + "/" + hash;
    }
  
    // else choose a new cache and assign the file to it
    int chosen_cache = _chooseCache(url);
    std::string path = _caches[chosen_cache].cache_path + "/" + CACHE_DATA_DIR + "/" + hash;
  
    // update the cache map with the new file
    _cache_map.insert(std::make_pair(url, _caches[chosen_cache]));
    return path;
  }

  bool FileCache::Link(const std::string& dest_path,
                       const std::string& url,
                       bool copy,
                       bool executable,
                       bool holding_lock,
                       bool& try_again) {

    if (!(*this))
      return false;

    try_again = false;
    std::string cache_file = File(url);
    std::string hard_link_path;
    std::string cache_link_path;

    // Mod time of cache file
    Arc::Time modtime;

    // check the original file exists and if so in which cache
    struct stat fileStat;
    if (FileStat(cache_file, &fileStat, false)) {
      // look up the map to get the cache params this url is mapped to (set in File())
      std::map <std::string, struct CacheParameters>::iterator iter = _cache_map.find(url);
      if (iter == _cache_map.end()) {
        logger.msg(ERROR, "Cache not found for file %s", cache_file);
        return false;
      }
      hard_link_path = _cache_map[url].cache_path + "/" + CACHE_JOB_DIR + "/" +_id;
      cache_link_path = _cache_map[url].cache_link_path;
      modtime = Arc::Time(fileStat.st_mtime);
      // if modtime is now (to second granularity) sleep for a second to avoid
      // race condition with another process locking, modifying and unlocking
      // during link
      if (!holding_lock && modtime.GetTime() == Arc::Time().GetTime()) {
        logger.msg(VERBOSE, "Cache file %s was modified in the last second, sleeping 1 second to avoid race condition", cache_file);
        sleep(1);
      }
    }
    else if (errno == ENOENT) {
      if (holding_lock || _remote_caches.empty()) {
        logger.msg(WARNING, "Cache file %s does not exist", cache_file);
        try_again = true;
        return false;
      }
      // file may have been found in a remote cache
      std::string hash(_getHash(url));
      // go through remote caches and try to find file
      for (std::vector<struct CacheParameters>::iterator it = _remote_caches.begin(); it != _remote_caches.end(); it++) {
        std::string remote_file = it->cache_path+"/"+CACHE_DATA_DIR+"/"+hash;
        if (FileStat(remote_file, &fileStat, true)) {
          cache_file = remote_file;
          hard_link_path = it->cache_path + "/" + CACHE_JOB_DIR + "/" +_id;
          cache_link_path = it->cache_link_path;
          modtime = Arc::Time(fileStat.st_mtime);
          break;
        }
      }
      if (hard_link_path.empty()) {
        // another process could have deleted it, so try again
        logger.msg(WARNING, "Cache file for %s not found in any local or remote cache", url);
        try_again = true;
        return false;
      }
      logger.msg(VERBOSE, "Using remote cache file %s for url %s", cache_file, url);
    }
    else {
      logger.msg(ERROR, "Error accessing cache file %s: %s", cache_file, StrError(errno));
      return false;
    }

    // create per-job hard link dir if necessary, making the final dir readable only by the job user
    if (!DirCreate(hard_link_path, S_IRWXU, true)) {
      logger.msg(ERROR, "Cannot create directory \"%s\" for per-job hard links", hard_link_path);
      return false;
    }
    if (chown(hard_link_path.c_str(), _uid, _gid) != 0) {
      logger.msg(ERROR, "Cannot change owner of %s: %s ", hard_link_path, StrError(errno));
      return false;
    }

    std::string filename = dest_path.substr(dest_path.rfind("/") + 1);
    std::string hard_link_file = hard_link_path + "/" + filename;
    std::string session_dir = dest_path.substr(0, dest_path.rfind("/"));

    // make the hard link
    if (!FileLink(cache_file, hard_link_file, false)) {
      // if the link we want to make already exists, delete and make new one
      if (errno == EEXIST) {
        if (!FileDelete(hard_link_file)) {
          logger.msg(ERROR, "Failed to remove existing hard link at %s: %s", hard_link_file, StrError(errno));
          return false;
        }
        if (!FileLink(cache_file, hard_link_file, false)) {
          logger.msg(ERROR, "Failed to create hard link from %s to %s: %s", hard_link_file, cache_file, StrError(errno));
          return false;
        }
      }
      else if (errno == ENOENT) {
        // another process could have deleted the cache file, so try again
        logger.msg(WARNING, "Cache file %s not found", cache_file);
        try_again = true;
        return false;
      }
      else {
        logger.msg(ERROR, "Failed to create hard link from %s to %s: %s", hard_link_file, cache_file, StrError(errno));
        return false;
      }
    }
    // ensure the hard link is readable by all and owned by root (or GM user)
    // to make cache file immutable but readable by mapped user

    // Using chmod as a temporary solution until it is possible to
    // specify mode when writing with File DMC
    if (chmod(hard_link_file.c_str(), S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH) != 0) {
      logger.msg(ERROR, "Failed to change permissions or set owner of hard link %s: %s", hard_link_file, StrError(errno));
      return false;
    }

    // Hard link is created so release any locks on the cache file
    if (holding_lock) {
      FileLock lock(cache_file, CACHE_LOCK_TIMEOUT);
      if (!lock.release()) {
        logger.msg(WARNING, "Failed to release lock on cache file %s", cache_file);
      } else {
        _urls_unlocked.insert(url);
      }
    }
    else {
      // check that the cache file wasn't locked or modified during the link/copy
      // if we are holding the lock, assume none of these checks are necessary
      struct stat lockStat;
      // check if lock file exists
      if (FileStat(cache_file+FileLock::getLockSuffix(), &lockStat, false)) {
        logger.msg(WARNING, "Cache file %s was locked during link/copy, must start again", cache_file);
        return _cleanFilesAndReturnFalse(hard_link_file, try_again);
      }
      // check cache file is still there
      if (!FileStat(cache_file, &fileStat, false)) {
        logger.msg(WARNING, "Cache file %s was deleted during link/copy, must start again", cache_file);
        return _cleanFilesAndReturnFalse(hard_link_file, try_again);
      }
      // finally check the mod time of the cache file
      if (Arc::Time(fileStat.st_mtime) > modtime) {
        logger.msg(WARNING, "Cache file %s was modified while linking, must start again", cache_file);
        return _cleanFilesAndReturnFalse(hard_link_file, try_again);
      }
    }

    // make necessary dirs for the soft link
    // the session dir should already exist but in the case of arccp with cache it may not
    // here we use the mapped user to access session dir

    if (!DirCreate(session_dir, _uid, _gid, S_IRWXU, true)) {
      logger.msg(ERROR, "Failed to create directory %s: %s", session_dir, StrError(errno));
      return false;
    }

    // if _cache_link_path is '.' or copy or executable is true then copy instead
    // "replicate" should not be possible, but including just in case
    if (copy || executable || cache_link_path == "." || cache_link_path == "replicate") {
      if (!FileCopy(hard_link_file, dest_path, _uid, _gid)) {
        logger.msg(ERROR, "Failed to copy file %s to %s: %s", hard_link_file, dest_path, StrError(errno));
        return false;
      }
      if (executable) {
        FileAccess fa;
        if (!fa) {
          logger.msg(ERROR, "Failed to set executable bit on file %s", dest_path);
          return false;
        }
        if (!fa.setuid(_uid, _gid) || !fa.chmod(dest_path, S_IRWXU)) {
          errno = fa.geterrno();
          logger.msg(ERROR, "Failed to set executable bit on file %s: %s", dest_path, StrError(errno));
          return false;
        }
      }
    }
    else {
      // make the soft link, changing the target if cache_link_path is defined
      if (!cache_link_path.empty())
        hard_link_file = cache_link_path + "/" + CACHE_JOB_DIR + "/" + _id + "/" + filename;

      if (!FileLink(hard_link_file, dest_path, _uid, _gid, true)) {
        // if the link we want to make already exists, delete and make new one
        if (errno == EEXIST) {
          if (!FileDelete(dest_path, _uid, _gid)) {
            logger.msg(ERROR, "Failed to remove existing symbolic link at %s: %s", dest_path, StrError(errno));
            return false;
          }
          if (!FileLink(hard_link_file, dest_path, _uid, _gid, true)) {
            logger.msg(ERROR, "Failed to create symbolic link from %s to %s: %s", dest_path, hard_link_file, StrError(errno));
            return false;
          }
        }
        else {
          logger.msg(ERROR, "Failed to create symbolic link from %s to %s: %s", dest_path, hard_link_file, StrError(errno));
          return false;
        }
      }
    }
    // file was safely linked/copied
    return true;
  }

  bool FileCache::Release() const {

    // go through all caches (including remote caches and draining caches)
    // and remove per-job dirs for our job id
    std::vector<std::string> job_dirs;
    for (int i = 0; i < (int)_caches.size(); i++)
      job_dirs.push_back(_caches[i].cache_path + "/" + CACHE_JOB_DIR + "/" + _id);
    for (int i = 0; i < (int)_remote_caches.size(); i++)
      job_dirs.push_back(_remote_caches[i].cache_path + "/" + CACHE_JOB_DIR + "/" + _id);
    for (int i = 0; i < (int)_draining_caches.size(); i++)
      job_dirs.push_back(_draining_caches[i].cache_path + "/" + CACHE_JOB_DIR + "/" + _id); 

    for (int i = 0; i < (int)job_dirs.size(); i++) {
      std::string job_dir = job_dirs[i];
      // check if job dir exists
      struct stat fileStat;
      if (!FileStat(job_dir, &fileStat, true) && errno == ENOENT)
        continue;

      logger.msg(DEBUG, "Removing %s", job_dir);
      if (!DirDelete(job_dir)) {
        logger.msg(ERROR, "Failed to remove cache per-job dir %s: %s", job_dir, StrError(errno));
        return false;
      }
    }
    return true;
  }

  bool FileCache::AddDN(const std::string& url, const std::string& DN, const Time& exp_time) {

    if (DN.empty())
      return false;
    Time expiry_time(exp_time);
    if (expiry_time == Time(0))
      expiry_time = Time(time(NULL) + CACHE_DEFAULT_AUTH_VALIDITY);

    // add DN to the meta file. If already there, renew the expiry time
    std::string meta_file = _getMetaFileName(url);
    struct stat fileStat;
    if (!FileStat(meta_file, &fileStat, true)) {
      logger.msg(ERROR, "Error reading meta file %s: %s", meta_file, StrError(errno));
      return false;
    }
    std::list<std::string> lines;
    if (!FileRead(meta_file, lines)) {
      logger.msg(ERROR, "Error opening meta file %s", meta_file);
      return false;
    }

    if (lines.empty()) {
      logger.msg(ERROR, "meta file %s is empty", meta_file);
      return false;
    }
    // first line contains the URL
    std::list<std::string>::iterator line = lines.begin();
    std::string first_line(*line);

    // check for possible hash collisions between URLs
    // taking into account old meta file format
    if (first_line != url && first_line.substr(0, first_line.rfind(' ')) != url) {
      logger.msg(ERROR, "File %s is already cached at %s under a different URL: %s - will not add DN to cached list",
                 url, File(url), first_line);
      return false;
    }

    std::string newdnlist(first_line + '\n');

    // second line may contain validity time
    ++line;
    if (line != lines.end()) {
      std::string::size_type space_pos = line->rfind(' ');
      if (space_pos == std::string::npos) {
        newdnlist += std::string(*line + '\n');
        ++line;
      }

      // check list of DNs for expired and this DN
      for (; line != lines.end(); ++line) {
        space_pos = line->rfind(' ');
        if (space_pos == std::string::npos) {
          logger.msg(WARNING, "Bad format detected in file %s, in line %s", meta_file, *line);
          continue;
        }
        // remove expired DNs (after some grace period)
        if (line->substr(0, space_pos) != DN) {
          Time exp_time(line->substr(space_pos + 1));
          if (exp_time > Time(time(NULL) - CACHE_DEFAULT_AUTH_VALIDITY))
            newdnlist += std::string(*line + '\n');
        }
      }
    }
    newdnlist += std::string(DN + ' ' + expiry_time.str(MDSTime) + '\n');

    // write everything back to the file
    FileLock meta_lock(meta_file, CACHE_META_LOCK_TIMEOUT);
    if (!meta_lock.acquire()) {
      // not critical if writing fails
      logger.msg(INFO, "Could not acquire lock on meta file %s", meta_file);
      return false;
    }
    if (!FileCreate(meta_file, newdnlist)) {
      logger.msg(ERROR, "Error opening meta file for writing %s", meta_file);
      meta_lock.release();
      return false;
    }
    meta_lock.release();
    return true;
  }

  bool FileCache::CheckDN(const std::string& url, const std::string& DN) {

    if (DN.empty())
      return false;

    std::string meta_file = _getMetaFileName(url);
    struct stat fileStat;
    if (!FileStat(meta_file, &fileStat, true)) {
      if (errno != ENOENT)
        logger.msg(ERROR, "Error reading meta file %s: %s", meta_file, StrError(errno));
      return false;
    }
    std::list<std::string> lines;
    if (!FileRead(meta_file, lines)) {
      logger.msg(ERROR, "Error opening meta file %s", meta_file);
      return false;
    }
    if (lines.empty()) {
      logger.msg(ERROR, "meta file %s is empty", meta_file);
      return false;
    }

    // read list of DNs until we find this one
    for (std::list<std::string>::iterator line = lines.begin(); line != lines.end(); ++line) {
      std::string::size_type space_pos = line->rfind(' ');
      if (line->substr(0, space_pos) == DN) {
        std::string exp_time = line->substr(space_pos + 1);
        if (Time(exp_time) > Time()) {
          logger.msg(VERBOSE, "DN %s is cached and is valid until %s for URL %s", DN, Time(exp_time).str(), url);
          return true;
        }
        else {
          logger.msg(VERBOSE, "DN %s is cached but has expired for URL %s", DN, url);
          return false;
        }
      }
    }
    return false;
  }

  bool FileCache::CheckCreated(const std::string& url) {

    // check the cache file exists - if so we can get the creation date
    // follow symlinks
    std::string cache_file = File(url);
    struct stat fileStat;
    return FileStat(cache_file, &fileStat, true);
  }

  Time FileCache::GetCreated(const std::string& url) {

    // check the cache file exists
    std::string cache_file = File(url);
    // follow symlinks
    struct stat fileStat;
    if (!FileStat(cache_file, &fileStat, true)) {
      if (errno == ENOENT)
        logger.msg(ERROR, "Cache file %s does not exist", cache_file);
      else
        logger.msg(ERROR, "Error accessing cache file %s: %s", cache_file, StrError(errno));
      return 0;
    }

    time_t mtime = fileStat.st_mtime;
    if (mtime <= 0)
      return Time(0);
    return Time(mtime);
  }

  bool FileCache::CheckValid(const std::string& url) {
    return (GetValid(url) != Time(0));
  }

  Time FileCache::GetValid(const std::string& url) {

    // open meta file and pick out expiry time if it exists
    std::string meta_file(_getMetaFileName(url));
    struct stat fileStat;
    if (!FileStat(meta_file, &fileStat, true)) {
      if (errno != ENOENT)
        logger.msg(ERROR, "Error reading meta file %s: %s", meta_file, StrError(errno));
      return Time(0);
    }
    std::list<std::string> lines;
    if (!FileRead(meta_file, lines)) {
      logger.msg(ERROR, "Error opening meta file %s", meta_file);
      return Time(0);
    }
    if (lines.empty()) {
      logger.msg(ERROR, "meta file %s is empty", meta_file);
      return Time(0);
    }

    // if the file contains only one line, we don't have an expiry time
    if (lines.size() == 1)
      return Time(0);

    // second line may contain expiry time in form 20080101123456Z,
    // but it could be a cached DN
    std::string meta_str(*(lines.erase(lines.begin())));

    if (meta_str.find(' ') != std::string::npos || meta_str.length() != 15)
      return Time(0);

    // convert to Time object
    return Time(meta_str);
  }

  bool FileCache::SetValid(const std::string& url, const Time& val) {

    std::string meta_file(_getMetaFileName(url));
    std::string file_data(url + '\n' + val.str(MDSTime));
    FileLock meta_lock(meta_file, CACHE_META_LOCK_TIMEOUT);
    if (!meta_lock.acquire()) {
      // not critical if writing fails
      logger.msg(INFO, "Could not acquire lock on meta file %s", meta_file);
      return false;
    }
    if (!FileCreate(meta_file, file_data)) {
      logger.msg(ERROR, "Error opening meta file for writing %s", meta_file);
      meta_lock.release();
      return false;
    }
    meta_lock.release();
    return true;
  }

  bool FileCache::operator==(const FileCache& a) {
    if (a._caches.size() != _caches.size()) return false;
    for (int i = 0; i < (int)a._caches.size(); i++) {
      if (a._caches.at(i).cache_path != _caches.at(i).cache_path) return false;
      if (a._caches.at(i).cache_link_path != _caches.at(i).cache_link_path) return false;
    }
    return (a._id == _id &&
            a._uid == _uid &&
            a._gid == _gid);
  }

  bool FileCache::_checkMetaFile(const std::string& filename, const std::string& url, bool& is_locked) {
    std::string meta_file(filename + CACHE_META_SUFFIX);
    struct stat fileStat;

    if (FileStat(meta_file, &fileStat, true)) {
      // check URL inside file for possible hash collisions
      std::list<std::string> lines;
      if (!FileRead(meta_file, lines)) {
        // file was probably deleted by another process - try again
        logger.msg(WARNING, "Failed to read meta file %s", meta_file);
        is_locked = true;
        return false;
      }
      if (lines.empty()) {
        logger.msg(WARNING, "Meta file %s is empty, will recreate", meta_file);
        FileLock meta_lock(meta_file, CACHE_META_LOCK_TIMEOUT);
        if (!meta_lock.acquire()) {
          logger.msg(WARNING, "Failed to acquire lock on cache meta file %s", meta_file);
          is_locked = true;
          return false;
        }
        if (!FileCreate(meta_file, std::string(url + '\n'))) {
          logger.msg(WARNING, "Failed to create meta file %s", meta_file);
        }
        meta_lock.release(); // there is a small timeout so don't bother to report error
      }
      else {
        std::string meta_str = lines.front();
        // check new and old format for validity time - if old change to new
        if (meta_str != url) {
          if (meta_str.substr(0, meta_str.rfind(' ')) != url) {
            logger.msg(ERROR, "File %s is already cached at %s under a different URL: %s - this file will not be cached",
                       url, filename, meta_str);
            return false;
          } else {
            logger.msg(VERBOSE, "Changing old validity time format to new in %s", meta_file);
            std::string new_meta(url + '\n' + meta_str.substr(meta_str.rfind(' ')+1) + '\n');
            lines.pop_front();
            for (std::list<std::string>::const_iterator i = lines.begin(); i != lines.end(); ++i) {
              new_meta += *i + '\n';
            }
            FileLock meta_lock(meta_file, CACHE_META_LOCK_TIMEOUT);
            if (meta_lock.acquire()) {
              if (!FileCreate(meta_file, new_meta)) {
                logger.msg(WARNING, "Could not write meta file %s", meta_file);
              }
              meta_lock.release(); // there is a small timeout so don't bother to report error
            }
            else logger.msg(VERBOSE, "Failed to acquire lock on cache meta file %s", meta_file);
          }
        }
      }
    }
    else if (errno == ENOENT) {
      // create new file
      FileLock meta_lock(meta_file, CACHE_META_LOCK_TIMEOUT);
      if (!meta_lock.acquire()) {
        logger.msg(WARNING, "Failed to acquire lock on cache meta file %s", meta_file);
        is_locked = true;
        return false;
      }
      if (!FileCreate(meta_file, std::string(url + '\n'))) {
        logger.msg(WARNING, "Failed to create meta file %s", meta_file);
      }
      meta_lock.release(); // there is a small timeout so don't bother to report error
    }
    else {
      logger.msg(ERROR, "Error looking up attributes of meta file %s: %s", meta_file, StrError(errno));
      return false;
    }
    return true;
  }

  std::string FileCache::_getMetaFileName(const std::string& url) {
    return File(url) + CACHE_META_SUFFIX;
  }

  std::string FileCache::_getHash(const std::string& url) const {
    // get the hash of the url
    std::string hash = FileCacheHash::getHash(url);
    int index = 0;
    for (int level = 0; level < CACHE_DIR_LEVELS; level ++) {
       hash.insert(index + CACHE_DIR_LENGTH, "/");
       // go to next slash position, add one since we just inserted a slash
       index += CACHE_DIR_LENGTH + 1;
    }
    return hash;
  }

  int FileCache::_chooseCache(const std::string& url) const {

    int caches_size = _caches.size();
  
    // When there is only one cache directory   
    if (caches_size == 1) return 0;

    std::string hash(_getHash(url));
    struct stat fileStat;
    // check the fs to see if the file is already there
    for (int i = 0; i < caches_size ; i++) { 
      std::string c_file = _caches[i].cache_path + "/" + CACHE_DATA_DIR +"/" + hash;  
      if (FileStat(c_file, &fileStat, true)) {
        return i; 
      }  
    }
    // check to see if a lock file already exists, since cache could be
    // started but no file download was done
    for (int i = 0; i < caches_size ; i++) {
      std::string c_file = _caches[i].cache_path + "/" + CACHE_DATA_DIR +"/" + hash + FileLock::getLockSuffix();
      if (FileStat(c_file, &fileStat, true)) {
        return i;
      }
    }
  
    // find a cache with the most unused space and also the cache_size parameter defined in "arc.conf"
    std::map<int ,std::pair<unsigned long long, float> > cache_map;
    // caches which are under the usage percent of the "arc.conf": < cache number, chance to select this cache >
    std::map <int, int>  under_limit;
    // caches which are over the usage percent of the "arc.conf" < cache free space, cache number> 
    std::map<unsigned long long, int> over_limit;
    // sum of all caches 
    long total_size = 0; 
    // get the free spaces of the caches 
    for (int i = 0; i < caches_size; i++ ) {
      std::pair <unsigned long long, unsigned long long> p = _getCacheInfo(_caches[i].cache_path);
      cache_map.insert(std::make_pair(i, p));
      total_size = total_size + p.first;
    }
    for ( std::map< int, std::pair<unsigned long long,float> >::iterator cache_it = cache_map.begin(); cache_it != cache_map.end(); cache_it++) {
      // check if the usage percent is passed
      if ((100 - (100 * cache_it->second.second)/ cache_it->second.first) < _max_used) {                       
        // caches which are under the defined percentage 
        // chance of this cache being used is ratio of cache size to total of all cache sizes
        // can't use roundf() since it is not supported on all platforms
        int chance = (int)((float) cache_it->second.first/total_size*10 + 0.5);
        under_limit.insert(std::make_pair(cache_it->first, chance));
      } else {
        // caches which are passed the defined percentage
        over_limit.insert(std::make_pair((unsigned long long)(cache_it->second.second), cache_it->first));
      }
    }
    int cache_no = 0;
    if (!under_limit.empty()) {
      std::vector<int> utility_cache;
      for ( std::map<int,int> ::iterator cache_it = under_limit.begin(); cache_it != under_limit.end(); cache_it++) {
        // fill the vector with the frequency of cache number according to the cache size. 
        // for instance, a cache with 70% of the total cache space will appear 7 times in this vector and a cache with 30% will appear 3 times.           
        if (cache_it->second == 0) {
          utility_cache.push_back(cache_it->first);
        } else { 
          for (int i = 0; i < cache_it->second; i++) {
            utility_cache.push_back(cache_it->first);
          }
        }
      } 
      // choose a cache from the weighted list   
      cache_no = utility_cache.at((int)rand()%(utility_cache.size()));
    } else {
      // find a max free space amoung the caches that are passed the limit of usage
      cache_no = max_element(over_limit.begin(), over_limit.end(), over_limit.value_comp())->second;
    }
    return cache_no;
  }
  
  std::pair <unsigned long long, unsigned long long> FileCache::_getCacheInfo(const std::string& path) const {
  
    struct statvfs info;
    if (statvfs(path.c_str(), &info) != 0) {
      logger.msg(ERROR, "Error getting info from statvfs for the path: %s", path);
    }
    // return a pair of <cache total size (KB), cache free space (KB)>
    return std::make_pair((info.f_blocks * info.f_bsize)/1024, (info.f_bfree * info.f_bsize)/1024); 
  }

  bool FileCache::_cleanFilesAndReturnFalse(const std::string& hard_link_file,
                                            bool& locked) {
    if (!FileDelete(hard_link_file)) logger.msg(ERROR, "Failed to clean up file %s: %s", hard_link_file, StrError(errno));
    locked = true;
    return false;
  }

} // namespace Arc

#endif /*WIN32*/
