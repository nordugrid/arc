// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <cstdlib>
// NOTE: On Solaris errno is not working properly if cerrno is included first
#include <cerrno>

#include <fcntl.h>
#include <unistd.h>
#include <glibmm.h>
#include <sys/stat.h>

#ifdef WIN32
#include <arc/win32.h>
#define NOGDI
#include <winsock2.h>
#endif

#include <arc/StringConv.h>
#include <arc/Utils.h>
#include <arc/User.h>

#include "FileLock.h"

namespace Arc {

  const int FileLock::DEFAULT_LOCK_TIMEOUT = 30;
  const std::string FileLock::LOCK_SUFFIX = ".lock";
  Logger FileLock::logger(Logger::getRootLogger(), "FileLock");

  FileLock::FileLock(const std::string& filename,
                     unsigned int timeout,
                     bool use_pid)
    : filename(filename),
      lock_file(filename + LOCK_SUFFIX),
      timeout(timeout),
      use_pid(use_pid),
      pid(""),
      hostname("") {
    if (use_pid) {
      // get our hostname and pid
      char host[256];
      if (gethostname(host, sizeof(host)) != 0)
        logger.msg(ERROR, "Cannot determine hostname from gethostname()");
      else
        hostname = host;
      int pid_i = getpid();
      pid = Arc::tostring(pid_i);
    }
  }

  bool FileLock::acquire() {
    bool lock_removed = false;
    return acquire(lock_removed);
  }

  bool FileLock::acquire(bool& lock_removed) {
    // locking mechanism:
    // - check if lock is there
    // - if not, create tmp file and check again
    // - if lock is still not there copy tmp file to .lock file
    // - check pid inside lock file matches ours
    return acquire_(lock_removed);
  }


  bool FileLock::acquire_(bool& lock_removed) {

    struct stat fileStat;
    int err = stat(lock_file.c_str(), &fileStat);
    if (0 != err) {
      if (errno == EACCES) {
        logger.msg(ERROR, "EACCES Error opening lock file %s: %s", lock_file, StrError(errno));
        return false;
      }
      else if (errno != ENOENT) {
        // some other error occurred opening the lock file
        logger.msg(ERROR, "Error opening lock file %s in initial check: %s", lock_file, StrError(errno));
        return false;
      }
      // lock does not exist - create tmp file
      std::string tmpfile = lock_file + ".XXXXXX";
      int h = Glib::mkstemp(tmpfile);
      if (h == -1) {
        logger.msg(ERROR, "Error creating file %s with mkstemp(): %s", tmpfile, StrError(errno));
        return false;
      }
      // write pid@hostname to the lock file
      if (use_pid) {
        std::string buf = pid + "@" + hostname;
        if (write(h, buf.c_str(), buf.length()) == -1) {
          logger.msg(ERROR, "Error writing to tmp lock file %s: %s", tmpfile, StrError(errno));
          // not much we can do if this doesn't work, but it is only a tmp file
          remove(tmpfile.c_str());
          close(h);
          return false;
        }
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
            logger.msg(ERROR, "Error renaming tmp file %s to lock file %s: %s", tmpfile, lock_file, StrError(errno));
            remove(tmpfile.c_str());
            return false;
          }
          // check it's really there
          err = stat(lock_file.c_str(), &fileStat);
          if (0 != err) {
            logger.msg(ERROR, "Error renaming lock file %s, even though rename() did not return an error: %s",
                       lock_file, StrError(errno));
            return false;
          }
          // check the pid inside the lock file, just in case...
          if (use_pid) {
            FILE *pFile;
            char lock_info[100]; // should be long enough for a pid + hostname
            pFile = fopen((char*)lock_file.c_str(), "r");
            if (pFile == NULL) {
              logger.msg(ERROR, "Error opening lock file %s: %s", lock_file, StrError(errno));
              return false;
            }
            if (fgets(lock_info, 100, pFile) == NULL && errno != 0) {
              logger.msg(ERROR, "Error reading lock file %s: %s", lock_file, StrError(errno));
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

            if (!lock_info_s.substr(index + 1).empty() && lock_info_s.substr(index + 1) != hostname) {
              logger.msg(VERBOSE, "Lock %s is owned by a different host", lock_file);
              // TODO: here do ssh login and check
              return false;
            }
            if (lock_info_s.substr(0, index) != pid) {
              logger.msg(ERROR, "Another process owns the lock on file %s. Must go back to acquire()", filename);
              return false;
            }
          }
        }
        else if (errno == EACCES) {
          logger.msg(ERROR, "EACCES Error opening lock file %s: %s", lock_file, StrError(errno));
          remove(tmpfile.c_str());
          return false;
        }
        else {
          // some other error occurred opening the lock file
          logger.msg(ERROR, "Error opening lock file we just renamed successfully %s: %s", lock_file, StrError(errno));
          remove(tmpfile.c_str());
          return false;
        }
      }
      else {
        logger.msg(VERBOSE, "The file %s is currently locked with a valid lock", filename);
        remove(tmpfile.c_str());
        return false;
      }
    }
    else {
      // the lock already exists, check if it has expired
      // look at modification time
      time_t mod_time = fileStat.st_mtime;
      time_t now = time(NULL);
      logger.msg(VERBOSE, "%li seconds since lock file %s was created", now - mod_time, lock_file);

      if ((now - mod_time) > timeout) {
        logger.msg(VERBOSE, "Timeout has expired, will remove lock file %s", lock_file);
        // TODO: kill the process holding the lock, only if we know it was the original
        // process which created it
        if (remove(lock_file.c_str()) != 0 && errno != ENOENT) {
          logger.msg(ERROR, "Failed to remove file %s: %s", lock_file, StrError(errno));
          return false;
        }
        // lock has expired and has been removed. Call acquire() again
        lock_removed = true;
        return acquire_(lock_removed);
      }

      // lock is still valid, check if we own it
      if (use_pid) {
        // empty lock could have been created with use_pid = false
        // in this case we have to wait for timeout
        if (fileStat.st_size == 0) {
          logger.msg(ERROR, "Found empty lock file %s", lock_file);
          return false;
        }

        FILE *pFile;
        char lock_info[100]; // should be long enough for a pid + hostname
        pFile = fopen((char*)lock_file.c_str(), "r");
        if (pFile == NULL) {
          // lock could have been released by another process, so call acquire again
          if (errno == ENOENT) {
            logger.msg(VERBOSE, "Lock %s that recently existed has been deleted by another process, calling acquire() again", lock_file);
            return acquire_(lock_removed);
          }
          logger.msg(ERROR, "Error opening valid and existing lock file %s: %s", lock_file, StrError(errno));
          return false;
        }
        if (fgets(lock_info, 100, pFile) == NULL && errno != 0) {
          logger.msg(ERROR, "Error reading valid and existing lock file %s: %s", lock_file, StrError(errno));
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

        if (!lock_info_s.substr(index + 1).empty() && lock_info_s.substr(index + 1) != hostname) {
          logger.msg(VERBOSE, "Lock %s is owned by a different host", lock_file);
          // TODO: here do ssh login and check
          return false;
        }
        std::string lock_pid = lock_info_s.substr(0, index);
        if (lock_pid == pid)
          // safer to wait until lock expires than risk corruption
          logger.msg(INFO, "This process already owns the lock on %s", filename);
        else {
          // check if the pid owning the lock is still running - if not we can claim the lock
          // this check only works on systems with /proc. On other systems locks will always be valid
          std::string procdir("/proc/");
          if (stat(procdir.c_str(), &fileStat) == 0) {
            procdir = procdir.append(lock_pid);
            if (stat(procdir.c_str(), &fileStat) != 0 && errno == ENOENT) {
              logger.msg(VERBOSE, "The process owning the lock on %s is no longer running, will remove lock", filename);
              if (remove(lock_file.c_str()) != 0) {
                logger.msg(ERROR, "Failed to remove file %s: %s", lock_file, StrError(errno));
                return false;
              }
              // call acquire() again
              lock_removed = true;
              return acquire_(lock_removed);
            }
          }
          else {
            logger.msg(INFO, "Cannot check process info - assuming lock on %s is still valid", filename);
          }
        }
      }
      logger.msg(VERBOSE, "The file %s is currently locked with a valid lock", filename);
      return false;
    }

    // if we get to here we have acquired the lock
    return true;
  }

