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
#include <dirent.h>
#include <unistd.h>
#include <sys/utsname.h>
#include <sys/statvfs.h>

#include <glibmm.h>

#include <arc/Logger.h>
#include <arc/FileUtils.h>

#include "FileCache.h"

namespace Arc {

  const std::string FileCache::CACHE_DATA_DIR = "data";
  const std::string FileCache::CACHE_JOB_DIR = "joblinks";
  const int FileCache::CACHE_DIR_LENGTH = 2;
  const int FileCache::CACHE_DIR_LEVELS = 1;
  const std::string FileCache::CACHE_LOCK_SUFFIX = ".lock";
  const std::string FileCache::CACHE_META_SUFFIX = ".meta";
  const int FileCache::CACHE_DEFAULT_AUTH_VALIDITY = 86400; // 24 h

  Logger FileCache::logger(Logger::getRootLogger(), "FileCache");

  static bool fgets(FILE *file, int size, std::string& str) {
    bool res = false;
    char* tstr = NULL;
    try {
      tstr = new char[size+1];
      if (tstr) {
        if (fgets(tstr, size+1, file) != NULL) {
          str = tstr;
          res = true;
        }
      }
    } catch(std::exception& e) { }
    if(tstr) delete[] tstr;
    return res;
  }
 
  FileCache::FileCache(std::string cache_path,
                       std::string id,
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

  FileCache::FileCache(std::vector<std::string> caches,
                       std::string id,
                       uid_t job_uid,
                       gid_t job_gid) {

    std::vector<std::string> remote_caches;
    std::vector<std::string> draining_caches;

    // if problem in init, clear _caches so object is invalid
    if (!_init(caches, remote_caches, draining_caches, id, job_uid, job_gid))
      _caches.clear();
  }

  FileCache::FileCache(std::vector<std::string> caches,
                       std::vector<std::string> remote_caches,
                       std::vector<std::string> draining_caches,
                       std::string id,
                       uid_t job_uid,
                       gid_t job_gid,
                       int cache_max,
                       int cache_min) {
  
    // if problem in init, clear _caches so object is invalid
    if (! _init(caches, remote_caches, draining_caches, id, job_uid, job_gid, cache_max, cache_min))
      _caches.clear();
  }

  bool FileCache::_init(std::vector<std::string> caches,
                        std::vector<std::string> remote_caches,
                        std::vector<std::string> draining_caches,
                        std::string id,
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
      if (!_cacheMkDir(cache_path + "/" + CACHE_DATA_DIR, true)) {
        logger.msg(ERROR, "Cannot create directory \"%s\" for cache", cache_path + "/" + CACHE_DATA_DIR);
        return false;
      }
      if (!_cacheMkDir(cache_path + "/" + CACHE_JOB_DIR, true)) {
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
      // our hostname and pid
    struct utsname buf;
    if (uname(&buf) != 0) {
      logger.msg(ERROR, "Cannot determine hostname from uname()");
      return false;
    }
    _hostname = buf.nodename;
    int pid_i = getpid();
    std::stringstream ss;
    ss << pid_i;
    ss >> _pid;
    return true;
  }

  bool FileCache::Start(std::string url, bool& available, bool& is_locked, bool use_remote) {

    if (!(*this))
      return false;

    available = false;
    is_locked = false;
    std::string filename = File(url);
    std::string lock_file = _getLockFileName(url);

    // create directory structure if required, only readable by GM user
    if (!_cacheMkDir(lock_file.substr(0, lock_file.rfind("/")), false))
      return false;

    int lock_timeout = 86400; // one day timeout on lock TODO: make configurable?

    // locking mechanism:
    // - check if lock is there
    // - if not, create tmp file and check again
    // - if lock is still not there copy tmp file to cache lock file
    // - check pid inside lock file matches ours

    struct stat fileStat;
    int err = stat(lock_file.c_str(), &fileStat);
    if (0 != err) {
      if (errno == EACCES) {
        logger.msg(ERROR, "EACCES Error opening lock file %s: %s", lock_file, strerror(errno));
        return false;
      }
      else if (errno != ENOENT) {
        // some other error occurred opening the lock file
        logger.msg(ERROR, "Error opening lock file %s in initial check: %s", lock_file, strerror(errno));
        return false;
      }
      // lock does not exist - create tmp file
      std::string tmpfile = lock_file + ".XXXXXX";
      int h = Glib::mkstemp(tmpfile);
      if (h == -1) {
        logger.msg(ERROR, "Error creating file %s with mkstemp(): %s", tmpfile, strerror(errno));
        return false;
      }
      // write pid@hostname to the lock file
      std::string buf = _pid + "@" + _hostname;
      if (write(h, buf.c_str(), buf.length()) == -1) {
        logger.msg(ERROR, "Error writing to tmp lock file %s: %s", tmpfile, strerror(errno));
        // not much we can do if this doesn't work, but it is only a tmp file
        remove(tmpfile.c_str());
        close(h);
        return false;
      }
      if (close(h) != 0)
        // not critical as file will be removed after we are done
        logger.msg(WARNING, "Warning: closing tmp lock file %s failed", tmpfile);
      // check again if lock exists, in case creating the tmp file took some time
      err = stat(lock_file.c_str(), &fileStat);
      if (0 != err) {
        if (errno == ENOENT) {
          // ok, we can create lock
          if (rename(tmpfile.c_str(), lock_file.c_str()) != 0) {
            logger.msg(ERROR, "Error renaming tmp file %s to lock file %s: %s", tmpfile, lock_file, strerror(errno));
            remove(tmpfile.c_str());
            return false;
          }
          // check it's really there
          err = stat(lock_file.c_str(), &fileStat);
          if (0 != err) {
            logger.msg(ERROR, "Error renaming lock file, even though rename() did not return an error");
            return false;
          }
          // check the pid inside the lock file, just in case...
          if (!_checkLock(url)) {
            is_locked = true;
            return false;
          }
        }
        else if (errno == EACCES) {
          logger.msg(ERROR, "EACCES Error opening lock file %s: %s", lock_file, strerror(errno));
          remove(tmpfile.c_str());
          return false;
        }
        else {
          // some other error occurred opening the lock file
          logger.msg(ERROR, "Error opening lock file we just renamed successfully %s: %s", lock_file, strerror(errno));
          remove(tmpfile.c_str());
          return false;
        }
      }
      else {
        logger.msg(VERBOSE, "The file is currently locked with a valid lock");
        remove(tmpfile.c_str());
        is_locked = true;
        return false;
      }
    }
    else {
      // the lock already exists, check if it has expired
      // look at modification time
      time_t mod_time = fileStat.st_mtime;
      time_t now = time(NULL);
      logger.msg(VERBOSE, "%li seconds since lock file was created", now - mod_time);

      if ((now - mod_time) > lock_timeout) {
        logger.msg(VERBOSE, "Timeout has expired, will remove lock file");
        // TODO: kill the process holding the lock, only if we know it was the original
        // process which created it
        if (remove(lock_file.c_str()) != 0 && errno != ENOENT) {
          logger.msg(ERROR, "Failed to unlock file %s: %s", lock_file, strerror(errno));
          return false;
        }
        // lock has expired and has been removed. Try to remove cache file and call Start() again
        if (remove(filename.c_str()) != 0 && errno != ENOENT) {
          logger.msg(ERROR, "Error removing cache file %s: %s", filename, strerror(errno));
          return false;
        }
        return Start(url, available, is_locked, use_remote);
      }

      // lock is still valid, check if we own it
      FILE *pFile;
      char lock_info[100]; // should be long enough for a pid + hostname
      pFile = fopen((char*)lock_file.c_str(), "r");
      if (pFile == NULL) {
        // lock could have been released by another process, so call Start again
        if (errno == ENOENT) {
          logger.msg(VERBOSE, "Lock that recently existed has been deleted by another process, calling Start() again");
          return Start(url, available, is_locked, use_remote);
        }
        logger.msg(ERROR, "Error opening valid and existing lock file %s: %s", lock_file, strerror(errno));
        return false;
      }
      if (fgets(lock_info, 100, pFile) == NULL) {
        logger.msg(ERROR, "Error reading valid and existing lock file %s: %s", lock_file, strerror(errno));
        fclose(pFile);
        return false;
      }
      fclose(pFile);

      std::string lock_info_s(lock_info);
      std::string::size_type index = lock_info_s.find("@", 0);
      if (index == std::string::npos) {
        logger.msg(ERROR, "Error with formatting in lock file %s: %s", lock_file, lock_info_s);
        return false;
      }

      if (lock_info_s.substr(index + 1) != _hostname) {
        logger.msg(VERBOSE, "Lock is owned by a different host");
        // TODO: here do ssh login and check
        is_locked = true;
        return false;
      }
      std::string lock_pid = lock_info_s.substr(0, index);
      if (lock_pid == _pid)
        // safer to wait until lock expires than use cached file or re-download
        logger.msg(WARNING, "Warning: This process already owns the lock");
      else {
        // check if the pid owning the lock is still running - if not we can claim the lock
        // this is not really portable... but no other way to do it
        std::string procdir("/proc/");
        procdir = procdir.append(lock_pid);
        if (stat(procdir.c_str(), &fileStat) != 0 && errno == ENOENT) {
          logger.msg(VERBOSE, "The process owning the lock is no longer running, will remove lock");
          if (remove(lock_file.c_str()) != 0) {
            logger.msg(ERROR, "Failed to unlock file %s: %s", lock_file, strerror(errno));
            return false;
          }
          // lock has been removed. try to delete cache file and call Start() again
          if (remove(filename.c_str()) != 0 && errno != ENOENT) {
            logger.msg(ERROR, "Error removing cache file %s: %s", filename, strerror(errno));
            return false;
          }
          return Start(url, available, is_locked, use_remote);
        }
      }

      logger.msg(VERBOSE, "The file is currently locked with a valid lock");
      is_locked = true;
      return false;
    }

    // if we get to here we have acquired the lock

    // create the meta file to store the URL, if it does not exist
    std::string meta_file = _getMetaFileName(url);
    err = stat(meta_file.c_str(), &fileStat);
    if (0 == err) {
      // check URL inside file for possible hash collisions
      FILE *pFile;
      pFile = fopen((char*)meta_file.c_str(), "r");
      if (pFile == NULL) {
        logger.msg(ERROR, "Error opening meta file %s: %s", _getMetaFileName(url), strerror(errno));
        remove(lock_file.c_str());
        return false;
      }
      std::string meta_str;
      if(!fgets(pFile, fileStat.st_size, meta_str)) {
        logger.msg(ERROR, "Error reading valid and existing lock file %s: %s", lock_file, strerror(errno));
        fclose(pFile);
        return false;
      }
      fclose(pFile);

      // get the first line
      if (meta_str.find('\n') != std::string::npos)
        meta_str.resize(meta_str.find('\n'));

      std::string::size_type space_pos = meta_str.find(' ', 0);
      if (meta_str.substr(0, space_pos) != url) {
        logger.msg(ERROR, "Error: File %s is already cached at %s under a different URL: %s - this file will not be cached", url, filename, meta_str.substr(0, space_pos));
        remove(lock_file.c_str());
        return false;
      }
    }
    else if (errno == ENOENT) {
      // create new file
      FILE *pFile;
      pFile = fopen((char*)meta_file.c_str(), "w");
      if (pFile == NULL) {
        logger.msg(ERROR, "Failed to create info file %s: %s", meta_file, strerror(errno));
        remove(lock_file.c_str());
        return false;
      }
      fputs((char*)url.c_str(), pFile);
      fputs("\n", pFile);
      fclose(pFile);
      // make read/writeable only by GM user
      chmod(meta_file.c_str(), S_IRUSR | S_IWUSR);
    }
    else {
      logger.msg(ERROR, "Error looking up attributes of meta file %s: %s", meta_file, strerror(errno));
      remove(lock_file.c_str());
      return false;
    }
    // now check if the cache file is there already
    err = stat(filename.c_str(), &fileStat);
    if (0 == err)
      available = true;
      
    // if the file is not there. check remote caches
    else if (errno == ENOENT) {
      if (!use_remote) return true;    
      // get the hash of the url
      std::string hash = FileCacheHash::getHash(url);
    
      int index = 0;
      for(int level = 0; level < CACHE_DIR_LEVELS; level ++) {
        hash.insert(index + CACHE_DIR_LENGTH, "/");
        // go to next slash position, add one since we just inserted a slash
        index += CACHE_DIR_LENGTH + 1;
      }
      std::string remote_cache_file;
      std::string remote_cache_link;
      for (std::vector<struct CacheParameters>::iterator it = _remote_caches.begin(); it != _remote_caches.end(); it++) {
        std::string remote_file = it->cache_path+"/"+CACHE_DATA_DIR+"/"+hash;
        if (stat(remote_file.c_str(), &fileStat) == 0) {
          remote_cache_file = remote_file;
          remote_cache_link = it->cache_link_path;
          break;
        }
      }
      if (remote_cache_file.empty()) return true;
      
      logger.msg(INFO, "Found file %s in remote cache at %s", url, remote_cache_file);
      // if found, create lock file in remote cache
      std::string remote_lock_file = remote_cache_file+".lock";
      err = stat( remote_lock_file.c_str(), &fileStat );
      // if lock exists, exit
      if (0 == err) {
        logger.msg(VERBOSE, "File exists in remote cache at %s but is locked. Will download from source", remote_cache_file);
        return true;
      }
    
      // lock does not exist - create tmp file
      std::string remote_tmpfile = remote_lock_file + ".XXXXXX";
      int h = Glib::mkstemp(remote_tmpfile);
      if (h == -1) {
        logger.msg(WARNING, "Error creating tmp file %s for remote lock with mkstemp(): %s", remote_tmpfile, strerror(errno));
        return true;
      }
      // write pid@hostname to the lock file
      std::string buf2 = _pid + "@" + _hostname;
      if (write(h, buf2.c_str(), buf2.length()) == -1) {
        logger.msg(WARNING, "Error writing to tmp lock file for remote lock %s: %s", remote_tmpfile, strerror(errno));
        // not much we can do if this doesn't work, but it is only a tmp file
        remove(remote_tmpfile.c_str());
        close(h);
        return true;
      }
      if (close(h) != 0) {
        // not critical as file will be removed after we are done
        logger.msg(WARNING, "Warning: closing tmp lock file for remote lock %s failed", remote_tmpfile);
      }
      // check again if lock exists, in case creating the tmp file took some time
      err = stat( remote_lock_file.c_str(), &fileStat ); 
      if (0 != err) {
        if (errno == ENOENT) {
          // ok, we can create lock
          if (rename(remote_tmpfile.c_str(), remote_lock_file.c_str()) != 0) {
            logger.msg(WARNING, "Error renaming tmp file %s to lock file %s for remote lock: %s", remote_tmpfile, remote_lock_file, strerror(errno));
            remove(remote_tmpfile.c_str());
            return true;
          }
          // check it's really there
          err = stat( remote_lock_file.c_str(), &fileStat ); 
          if (0 != err) {
            logger.msg(WARNING, "Error renaming lock file for remote lock, even though rename() did not return an error: %s", strerror(errno));
            return true;
          }
        }
        else {
          // some error occurred opening the lock file
          logger.msg(WARNING, "Error opening lock file for remote lock we just renamed successfully %s: %s", remote_lock_file, strerror(errno));
          remove(remote_tmpfile.c_str());
          return true;
        }
      }
      else {
        logger.msg(VERBOSE, "The remote cache file is currently locked with a valid lock, will download from source");
        remove(remote_tmpfile.c_str());
        return true;
      }
      
      // we have locked the remote file - so find out what to do with it
      if (remote_cache_link == "replicate") {
        // copy the file to the local cache, remove remote lock and exit with available=true
        logger.msg(VERBOSE, "Replicating file %s to local cache file %s", remote_cache_file, filename);

        int fdest = FileOpen(filename, O_WRONLY | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
        if(fdest == -1) {
          logger.msg(ERROR, "Failed to create file %s for writing: %s",filename, strerror(errno));
          return false;
        };
        
        int fsource = FileOpen(remote_cache_file, O_RDONLY, 0);
        if(fsource == -1) {
          close(fdest);
          logger.msg(ERROR, "Failed to open file %s for reading: %s", remote_cache_file, strerror(errno));
          return false;
        };
        
        if(!FileCopy(fsource,fdest)) {
          close(fdest); close(fsource);
          logger.msg(ERROR, "Failed to copy file %s to %s: %s", remote_cache_file, filename, strerror(errno));
          return false;
        };
        close(fdest); close(fsource);
        if (remove(remote_lock_file.c_str()) != 0) {
          logger.msg(ERROR, "Failed to remove remote lock file %s: %s. Some manual intervention may be required", remote_lock_file, strerror(errno));
          return true; // ?
        }
      }
      // create symlink from file in this cache to other cache
      else {
        logger.msg(VERBOSE, "Creating temporary link from %s to remote cache file %s", filename, remote_cache_file);
        if (symlink(remote_cache_file.c_str(), filename.c_str()) != 0) {
          logger.msg(ERROR, "Failed to create soft link to remote cache: %s Will download %s from source", strerror(errno), url);
          if (remove(remote_lock_file.c_str()) != 0) {
            logger.msg(ERROR, "Failed to remove remote lock file %s: %s Some manual intervention may be required", remote_lock_file, strerror(errno));
          }
          return true;
        }
      }
      available = true;
    }
    else {
      // this is ok, we will download again
      logger.msg(WARNING, "Warning: error looking up attributes of cached file: %s", strerror(errno));
    }
    return true;
  }

  bool FileCache::Stop(std::string url) {

    if (!(*this))
      return false;

    // if cache file is a symlink, remove remote cache lock and symlink
    std::string filename = File(url);
    struct stat fileStat;
    if (lstat(filename.c_str(), &fileStat) == 0 && S_ISLNK(fileStat.st_mode)) {
      char buf[1024];
      int link_size = readlink(filename.c_str(), buf, sizeof(buf));
      if (link_size == -1) {
        logger.msg(ERROR, "Could not read target of link %s: %s. Manual intervention may be required to remove lock in remote cache", filename, strerror(errno));
        return false;
      }
      std::string remote_lock(buf); remote_lock.resize(link_size); remote_lock += ".lock";
      if (remove(remote_lock.c_str()) != 0 && errno != ENOENT) {
        logger.msg(ERROR, "Failed to unlock remote cache lock %s: %s. Manual intervention may be required", remote_lock, strerror(errno));
        return false;
      }
      if (remove(filename.c_str()) != 0) {
        logger.msg(ERROR, "Error removing file %s: %s. Manual intervention may be required", filename, strerror(errno));
        return false;
      }
    }
    
     // check the lock is ok to delete
    if (!_checkLock(url))
      return false;

    // delete the lock
    if (remove(_getLockFileName(url).c_str()) != 0) {
      logger.msg(ERROR, "Failed to unlock file with lock %s: %s", _getLockFileName(url), strerror(errno));
      return false;
    }
    // get the hash of the url
    std::string hash = FileCacheHash::getHash(url);
    int index = 0;
    for(int level = 0; level < CACHE_DIR_LEVELS; level ++) {
      hash.insert(index + CACHE_DIR_LENGTH, "/");
      // go to next slash position, add one since we just inserted a slash
      index += CACHE_DIR_LENGTH + 1;
    }
    
    // remove the file from the cache map
    _cache_map.erase(hash);
    return true;
  }

  bool FileCache::StopAndDelete(std::string url) {

    if (!(*this))
      return false;
    
    // if cache file is a symlink, remove remote cache lock
    std::string filename = File(url);
    struct stat fileStat;
    if (lstat(filename.c_str(), &fileStat) == 0 && S_ISLNK(fileStat.st_mode)) {
      char buf[1024];
      int link_size = readlink(filename.c_str(), buf, sizeof(buf));
      if (link_size == -1) {
        logger.msg(ERROR, "Could not read target of link %s: %s. Manual intervention may be required to remove lock in remote cache", filename, strerror(errno));
        return false;
      }
      std::string remote_lock(buf); remote_lock.resize(link_size); remote_lock += ".lock";
      if (remove(remote_lock.c_str()) != 0 && errno != ENOENT) {
        logger.msg(ERROR, "Failed to unlock remote cache lock %s: %s. Manual intervention may be required", remote_lock, strerror(errno));
        return false;
      }
    }

    // check the lock is ok to delete, and if so, remove the file and the
    // associated lock
    if (!_checkLock(url))
      return false;

    // delete the cache file
    if (remove(filename.c_str()) != 0 && errno != ENOENT) {
      logger.msg(ERROR, "Error removing cache file %s: %s", filename, strerror(errno));
      return false;
    }

    // delete the meta file - not critical so don't fail on error
    if (remove(_getMetaFileName(url).c_str()) != 0)
      logger.msg(ERROR, "Failed to unlock file with lock %s: %s", _getLockFileName(url), strerror(errno));

    // delete the lock
    if (remove(_getLockFileName(url).c_str()) != 0) {
      logger.msg(ERROR, "Failed to unlock file with lock %s: %s", _getLockFileName(url), strerror(errno));
      return false;
    }
    
    // get the hash of the url
    std::string hash = FileCacheHash::getHash(url);
    int index = 0;
    for(int level = 0; level < CACHE_DIR_LEVELS; level ++) {
      hash.insert(index + CACHE_DIR_LENGTH, "/");
      // go to next slash position, add one since we just inserted a slash
      index += CACHE_DIR_LENGTH + 1;
    }
  
    // remove the file from the cache map
    _cache_map.erase(hash);
    return true;
  }

  std::string FileCache::File(std::string url) {

    if (!(*this))
      return "";

    // get the hash of the url
    std::string hash = FileCacheHash::getHash(url);

    int index = 0;
    for (int level = 0; level < CACHE_DIR_LEVELS; level++) {
      hash.insert(index + CACHE_DIR_LENGTH, "/");
      // go to next slash position, add one since we just inserted a slash
      index += CACHE_DIR_LENGTH + 1;
    }
    // look up the cache map to see if the file is already in
    std::map <std::string, int>::iterator iter = _cache_map.find(hash) ;
    if (iter != _cache_map.end()) {
      return _caches[iter->second].cache_path + "/" + CACHE_DATA_DIR + "/" + hash;
    } 
  
    // else choose a new cache and assign the file to it
    int chosen_cache = _chooseCache(url);
    std::string path  = _caches[chosen_cache].cache_path + "/" + CACHE_DATA_DIR + "/" + hash;
  
    // update the cache map with the new file
    _cache_map.insert(std::make_pair(hash, chosen_cache));
    return path;
  }

  bool FileCache::Link(std::string link_path, std::string url) {

    if (!(*this))
      return false;

    // check the original file exists
    std::string cache_file = File(url);
    struct stat fileStat;
  
    if (lstat(cache_file.c_str(), &fileStat) != 0) {
      if (errno == ENOENT)
        logger.msg(ERROR, "Error: Cache file %s does not exist", cache_file);
      else
        logger.msg(ERROR, "Error accessing cache file %s: %s", cache_file, strerror(errno));
      return false;
    }
  
    // get the hash of the url
    std::string hash = FileCacheHash::getHash(url);
    int index = 0;
    for (int level = 0; level < CACHE_DIR_LEVELS; level ++) {
      hash.insert(index + CACHE_DIR_LENGTH, "/");
      // go to next slash position, add one since we just inserted a slash
      index += CACHE_DIR_LENGTH + 1;
    }
  
    // look up the map file to see if the file is already mapped with a cache  
    std::map <std::string, int>::iterator iter = _cache_map.find(hash);
    int cache_no = 0;
    if (iter != _cache_map.end()) {
      cache_no = iter->second;}
    else {
      logger.msg(ERROR, "Error: Cache not found for file %s", cache_file);
      return false;
    }

    // choose cache
    struct CacheParameters cache_params = _caches[cache_no];
    std::string hard_link_path = cache_params.cache_path + "/" + CACHE_JOB_DIR + "/" +_id;
    std::string cache_link_path = cache_params.cache_link_path;

    // check if cached file is a symlink - if so get link path from the remote cache
    if (S_ISLNK(fileStat.st_mode)) {
      char link_target_buf[1024];
      int link_size = readlink(cache_file.c_str(), link_target_buf, sizeof(link_target_buf));
      if (link_size == -1) {
        logger.msg(ERROR, "Could not read target of link %s: %s", cache_file, strerror(errno));
        return false;
      }
      // need to match the symlink target against the list of remote caches
      std::string link_target(link_target_buf); link_target.resize(link_size);
      for (std::vector<struct CacheParameters>::iterator it = _remote_caches.begin(); it != _remote_caches.end(); it++) {
        std::string remote_data_dir = it->cache_path+"/"+CACHE_DATA_DIR;
        if (link_target.find(remote_data_dir) == 0) {
          hard_link_path = it->cache_path+"/"+CACHE_JOB_DIR + "/" + _id;
          cache_link_path = it->cache_link_path;
          cache_file = link_target;
          break;
        }
      }
      if (hard_link_path == cache_params.cache_path + "/" + CACHE_JOB_DIR + "/" +_id) {
        logger.msg(ERROR, "Couldn't match link target %s to any remote cache", link_target);
        return false;
      }
    }

    // if _cache_link_path is '.' then copy instead, bypassing the hard-link
    if (cache_params.cache_link_path == ".")
      return Copy(link_path, url);

    // create per-job hard link dir if necessary, making the final dir readable only by the job user
    if (!_cacheMkDir(hard_link_path, true)) {
      logger.msg(ERROR, "Cannot create directory \"%s\" for per-job hard links", hard_link_path);
      return false;
    }
    if (chown(hard_link_path.c_str(), _uid, _gid) != 0) {
      logger.msg(ERROR, "Cannot change owner of %s", hard_link_path);
      return false;
    }
    if (chmod(hard_link_path.c_str(), S_IRWXU) != 0) {
      logger.msg(ERROR, "Cannot change permissions of \"%s\" to 0700", hard_link_path);
      return false;
    }

    std::string filename = link_path.substr(link_path.rfind("/") + 1);
    std::string hard_link_file = hard_link_path + "/" + filename;
    std::string session_dir = link_path.substr(0, link_path.rfind("/"));

    // make the hard link
    if (link(cache_file.c_str(), hard_link_file.c_str()) != 0) {
      logger.msg(ERROR, "Failed to create hard link from %s to %s: %s", hard_link_file, cache_file, strerror(errno));
      return false;
    }
    // ensure the hard link is readable by all and owned by root (or GM user)
    // (to make cache file immutable but readable by all)
    if (chown(hard_link_file.c_str(), getuid(), getgid()) != 0) {
      logger.msg(ERROR, "Failed to change owner of hard link to %i: %s", getuid(), strerror(errno));
      return false;
    }
    if (chmod(hard_link_file.c_str(), S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH) != 0) {
      logger.msg(ERROR, "Failed to change permissions of hard link to 0644: %s", strerror(errno));
      return false;
    }

    // make necessary dirs for the soft link
    // this probably should have already been done... somewhere...
    if (!_cacheMkDir(session_dir, true))
      return false;
    if (chown(session_dir.c_str(), _uid, _gid) != 0) {
      logger.msg(ERROR, "Failed to change owner of session dir to %i: %s", _uid, strerror(errno));
      return false;
    }
    if (chmod(session_dir.c_str(), S_IRWXU) != 0) {
      logger.msg(ERROR, "Failed to change permissions of session dir to 0700: %s", strerror(errno));
      return false;
    }

    // make the soft link, changing the target if cache_link_path is defined
    if (!cache_params.cache_link_path.empty())
      hard_link_file = cache_params.cache_link_path + "/" + CACHE_JOB_DIR + "/" + _id + "/" + filename;
    if (symlink(hard_link_file.c_str(), link_path.c_str()) != 0) {
      logger.msg(ERROR, "Failed to create soft link: %s", strerror(errno));
      return false;
    }

    // change the owner of the soft link to the job user
    if (lchown(link_path.c_str(), _uid, _gid) != 0) {
      logger.msg(ERROR, "Failed to change owner of session dir to %i: %s", _uid, strerror(errno));
      return false;
    }
    return true;
  }

  bool FileCache::Copy(std::string dest_path, std::string url, bool executable) {

    if (!(*this))
      return false;

    // check the original file exists
    std::string cache_file = File(url);
    struct stat fileStat;
    if (stat(cache_file.c_str(), &fileStat) != 0) {
      if (errno == ENOENT)
        logger.msg(ERROR, "Cache file %s does not exist", cache_file);
      else
        logger.msg(ERROR, "Error accessing cache file %s: %s", cache_file, strerror(errno));
      return false;
    }

    // make necessary dirs for the copy
    // this probably should have already been done... somewhere...
    std::string dest_dir = dest_path.substr(0, dest_path.rfind("/"));
    if (!_cacheMkDir(dest_dir, true))
      return false;
    if (chown(dest_dir.c_str(), _uid, _gid) != 0) {
      logger.msg(ERROR, "Failed to change owner of destination dir to %i: %s", _uid, strerror(errno));
      return false;
    }
    if (chmod(dest_dir.c_str(), S_IRWXU) != 0) {
      logger.msg(ERROR, "Failed to change permissions of session dir to 0700: %s", strerror(errno));
      return false;
    }

    mode_t perm = S_IRUSR | S_IWUSR;
    if (executable)
      perm |= S_IXUSR;
    int fdest = FileOpen(dest_path, O_WRONLY | O_CREAT | O_EXCL, perm);
    if (fdest == -1) {
      logger.msg(ERROR, "Failed to create file %s for writing: %s", dest_path, strerror(errno));
      return false;
    }
    if (fchown(fdest, _uid, _gid) == -1) {
      logger.msg(ERROR, "Failed change ownership of destination file %s: %s", dest_path, strerror(errno));
      close(fdest);
      return false;
    }

    int fsource = FileOpen(cache_file, O_RDONLY, 0);
    if (fsource == -1) {
      close(fdest);
      logger.msg(ERROR, "Failed to open file %s for reading: %s", cache_file, strerror(errno));
      return false;
    }

    if(!FileCopy(fsource, fdest)) {
      close(fdest);
      close(fsource);
      logger.msg(ERROR, "Failed to copy file %s to %s: %s", cache_file, dest_path, strerror(errno));
      return false;
    }
    close(fdest);
    close(fsource);
    return true;
  }

  bool FileCache::Release() {

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
      DIR *dirp = opendir(job_dir.c_str());
      if (dirp == NULL) {
        if (errno == ENOENT)
          continue;
        logger.msg(ERROR, "Error opening per-job dir %s: %s", job_dir, strerror(errno));
        return false;
      }

      // list all files in the dir and delete them
      struct dirent *dp;
      errno = 0;
      while ((dp = readdir(dirp))) {
        if (strcmp(dp->d_name, ".") == 0 || strcmp(dp->d_name, "..") == 0)
          continue;
        std::string to_delete = job_dir + "/" + dp->d_name;
        logger.msg(VERBOSE, "Removing %s", to_delete);
        if (remove(to_delete.c_str()) != 0) {
          logger.msg(ERROR, "Failed to remove hard link %s: %s", to_delete, strerror(errno));
          closedir(dirp);
          return false;
        }
      }
      closedir(dirp);

      if (errno != 0) {
        logger.msg(ERROR, "Error listing dir %s: %s", job_dir, strerror(errno));
        return false;
      }

      // remove now-empty dir
      logger.msg(VERBOSE, "Removing %s", job_dir);
      if (rmdir(job_dir.c_str()) != 0) {
        logger.msg(ERROR, "Failed to remove cache per-job dir %s: %s", job_dir, strerror(errno));
        return false;
      }
    }
    return true;
  }

  bool FileCache::AddDN(std::string url, std::string DN, Time expiry_time) {

    if (DN.empty())
      return false;
    if (expiry_time == Time(0))
      expiry_time = Time(time(NULL) + CACHE_DEFAULT_AUTH_VALIDITY);

    // add DN to the meta file. If already there, renew the expiry time
    std::string meta_file = _getMetaFileName(url);
    struct stat fileStat;
    int err = stat(meta_file.c_str(), &fileStat);
    if (0 != err) {
      logger.msg(ERROR, "Error reading meta file %s: %s", meta_file, strerror(errno));
      return false;
    }
    FILE *pFile;
    pFile = fopen(meta_file.c_str(), "r");
    if (pFile == NULL) {
      logger.msg(ERROR, "Error opening meta file %s: %s", meta_file, strerror(errno));
      return false;
    }
    // get the first line
    std::string first_line;
    if(!fgets(pFile, fileStat.st_size, first_line)) {
      logger.msg(ERROR, "Error reading meta file %s: %s", meta_file, strerror(errno));
      return false;
    }

    // check for correct formatting and possible hash collisions between URLs
    if (first_line.find('\n') == std::string::npos)
      first_line += '\n';
    std::string::size_type space_pos = first_line.rfind(' ');
    if (space_pos == std::string::npos)
      space_pos = first_line.length() - 1;

    if (first_line.substr(0, space_pos) != url) {
      logger.msg(ERROR, "Error: File %s is already cached at %s under a different URL: %s - will not add DN to cached list", url, File(url), first_line.substr(0, space_pos));
      fclose(pFile);
      return false;
    }

    // read in list of DNs
    std::vector<std::string> dnlist;
    dnlist.push_back(DN + ' ' + expiry_time.str(MDSTime) + '\n');

    std::string dnstring;
    bool res = fgets(pFile, fileStat.st_size, dnstring);
    while (res) {
      space_pos = dnstring.rfind(' ');
      if (space_pos == std::string::npos) {
        logger.msg(WARNING, "Bad format detected in file %s, in line %s", meta_file, dnstring);
        res = fgets(pFile, fileStat.st_size, dnstring);
        continue;
      }
      // remove expired DNs (after some grace period)
      if (dnstring.substr(0, space_pos) != DN) {
        if (dnstring.find('\n') != std::string::npos)
          dnstring.resize(dnstring.find('\n'));
        Time exp_time(dnstring.substr(space_pos + 1));
        if (exp_time > Time(time(NULL) - CACHE_DEFAULT_AUTH_VALIDITY))
          dnlist.push_back(dnstring + '\n');
      }
      res = fgets(pFile, fileStat.st_size, dnstring);
    }
    fclose(pFile);

    // write everything back to the file
    pFile = fopen(meta_file.c_str(), "w");
    if (pFile == NULL) {
      logger.msg(ERROR, "Error opening meta file for writing %s: %s", meta_file, strerror(errno));
      return false;
    }
    fputs((char*)first_line.c_str(), pFile);
    for (std::vector<std::string>::iterator i = dnlist.begin(); i != dnlist.end(); i++)
      fputs((char*)i->c_str(), pFile);
    fclose(pFile);
    return true;
  }

  bool FileCache::CheckDN(std::string url, std::string DN) {

    if (DN.empty())
      return false;

    std::string meta_file = _getMetaFileName(url);
    struct stat fileStat;
    int err = stat(meta_file.c_str(), &fileStat);
    if (0 != err) {
      if (errno != ENOENT)
        logger.msg(ERROR, "Error reading meta file %s: %s", meta_file, strerror(errno));
      return false;
    }
    FILE *pFile;
    pFile = fopen(meta_file.c_str(), "r");
    if (pFile == NULL) {
      logger.msg(ERROR, "Error opening meta file %s: %s", meta_file, strerror(errno));
      return false;
    }

    std::string dnstring;
    fgets(pFile, fileStat.st_size, dnstring); // first line
    // read in list of DNs
    bool res = fgets(pFile, fileStat.st_size, dnstring);
    while (res) {
      std::string::size_type space_pos = dnstring.rfind(' ');
      if (dnstring.substr(0, space_pos) == DN) {
        if (dnstring.find('\n') != std::string::npos)
          dnstring.resize(dnstring.find('\n'));
        std::string exp_time = dnstring.substr(space_pos + 1);
        if (Time(exp_time) > Time()) {
          logger.msg(VERBOSE, "DN %s is cached and is valid until %s for URL %s", DN, Time(exp_time).str(), url);
          fclose(pFile);
          return true;
        }
        else {
          logger.msg(VERBOSE, "DN %s is cached but has expired for URL %s", DN, url);
          fclose(pFile);
          return false;
        }
      }
      res = fgets(pFile, fileStat.st_size, dnstring);
    }
    fclose(pFile);
    return false;
  }

  bool FileCache::CheckCreated(std::string url) {

    // check the cache file exists - if so we can get the creation date
    // follow symlinks
    std::string cache_file = File(url);
    struct stat fileStat;
    return (stat(cache_file.c_str(), &fileStat) == 0) ? true : false;
  }

  Time FileCache::GetCreated(std::string url) {

    // check the cache file exists
    std::string cache_file = File(url);
    // follow symlinks
    struct stat fileStat;
    if (stat(cache_file.c_str(), &fileStat) != 0) {
      if (errno == ENOENT)
        logger.msg(ERROR, "Cache file %s does not exist", cache_file);
      else
        logger.msg(ERROR, "Error accessing cache file %s: %s", cache_file, strerror(errno));
      return 0;
    }

    time_t ctime = fileStat.st_ctime;
    if (ctime <= 0)
      return Time(0);
    return Time(ctime);
  }

  bool FileCache::CheckValid(std::string url) {
    return (GetValid(url) != Time(0));
  }

  Time FileCache::GetValid(std::string url) {

    // open meta file and pick out expiry time if it exists

    FILE *pFile;
    char mystring[1024]; // should be long enough for a pid or url...
    pFile = fopen((char*)_getMetaFileName(url).c_str(), "r");
    if (pFile == NULL) {
      logger.msg(ERROR, "Error opening meta file %s: %s", _getMetaFileName(url), strerror(errno));
      return Time(0);
    }
    if (fgets(mystring, sizeof(mystring), pFile) == NULL) {
      logger.msg(ERROR, "Error reading meta file %s: %s", _getMetaFileName(url), strerror(errno));
      fclose(pFile);
      return Time(0);
    }
    fclose(pFile);

    std::string meta_str(mystring);
    // get the first line
    if (meta_str.find('\n') != std::string::npos)
      meta_str.resize(meta_str.find('\n'));

    // if the file contains only the url, we don't have an expiry time
    if (meta_str == url)
      return Time(0);

    // check sensible formatting - should be like "rls://rls1.ndgf.org/file1 20080101123456Z"
    if (meta_str.substr(0, url.length() + 1) != url + " ") {
      logger.msg(ERROR, "Mismatching url in file %s: %s Expected %s", _getMetaFileName(url), meta_str, url);
      return Time(0);
    }
    if (meta_str.length() != url.length() + 16) {
      logger.msg(ERROR, "Bad format in file %s: %s", _getMetaFileName(url), meta_str);
      return Time(0);
    }
    if (meta_str.substr(url.length(), 1) != " ") {
      logger.msg(ERROR, "Bad separator in file %s: %s", _getMetaFileName(url), meta_str);
      return Time(0);
    }
    if (meta_str.substr(url.length() + 1).length() != 15) {
      logger.msg(ERROR, "Bad value of expiry time in %s: %s", _getMetaFileName(url), meta_str);
      return Time(0);
    }

    // convert to Time object
    return Time(meta_str.substr(url.length() + 1));
  }

  bool FileCache::SetValid(std::string url, Time val) {

    std::string meta_file = _getMetaFileName(url);
    FILE *pFile;
    pFile = fopen((char*)meta_file.c_str(), "w");
    if (pFile == NULL) {
      logger.msg(ERROR, "Error opening meta file %s: %s", meta_file, strerror(errno));
      return false;
    }
    std::string file_data = url + " " + val.str(MDSTime);
    fputs((char*)file_data.c_str(), pFile);
    fclose(pFile);
    return true;
  }

  bool FileCache::operator==(const FileCache& a) {
    if (a._caches.size() != _caches.size())
      return false;
    for (int i = 0; i < (int)a._caches.size(); i++) {
      if (a._caches.at(i).cache_path != _caches.at(i).cache_path)
        return false;
      if (a._caches.at(i).cache_link_path != _caches.at(i).cache_link_path)
        return false;
    }
    return (
             a._id == _id &&
             a._uid == _uid &&
             a._gid == _gid
             );
  }
  bool FileCache::_checkLock(std::string url) {

    std::string filename = File(url);
    std::string lock_file = _getLockFileName(url);

    // check for existence of lock file
    struct stat fileStat;
    int err = stat(lock_file.c_str(), &fileStat);
    if (0 != err) {
      if (errno == ENOENT)
        logger.msg(ERROR, "Lock file %s doesn't exist", lock_file);
      else
        logger.msg(ERROR, "Error listing lock file %s: %s", lock_file, strerror(errno));
      return false;
    }

    // check the lock file's pid and hostname matches ours
    FILE *pFile;
    char lock_info[100]; // should be long enough for a pid + hostname
    pFile = fopen((char*)lock_file.c_str(), "r");
    if (pFile == NULL) {
      logger.msg(ERROR, "Error opening lock file %s: %s", lock_file, strerror(errno));
      return false;
    }
    if (fgets(lock_info, 100, pFile) == NULL) {
      logger.msg(ERROR, "Error reading lock file %s: %s", lock_file, strerror(errno));
      fclose(pFile);
      return false;
    }
    fclose(pFile);

    std::string lock_info_s(lock_info);
    std::string::size_type index = lock_info_s.find("@", 0);
    if (index == std::string::npos) {
      logger.msg(ERROR, "Error with formatting in lock file %s: %s", lock_file, lock_info_s);
      return false;
    }

    if (lock_info_s.substr(index + 1) != _hostname) {
      logger.msg(VERBOSE, "Lock is owned by a different host");
      // TODO: here do ssh login and check
      return false;
    }
    if (lock_info_s.substr(0, index) != _pid) {
      logger.msg(ERROR, "Another process owns the lock on file %s. Must go back to Start()", filename);
      return false;
    }
    return true;
  }

  std::string FileCache::_getLockFileName(std::string url) {
    return File(url) + CACHE_LOCK_SUFFIX;
  }

  std::string FileCache::_getMetaFileName(std::string url) {
    return File(url) + CACHE_META_SUFFIX;
  }

  bool FileCache::_cacheMkDir(std::string dir, bool all_read) {

    struct stat fileStat;
    int err = stat(dir.c_str(), &fileStat);
    if (0 != err) {
      logger.msg(VERBOSE, "Creating directory %s", dir);
      std::string::size_type slashpos = 0;

      // set perms based on all_read
      mode_t perm = S_IRWXU;
      if (all_read)
        perm |= S_IRGRP | S_IROTH | S_IXGRP | S_IXOTH;

      do {
        slashpos = dir.find("/", slashpos + 1);
        std::string dirname = dir.substr(0, slashpos);
        // list dir to see if it exists (we can't tell the difference between
        // dir already exists and permission denied)
        struct stat statbuf;
        if (stat(dirname.c_str(), &statbuf) == 0)
          continue;

        if (mkdir(dirname.c_str(), perm) != 0)
          if (errno != EEXIST) {
            logger.msg(ERROR, "Error creating required dirs: %s", strerror(errno));
            return false;
          }
        // chmod to get around GM umask setting
        if (chmod(dirname.c_str(), perm) != 0) {
          logger.msg(ERROR, "Error changing permission of dir %s: %s", dirname, strerror(errno));
          return false;
        }
      } while (slashpos != std::string::npos);
    }
    return true;
  }

  int FileCache::_chooseCache(std::string url) {
    
    // get the hash of the url
    std::string hash = FileCacheHash::getHash(url);
    int index = 0;
    for (int level = 0; level < CACHE_DIR_LEVELS; level ++) {
       hash.insert(index + CACHE_DIR_LENGTH, "/");
       // go to next slash position, add one since we just inserted a slash
       index += CACHE_DIR_LENGTH + 1;
    }
  
    int caches_size = _caches.size();
  
    // When there is only one cache directory   
    if (caches_size == 1) {
      return 0;
    }
    // check the fs to see if the file is already there
    for (int i = 0; i < caches_size ; i++) { 
      struct stat fileStat;  
      std::string c_file = _caches[i].cache_path + "/" + CACHE_DATA_DIR +"/" + hash;  
      if (stat(c_file.c_str(), &fileStat) == 0) {
        return i; 
      }  
    }
  
    // find a cache with the most unsed space and also the cache_size parameter defined in "arc.conf"
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
        under_limit.insert(std::make_pair(cache_it->first, roundf((float) cache_it->second.first/total_size*10)));
      } else {
        // caches which are passed the defined percentage
        over_limit.insert(std::make_pair(cache_it->second.second, cache_it->first));
      }
    }
    int cache_no = 0;
    if (under_limit.size() > 0) {
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
  
  std::pair <unsigned long long, unsigned long long> FileCache::_getCacheInfo(std::string path) {
  
    struct statvfs info;
    if (statvfs(path.c_str(), &info) != 0) {
      logger.msg(ERROR, "Error getting info from statvfs for the path: %s", path);
    }
    // return a pair of <cache total size (KB), cache free space (KB)>
    return std::make_pair((info.f_blocks * info.f_bsize)/1024, (info.f_bfree * info.f_bsize)/1024); 
  }

} // namespace Arc

#endif /*WIN32*/
