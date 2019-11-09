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
#include <signal.h>
#include <sys/stat.h>

#include <arc/FileUtils.h>
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
      if (gethostname(host, sizeof(host)) != 0) {
        logger.msg(WARNING, "Cannot determine hostname from gethostname()");
      } else {
        host[sizeof(host)-1] = 0;
        hostname = host;
      }
      int pid_i = getpid();
      pid = Arc::tostring(pid_i);
    }
  }

  bool FileLock::acquire() {
    bool lock_removed = false;
    return acquire(lock_removed);
  }

  bool FileLock::acquire(bool& lock_removed) {
    return acquire_(lock_removed);
  }

  bool FileLock::write_pid(int h) {
    if (!use_pid) return true;
    std::string buf = pid + "@" + hostname;
    std::string::size_type p = 0;
    for (;p<buf.length();) {
      ssize_t ll = ::write(h, buf.c_str()+p, buf.length()-p);
      if (ll == -1) {
        if(errno == EINTR) continue;
        return false;
      }
      p += ll;
    }
    return true;
  }

  bool FileLock::acquire_(bool& lock_removed) {
    // locking mechanism:
    // - check if lock is there
    //   - if not, create tmp file and attempt link to lock file
    //   - if linking fails with file exists, go back to start
    //   - if linking fails with wrong permission (linking not supported) exclusively
    //     create lock file.
    //   - if creation fails with file exists, go back to start
    //   - if nothing works - fail
    // - if lock exists check pid and host
    //   - if lock has timed out or pid no longer exists delete it and go back to start

    struct stat fileStat;
    if (!FileStat(lock_file, &fileStat, false)) {
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
      std::string lock_content = use_pid ? pid + "@" + hostname : "";
      if (!TmpFileCreate(tmpfile, lock_content)) {
        logger.msg(ERROR, "Error creating temporary file %s: %s", tmpfile, StrError(errno));
        return false;
      }
      // create lock by creating hard link - should be an atomic operation
      errno = EPERM; // Workaround for systems without links is open with O_EXCL
      if (!FileLink(tmpfile.c_str(), lock_file.c_str(), false)) {
        remove(tmpfile.c_str());
        if (errno == EEXIST) {
          // another process got there first
          logger.msg(INFO, "Could not create link to lock file %s as it already exists", lock_file);
          // should recursion depth be limited somehow?
          return acquire_(lock_removed);
        } else if (errno == EPERM) {
          // Most probably links not supported
          // Trying to create lock file
          // Which permissions should it have?
          int h = ::open(lock_file.c_str(),O_WRONLY|O_CREAT|O_EXCL,S_IRUSR|S_IWUSR);
          if(h == -1) {
            if(errno == EEXIST) {
              // lock is taken
              logger.msg(INFO, "Could not create lock file %s as it already exists", lock_file);
              // should recursion depth be limited somehow?
              return acquire_(lock_removed);
            }
            logger.msg(ERROR, "Error creating lock file %s: %s", lock_file, StrError(errno));
            return false;
          }
          // success
          if (!write_pid(h)) {
            logger.msg(ERROR, "Error writing to lock file %s: %s", lock_file, StrError(errno));
            remove(lock_file.c_str());
            close(h);
            return false;
          }
          close(h);
          // fall through to success handling code
        } else {
          logger.msg(ERROR, "Error linking tmp file %s to lock file %s: %s", tmpfile, lock_file, StrError(errno));
          return false;
        }
      }
      else {
        remove(tmpfile.c_str());
      }
      // check it's really there with the correct pid and hostname
      if (check(true) != 0) {
        logger.msg(ERROR, "Error in lock file %s, even though linking did not return an error", lock_file);
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
          logger.msg(ERROR, "Failed to remove stale lock file %s: %s", lock_file, StrError(errno));
          return false;
        }
        // lock has expired and has been removed. Call acquire() again
        lock_removed = true;
        return acquire_(lock_removed);
      }

      // lock is still valid, check if we own it
      int lock_pid = check(false);
      if (lock_pid == 0) {
        // safer to wait until lock expires than risk corruption
        logger.msg(INFO, "This process already owns the lock on %s", filename);
      }
      else if (lock_pid != -1) {
        // check if the pid owning the lock is still running - if not we can claim the lock
        if (kill(lock_pid, 0) != 0 && errno == ESRCH) {
          logger.msg(VERBOSE, "The process owning the lock on %s is no longer running, will remove lock", filename);
          if (remove(lock_file.c_str()) != 0 && errno != ENOENT) {
            logger.msg(ERROR, "Failed to remove file %s: %s", lock_file, StrError(errno));
            return false;
          }
          // call acquire() again
          lock_removed = true;
          return acquire_(lock_removed);
        }
      }
      logger.msg(VERBOSE, "The file %s is currently locked with a valid lock", filename);
      return false;
    }

    // if we get to here we have acquired the lock
    return true;
  }

  bool FileLock::release(bool force) {

    if (!force && check(true) != 0)
      return false;

    // delete the lock
    if (remove(lock_file.c_str()) != 0 && errno != ENOENT) {
      logger.msg(ERROR, "Failed to unlock file with lock %s: %s", lock_file, StrError(errno));
      return false;
    }
    return true;
  }

  int FileLock::check(bool log_error) {
    LogLevel log_level = (log_error ? ERROR : INFO);
    // check for existence of lock file
    struct stat fileStat;
    if (!FileStat(lock_file, &fileStat, false)) {
      if (errno == ENOENT)
        logger.msg(log_level, "Lock file %s doesn't exist", lock_file);
      else
        logger.msg(log_level, "Error listing lock file %s: %s", lock_file, StrError(errno));
      return -1;
    }
    if (use_pid) {
      // an empty lock was created while we held the lock
      if (fileStat.st_size == 0) {
        logger.msg(log_level, "Found unexpected empty lock file %s. Must go back to acquire()", lock_file);
        return -1;
      }
      // check the lock file's pid and hostname matches ours
      std::list<std::string> lock_info;
      if (!FileRead(lock_file, lock_info)) {
        logger.msg(log_level, "Error reading lock file %s: %s", lock_file, StrError(errno));
        return -1;
      }
      if (lock_info.size() != 1 || lock_info.front().find('@') == std::string::npos) {
        logger.msg(log_level, "Error with formatting in lock file %s", lock_file);
        return -1;
      }
      std::vector<std::string> lock_bits;
      tokenize(trim(lock_info.front()), lock_bits, "@");

      // check hostname if given
      if (lock_bits.size() == 2) {
        std::string lock_host = lock_bits.at(1);
        if (!lock_host.empty() && lock_host != hostname) {
          logger.msg(VERBOSE, "Lock %s is owned by a different host (%s)", lock_file, lock_host);
          return -1;
        }
      }
      // check pid
      std::string lock_pid = lock_bits.at(0);
      if (lock_pid != pid) {
        int lock_pid_i(stringtoi(lock_pid));
        if (lock_pid_i == 0) {
          logger.msg(log_level, "Badly formatted pid %s in lock file %s", lock_pid, lock_file);
          return -1;
        }
        logger.msg(log_level, "Another process (%s) owns the lock on file %s", lock_pid, filename);
        return lock_pid_i;
      }
    }
    return 0;
  }

  std::string FileLock::getLockSuffix() {
    return LOCK_SUFFIX;
  }

} // namespace Arc

