#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "MkDirRecursive.h"
#include <arc/StringConv.h>
#include <arc/Logger.h>
#include "DataCache.h"
#include <cerrno>
#include <fcntl.h>

namespace Arc {

  Logger DataCache::logger(Logger::getRootLogger(), "DataCache");

  DataCache::DataCache() : have_url(false), cache_user(0) {}

  DataCache::DataCache(const DataCache& cache) :
    cache_path(cache.cache_path),
    cache_data_path(cache.cache_data_path),
    cache_link_path(cache.cache_link_path),
    id(cache.id),
    cache_user(cache.cache_user),
    have_url(false) {
    logger.msg(VERBOSE, "DataCache: constructor with copy");
    if(cache_path.length() == 0)
      return;
    if(cache.have_url) {
      logger.msg(VERBOSE, "DataCache: constructor with copy: calling start");
      bool available;
      start(cache.cache_url, available);
    }
  }

  DataCache::DataCache(const std::string& cache_path_,
                       const std::string& cache_data_path_,
                       const std::string& cache_link_path_,
                       const std::string& id,
                       const Arc::User &cache_user) :
    cache_path(cache_path_),
    cache_data_path(cache_data_path_),
    cache_link_path(cache_link_path_),
    id(id),
    cache_user(cache_user),
    have_url(false) {
    if(cache_path.empty()) {
      cache_data_path.clear();
      cache_link_path.clear();
    }
    else {
      if(cache_data_path.empty())
        cache_data_path = cache_path;
      if(cache_link_path.empty())
        cache_link_path = cache_data_path;
    }
  }

  DataCache::~DataCache() {
    if(have_url)
      stop(file_download_failed);
  }

  bool DataCache::start(const URL& base_url, bool& available) {
    if(have_url)
      return false;
    available = false;
    cache_file = "";
    std::string u;
    std::string url_options;
    /* Find or create new url in cache. This also claims url */
    std::string fname;
    if(cache_find_url(cache_path, cache_data_path, cache_user,
                      base_url.str(), id, url_options, fname) != 0)
      return false;
    cache_url = base_url;
    /* options contain 2 numbers: creation and expiration timestamps */
    if(url_options.length() != 0) {
      std::string::size_type n = url_options.find(' ');
      if(n == std::string::npos)
        n = url_options.length();
      std::string s = url_options.substr(0, n);
      if(s != ".")
        SetCreated(stringtoull(s));
      s = url_options.substr(n + 1);
      if((s.length() != 0) && (s != "."))
        SetValid(stringtoull(s));
      if(CheckCreated() && !CheckValid())
        SetValid(GetCreated() + 24 * 60 * 60);
      if(!CheckValid())
        SetValid(Time() + 24 * 60 * 60);
    }
    switch(cache_download_file_start(cache_path, cache_data_path, cache_user,
                                     fname, id, cdh)) {
    case 1:
      // error
      logger.msg(ERROR, "Error while locking file in cache");
      cache_release_file(cache_path, cache_data_path, cache_user,
                         fname, id, true);
      return false;
    case 2:
      // already have it
      // reread options because they could be changed by process which
      // just downloded that file
      url_options = "";
      if(cache_find_file(cache_path, cache_data_path, cache_user,
                         fname, u, url_options) == 0) {
        if(url_options.length() != 0) {
          std::string::size_type n = url_options.find(' ');
          if(n == std::string::npos)
            n = url_options.length();
          std::string s = url_options.substr(0, n);
          if(s != ".")
            SetCreated(stringtoll(s));
          s = url_options.substr(n + 1);
          if((s.length() != 0) && (s != "."))
            SetValid(stringtoull(s));
          if(CheckCreated() && !CheckValid())
            SetValid(GetCreated() + 24 * 60 * 60);
          if(!CheckValid())
            SetValid(Time() + 24 * 60 * 60);
        }
      }
      available = true;
      break;
    case 0:
      // have to download
      available = false;
      break;
    default:
      logger.msg(ERROR, "Unknown error while locking file in cache");
      cache_release_file(cache_path, cache_data_path, cache_user,
                         fname, id, true);
      return false;
    }
    cache_file = cdh.name();
    have_url = true;
    return true;
  }

