// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

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
    std::vector<std::string> draining_caches;
    std::vector<std::string> readonly_caches;
    if (!cache_path.empty()) 
      caches.push_back(cache_path);

    // if problem in init, clear _caches so object is invalid
    if (!_init(caches, draining_caches, readonly_caches, id, job_uid, job_gid))
      _caches.clear();
  }

  FileCache::FileCache(const std::vector<std::string>& caches,
                       const std::string& id,
                       uid_t job_uid,
                       gid_t job_gid) {

    std::vector<std::string> draining_caches;
    std::vector<std::string> readonly_caches;

    // if problem in init, clear _caches so object is invalid
    if (!_init(caches, draining_caches, readonly_caches, id, job_uid, job_gid))
      _caches.clear();
  }

  FileCache::FileCache(const std::vector<std::string>& caches,
                       const std::vector<std::string>& draining_caches,
                       const std::string& id,
                       uid_t job_uid,
                       gid_t job_gid) {

    std::vector<std::string> readonly_caches;
    // if problem in init, clear _caches so object is invalid
    if (!_init(caches, draining_caches, readonly_caches, id, job_uid, job_gid))
      _caches.clear();
  }

  FileCache::FileCache(const std::vector<std::string>& caches,
                       const std::vector<std::string>& draining_caches,
                       const std::vector<std::string>& readonly_caches,
                       const std::string& id,
                       uid_t job_uid,
                       gid_t job_gid) {

    // if problem in init, clear _caches so object is invalid
    if (!_init(caches, draining_caches, readonly_caches, id, job_uid, job_gid))
      _caches.clear();
  }

  bool FileCache::_init(const std::vector<std::string>& caches,
                        const std::vector<std::string>& draining_caches,
                        const std::vector<std::string>& readonly_caches,
                        const std::string& id,
                        uid_t job_uid,
                        gid_t job_gid) {

    _id = id;
    _uid = job_uid;
    _gid = job_gid;

    // for each cache
    for (int i = 0; i < (int)caches.size(); i++) {
      std::string cache = caches[i];
      std::string cache_path = cache.substr(0, cache.find(" "));
      if (cache_path.empty()) {
        logger.msg(ERROR, "No cache directory specified");
        return false;
      }
      std::string cache_link_path = "";
      if (cache.find(" ") != std::string::npos) cache_link_path = cache.substr(cache.find_last_of(" ") + 1, cache.length() - cache.find_last_of(" ") + 1);

      // tidy up paths - take off any trailing slashes
      if (cache_path.rfind("/") == cache_path.length() - 1) cache_path = cache_path.substr(0, cache_path.length() - 1);
      if (cache_link_path.rfind("/") == cache_link_path.length() - 1) cache_link_path = cache_link_path.substr(0, cache_link_path.length() - 1);

      // add this cache to our list
      struct CacheParameters cache_params;
      cache_params.cache_path = cache_path;
      cache_params.cache_link_path = cache_link_path;
      _caches.push_back(cache_params);
    }
    if (!caches.empty() && _caches.empty()) {
      logger.msg(ERROR, "No usable caches");
      return false;
    }

    // for each draining cache
    for (int i = 0; i < (int)draining_caches.size(); i++) {
      std::string cache = draining_caches[i];
      std::string cache_path = cache.substr(0, cache.find(" "));
      if (cache_path.empty()) {
        logger.msg(ERROR, "No draining cache directory specified");
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

    // for each readonly cache
    for (int i = 0; i < (int)readonly_caches.size(); i++) {
      std::string cache = readonly_caches[i];
      std::string cache_path = cache.substr(0, cache.find(" "));
      if (cache_path.empty()) {
        logger.msg(ERROR, "No read-only cache directory specified");
        return false;
      }
      // tidy up paths - take off any trailing slashes
      if (cache_path.rfind("/") == cache_path.length()-1) cache_path = cache_path.substr(0, cache_path.length()-1);

      // add this cache to our list
      struct CacheParameters cache_params;
      cache_params.cache_path = cache_path;
      cache_params.cache_link_path = "";
      _readonly_caches.push_back(cache_params);
    }

    return true;
  }

  bool FileCache::Start(const std::string& url, bool& available, bool& is_locked, bool delete_first) {

    if (!(*this))
      return false;

    available = false;
    is_locked = false;
    _cache_map.erase(url);
    std::string filename = File(url);

    // create directory structure if required, with last dir only readable by A-REX user
    // try different caches until one succeeds
    while (!DirCreate(filename.substr(0, filename.rfind("/")), S_IRWXU | S_IRGRP | S_IROTH | S_IXGRP | S_IXOTH, true)) {
      logger.msg(WARNING, "Failed to create cache directory for file %s: %s", filename, StrError(errno));
      // remove the cache that failed from the cache list and try File() again
      std::vector<struct CacheParameters>::iterator i = _caches.begin();
      for (; i != _caches.end(); ++i) {
        if (i->cache_path == _cache_map[url].cache_path) {
          _caches.erase(i);
          break;
        }
      }
      if (_caches.empty()) {
        logger.msg(ERROR, "Failed to create any cache directories for %s", url);
        return false;
      }
      _cache_map.erase(url);
      filename = File(url);
    }
    if (errno != EEXIST && chmod(filename.substr(0, filename.rfind("/")).c_str(), S_IRWXU) != 0) {
      logger.msg(ERROR, "Failed to change permissions on %s: %s", filename.substr(0, filename.rfind("/")), StrError(errno));
      return false;
    }

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
    else if (errno != ENOENT) {
      // this is ok, we will download again
      logger.msg(WARNING, "Failed looking up attributes of cached file: %s", StrError(errno));
    }
    if (!available || delete_first) { // lock in preparation for writing
      FileLock lock(filename, CACHE_LOCK_TIMEOUT);
      bool lock_removed = false;
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
    struct CacheParameters chosen_cache = _chooseCache(url);
    std::string path = chosen_cache.cache_path + "/" + CACHE_DATA_DIR + "/" + hash;
  
    // update the cache map with the new file
    _cache_map.insert(std::make_pair(url, chosen_cache));
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
      logger.msg(WARNING, "Cache file %s does not exist", cache_file);
      try_again = true;
      return false;
    }
    else {
      logger.msg(ERROR, "Error accessing cache file %s: %s", cache_file, StrError(errno));
      return false;
    }

    // create per-job hard link dir if necessary, making the final dir readable only by the job user
    if (!DirCreate(hard_link_path, S_IRWXU | S_IRGRP | S_IROTH | S_IXGRP | S_IXOTH, true)) {
      logger.msg(ERROR, "Cannot create directory %s for per-job hard links", hard_link_path);
      return false;
    }
    if (errno != EEXIST) {
      if (chmod(hard_link_path.c_str(), S_IRWXU) != 0) {
        logger.msg(ERROR, "Cannot change permission of %s: %s ", hard_link_path, StrError(errno));
        return false;
      }
      if (chown(hard_link_path.c_str(), _uid, _gid) != 0) {
        logger.msg(ERROR, "Cannot change owner of %s: %s ", hard_link_path, StrError(errno));
        return false;
      }
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
        return _cleanFilesAndReturnFalse(hard_link_file, try_again);
      }
      _urls_unlocked.insert(url);
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
        if (!fa.fa_setuid(_uid, _gid) || !fa.fa_chmod(dest_path, S_IRWXU)) {
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

    // go through all caches (including read-only and draining caches)
    // and remove per-job dirs for our job id
    std::vector<std::string> job_dirs;
    for (int i = 0; i < (int)_caches.size(); i++)
      job_dirs.push_back(_caches[i].cache_path + "/" + CACHE_JOB_DIR + "/" + _id);
    for (int i = 0; i < (int)_draining_caches.size(); i++)
      job_dirs.push_back(_draining_caches[i].cache_path + "/" + CACHE_JOB_DIR + "/" + _id); 
    for (int i = 0; i < (int)_readonly_caches.size(); i++)
      job_dirs.push_back(_readonly_caches[i].cache_path + "/" + CACHE_JOB_DIR + "/" + _id);

    for (int i = 0; i < (int)job_dirs.size(); i++) {
      std::string job_dir = job_dirs[i];
      // check if job dir exists
      struct stat fileStat;
      if (!FileStat(job_dir, &fileStat, true) && errno == ENOENT)
        continue;

      logger.msg(DEBUG, "Removing %s", job_dir);
      if (!DirDelete(job_dir)) {
        logger.msg(WARNING, "Failed to remove cache per-job dir %s: %s", job_dir, StrError(errno));
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
    ++line;

    // check for possible hash collisions between URLs
    if (first_line != url) {
      logger.msg(ERROR, "File %s is already cached at %s under a different URL: %s - will not add DN to cached list",
                 url, File(url), first_line);
      return false;
    }

    std::string newdnlist(first_line + '\n');

    // check list of DNs for expired and this DN
    for (; line != lines.end(); ++line) {
      std::string::size_type space_pos = line->rfind(' ');
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

  bool FileCache::_createMetaFile(const std::string& meta_file, const std::string& content, bool& is_locked) {
    FileLock meta_lock(meta_file, CACHE_META_LOCK_TIMEOUT);
    if (!meta_lock.acquire()) {
      logger.msg(WARNING, "Failed to acquire lock on cache meta file %s", meta_file);
      is_locked = true;
      return false;
    }
    if (!FileCreate(meta_file, content)) {
      logger.msg(WARNING, "Failed to create cache meta file %s", meta_file);
    }
    meta_lock.release(); // there is a small timeout so don't bother to report error
    return true;
  }

  bool FileCache::_checkMetaFile(const std::string& filename, const std::string& url, bool& is_locked) {
    std::string meta_file(filename + CACHE_META_SUFFIX);
    struct stat fileStat;

    if (FileStat(meta_file, &fileStat, true)) {
      // check URL inside file for possible hash collisions
      std::list<std::string> lines;
      if (!FileRead(meta_file, lines)) {
        // file was probably deleted by another process - try again
        logger.msg(WARNING, "Failed to read cache meta file %s", meta_file);
        is_locked = true;
        return false;
      }
      if (lines.empty()) {
        logger.msg(WARNING, "Cache meta file %s is empty, will recreate", meta_file);
        return _createMetaFile(meta_file, std::string(url + '\n'), is_locked);
      }
      std::string meta_str = lines.front();
      if (meta_str.empty()) {
        logger.msg(WARNING, "Cache meta file %s possibly corrupted, will recreate", meta_file);
        return _createMetaFile(meta_file, std::string(url + '\n'), is_locked);
      }
      if (meta_str != url) {
        logger.msg(WARNING, "File %s is already cached at %s under a different URL: %s - this file will not be cached",
                   url, filename, meta_str);
        return false;
      }
    }
    else if (errno == ENOENT) {
      // create new file
      return _createMetaFile(meta_file, std::string(url + '\n'), is_locked);
    }
    else {
      logger.msg(ERROR, "Error looking up attributes of cache meta file %s: %s", meta_file, StrError(errno));
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

  struct CacheParameters FileCache::_chooseCache(const std::string& url) const {

    // When there is only one cache directory
    if (_caches.size() == 1 && _readonly_caches.empty()) return _caches.front();

    std::string hash(_getHash(url));
    struct stat fileStat;
    // check the fs to see if the file is already there
    for (std::vector<struct CacheParameters>::const_iterator i = _caches.begin(); i != _caches.end(); ++i) {
      std::string c_file = i->cache_path + "/" + CACHE_DATA_DIR +"/" + hash;
      if (FileStat(c_file, &fileStat, true)) {
        return *i;
      }
    }

    // check the read-only caches
    for (std::vector<struct CacheParameters>::const_iterator i = _readonly_caches.begin(); i != _readonly_caches.end(); ++i) {
      std::string c_file = i->cache_path + "/" + CACHE_DATA_DIR +"/" + hash;
      if (FileStat(c_file, &fileStat, true)) {
        return *i;
      }
    }

    // check to see if a lock file already exists, since cache could be
    // started but no file download was done
    for (std::vector<struct CacheParameters>::const_iterator i = _caches.begin(); i != _caches.end(); ++i) {
      std::string c_file = i->cache_path + "/" + CACHE_DATA_DIR +"/" + hash + FileLock::getLockSuffix();
      if (FileStat(c_file, &fileStat, true)) {
        return *i;
      }
    }

    // map of cache number and unused space in GB
    std::map<int, float> cache_map;
    // sum of all cache free space
    float total_free = 0;
    // get the free spaces of the caches 
    for (unsigned int i = 0; i < _caches.size(); ++i) {
      float free_space = _getCacheInfo(_caches.at(i).cache_path);
      cache_map[i] = free_space;
      total_free += free_space;
    }

    // Select a random cache using the free space as a weight
    // r is a random number between 0 and total_space
    float r = total_free * ((float)rand()/(float)RAND_MAX);
    for (std::map<int, float>::iterator cache_it = cache_map.begin(); cache_it != cache_map.end(); ++cache_it) {
      r -= cache_it->second;
      if (r <= 0) {
        logger.msg(DEBUG, "Using cache %s", _caches.at(cache_it->first).cache_path);
        return _caches.at(cache_it->first);
      }
    }
    // shouldn't be possible to get here
    return _caches.front();
  }

  float FileCache::_getCacheInfo(const std::string& path) const {

    struct statvfs info;
    if (statvfs(path.c_str(), &info) != 0) {
      // if path does not exist info is undefined but the dir will be created in Start() anyway
      if (errno != ENOENT) {
        logger.msg(ERROR, "Error getting info from statvfs for the path %s: %s", path, StrError(errno));
        return 0;
      }
    }
    // return free space in GB
    float space = (float)(info.f_bfree * info.f_bsize) / (float)(1024 * 1024 * 1024);
    logger.msg(DEBUG, "Cache %s: Free space %f GB", path, space);
    return space;
  }

  bool FileCache::_cleanFilesAndReturnFalse(const std::string& hard_link_file,
                                            bool& locked) {
    if (!FileDelete(hard_link_file)) logger.msg(ERROR, "Failed to clean up file %s: %s", hard_link_file, StrError(errno));
    locked = true;
    return false;
  }

} // namespace Arc
