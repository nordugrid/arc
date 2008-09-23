#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <cerrno>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <unistd.h>

#include <arc/Logger.h>

#include "FileCache.h"

namespace Arc {

const std::string FileCache::CACHE_LOCK_SUFFIX = ".lock";
const std::string FileCache::CACHE_META_SUFFIX = ".meta";

Logger FileCache::logger(Logger::getRootLogger(), "FileCache");

FileCache::FileCache(std::string cache_path,
    std::string cache_job_dir_path,
    std::string cache_link_path,
    std::string id,
    uid_t job_uid,
    gid_t job_gid,
    int cache_dir_length,
    int cache_dir_levels) {
  
  // make a vector of one item and call _init
  struct CacheParameters cache_info;
  cache_info.cache_path = cache_path;
  cache_info.cache_job_dir_path = cache_job_dir_path;
  cache_info.cache_link_path = cache_link_path;
  
  std::vector<struct CacheParameters> caches;
  caches.push_back(cache_info);
  
  _init(caches, id, job_uid, job_gid, cache_dir_length, cache_dir_levels);
}

FileCache::FileCache(std::vector<struct CacheParameters> caches,
    std::string id,
    uid_t job_uid,
    gid_t job_gid,
    int cache_dir_length,
    int cache_dir_levels) {
  
  _init(caches, id, job_uid, job_gid, cache_dir_length, cache_dir_levels);
}

bool FileCache::_init(std::vector<struct CacheParameters> caches,
    std::string id,
    uid_t job_uid,
    gid_t job_gid,
    int cache_dir_length,
    int cache_dir_levels) {
  
  _id = id;
  _uid = job_uid;
  _gid = job_gid;
  _cache_dir_length = cache_dir_length;
  _cache_dir_levels = cache_dir_levels;
  
  // check for sane values
  if (_cache_dir_length * _cache_dir_levels >= FileCacheHash::maxLength()) {
    logger.msg(ERROR, "Too many subdirectory levels and/or subdirectory length too long");
    return false;
  }
  
  // for each cache
  for (int i = 0; i < (int)caches.size(); i++) {
    struct CacheParameters cache_params = caches.at(i);
    std::string cache_path = cache_params.cache_path;
    if (cache_path.empty()) {
      logger.msg(ERROR, "No cache directory specified");
      return false;
    }
    std::string cache_job_dir_path = cache_params.cache_job_dir_path;
    if(cache_job_dir_path.empty()) {
      logger.msg(ERROR, "No per-job directory specified");
      return false;
    }
    std::string cache_link_path = cache_params.cache_link_path;
    
    // tidy up paths - take off any trailing slashes
    if (cache_path.rfind("/") == cache_path.length()-1) cache_path = cache_path.substr(0, cache_path.length()-1);
    if (cache_job_dir_path.rfind("/") == cache_job_dir_path.length()-1) cache_job_dir_path = cache_job_dir_path.substr(0, cache_job_dir_path.length()-1);
    if (cache_link_path.rfind("/") == cache_link_path.length()-1) cache_link_path = cache_link_path.substr(0, cache_link_path.length()-1);
  
    // create cache dir
    if (!_cacheMkDir(cache_path, true)) {
      logger.msg(ERROR, "Cannot create directory \"%s\" for cache", cache_path);
      return false;
    }
    // create dir for per-job hard links, making the final dir readable only by the job user
    if (cache_job_dir_path.substr(0, cache_path.length()+1) == cache_path+"/") {
      logger.msg(ERROR, "Cannot have per job link directory in the main cache directory");
      return false;
    }
    std::string hard_link_path = cache_job_dir_path + "/" + _id;
    if (!_cacheMkDir(hard_link_path, true)) {
      logger.msg(ERROR, "Cannot create directory \"%s\" for per-job hard links", hard_link_path);
      return false;
    }
    if (chown(hard_link_path.c_str(), _uid, _gid) != 0) {
      logger.msg(ERROR, "Cannot change owner of %s", cache_path);
      return false;
    }
    if (chmod(hard_link_path.c_str(), S_IRWXU) != 0) {
      logger.msg(ERROR, "Cannot change permissions of \"%s\" to 0700", cache_path);
      return false;
    }

    // add this cache to our list
    cache_params.cache_path = cache_path;
    cache_params.cache_job_dir_path = cache_job_dir_path;
    cache_params.cache_link_path = cache_link_path;
    _caches.push_back(cache_params);
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
}

    
FileCache::FileCache(const FileCache& cache) {
  _caches = cache._caches;
  _cache_path = cache._cache_path;
  _cache_job_dir_path = cache._cache_job_dir_path;
  _cache_link_path = cache._cache_link_path;
  _id = cache._id;
  _uid = cache._uid;
  _gid = cache._gid;
  _cache_dir_length = cache._cache_dir_length;
  _cache_dir_levels = cache._cache_dir_levels;
  
  // our hostname and pid
  struct utsname buf;
  if (uname(&buf) != 0) {
    logger.msg(ERROR, "Cannot determine hostname from uname()");
    // return false;
  }
  _hostname = buf.nodename;
  int pid_i = getpid();
  std::stringstream ss;
  ss << pid_i;
  ss >> _pid;
  
}

FileCache::~FileCache(void) {
}

bool FileCache::Start(std::string url, bool &available, bool &is_locked) {
	
  available = false;
  is_locked = false;
  std::string filename = File(url);
  std::string lock_file = _getLockFileName(url);

  // create directory structure if required, only readable by GM user
  if (!_cacheMkDir(lock_file.substr(0, lock_file.rfind("/")), false)) return false;
  
  int lock_timeout = 86400; // one day timeout on lock TODO: make configurable?
  
  // locking mechanism:
  // - check if lock is there
  // - if not, create tmp file and check again
  // - if lock is still not there copy tmp file to cache lock file
  // - check pid inside lock file matches ours
  
  struct stat fileStat;
  int err = stat( lock_file.c_str(), &fileStat ); 
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
    // ugly char manipulation to get mkstemp() to work...
    char tmpfile[256];
    tmpfile[0] = '\0';
    strcat(tmpfile, lock_file.c_str());
    strcat(tmpfile, ".XXXXXX");
    int h = mkstemp(tmpfile);
    if (h == -1) {
      logger.msg(ERROR, "Error creating file %s with mkstemp(): %s", tmpfile, strerror(errno));
      return false;
    }
    // write pid@hostname to the lock file
    char buf[_pid.length()+_hostname.length()+2];
    sprintf(buf, "%s@%s", _pid.c_str(), _hostname.c_str());
    if (write(h, &buf, strlen(buf)) == -1) {
      logger.msg(ERROR, "Error writing to tmp lock file %s: %s", tmpfile, strerror(errno));
      close(h);
      // not much we can do if this doesn't work, but it is only a tmp file
      remove(tmpfile);
      return false;
    }
    if (close(h) != 0) {
      // not critical as file will be removed after we are done
      logger.msg(WARNING, "Warning: closing tmp lock file %s failed", tmpfile);
    }
    // check again if lock exists, in case creating the tmp file took some time
    err = stat( lock_file.c_str(), &fileStat ); 
    if (0 != err) {
      if (errno == ENOENT) {
        // ok, we can create lock
        if (rename(tmpfile, lock_file.c_str()) != 0) {
          logger.msg(ERROR, "Error renaming tmp file %s to lock file %s: %s", tmpfile, lock_file, strerror(errno));
          remove(tmpfile);
          return false;
        }
        // check it's really there
        err = stat( lock_file.c_str(), &fileStat ); 
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
        return false;
      }
      else {
        // some other error occurred opening the lock file
        logger.msg(ERROR, "Error opening lock file we just renamed successfully %s: %s", lock_file, strerror(errno));
        return false;
      }
    }
    else {
      logger.msg(DEBUG, "The file is currently locked with a valid lock");
      is_locked = true;
      return false;
    }
  }
  else {
    // the lock already exists, check if it has expired
    // look at modification time
    time_t mod_time = fileStat.st_mtime;
    time_t now = time(NULL);
    logger.msg(DEBUG, "%l seconds since lock file was created", now - mod_time);
    
    if ((now - mod_time) > lock_timeout) {
      logger.msg(DEBUG, "Timeout has expired, will remove lock file");
      // TODO: kill the process holding the lock, only if we know it was the original
      // process which created it
      if (remove(lock_file.c_str()) != 0 && errno != ENOENT) {
        logger.msg(ERROR, "Failed to unlock file %s: %s", lock_file, strerror(errno));
        return false;
      }
      // lock has expired and has been removed. Try to remove cache file and call Start() again
      if (remove(filename.c_str()) != 0 && errno != ENOENT) {
        logger.msg(ERROR, "Error removing cache file %s: %s", File(url), strerror(errno));
        return false;
      }
      return Start(url, available, is_locked);
    }
    
    // lock is still valid, check if we own it
    FILE * pFile;
    char lock_info [100]; // should be long enough for a pid + hostname
    pFile = fopen ((char*)lock_file.c_str(), "r");
    if (pFile == NULL) {
      // lock could have been released by another process, so call Start again
      if (errno == ENOENT) {
        logger.msg(DEBUG, "Lock that recently existed has been deleted by another process, calling Start() again");
        return Start(url, available, is_locked);
      }
      logger.msg(ERROR, "Error opening valid and existing lock file %s: %s", lock_file, strerror(errno));
      return false;
    }
    fgets (lock_info, 100, pFile);
    fclose (pFile);
    
    std::string lock_info_s(lock_info);
    std::string::size_type index = lock_info_s.find("@", 0);
    if (index == std::string::npos) {
      logger.msg(ERROR, "Error with formatting in lock file %s: %s", lock_file, lock_info_s);
      return false;
    }
    
    if (lock_info_s.substr(index+1) != _hostname) {
      logger.msg(DEBUG, "Lock is owned by a different host");
      // TODO: here do ssh login and check
      is_locked = true;
      return false;
    }
    std::string lock_pid = lock_info_s.substr(0, index);
    if (lock_pid == _pid) {
      // safer to wait until lock expires than use cached file or re-download
      logger.msg(WARNING, "Warning: This process already owns the lock");
    }
    else {
      // check if the pid owning the lock is still running - if not we can claim the lock
      // this is not really portable... but no other way to do it
      std::string procdir("/proc/");
      procdir = procdir.append(lock_pid);
      if (stat(procdir.c_str(), &fileStat) != 0 && errno == ENOENT) {
        logger.msg(DEBUG, "The process owning the lock is no longer running, will remove lock");
        if (remove(lock_file.c_str()) != 0) {
          logger.msg(ERROR, "Failed to unlock file %s: %s", lock_file, strerror(errno));
          return false;
        }
        // lock has been removed. try to delete cache file and call Start() again
        if (remove(filename.c_str()) != 0 && errno != ENOENT) {
          logger.msg(ERROR, "Error removing cache file %s: %s", File(url), strerror(errno));
          return false;
        }
        return Start(url, available, is_locked);
      }
    }
    
    logger.msg(DEBUG, "The file is currently locked with a valid lock");
    is_locked = true;
    return false;
  }

  // if we get to here we have acquired the lock

  // create the meta file to store the URL, if it does not exist
  std::string meta_file = _getMetaFileName(url);
  err = stat( meta_file.c_str(), &fileStat ); 
  if (0 == err) {
    // check URL inside file for possible hash collisions
    FILE * pFile;
    char mystring [1024]; // should be long enough for a pid or url...
    pFile = fopen ((char*)_getMetaFileName(url).c_str(), "r");
    if (pFile == NULL) {
      logger.msg(ERROR, "Error opening meta file %s: %s", _getMetaFileName(url), strerror(errno));
      remove(lock_file.c_str());
      return false;
    }
    fgets (mystring, sizeof(mystring), pFile);
    fclose (pFile);
    
    std::string meta_str(mystring);
    std::string::size_type space_pos = meta_str.find(' ', 0);
    if (space_pos == std::string::npos) space_pos = meta_str.length(); 
    if (meta_str.substr(0, space_pos) != url) {
      logger.msg(ERROR, "Error: File %s is already cached at %s under a different URL: %s - this file will not be cached", url, filename, meta_str.substr(0, space_pos));
      remove(lock_file.c_str());
      return false;
    }
  }
  else if (errno == ENOENT) {
    // create new file
	  FILE * pFile;
	  pFile = fopen ((char*)meta_file.c_str(), "w");
	  if (pFile == NULL) {
	  	logger.msg(ERROR, "Failed to create info file %s: %s", meta_file, strerror(errno));
	  	remove(lock_file.c_str());
	  	return false;
    }
	  fputs ((char*)url.c_str(), pFile);
	  fclose (pFile);
	  // make read/writeable only by GM user
	  chmod(meta_file.c_str(), S_IRUSR | S_IWUSR);
  }
  else {
  	logger.msg(ERROR, "Error looking up attributes of meta file %s: %s", meta_file, strerror(errno));
  	remove(lock_file.c_str());
  	return false;
  }
  // now check if the cache file is there already
  err = stat( filename.c_str(), &fileStat );
  if (0 == err)  available = true;
  else if (errno != ENOENT) {
    // this is ok, we will download again
    logger.msg(WARNING, "Warning: error looking up attributes of cached file: %s", strerror(errno));
  }
  return true;
}

bool FileCache::Stop(std::string url) {
  
  // check the lock is ok to delete
  if (!_checkLock(url)) return false;

  // delete the lock
  if (remove(_getLockFileName(url).c_str()) != 0) {
    logger.msg(ERROR, "Failed to unlock file with lock %s: %s", _getLockFileName(url), strerror(errno));
    return false;
  }
  return true;
}

bool FileCache::StopAndDelete(std::string url) {
  
  // check the lock is ok to delete, and if so, remove the file and the
  // associated lock
  if (!_checkLock(url)) return false;
  
  // delete the cache file
  if (remove(File(url).c_str()) != 0 && errno != ENOENT) {
    logger.msg(ERROR, "Error removing cache file %s: %s", File(url), strerror(errno));
    return false;
  }
  
  // delete the meta file - not critical so don't fail on error
  if (remove(_getMetaFileName(url).c_str()) != 0) {
    logger.msg(ERROR, "Failed to unlock file with lock %s: %s", _getLockFileName(url), strerror(errno));
  }
  
  // delete the lock
  if (remove(_getLockFileName(url).c_str()) != 0) {
    logger.msg(ERROR, "Failed to unlock file with lock %s: %s", _getLockFileName(url), strerror(errno));
    return false;
  }
  return true;
}

std::string FileCache::File(std::string url) {
  
  // get the hash of the url
  std::string hash = FileCacheHash::getHash(url);
  
  int index = 0;
  for(int level = 0; level < _cache_dir_levels; level ++) {
    hash.insert(index + _cache_dir_length, "/");
    // go to next slash position, add one since we just inserted a slash
    index += _cache_dir_length + 1;
  }
  return _caches.at(_chooseCache(hash)).cache_path+"/"+hash;
}

bool FileCache::LinkFile(std::string link_path, std::string url) {

  // choose cache
  struct CacheParameters cache_params = _caches.at(_chooseCache(FileCacheHash::getHash(url)));
  
  // if _cache_link_path is '.' then copy instead, bypassing the hard-link
  if (cache_params.cache_link_path == ".") return CopyFile(link_path, url);
   
  std::string hard_link_path = cache_params.cache_job_dir_path + "/" + _id;
  std::string filename = link_path.substr(link_path.rfind("/")+1);
  std::string hard_link_file = hard_link_path + "/" + filename;
  std::string session_dir = link_path.substr(0, link_path.rfind("/"));
  
  // check the original file exists
  struct stat fileStat;
  if (stat(File(url).c_str(), &fileStat) != 0) {
    if (errno == ENOENT) logger.msg(ERROR, "Cache file %s does not exist", File(url));
    else logger.msg(ERROR, "Error accessing cache file %s: %s", File(url), strerror(errno));
    return false;
  }
  
  // make the hard link
  if (link(File(url).c_str(), hard_link_file.c_str()) != 0) {
    logger.msg(ERROR, "Failed to create hard link from %s to %s: %s", hard_link_file, File(url), strerror(errno));
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
  if (!_cacheMkDir(session_dir, true)) return false;
  if (chown(session_dir.c_str(), _uid, _gid) != 0) {
    logger.msg(ERROR, "Failed to change owner of session dir to %i: %s", _uid, strerror(errno));
    return false;
  }
  if (chmod(session_dir.c_str(), S_IRWXU) != 0) {
    logger.msg(ERROR, "Failed to change permissions of session dir to 0700: %s", strerror(errno));
    return false;
  }
  
  // make the soft link, changing the target if cache_link_path is defined
  if (!_cache_link_path.empty()) {
    hard_link_file.replace(0, cache_params.cache_job_dir_path.length(), _cache_link_path, 0, cache_params.cache_link_path.length());
  }
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

bool FileCache::CopyFile(std::string dest_path, std::string url) {
  
  // check the original file exists
  std::string cache_file = File(url);
  struct stat fileStat;
  if (stat(cache_file.c_str(), &fileStat) != 0) {
    if (errno == ENOENT) logger.msg(ERROR, "Cache file %s does not exist", cache_file);
    else logger.msg(ERROR, "Error accessing cache file %s: %s", cache_file, strerror(errno));
    return false;
  }
  
  // make necessary dirs for the copy
  // this probably should have already been done... somewhere...
  std::string dest_dir = dest_path.substr(0, dest_path.rfind("/"));
  if (!_cacheMkDir(dest_dir, true)) return false;
  if (chown(dest_dir.c_str(), _uid, _gid) != 0) {
    logger.msg(ERROR, "Failed to change owner of destination dir to %i: %s", _uid, strerror(errno));
    return false;
  }
  if (chmod(dest_dir.c_str(), S_IRWXU) != 0) {
    logger.msg(ERROR, "Failed to change permissions of session dir to 0700: %s", strerror(errno));
    return false;
  }

  // do the copy - taken directly from old datacache.cc
  char buf[65536];
  int fdest = open(dest_path.c_str(), O_WRONLY | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR);
  if(fdest == -1) {
    logger.msg(ERROR, "Failed to create file %s for writing: %s", dest_path, strerror(errno));
    return false;
  };
  fchown(fdest, _uid, _gid);
  
  int fsource = open(cache_file.c_str(), O_RDONLY);
  if(fsource == -1) {
    close(fdest);
    logger.msg(ERROR, "Failed to open file %s for reading: %s", cache_file, strerror(errno));
    return false;
  };
  
  // source and dest opened ok - copy in chunks
  for(;;) {
    ssize_t lin = read(fsource, buf, sizeof(buf));
    if(lin == -1) {
      close(fdest); close(fsource);
      logger.msg(ERROR, "Failed to read file %s: %s", cache_file, strerror(errno));
      return false;
    };
    if(lin == 0) break; // eof
    
    for(ssize_t lout = 0; lout < lin;) {
      ssize_t lwritten = write(fdest, buf+lout, lin-lout);
      if(lwritten == -1) {
        close(fdest); close(fsource);
        logger.msg(ERROR, "Failed to write file %s: %s", dest_path, strerror(errno));
        return false;
      };
      lout += lwritten;
    };
  };
  close(fdest); close(fsource);
  return true;
}

bool FileCache::Release() {
  
  // go through all caches and remove per-job dirs for our job id
  for (int i = 0; i < (int)_caches.size(); i++) {
    
    std::string job_dir = _caches.at(i).cache_job_dir_path + "/" + _id;
    
    // check if job dir exists
    DIR * dirp = opendir(job_dir.c_str());
    if ( dirp == NULL) {
      if (errno == ENOENT) continue;
      logger.msg(ERROR, "Error opening per-job dir %s: %s", job_dir, strerror(errno));
      return false;
    }
    
    // list all files in the dir and delete them
    struct dirent *dp;
    errno = 0;
    while ((dp = readdir(dirp)))  {
      if (strcmp(dp->d_name, ".") == 0 || strcmp(dp->d_name, "..") == 0) continue;
      std::string to_delete = job_dir + "/" + dp->d_name; 
      logger.msg(DEBUG, "Removing %s", to_delete);
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
    logger.msg(DEBUG, "Removing %s", job_dir);
    if (rmdir(job_dir.c_str()) != 0) {
      logger.msg(ERROR, "Failed to remove cache per-job dir %s: %s", job_dir, strerror(errno));
      return false;
    }
  }
  return true;
}

bool FileCache::CheckCreated(std::string url) {
  
  // check the cache file exists - if so we can get the creation date
  std::string cache_file = File(url);
  struct stat fileStat;
  return (stat(cache_file.c_str(), &fileStat) == 0) ? true : false;
}

Time FileCache::GetCreated(std::string url) {
  
  // check the cache file exists
  std::string cache_file = File(url);
  struct stat fileStat;
  if (stat(cache_file.c_str(), &fileStat) != 0) {
    if (errno == ENOENT) logger.msg(ERROR, "Cache file %s does not exist", cache_file);
    else logger.msg(ERROR, "Error accessing cache file %s: %s", cache_file, strerror(errno));
    return 0;
  }
  
  time_t ctime = fileStat.st_ctime;
  if (ctime <= 0) return 0;
  return ctime;
}

bool FileCache::CheckValid(std::string url) {
  return (GetValid(url) != -0);
}

Time FileCache::GetValid(std::string url) {
  
  // open meta file and pick out expiry time if it exists
  
  FILE * pFile;
  char mystring [1024]; // should be long enough for a pid or url...
  pFile = fopen ((char*)_getMetaFileName(url).c_str(), "r");
  if (pFile == NULL) {
    logger.msg(ERROR, "Error opening meta file %s: %s", _getMetaFileName(url), strerror(errno));
    return -1;
  }
  fgets (mystring, sizeof(mystring), pFile);
  fclose (pFile);
  
  std::string meta_str(mystring);
  // if the file contains only the url, we don't have an expiry time
  if (meta_str == url) return 0;

  // check sensible formatting - should be like "rls://rls1.ndgf.org/file1 1234567890"
  if (meta_str.substr(0, url.length()+1) != url+" ") {
    logger.msg(ERROR, "Mismatching url in file %s: %s Expected %s", _getMetaFileName(url), meta_str, url);
    return 0;
  }
  if (meta_str.length() != url.length() + 11) {
    logger.msg(ERROR, "Bad format in file %s: %s", _getMetaFileName(url), meta_str);
    return 0;
  }
  if (meta_str.substr(url.length(), 1) != " ") {
    logger.msg(ERROR, "Bad separator in file %s: %s", _getMetaFileName(url), meta_str);
    return 0;    
  }
  if (meta_str.substr(url.length() + 1).length() != 10) {
    logger.msg(ERROR, "Bad value of expiry time in %s: %s", _getMetaFileName(url), meta_str);
    return 0;
  }
  
  // convert to int
  int exp_time;
  if(EOF == sscanf(meta_str.substr(url.length() + 1).c_str(), "%i", &exp_time) || exp_time < 0) {
    logger.msg(ERROR, "Error with converting time in file %s: %s", _getMetaFileName(url), meta_str);
    return 0;
  }
  return (time_t)exp_time;
}

bool FileCache::SetValid(std::string url, Time val) {
  
  std::string meta_file = _getMetaFileName(url);
  FILE * pFile;
  pFile = fopen ((char*)meta_file.c_str(), "w");
  if (pFile == NULL) {
    logger.msg(ERROR, "Error opening meta file %s: %s", meta_file, strerror(errno));
    return false;
  }
  // convert time to string
  std::stringstream out;
  out << val;
  std::string file_data = url + " " + out.str();
  fputs ((char*)file_data.c_str(), pFile);
  fclose (pFile);
  return true;
}

bool FileCache::operator==(const FileCache& a) {
  if (a._caches.size() != _caches.size()) return false;
  for (int i = 0; i < (int)a._caches.size(); i++) {
    if (a._caches.at(i).cache_path != _caches.at(i).cache_path) return false;
    if (a._caches.at(i).cache_job_dir_path != _caches.at(i).cache_job_dir_path) return false;
    if (a._caches.at(i).cache_link_path != _caches.at(i).cache_link_path) return false;
  }
  return (
    a._cache_path == _cache_path &&
    a._cache_job_dir_path == _cache_job_dir_path &&
    a._cache_link_path == _cache_link_path &&
    a._id == _id &&
    a._uid == _uid &&
    a._gid == _gid &&
    a._cache_dir_length == _cache_dir_length &&
    a._cache_dir_levels == _cache_dir_levels
  );
}
bool FileCache::_checkLock(std::string url) {
  
  std::string filename = File(url);
  std::string lock_file = _getLockFileName(url);
  
  // check for existence of lock file
  struct stat fileStat;
  int err = stat( lock_file.c_str(), &fileStat ); 
  if (0 != err) {
    if (errno == ENOENT) logger.msg(ERROR, "Lock file %s doesn't exist", lock_file);
    else logger.msg(ERROR, "Error listing lock file %s: %s", lock_file, strerror(errno));
    return false;
  }
  
  // check the lock file's pid and hostname matches ours
  FILE * pFile;
  char lock_info [100]; // should be long enough for a pid + hostname
  pFile = fopen ((char*)lock_file.c_str(), "r");
  if (pFile == NULL) {
    logger.msg(ERROR, "Error opening lock file %s: %s", lock_file, strerror(errno));
    return false;
  }
  fgets (lock_info, 100, pFile);
  fclose (pFile);

  std::string lock_info_s(lock_info);
  std::string::size_type index = lock_info_s.find("@", 0);
  if (index == std::string::npos) {
    logger.msg(ERROR, "Error with formatting in lock file %s: %s", lock_file, lock_info_s);
    return false;
  }
  
  if (lock_info_s.substr(index+1) != _hostname) {
    logger.msg(DEBUG, "Lock is owned by a different host");
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
  return File(url)+CACHE_LOCK_SUFFIX;
}

std::string FileCache::_getMetaFileName(std::string url) {
  return File(url)+CACHE_META_SUFFIX;
}

bool FileCache::_cacheMkDir(std::string dir, bool all_read) {
  
  struct stat fileStat;
  int err = stat( dir.c_str(), &fileStat ); 
  if (0 != err) {
    logger.msg(DEBUG, "Creating directory %s", dir);
    std::string::size_type slashpos = 0;
    do {
      slashpos = dir.find("/", slashpos+1);
      std::string dirname = dir.substr(0, slashpos);
      // list dir to see if it exists (we can't tell the difference between
      // dir already exists and permission denied)
      struct stat statbuf;
      if (stat(dirname.c_str(), &statbuf) == 0) {
        continue;
      };
      
      // set perms based on all_read
      mode_t perm = S_IRWXU;
      if (all_read) perm |= S_IRGRP | S_IROTH | S_IXGRP | S_IXOTH;
      if (mkdir(dirname.c_str(), perm) != 0) {
        if (errno != EEXIST) {
          logger.msg(ERROR, "Error creating required dirs: %s", strerror(errno));
          return false;
        };
      };
      // chmod to get around GM umask setting
      if (chmod(dirname.c_str(), perm) != 0) {
        logger.msg(ERROR, "Error changing permission of dir %s: %s", dirname, strerror(errno));
        return false;
      };
    } while (slashpos != std::string::npos);
  }
  return true;
}

int FileCache::_chooseCache(std::string hash) {
  // choose which cache to use
  // divide based on the first character of the hash, choosing the cache
  // based on the letter mod number of caches
  // this algorithm limits the number of caches to 16
  
  char firstletter = hash.at(0);
  int cacheno = firstletter % _caches.size();
  return cacheno;
}

} // namespace Arc