  bool DataCache::stop(int file_state) {
    if(!have_url)
      // this object was not initialized
      return false;
    std::string url_options;
    if(!(file_state & (file_download_failed | file_not_valid)))
      // compose url with options
      url_options = cache_url + "\n" +
                    (CheckCreated() ? GetCreated().str() : ".") + " " +
                    (CheckValid() ? GetValid().str() : ".");
    cache_download_url_end(cache_path, cache_data_path, cache_user,
                           url_options, cdh,
      !(file_state & file_download_failed));
    if(file_state & file_not_valid)
      cache_invalidate_url(cache_path, cache_data_path, cache_user,
                           cdh.cache_name());
    if((file_state & (file_download_failed | file_not_valid)) &&
       !(file_state & file_keep))
      // failed to download or outdated file
      cache_release_file(cache_path, cache_data_path, cache_user,
                         cdh.cache_name(), id, true);
    have_url = false;
    cache_file = "";
    return true;
  }

  bool DataCache::clean(unsigned long long int size) {
    logger.msg(INFO, "Cache cleaning requested: %s, %llu bytes",
               cache_path.c_str(), size);
    unsigned long long int freed = cache_clean(cache_path, cache_data_path,
                                               cache_user, size);
    logger.msg(DEBUG, "Cache cleaned: %s, %llu bytes",
               cache_path.c_str(), freed);
    if(freed < size)
      return false;
    return true;
  }

  bool DataCache::link(const std::string& link_path) {
    Arc::User user;
    return link(link_path, user);
  }

  bool DataCache::copy(const std::string& link_path) {
    Arc::User user;
    return copy(link_path, user);
  }

  bool DataCache::link(const std::string& link_path, const Arc::User &user) {
    std::string dirpath = link_path;
    std::string::size_type n = dirpath.rfind('/');
    if(n == std::string::npos)
      dirpath = "/";
    else
      dirpath.erase(n, dirpath.length() - n + 1);
    if(mkdir_recursive(NULL, dirpath, S_IRWXU, user) != 0) {
      if(errno != EEXIST) {
        logger.msg(ERROR, "Failed to create/find directory %s",
                   dirpath.c_str());
        return false;
      }
    }
    if(cache_link_path == ".")
      /* special link - copy request */
      return copy_file(link_path, user);
    else
      return link_file(link_path, user);
  }

  bool DataCache::link_file(const std::string& link_path,
                            const Arc::User &user) {
    std::string fname(cache_file.substr (cache_data_path.length()));
    fname = cache_link_path + fname;
    if(symlink(fname.c_str(), link_path.c_str()) == -1) {
      logger.msg(ERROR, "Failed to make symbolic link %s to %s",
                 link_path.c_str(), fname.c_str());
      return false;
    }
    lchown(link_path.c_str(), user.get_uid(), user.get_gid());
    return true;
  }

  bool DataCache::copy(const std::string& link_path, const Arc::User &user) {
    std::string dirpath = link_path;
    std::string::size_type n = dirpath.rfind('/');
    if(n == std::string::npos)
      dirpath = "/";
    else
      dirpath.erase(n, dirpath.length() - n + 1);
    if(mkdir_recursive(NULL, dirpath, S_IRWXU, user) != 0) {
      if(errno != EEXIST) {
        logger.msg(ERROR, "Failed to create/find directory %s",
                   dirpath.c_str());
        return false;
      }
    }
    return copy_file(link_path, user);
  }

  bool DataCache::copy_file(const std::string& link_path,
                            const Arc::User &user) {
    char buf[65536];
    int fd = open(link_path.c_str(),
                  O_WRONLY | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR);
    if(fd == -1) {
      logger.msg(ERROR, "Failed to create file for writing: %s",
                 link_path.c_str());
      return false;
    }
    fchown(fd, user.get_uid(), user.get_gid());
    int fd_ = open(cache_file.c_str(), O_RDONLY);
    if(fd_ == -1) {
      close(fd);
      logger.msg(ERROR, "Failed to open file for reading: %s",
                 cache_file.c_str());
      return false;
    }
    for(;;) {
      int l = read(fd_, buf, sizeof(buf));
      if(l == -1) {
        close(fd);
        close(fd_);
        logger.msg(ERROR, "Failed to read file: %s", cache_file.c_str());
        return false;
      }
      if(l == 0)
        break;
      for(int ll = 0; ll < l;) {
        int l_ = write(fd, buf + ll, l - ll);
        if(l_ == -1) {
          close(fd);
          close(fd_);
          logger.msg(ERROR, "Failed to write file: %s", link_path.c_str());
          return false;
        }
        ll += l_;
      }
    }
    close(fd);
    close(fd_);
    return true;
  }

  bool DataCache::cb(unsigned long long int size) {
    if(size == 0)
      size = 1;
    return clean(size);
  }

} // namespace Arc
