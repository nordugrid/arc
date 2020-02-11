// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <cstdlib>
// NOTE: On Solaris errno is not working properly if cerrno is included first
#include <cerrno>

#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <utime.h>

#include <glibmm.h>

#include <arc/DateTime.h>
#include <arc/FileLock.h>
#include <arc/FileUtils.h>
#include <arc/Logger.h>
#include <arc/Thread.h>
#include <arc/URL.h>
#include <arc/User.h>
#include <arc/UserConfig.h>
#include <arc/Utils.h>
#include <arc/credential/Credential.h>
#include <arc/data/DataBuffer.h>
#include <arc/CheckSum.h>
#include <arc/data/DataMover.h>
#include <arc/data/DataPoint.h>
#include <arc/data/DataHandle.h>
#include <arc/data/FileCache.h>
#include <arc/data/URLMap.h>

namespace Arc {

  static void transfer_cb(unsigned long long int bytes_transferred) {
    fprintf (stderr, "\r%llu kB                  \r", bytes_transferred / 1024);
  }

  Logger DataMover::logger(Logger::getRootLogger(), "DataMover");

  DataMover::DataMover()
    : be_verbose(false),
      force_secure(false),
      force_passive(false),
      force_registration(false),
      do_checks(true),
      do_retries(true),
      default_min_speed(0),
      default_min_speed_time(0),
      default_min_average_speed(0),
      default_max_inactivity_time(300),
      show_progress(NULL),
      cancelled(false) {}

  DataMover::~DataMover() {
    Cancel();
    // Wait for Transfer() to finish with lock
    Glib::Mutex::Lock lock(lock_);
  }

  bool DataMover::verbose() {
    return be_verbose;
  }

  void DataMover::verbose(bool val) {
    be_verbose = val;
  }

  void DataMover::verbose(const std::string& prefix) {
    be_verbose = true;
    verbose_prefix = prefix;
  }

  bool DataMover::retry() {
    return do_retries;
  }

  void DataMover::retry(bool val) {
    do_retries = val;
  }

  bool DataMover::checks() {
    return do_checks;
  }

  void DataMover::checks(bool val) {
    do_checks = val;
  }

  typedef struct {
    DataPoint *source;
    DataPoint *destination;
    FileCache *cache;
    const URLMap *map;
    unsigned long long int min_speed;
    time_t min_speed_time;
    unsigned long long int min_average_speed;
    time_t max_inactivity_time;
    DataMover::callback cb;
    DataMover *it;
    void *arg;
    const char *prefix;
  } transfer_struct;

  DataStatus DataMover::Delete(DataPoint& url, bool errcont) {
    DataStatus res = DataStatus::Success;
    bool remove_lfn = !url.HaveLocations(); // pfn or plain url
    if (!url.Resolve(true).Passed()) {
      // TODO: Check if error is real or "not exist".
      if (remove_lfn) {
        logger.msg(INFO,
                   "No locations found - probably no more physical instances");
      };
    };
    std::list<URL> removed_urls;
    if (url.HaveLocations())
      for (; url.LocationValid();) {
        logger.msg(INFO, "Removing %s", url.CurrentLocation().str());
        // It can happen that after resolving list contains duplicated
        // physical locations obtained from different meta-data-services.
        // Because not all locations can reliably say if files does not exist
        // or access is not allowed, avoid duplicated delete attempts.
        bool url_was_deleted = false;
        for (std::list<URL>::iterator u = removed_urls.begin(); u
             != removed_urls.end(); ++u)
          if (url.CurrentLocation() == (*u)) {
            url_was_deleted = true;
            break;
          }
        if (url_was_deleted) {
          logger.msg(DEBUG, "This instance was already deleted");
        }
        else {
          url.SetSecure(false);
          DataStatus r = url.Remove();
          if (!r) {
            logger.msg(INFO, "Failed to delete physical file");
            res = r;
            if (!errcont) {
              url.NextLocation();
              continue;
            }
          }
          else
            removed_urls.push_back(url.CurrentLocation());
        }
        if (url.IsIndex()) {
          logger.msg(INFO, "Removing metadata in %s",
                     url.CurrentLocationMetadata());
          DataStatus r = url.Unregister(false);
          if (!r) {
            logger.msg(INFO, "Failed to delete meta-information");
            res = r;
            url.NextLocation();
          }
          else
            url.RemoveLocation();
        }
        else {
          // Leave immediately in case of direct URL
          break;
        }
      }
    if (url.IsIndex()) {
      if (url.HaveLocations()) {
        logger.msg(INFO, "Failed to remove all physical instances");
        return res;
      }
      if (remove_lfn) {
        logger.msg(INFO, "Removing logical file from metadata %s", url.str());
        DataStatus r = url.Unregister(true);
        if (!r) {
          logger.msg(INFO, "Failed to delete logical file");
          return r;
        }
      }
    }
    else {
      if (!url.LocationValid()) {
        logger.msg(INFO, "Failed to remove instance");
        return res;
      }
    }
    return DataStatus::Success;
  }

  void transfer_func(void *arg) {
    transfer_struct *param = (transfer_struct*)arg;
    DataStatus res = param->it->Transfer(*(param->source),
                                         *(param->destination), *(param->cache), *(param->map),
                                         param->min_speed, param->min_speed_time,
                                         param->min_average_speed, param->max_inactivity_time,
                                         NULL, NULL, param->prefix);
    (*(param->cb))(param->it, res, param->arg);
    if (param->prefix) free((void*)(param->prefix));
    if (param->cache) delete param->cache;
    free(param);
  }