  bool FileLock::release(bool force) {

    if (!force && !check())
      return false;

    // delete the lock
    if (remove(lock_file.c_str()) != 0 && errno != ENOENT) {
      logger.msg(ERROR, "Failed to unlock file with lock %s: %s", lock_file, StrError(errno));
      return false;
    }
    return true;
  }

  bool FileLock::check() {
    // check for existence of lock file
    struct stat fileStat;
    int err = stat(lock_file.c_str(), &fileStat);
    if (0 != err) {
      if (errno == ENOENT)
        logger.msg(ERROR, "Lock file %s doesn't exist", lock_file);
      else
        logger.msg(ERROR, "Error listing lock file %s: %s", lock_file, StrError(errno));
      return false;
    }
    if (use_pid) {
      // an empty lock was created while we held the lock
      if (fileStat.st_size == 0) {
        logger.msg(ERROR, "Found unexpected empty lock file %s. Must go back to acquire()", lock_file);
        return false;
      }
      // check the lock file's pid and hostname matches ours
      FILE *pFile;
      char lock_info[100]; // should be long enough for a pid + hostname
      pFile = fopen((char*)lock_file.c_str(), "r");
      if (pFile == NULL) {
        logger.msg(ERROR, "Error opening lock file %s: %s", lock_file, StrError(errno));
        return false;
      }
      if (fgets(lock_info, 100, pFile) == NULL && errno != 0) {
        logger.msg(ERROR, "Error reading lock file %s: %s", lock_file, StrError(errno));
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

      if (!lock_info_s.substr(index + 1).empty() && lock_info_s.substr(index + 1) != hostname) {
        logger.msg(VERBOSE, "Lock %s is owned by a different host", lock_file);
        // TODO: here do ssh login and check
        return false;
      }
      if (lock_info_s.substr(0, index) != pid) {
        logger.msg(ERROR, "Another process owns the lock on file %s. Must go back to acquire()", filename);
        return false;
      }
    }
    return true;
  }

  std::string FileLock::getLockSuffix() {
    return LOCK_SUFFIX;
  }

} // namespace Arc