  /* transfer data from source to destination */
  DataStatus DataMover::Transfer(DataPoint& source, DataPoint& destination,
                                 FileCache& cache, const URLMap& map,
                                 DataMover::callback cb, void *arg, const char *prefix) {
    return Transfer(source, destination, cache, map, default_min_speed,
                    default_min_speed_time, default_min_average_speed,
                    default_max_inactivity_time, cb, arg, prefix);
  }

  DataStatus DataMover::Transfer(DataPoint& source, DataPoint& destination,
                                 FileCache& cache, const URLMap& map, unsigned long long int min_speed,
                                 time_t min_speed_time, unsigned long long int
                                 min_average_speed, time_t max_inactivity_time,
                                 DataMover::callback cb, void *arg,
                                 const char *prefix) {
    class DataPointStopper {
     private:
      DataPoint& point_;
     public:
      DataPointStopper(DataPoint& p):point_(p) {};
      ~DataPointStopper(void) {
        point_.StopReading();
        point_.FinishReading();
        point_.StopWriting();
        point_.FinishWriting();
      };
    };

    if (cb != NULL) {
      logger.msg(VERBOSE, "DataMover::Transfer : starting new thread");
      transfer_struct *param = (transfer_struct*)malloc(sizeof(transfer_struct));
      if (param == NULL)
        return DataStatus::TransferError;
      param->source = &source;
      param->destination = &destination;
      param->cache = new FileCache(cache);
      param->map = &map;
      param->min_speed = min_speed;
      param->min_speed_time = min_speed_time;
      param->min_average_speed = min_average_speed;
      param->max_inactivity_time = max_inactivity_time;
      param->cb = cb;
      param->it = this;
      param->arg = arg;
      param->prefix = NULL;
      if (prefix)
        param->prefix = strdup(prefix);
      if (param->prefix == NULL)
        param->prefix = strdup(verbose_prefix.c_str());
      if (!CreateThreadFunction(&transfer_func, param)) {
        if(param->prefix) free((void*)(param->prefix));
        if(param->cache) delete param->cache;
        free(param);
        return DataStatus::TransferError;
      }
      return DataStatus::Success;
    }
    logger.msg(INFO, "Transfer from %s to %s", source.str(), destination.str());
    if (!source) {
      logger.msg(ERROR, "Not valid source");
      source.NextTry();
      return DataStatus(DataStatus::ReadAcquireError, EINVAL, "Source is not valid");
    }
    if (!destination) {
      logger.msg(ERROR, "Not valid destination");
      destination.NextTry();
      return DataStatus(DataStatus::WriteAcquireError, EINVAL, "Destination is not valid");
    }
    // initial cache check, if the DN is cached we can exit straight away
    bool cacheable = false;
    bool executable = (source.GetURL().Option("exec") == "yes") ? true : false;
    bool cache_copy = (source.GetURL().Option("cache") == "copy") ? true : false;
    // DN is used for cache permissions
    std::string dn;
    Time exp_time(0);
    if (source.Cache() && destination.Local() && cache) {
      cacheable = true;
      try {
        // TODO (important) load credential in unified way or 
        // use already loaded one
        Credential ci(source.GetUserConfig().ProxyPath(), source.GetUserConfig().ProxyPath(), source.GetUserConfig().CACertificatesDirectory(), "");
        dn = ci.GetIdentityName();
        exp_time = ci.GetEndTime();
      } catch (CredentialError& e) {
        logger.msg(WARNING, "Couldn't handle certificate: %s", e.what());
      }
    }
    if (cacheable && source.GetURL().Option("cache") != "renew" && source.GetURL().Option("cache") != "check") {
      std::string canonic_url = source.str();
      bool is_in_cache = false;
      bool is_locked = false;
      if (cache.Start(canonic_url, is_in_cache, is_locked)) {
        if (is_in_cache) {
          logger.msg(INFO, "File %s is cached (%s) - checking permissions",
                     canonic_url, cache.File(canonic_url));
          // check the list of cached DNs
          if (cache.CheckDN(canonic_url, dn)) {
            logger.msg(VERBOSE, "Permission checking passed");
            logger.msg(INFO, "Linking/copying cached file");
            bool cache_link_result = cache.Link(destination.CurrentLocation().Path(),
                                                canonic_url,
                                                (!source.ReadOnly() || executable || cache_copy),
                                                executable,
                                                false,
                                                is_locked);
            source.NextTry(); /* to decrease retry counter */

            if (cache_link_result) return DataStatus::SuccessCached;
            // if it failed with lock problems - continue and try later
            if (!is_locked) return DataStatus::CacheError;
          }
        } else {
          cache.Stop(canonic_url);
        }
      }
    }

    for (;;) {
      DataStatus dres = source.Resolve(true);
      if (dres.Passed()) {
        if (source.HaveLocations())
          break;
        logger.msg(ERROR, "No locations for source found: %s", source.str());
        dres = DataStatus(DataStatus::ReadResolveError, EARCRESINVAL, "No locations found");
      }
      else
        logger.msg(ERROR, "Failed to resolve source: %s", source.str());
      source.NextTry(); /* try again */
      if (!do_retries)
        return dres;
      if (!source.LocationValid())
        return dres;
    }
    for (;;) {
      DataStatus dres = destination.Resolve(false);
      if (dres.Passed()) {
        if (destination.HaveLocations())
          break;
        logger.msg(ERROR, "No locations for destination found: %s",
                   destination.str());
        dres = DataStatus(DataStatus::WriteResolveError, EARCRESINVAL, "No locations found");
      }
      else
        logger.msg(ERROR, "Failed to resolve destination: %s", destination.str());
      destination.NextTry(); /* try again */
      if (!do_retries)
        return dres;
      if (!destination.LocationValid())
        return dres;
    }
    bool replication = false;
    if (source.IsIndex() && destination.IsIndex())
      // check for possible replication
      if (source.GetURL() == destination.GetURL()) {
        replication = true;
        // we do not want to replicate to same physical file
        destination.RemoveLocations(source);
        if (!destination.HaveLocations()) {
          logger.msg(ERROR, "No locations for destination different from source "
                     "found: %s", destination.str());
          return DataStatus(DataStatus::WriteResolveError, EEXIST);
        }
      }
    // Try to avoid any additional checks meant to provide
    // meta-information whenever possible
    bool checks_required = destination.AcceptsMeta() && (!replication);
    bool destination_meta_initially_stored = destination.Registered();
    bool destination_overwrite = false;
    if (!replication) { // overwriting has no sense in case of replication
      std::string value = destination.GetURL().Option("overwrite", "no");
      if (strcasecmp(value.c_str(), "no") != 0)
        destination_overwrite = true;
    }
    // sort source replicas according to the expression supplied
    source.SortLocations(preferred_pattern, map);
    if (destination_overwrite) {
      if ((destination.IsIndex() && destination_meta_initially_stored)
          || (!destination.IsIndex())) {
        URL del_url = destination.GetURL();
        logger.msg(VERBOSE, "DataMover::Transfer: trying to destroy/overwrite "
                   "destination: %s", del_url.str());
        int try_num = destination.GetTries();
        for (;;) {
          DataHandle del(del_url, destination.GetUserConfig());
          del->SetTries(1);
          DataStatus res = Delete(*del);
          if (res == DataStatus::Success)
            break;
          if (!destination.IsIndex()) {
            // pfn has chance to be overwritten directly
            logger.msg(WARNING, "Failed to delete %s but will still try to copy", del_url.str());
            break;
          }
          logger.msg(INFO, "Failed to delete %s", del_url.str());
          destination.NextTry(); /* try again */
          if (!do_retries)
            return res;
          if ((--try_num) <= 0)
            return res;
        }
        if (destination.IsIndex()) {
          for (;;) {
            DataStatus dres = destination.Resolve(false);
            if (dres.Passed()) {
              if (destination.HaveLocations())
                break;
              logger.msg(ERROR, "No locations for destination found: %s",
                         destination.str());
            }
            else
              logger.msg(ERROR, "Failed to resolve destination: %s",
                         destination.str());
            destination.NextTry(); /* try again */
            if (!do_retries)
              return dres;
            if (!destination.LocationValid())
              return dres;
          }
          destination_meta_initially_stored = destination.Registered();
          if (destination_meta_initially_stored) {
            logger.msg(INFO, "Deleted but still have locations at %s",
                       destination.str());
            destination.NextTry(); /* try again */
            return DataStatus::WriteResolveError;
          }
        }
      }
    }
    DataStatus res = DataStatus::TransferError;
    int try_num;
    for (try_num = 0;; try_num++) { /* cycle for retries */
      Glib::Mutex::Lock lock(lock_);
      logger.msg(VERBOSE, "DataMover: cycle");
      if ((try_num != 0) && (!do_retries)) {
        logger.msg(VERBOSE, "DataMover: no retries requested - exit");
        return res;
      }
      if ((!source.LocationValid()) || (!destination.LocationValid())) {
        if (!source.LocationValid())
          logger.msg(VERBOSE, "DataMover: source out of tries - exit");
        if (!destination.LocationValid())
          logger.msg(VERBOSE, "DataMover: destination out of tries - exit");
        /* out of tries */
        return res;
      }
      DataBuffer buffer;
      // Make sure any transfer is stopped before buffer is destroyed
      DataPointStopper source_stop(source);
      DataPointStopper destination_stop(destination);
      logger.msg(INFO, "Real transfer from %s to %s", source.CurrentLocation().str(), destination.CurrentLocation().str());
      /* creating handler for transfer */
      source.SetSecure(force_secure);
      source.Passive(force_passive);
      destination.SetSecure(force_secure);
      destination.Passive(force_passive);
      destination.SetAdditionalChecks(do_checks);
      /* take suggestion from DataHandle about buffer, etc. */
      long long int bufsize;
      int bufnum;
      /* tune buffers */
      bufsize = 1048576; /* have reasonable buffer size */
      bool seekable = destination.WriteOutOfOrder();
      source.ReadOutOfOrder(seekable);
      bufnum = 1;
      if (source.BufSize() > bufsize)
        bufsize = source.BufSize();
      if (destination.BufSize() > bufsize)
        bufsize = destination.BufSize();
      if (seekable) {
        if (source.BufNum() > bufnum)
          bufnum = source.BufNum();
        if (destination.BufNum() > bufnum)
          bufnum = destination.BufNum();
      }
      bufnum = bufnum * 2;
      logger.msg(VERBOSE, "Creating buffer: %lli x %i", bufsize, bufnum);

      // Checksum logic:
      // if destination has meta, use default checksum it accepts, or override
      // with url option. If not check source checksum and calculate that one.
      // If not available calculate default checksum for source even if source
      // doesn't have it at the moment since it might be available after start_reading().

      CheckSumAny crc;
      CheckSumAny crc_source;
      CheckSumAny crc_dest;
      std::string crc_type("");

      // check if checksumming is turned off
      if ((source.GetURL().Option("checksum") == "no") ||
          (destination.GetURL().Option("checksum") == "no")) {
        logger.msg(VERBOSE, "DataMove::Transfer: no checksum calculation for %s", source.str());
      }
      // check if checksum is specified as a metadata attribute
      else if (!destination.GetURL().MetaDataOption("checksumtype").empty() && !destination.GetURL().MetaDataOption("checksumvalue").empty()) {
        crc_type = destination.GetURL().MetaDataOption("checksumtype");
        logger.msg(VERBOSE, "DataMove::Transfer: using supplied checksum %s:%s",
            destination.GetURL().MetaDataOption("checksumtype"), destination.GetURL().MetaDataOption("checksumvalue"));
        std::string csum = destination.GetURL().MetaDataOption("checksumtype") + ':' + destination.GetURL().MetaDataOption("checksumvalue");
        destination.SetCheckSum(csum);
      }
      else if (destination.AcceptsMeta() || destination.ProvidesMeta()) {
        if (destination.GetURL().Option("checksum").empty()) {
          crc_type = destination.DefaultCheckSum();
        } else {
          crc_type = destination.GetURL().Option("checksum");
        }
      }
      else if (source.CheckCheckSum()) {
        crc_type = source.GetCheckSum();
        crc_type = crc_type.substr(0, crc_type.find(':'));
      }
      else if (source.ProvidesMeta()) {
        crc_type = source.DefaultCheckSum();
      }

      if (!crc_type.empty()) {
        crc = crc_type.c_str();
        crc_source = crc_type.c_str();
        crc_dest = crc_type.c_str();
        if (crc.Type() != CheckSumAny::none) logger.msg(VERBOSE, "DataMove::Transfer: will calculate %s checksum", crc_type);
      }

      /* create buffer and tune speed control */
      buffer.set(&crc, bufsize, bufnum);
      if (!buffer) logger.msg(WARNING, "Buffer creation failed !");
      buffer.speed.set_min_speed(min_speed, min_speed_time);
      buffer.speed.set_min_average_speed(min_average_speed);
      buffer.speed.set_max_inactivity_time(max_inactivity_time);
      buffer.speed.verbose(be_verbose);
      if (be_verbose) {
        if (prefix)
          buffer.speed.verbose(std::string(prefix));
        else
          buffer.speed.verbose(verbose_prefix);
        buffer.speed.set_progress_indicator(show_progress);
      }
      /* checking if current source should be mapped to different location */
      /* TODO: make mapped url to be handled by source handle directly */
      bool mapped = false;
      URL mapped_url;
      if (destination.Local()) {
        mapped_url = source.CurrentLocation();
        mapped = map.map(mapped_url);
        /* TODO: copy options to mapped_url */
        if (!mapped)
          mapped_url = URL();
        else {
          logger.msg(VERBOSE, "URL is mapped to: %s", mapped_url.str());
          if (mapped_url.Protocol() == "link")
            /* can't cache links */
            cacheable = false;
        }
      }
      // Do not link if user asks. Replace link:// with file://
      if ((!source.ReadOnly()) && mapped) {
        if (mapped_url.Protocol() == "link") {
          mapped_url.ChangeProtocol("file");
        }
      }
      DataHandle mapped_h(mapped_url, source.GetUserConfig());
      DataPoint& mapped_p(*mapped_h);
      if (mapped_h) {
        mapped_p.SetSecure(force_secure);
        mapped_p.Passive(force_passive);
      }
      /* Try to initiate cache (if needed) */
      std::string canonic_url = source.str();
      if (cacheable) {
        res = DataStatus::Success;
        bool delete_first = (source.GetURL().Option("cache") == "renew");
        for (;;) { /* cycle for outdated cache files */
          bool is_in_cache = false;
          bool is_locked = false;
          if (!cache.Start(canonic_url, is_in_cache, is_locked, delete_first)) {
            if (is_locked) {
              logger.msg(VERBOSE, "Cached file is locked - should retry");
              source.NextTry(); /* to decrease retry counter */
              return DataStatus(DataStatus::CacheError, EAGAIN, "Cache file locked");
            }
            cacheable = false;
            logger.msg(INFO, "Failed to initiate cache");
            source.NextLocation(); /* try another source */
            break;
          }
          if (is_in_cache) {
            /* just need to check permissions */
            logger.msg(INFO, "File %s is cached (%s) - checking permissions",
                       canonic_url, cache.File(canonic_url));
            // check the list of cached DNs
            bool have_permission = false;
            // don't request metadata from source if user says not to
            bool check_meta = (source.GetURL().Option("cache") != "invariant");
            if (source.GetURL().Option("cache") != "check" && cache.CheckDN(canonic_url, dn))
              have_permission = true;
            else {
              DataStatus cres = source.Check(check_meta);
              if (!cres.Passed()) {
                logger.msg(ERROR, "Permission checking failed: %s", canonic_url);
                source.NextLocation(); /* try another source */
                logger.msg(VERBOSE, "source.next_location");
                res = cres;
                break;
              }
              cache.AddDN(canonic_url, dn, exp_time);
            }
            logger.msg(INFO, "Permission checking passed");
            /* check if file is fresh enough */
            bool outdated = true;
            if (have_permission || !check_meta)
              outdated = false; // cached DN means don't check creation date
            if (source.CheckModified() && cache.CheckCreated(canonic_url)) {
              Time sourcetime = source.GetModified();
              Time cachetime = cache.GetCreated(canonic_url);
              logger.msg(VERBOSE, "Source modification date: %s", sourcetime.str());
              logger.msg(VERBOSE, "Cache creation date: %s", cachetime.str());
              if (sourcetime <= cachetime)
                outdated = false;
            }
            if (outdated) {
              delete_first = true;
              logger.msg(INFO, "Cached file is outdated, will re-download");
              continue;
            }
            logger.msg(VERBOSE, "Cached copy is still valid");
            logger.msg(INFO, "Linking/copying cached file");
            if (!cache.Link(destination.CurrentLocation().Path(),
                            canonic_url,
                            (!source.ReadOnly() || executable || cache_copy),
                            executable,
                            false,
                            is_locked)) {
              source.NextTry(); /* to decrease retry counter */
              if (is_locked) {
                logger.msg(VERBOSE, "Cached file is locked - should retry");
                return DataStatus(DataStatus::CacheError, EAGAIN, "Cache file locked");
              }
              return DataStatus::CacheError;
            }
            return DataStatus::SuccessCached;
            // Leave here. Rest of code below is for transfer.
          }
          break;
        }
        if (cacheable && !res.Passed())
          continue;
      }
      if (mapped) {
        if ((mapped_url.Protocol() == "link")
            || (mapped_url.Protocol() == "file")) {
          /* check permissions first */
          logger.msg(INFO, "URL is mapped to local access - "
                     "checking permissions on original URL");
          DataStatus cres =  source.Check(false);
          if (!cres.Passed()) {
            logger.msg(ERROR, "Permission checking on original URL failed: %s",
                       source.str());
            source.NextLocation(); /* try another source */
            logger.msg(VERBOSE, "source.next_location");
            if (cacheable)
              cache.StopAndDelete(canonic_url);
            res = cres;
            continue;
          }
          logger.msg(VERBOSE, "Permission checking passed");
          if (mapped_url.Protocol() == "link") {
            logger.msg(VERBOSE, "Linking local file");
            const std::string& file_name = mapped_url.Path();
            const std::string& link_name = destination.CurrentLocation().Path();
            // create directory structure for link_name
            {
              User user;
              std::string dirpath = Glib::path_get_dirname(link_name);
              if(dirpath == ".") dirpath = G_DIR_SEPARATOR_S;
              if (!DirCreate(dirpath, user.get_uid(), user.get_gid(), S_IRWXU, true) != 0) {
                int err = errno;
                logger.msg(ERROR, "Failed to create directory %s", dirpath);
                source.NextLocation(); /* try another source */
                logger.msg(VERBOSE, "source.next_location");
                res = DataStatus(DataStatus::ReadStartError, err);
                continue;
              }
            }
            // make link
            if (symlink(file_name.c_str(), link_name.c_str()) == -1) {
              int err = errno;
              logger.msg(ERROR, "Failed to make symbolic link %s to %s : %s",
                         link_name, file_name, StrError());
              source.NextLocation(); /* try another source */
              logger.msg(VERBOSE, "source.next_location");
              res = DataStatus(DataStatus::ReadStartError, err);
              continue;
            }
            User user;
            if (lchown(link_name.c_str(), user.get_uid(), user.get_gid()) == -1) {
              logger.msg(WARNING, "Failed to change owner of symbolic link %s to %i", link_name, user.get_uid());
            }
            return DataStatus::Success;
            // Leave after making a link. Rest moves data.
          }
        }
      }
      URL churl;
      if (cacheable) {
        /* create new destination for cache file */
        churl = cache.File(canonic_url);
        logger.msg(INFO, "cache file: %s", churl.Path());
      }
      // don't switch user to access cache
      UserConfig usercfg(destination.GetUserConfig());
      usercfg.SetUser(User(getuid()));
      DataHandle chdest_h(churl, usercfg);
      DataPoint& chdest(*chdest_h);
      if (chdest_h) {
        chdest.SetSecure(force_secure);
        chdest.Passive(force_passive);
        chdest.SetAdditionalChecks(false); // don't pre-allocate space in cache
        chdest.SetMeta(destination); // share metadata
      }
      DataPoint& source_url = mapped ? mapped_p : source;
      DataPoint& destination_url = cacheable ? chdest : destination;
      // Disable checks meant to provide meta-information if not needed
      source_url.SetAdditionalChecks((do_checks && (checks_required || cacheable)) || (show_progress != NULL));
      if (source_url.GetAdditionalChecks()) {
        if (!source_url.IsIndex()) {
          FileInfo fileinfo;
          DataPoint::DataPointInfoType verb = (DataPoint::DataPointInfoType)
                                              (DataPoint::INFO_TYPE_TIMES |
                                               DataPoint::INFO_TYPE_CONTENT);
          DataStatus r = source_url.Stat(fileinfo, verb);
          if (!r.Passed()) {
            logger.msg(ERROR, "Failed to stat source %s", source_url.str());
            if (source.NextLocation())
              logger.msg(VERBOSE, "(Re)Trying next source");
            res = r;
            if (cacheable)
              cache.StopAndDelete(canonic_url);
            continue;
          }
        } else {
          // check location meta
          DataStatus r = source_url.CompareLocationMetadata();
          if (!r.Passed()) {
            if (r == DataStatus::InconsistentMetadataError)
              logger.msg(ERROR, "Meta info of source and location do not match for %s", source_url.str());
            if (source.NextLocation())
              logger.msg(VERBOSE, "(Re)Trying next source");
            res = r;
            if (cacheable)
              cache.StopAndDelete(canonic_url);
            continue;
          }
        }
        // check for high latency
        if (source_url.GetAccessLatency() == Arc::DataPoint::ACCESS_LATENCY_LARGE) {
          if (source.LastLocation()) {
            logger.msg(WARNING, "Replica %s has high latency, but no more sources exist so will use this one",
                       source_url.CurrentLocation().str());
          }
          else {
            logger.msg(INFO, "Replica %s has high latency, trying next source", source_url.CurrentLocation().str());
            source.NextLocation();
            if (cacheable)
              cache.StopAndDelete(canonic_url);
            continue;
          }
        }
      }
      source_url.AddCheckSumObject(&crc_source);
      bool try_another_transfer = true;
      if (try_another_transfer) {
        if (source_url.SupportsTransfer()) {
          logger.msg(INFO, "Using internal transfer method of %s", source_url.str());
          URL dest_url(cacheable ? chdest.GetURL() : destination.GetURL());
          DataStatus datares = source_url.Transfer(dest_url, true, show_progress ? transfer_cb : NULL);
          if (!datares.Passed()) {
            if (source.NextLocation()) {
              logger.msg(VERBOSE, "(Re)Trying next source");
              continue;
            }
            if (cacheable)
              cache.StopAndDelete(canonic_url);
            // Check if SupportsTransfer was too optimistic
            if (datares != DataStatus::UnimplementedError) return datares;
              logger.msg(INFO, "Internal transfer method is not supported for %s", source_url.str());
          } else {
            try_another_transfer = false;
          }
        }
      }
      if (try_another_transfer) {
        if (destination.SupportsTransfer()) {
          logger.msg(INFO, "Using internal transfer method of %s", destination.str());
          DataStatus datares = destination.Transfer(source_url.GetURL(), false, show_progress ? transfer_cb : NULL);
          if (!datares.Passed()) {
            if (source.NextLocation()) {
              logger.msg(VERBOSE, "(Re)Trying next source");
              continue;
            }
            // Check if SupportsTransfer was too optimistic
            if (datares != DataStatus::UnimplementedError) return datares;
            logger.msg(INFO, "Internal transfer method is not supported for %s", destination.str());
          } else {
            try_another_transfer = false;
          }
        }
      }
      if (try_another_transfer) {
        logger.msg(INFO, "Using buffered transfer method");
        unsigned int wait_time;
        DataStatus datares = source_url.PrepareReading(max_inactivity_time, wait_time);
        if (!datares.Passed()) {
          logger.msg(ERROR, "Failed to prepare source: %s",
                     source_url.str());
          source_url.FinishReading(true);
          res = datares;
          /* try another source */
          if (source.NextLocation())
            logger.msg(VERBOSE, "(Re)Trying next source");
          if (cacheable)
            cache.StopAndDelete(canonic_url);
          continue;
        }

        datares = source_url.StartReading(buffer);
        if (!datares.Passed()) {
          logger.msg(ERROR, "Failed to start reading from source: %s",
                     source_url.str());
          source_url.StopReading();
          source_url.FinishReading(true);
          res = datares;
          if (source.GetFailureReason() != DataStatus::UnknownError)
            res = source.GetFailureReason();
          /* try another source */
          if (source.NextLocation())
            logger.msg(VERBOSE, "(Re)Trying next source");
          if (cacheable)
            cache.StopAndDelete(canonic_url);
          continue;
        }
        if (mapped)
          destination.SetMeta(mapped_p);
        if (force_registration && destination.IsIndex()) {
          // at least compare metadata
          if (!destination.CompareMeta(source)) {
            logger.msg(ERROR, "Metadata of source and destination are different");
            source_url.StopReading();
            source_url.FinishReading(true);
            source.NextLocation(); /* not exactly sure if this would help */
            res = DataStatus::PreRegisterError;
            if (cacheable)
              cache.StopAndDelete(canonic_url);
            continue;
          }
        }
        // pass metadata gathered during start_reading()
        // from source to destination
        destination.SetMeta(source);
        if (chdest_h)
          chdest.SetMeta(source);
        if (destination.CheckSize())
          buffer.speed.set_max_data(destination.GetSize());
        datares = destination.PreRegister(replication, force_registration);
        if (!datares.Passed()) {
          logger.msg(ERROR, "Failed to preregister destination: %s",
                     destination.str());
          source_url.StopReading();
          source_url.FinishReading(true);
          destination.NextLocation(); /* not exactly sure if this would help */
          logger.msg(VERBOSE, "destination.next_location");
          res = datares;
          // Normally remote destination is not cached. But who knows.
          if (cacheable)
            cache.StopAndDelete(canonic_url);
          continue;
        }
        buffer.speed.reset();
        // cache files don't need prepared
        datares = destination.PrepareWriting(max_inactivity_time, wait_time);
        if (!datares.Passed()) {
          logger.msg(ERROR, "Failed to prepare destination: %s",
                     destination.str());
          destination.FinishWriting(true);
          source_url.StopReading();
          source_url.FinishReading(true);
          if (!destination.PreUnregister(replication ||
                                         destination_meta_initially_stored).Passed())
            logger.msg(ERROR, "Failed to unregister preregistered lfn. "
                       "You may need to unregister it manually: %s", destination.str());
          /* try another source */
          if (destination.NextLocation())
            logger.msg(VERBOSE, "(Re)Trying next destination");
          res = datares;
          continue;
        }
        DataStatus read_failure = DataStatus::Success;
        DataStatus write_failure = DataStatus::Success;
        std::string cache_lock;
        if (!cacheable) {
          destination.AddCheckSumObject(&crc_dest);
          datares = destination.StartWriting(buffer);
          if (!datares.Passed()) {
            logger.msg(ERROR, "Failed to start writing to destination: %s",
                       destination.str());
            destination.StopWriting();
            destination.FinishWriting(true);
            source_url.StopReading();
            source_url.FinishReading(true);
            if (!destination.PreUnregister(replication ||
                                           destination_meta_initially_stored).Passed())
              logger.msg(ERROR, "Failed to unregister preregistered lfn. "
                         "You may need to unregister it manually: %s", destination.str());
            if (destination.NextLocation())
              logger.msg(VERBOSE, "(Re)Trying next destination");
            res = datares;
            if(destination.GetFailureReason() != DataStatus::UnknownError)
              res = destination.GetFailureReason();
            continue;
          }
        }
        else {
          chdest.AddCheckSumObject(&crc_dest);
          datares = chdest.StartWriting(buffer);
          if (!datares.Passed()) {
            // TODO: put callback to clean cache into FileCache
            logger.msg(ERROR, "Failed to start writing to cache");
            chdest.StopWriting();
            source_url.StopReading();
            source_url.FinishReading(true);
            // hope there will be more space next time
            cache.StopAndDelete(canonic_url);
            if (!destination.PreUnregister(replication ||
                                           destination_meta_initially_stored).Passed())
              logger.msg(ERROR, "Failed to unregister preregistered lfn. "
                         "You may need to unregister it manually");
            destination.NextLocation(); /* to decrease retry counter */
            return DataStatus::CacheError; // repeating won't help here
          }
          cache_lock = chdest.GetURL().Path()+FileLock::getLockSuffix();
        }
        logger.msg(VERBOSE, "Waiting for buffer");
        // cancelling will make loop exit before eof, triggering error and destinatinon cleanup
        for (; (!buffer.eof_read() || !buffer.eof_write()) && !buffer.error() && !cancelled;) {
          buffer.wait_any();
          if (cacheable && !cache_lock.empty()) {
            // touch cache lock file regularly so it is still valid
            if (utime(cache_lock.c_str(), NULL) == -1) {
              logger.msg(WARNING, "Failed updating timestamp on cache lock file %s for file %s: %s",
                         cache_lock, source_url.str(), StrError(errno));
            }
          }
        }
        logger.msg(VERBOSE, "buffer: read EOF : %s", buffer.eof_read()?"yes":"no");
        logger.msg(VERBOSE, "buffer: write EOF: %s", buffer.eof_write()?"yes":"no");
        logger.msg(VERBOSE, "buffer: error    : %s, read: %s, write: %s", buffer.error()?"yes":"no", buffer.error_read()?"yes":"no", buffer.error_write()?"yes":"no");
        logger.msg(VERBOSE, "Closing read channel");
        read_failure = source_url.StopReading();
        source_url.FinishReading((!read_failure.Passed() || buffer.error()));
        if (cacheable && mapped) {
          source.SetMeta(mapped_p); // pass more metadata (checksum)

        }
        logger.msg(VERBOSE, "Closing write channel");
        // turn off checks during stop_writing() if force is turned on
        destination_url.SetAdditionalChecks(destination_url.GetAdditionalChecks() && !force_registration);
        if (!destination_url.StopWriting().Passed()) {
          destination_url.FinishWriting(true);
          buffer.error_write(true);
        }
        else if (!destination_url.FinishWriting(buffer.error())) {
          logger.msg(ERROR, "Failed to complete writing to destination");
          buffer.error_write(true);
        }

        if (buffer.error()) {
          if (cacheable)
            cache.StopAndDelete(canonic_url);
          if (!destination.PreUnregister(replication ||
                                         destination_meta_initially_stored).Passed())
            logger.msg(ERROR, "Failed to unregister preregistered lfn. "
                       "You may need to unregister it manually");

          // Check for cancellation
          if (cancelled) {
            logger.msg(INFO, "Transfer cancelled successfully");
            return DataStatus::SuccessCancelled;
          }
          // Analyze errors
          // Easy part first - if either read or write part report error
          // go to next endpoint.
          if (buffer.error_read()) {
            if (source.NextLocation())
              logger.msg(VERBOSE, "(Re)Trying next source");
            // check for error from callbacks etc
            if(source.GetFailureReason() != DataStatus::UnknownError)
              res=source.GetFailureReason();
            else
              res=DataStatus::ReadError;
          }
          else if (buffer.error_write()) {
            if (destination.NextLocation())
              logger.msg(VERBOSE, "(Re)Trying next destination");
            // check for error from callbacks etc
            if(destination.GetFailureReason() != DataStatus::UnknownError) {
              res=destination.GetFailureReason();
            } else {
              res=DataStatus::WriteError;
            }
          }
          else if (buffer.error_transfer()) {
            // Here is more complicated case - operation timeout
            // Let's first check if buffer was full
            res = DataStatus(DataStatus::TransferError, ETIMEDOUT);
            if (!buffer.for_read()) {
              // No free buffers for 'read' side. Buffer must be full.
              res.SetDesc(destination.GetFailureReason().GetDesc());
              if (destination.NextLocation()) {
                logger.msg(VERBOSE, "(Re)Trying next destination");
              }
            }
            else if (!buffer.for_write()) {
              // Buffer is empty
              res.SetDesc(source.GetFailureReason().GetDesc());
              if (source.NextLocation()) {
                logger.msg(VERBOSE, "(Re)Trying next source");
              }
            }
            else {
              // Both endpoints were very slow? Choose randomly.
              logger.msg(VERBOSE, "Cause of failure unclear - choosing randomly");
              Glib::Rand r;
              if (r.get_int() < (RAND_MAX / 2)) {
                res.SetDesc(source.GetFailureReason().GetDesc());
                if (source.NextLocation()) {
                  logger.msg(VERBOSE, "(Re)Trying next source");
                }
              }
              else {
                res.SetDesc(destination.GetFailureReason().GetDesc());
                if (destination.NextLocation()) {
                  logger.msg(VERBOSE, "(Re)Trying next destination");
                }
              }
            }
          }
          continue;
        }

        // compare checksum. For uploads this is done in StopWriting, but we also
        // need to check here if the sum is given in meta attributes. For downloads
        // compare to the original source (if available).
        std::string calc_csum;
        if (crc && buffer.checksum_valid()) {
          char buf[100];
          crc.print(buf,100);
          calc_csum = buf;
        } else if(crc_source) {
          char buf[100];
          crc_source.print(buf,100);
          calc_csum = buf;
        } else if(crc_dest) {
          char buf[100];
          crc_dest.print(buf,100);
          calc_csum = buf;
        }
        if (!calc_csum.empty()) {
          // compare calculated to any checksum given as a meta option
          if (!destination.GetURL().MetaDataOption("checksumtype").empty() &&
             !destination.GetURL().MetaDataOption("checksumvalue").empty() &&
             calc_csum.substr(0, calc_csum.find(":")) == destination.GetURL().MetaDataOption("checksumtype") &&
             calc_csum.substr(calc_csum.find(":")+1) != destination.GetURL().MetaDataOption("checksumvalue")) {
            // error here? yes since we'll have an inconsistent catalog otherwise
            logger.msg(ERROR, "Checksum mismatch between checksum given as meta option (%s:%s) and calculated checksum (%s)",
                destination.GetURL().MetaDataOption("checksumtype"), destination.GetURL().MetaDataOption("checksumvalue"), calc_csum);
            if (cacheable) {
              cache.StopAndDelete(canonic_url);
            }
            if (!destination.Unregister(replication || destination_meta_initially_stored)) {
              logger.msg(WARNING, "Failed to unregister preregistered lfn, You may need to unregister it manually");
            }
            res = DataStatus(DataStatus::TransferError, EARCCHECKSUM);
            if (!Delete(destination, true)) {
              logger.msg(WARNING, "Failed to delete destination, retry may fail");
            }
            if (destination.NextLocation()) {
              logger.msg(VERBOSE, "(Re)Trying next destination");
            }
            continue;
          }
          if (source.CheckCheckSum()) {
            std::string src_csum_s(source.GetCheckSum());
            if (src_csum_s.find(':') == src_csum_s.length() -1) {
              logger.msg(VERBOSE, "Cannot compare empty checksum");
            }
            // Check the checksum types match. Some buggy GridFTP servers return a
            // different checksum type than requested so also check that the checksum
            // length matches before comparing.
            else if (calc_csum.substr(0, calc_csum.find(":")) != src_csum_s.substr(0, src_csum_s.find(":")) ||
                     calc_csum.substr(calc_csum.find(":")).length() != src_csum_s.substr(src_csum_s.find(":")).length()) {
              logger.msg(VERBOSE, "Checksum type of source and calculated checksum differ, cannot compare");
            } else if (calc_csum.substr(calc_csum.find(":")) != src_csum_s.substr(src_csum_s.find(":"))) {
              logger.msg(ERROR, "Checksum mismatch between calcuated checksum %s and source checksum %s", calc_csum, source.GetCheckSum());
              if(cacheable) {
                cache.StopAndDelete(canonic_url);
              }
              res = DataStatus(DataStatus::TransferError, EARCCHECKSUM);
              if (source.NextLocation()) {
                logger.msg(VERBOSE, "(Re)Trying next source");
              }
              continue;
            }
            else {
              logger.msg(VERBOSE, "Calculated transfer checksum %s matches source checksum", calc_csum);
            }
          }
          // set the destination checksum to be what we calculated
          destination.SetCheckSum(calc_csum.c_str());
        } else {
          logger.msg(VERBOSE, "Checksum not computed");
        }

        destination.SetMeta(source); // pass more metadata (checksum)
        datares = destination.PostRegister(replication);
        if (!datares.Passed()) {
          logger.msg(ERROR, "Failed to postregister destination %s",
                     destination.str());
          if (!destination.PreUnregister(replication ||
                                         destination_meta_initially_stored).Passed())
            logger.msg(ERROR, "Failed to unregister preregistered lfn. "
                       "You may need to unregister it manually: %s", destination.str());
          destination.NextLocation(); /* not sure if this can help */
          logger.msg(VERBOSE, "destination.next_location");
          if(cacheable)
            cache.Stop(canonic_url);
          res = datares;
          continue;
        }
      } // not using internal transfer
      if (cacheable) {
        cache.AddDN(canonic_url, dn, exp_time);
        logger.msg(INFO, "Linking/copying cached file");
        bool is_locked = false;
        bool cache_link_result = cache.Link(destination.CurrentLocation().Path(),
                                            canonic_url,
                                            (!source.ReadOnly() || executable || cache_copy),
                                            executable,
                                            true,
                                            is_locked);
        cache.Stop(canonic_url);
        if (!cache_link_result) {
          if (!destination.PreUnregister(replication ||
                                         destination_meta_initially_stored).Passed())
            logger.msg(ERROR, "Failed to unregister preregistered lfn. "
                       "You may need to unregister it manually");
          source.NextTry(); /* to decrease retry count */
          if (is_locked) return DataStatus(DataStatus::CacheError, EAGAIN, "Cache file locked");
          return DataStatus::CacheError; // retry won't help
        }
      }
      if (buffer.error())
        continue; // should never happen - keep just in case
      break;
    }
    return DataStatus::Success;
  }

  void DataMover::Cancel() {
    cancelled = true;
  }

  void DataMover::secure(bool val) {
    force_secure = val;
  }

  void DataMover::passive(bool val) {
    force_passive = val;
  }

  void DataMover::force_to_meta(bool val) {
    force_registration = val;
  }

} // namespace Arc
