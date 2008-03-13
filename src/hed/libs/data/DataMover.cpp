#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/DateTime.h>
#include <arc/Logger.h>
#include <arc/Thread.h>
#include <arc/URL.h>

#include "CheckSum.h"
#include "DataPoint.h"
#include "DataHandle.h"
#include "DataBufferPar.h"
#include "URLMap.h"
#include "MkDirRecursive.h"

#include "DataMover.h"

#include <cerrno>

#ifndef BUFLEN
#define BUFLEN 1024
#endif

#ifndef HAVE_STRERROR_R
static char* strerror_r(int errnum, char *buf, size_t buflen) {
  char *estring = strerror(errnum);
  strncpy(buf, estring, buflen);
  return buf;
}
#endif

namespace Arc {

  Logger DataMover::logger(Logger::getRootLogger(), "DataMover");

  DataMover::DataMover() :
    be_verbose(false),
    force_secure(false),
    force_passive(false),
    force_registration(false),
    do_checks(true),
    do_retries(true),
    default_min_speed(0),
    default_min_speed_time(0),
    default_min_average_speed(0),
    default_max_inactivity_time(300),
    show_progress(NULL) {}

  DataMover::~DataMover() {}

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
    DataCache *cache;
    const URLMap *map;
    unsigned long long int min_speed;
    time_t min_speed_time;
    unsigned long long int min_average_speed;
    time_t max_inactivity_time;
    std::string *failure_description;
    DataMover::callback cb;
    DataMover *it;
    void *arg;
    const char *prefix;
  } transfer_struct;

  DataStatus DataMover::Delete(DataPoint& url, bool errcont) {
    bool remove_lfn = !url.HaveLocations(); // pfn or plain url
    if(!url.Resolve(true))
      // TODO: Check if error is real or "not exist".
      if(remove_lfn)
        logger.msg(INFO,
                   "No locations found - probably no more physical instances");
    std::list<URL> removed_urls;
    if(url.HaveLocations())
      for(; url.LocationValid();) {
        logger.msg(INFO, "Removing %s", url.CurrentLocation().str().c_str());
        // It can happen that after resolving list contains duplicated
        // physical locations obtained from different meta-data-services.
        // Because not all locations can reliably say if files does not exist
        // or access is not allowed, avoid duplicated delete attempts.
        bool url_was_deleted = false;
        for(std::list<URL>::iterator u = removed_urls.begin();
            u != removed_urls.end(); ++u) {
          if(url.CurrentLocation() == (*u)) {
            url_was_deleted = true;
            break;
          }
        }
        if(url_was_deleted)
          logger.msg(VERBOSE, "This instance was already deleted");
        else {
          url.SetSecure(false);
          if(!url.Remove()) {
            logger.msg(INFO, "Failed to delete physical file");
            if(!errcont) {
              url.NextLocation();
              continue;
            }
          }
          else
            removed_urls.push_back(url.CurrentLocation());
        }
        if (url.IsIndex()) {
          logger.msg(INFO, "Removing metadata in %s",
                     url.CurrentLocationMetadata().c_str());
          DataStatus err = url.Unregister(false);
          if (err) {
            logger.msg(ERROR, "Failed to delete meta-information");
            url.NextLocation();
          }
          else
            url.RemoveLocation();
        }
      }
    if(url.HaveLocations()) {
      logger.msg(ERROR, "Failed to remove all physical instances");
      return DataStatus::DeleteError;
    }
    if (url.IsIndex()) {
      if (remove_lfn) {
        logger.msg(INFO, "Removing logical file from metadata %s",
                   url.str().c_str());
        DataStatus err = url.Unregister(true);
        if (err) {
          logger.msg(ERROR, "Failed to delete logical file");
          return err;
        }
      }
    }
    return DataStatus::Success;
  }

  void transfer_func(void *arg) {
    transfer_struct *param = (transfer_struct *)arg;
    std::string failure_description;
    DataStatus res = param->it->Transfer(
      *(param->source), *(param->destination), *(param->cache), *(param->map),
      param->min_speed, param->min_speed_time,
      param->min_average_speed, param->max_inactivity_time,
      failure_description,
      NULL, NULL, param->prefix);
    if(param->failure_description)
      *(param->failure_description) = failure_description;
    (*(param->cb))(param->it, res, failure_description, param->arg);
    if(param->prefix)
      free((void*)(param->prefix));
    delete param->cache;
    free(param);
  }

  /* transfer data from source to destination */
  DataStatus DataMover::Transfer(DataPoint& source,
                                DataPoint& destination,
                                DataCache& cache, const URLMap& map,
                                std::string& failure_description,
                                DataMover::callback cb, void *arg,
                                const char *prefix) {
    return Transfer(source, destination, cache, map,
             default_min_speed, default_min_speed_time,
             default_min_average_speed,
             default_max_inactivity_time,
             failure_description, cb, arg, prefix);
  }

  DataStatus DataMover::Transfer(DataPoint& source,
                                DataPoint& destination,
                                DataCache& cache, const URLMap& map,
                                unsigned long long int min_speed,
                                time_t min_speed_time,
                                unsigned long long int
                                min_average_speed,
                                time_t max_inactivity_time,
                                std::string& failure_description,
                                DataMover::callback cb, void *arg,
                                const char *prefix) {
    if(cb != NULL) {
      logger.msg(DEBUG, "DataMover::Transfer : starting new thread");
      transfer_struct *param =
        (transfer_struct *)malloc(sizeof(transfer_struct));
      if(param == NULL)
        return DataStatus::TransferError;
      param->source = &source;
      param->destination = &destination;
      param->cache = new DataCache(cache);
      param->map = &map;
      param->min_speed = min_speed;
      param->min_speed_time = min_speed_time;
      param->min_average_speed = min_average_speed;
      param->max_inactivity_time = max_inactivity_time;
      param->failure_description = &failure_description;
      param->cb = cb;
      param->it = this;
      param->arg = arg;
      param->prefix = NULL;
      if(prefix)
        param->prefix = strdup(prefix);
      if(param->prefix == NULL)
        param->prefix = strdup(verbose_prefix.c_str());
      if(!CreateThreadFunction(&transfer_func, param)) {
        free(param);
        return DataStatus::TransferError;
      }
      return DataStatus::Success;
    }
    failure_description = "";
    logger.msg(INFO, "Transfer from %s to %s",
               source.str().c_str(), destination.str().c_str());
    if(!source) {
      logger.msg(ERROR, "Not valid source");
      return DataStatus::ReadAcquireError;
    }
    if(!destination) {
      logger.msg(ERROR, "Not valid destination");
      return DataStatus::WriteAcquireError;
    }
    for(;;) {
      // if(source.Resolve(true, map)) {
      if(source.Resolve(true)) {
        if(source.HaveLocations())
          break;
        logger.msg(ERROR, "No locations for source found: %s",
                   source.str().c_str());
      }
      else
        logger.msg(ERROR, "Failed to resolve source: %s",
                   source.str().c_str());
      source.NextLocation(); /* try again */
      if(!do_retries)
        return DataStatus::ReadResolveError;
      if(!source.LocationValid())
        return DataStatus::ReadResolveError;
    }
    for(;;) {
      // if(destination.Resolve(false, URLMap())) {
      if(destination.Resolve(false)) {
        if(destination.HaveLocations())
          break;
        logger.msg(ERROR, "No locations for destination found: %s",
                   destination.str().c_str());
      }
      else
        logger.msg(ERROR, "Failed to resolve destination: %s",
                   destination.str().c_str());
      destination.NextLocation(); /* try again */
      if(!do_retries)
        return DataStatus::WriteResolveError;
      if(!destination.LocationValid())
        return DataStatus::WriteResolveError;
    }
    bool replication = false;
    if(source.IsIndex() && destination.IsIndex()) {
      // check for possible replication
      if(source.GetURL() == destination.GetURL()) {
        replication = true;
        // we do not want to replicate to same site
        destination.RemoveLocations(source);
        if(!destination.HaveLocations()) {
          logger.msg(ERROR,
                     "No locations for destination different from source "
                     "found: %s", destination.str().c_str());
          return DataStatus::WriteResolveError;
        }
      }
    }
    // Try to avoid any additional checks meant to provide
    // meta-information whenever possible
    bool checks_required = destination.AcceptsMeta() && (!replication);
    bool destination_meta_initially_stored = destination.Registered();
    bool destination_overwrite = false;
    if(!replication) { // overwriting has no sense in case of replication
      std::string value = destination.GetURL().Option("overwrite", "no");
      if(strcasecmp(value.c_str(), "no") != 0)
        destination_overwrite = true;
    }
    if(destination_overwrite) {
      if((destination.IsIndex() && destination_meta_initially_stored) ||
         (!destination.IsIndex())) {
        URL del_url = destination.GetURL();
        logger.msg(DEBUG,
                   "DataMover::Transfer: trying to destroy/overwrite "
                   "destination: %s", del_url.str().c_str());
        int try_num = destination.GetTries();
        for(;;) {
          DataHandle del(del_url);
          del->SetTries(1);
          DataStatus res = Delete(*del);
          if(res == DataStatus::Success)
            break;
          if(!destination.IsIndex())
            break; // pfn has chance to be overwritten directly
          logger.msg(INFO, "Failed to delete %s", del_url.str().c_str());
          destination.NextLocation(); /* try again */
          if(!do_retries)
            return res;
          if((--try_num) <= 0)
            return res;
        }
        if(destination.IsIndex()) {
          for(;;) {
            if(destination.Resolve(false)) {
              if(destination.HaveLocations())
                break;
              logger.msg(ERROR, "No locations for destination found: %s",
                         destination.str().c_str());
            }
            else
              logger.msg(ERROR, "Failed to resolve destination: %s",
                         destination.str().c_str());
            destination.NextLocation(); /* try again */
            if(!do_retries)
              return DataStatus::WriteResolveError;
            if(!destination.LocationValid())
              return DataStatus::WriteResolveError;
          }
          destination_meta_initially_stored = destination.Registered();
          if(destination_meta_initially_stored) {
            logger.msg(INFO, "Deleted but still have locations at %s",
                       destination.str().c_str());
            return DataStatus::WriteResolveError;
          }
        }
      }
    }
    DataStatus res = DataStatus::TransferError;
    int try_num;
    for(try_num = 0;; try_num++) {/* cycle for retries */
      logger.msg(DEBUG, "DataMover: cycle");
      if((try_num != 0) && (!do_retries)) {
        logger.msg(DEBUG, "DataMover: no retries requested - exit");
        return res;
      }
      if((!source.LocationValid()) || (!destination.LocationValid())) {
        if(!source.LocationValid())
          logger.msg(DEBUG, "DataMover: source out of tries - exit");
        if(!destination.LocationValid())
          logger.msg(DEBUG, "DataMover: destination out of tries - exit");
        /* out of tries */
        return res;
      }
      // By putting DataBufferPar here, one makes sure it will be always
      // destroyed AFTER all DataHandle. This allows for not bothering
      // to call stop_reading/stop_writing because they are called in
      // destructor of DataHandle.
      DataBufferPar buffer;
      logger.msg(INFO, "Real transfer from %s to %s",
                 source.CurrentLocation().str().c_str(),
                 destination.CurrentLocation().str().c_str());
      /* creating handler for transfer */
      source.SetSecure(force_secure);
      source.Passive(force_passive);
      destination.SetSecure(force_secure);
      destination.Passive(force_passive);
      destination.SetAdditionalChecks(do_checks);
      /* take suggestion from DataHandle about buffer, etc. */
      bool cacheable = false;
      long int bufsize;
      int bufnum;
      if(source.Cache() && destination.Local())
        if(cache)
          cacheable = true;
      /* tune buffers */
      bufsize = 65536;/* have reasonable buffer size */
      bool seekable = destination.WriteOutOfOrder();
      source.ReadOutOfOrder(seekable);
      bufnum = 1;
      if(source.BufSize() > bufsize)
        bufsize = source.BufSize();
      if(destination.BufSize() > bufsize)
        bufsize = destination.BufSize();
      if(seekable) {
        if(source.BufNum() > bufnum)
          bufnum = source.BufNum();
        if(destination.BufNum() > bufnum)
          bufnum = destination.BufNum();
      }
      bufnum = bufnum * 2;
      logger.msg(DEBUG, "Creating buffer: %i x %i", bufsize, bufnum);
      /* prepare crc */
      CheckSumAny crc;
      // Shold we trust indexing service or always compute checksum ?
      // Let's trust.
      if(destination.AcceptsMeta()) { // may need to compute crc
        // Let it be CRC32 by default.
        std::string crc_type =
          destination.GetURL().Option("checksum", "cksum");
        logger.msg(DEBUG, "DataMover::Transfer: checksum type is %s",
                   crc_type.c_str());
        if(!source.CheckCheckSum()) {
          crc = crc_type.c_str();
          logger.msg(DEBUG, "DataMover::Transfer: will try to compute crc");
        }
        else if(CheckSumAny::Type(crc_type.c_str()) !=
                CheckSumAny::Type(source.GetCheckSum().c_str())) {
          crc = crc_type.c_str();
          logger.msg(DEBUG, "DataMover::Transfer: will try to compute crc");
        }
      }
      /* create buffer and tune speed control */
      buffer.set(&crc, bufsize, bufnum);
      if(!buffer)
        logger.msg(INFO, "Buffer creation failed !");
      buffer.speed.set_min_speed(min_speed, min_speed_time);
      buffer.speed.set_min_average_speed(min_average_speed);
      buffer.speed.set_max_inactivity_time(max_inactivity_time);
      buffer.speed.verbose(be_verbose);
      if(be_verbose) {
        if(prefix)
          buffer.speed.verbose(std::string(prefix));
        else
          buffer.speed.verbose(verbose_prefix);
        buffer.speed.set_progress_indicator(show_progress);
      }
      /* checking if current source should be mapped to different location */
      /* TODO: make mapped url to be handled by source handle directly */
      bool mapped = false;
      URL mapped_url;
      if(destination.Local()) {
        mapped_url = source.CurrentLocation();
        mapped = map.map(mapped_url);
        /* TODO: copy options to mapped_url */
        if(!mapped)
          mapped_url = URL();
        else {
          logger.msg(DEBUG, "Url is mapped to: %s", mapped_url.str().c_str());
          if((mapped_url.Protocol() == "link") ||
             (mapped_url.Protocol() == "file")) {
            /* can't cache links */
            cacheable = false;
          }
        }
      }
      // Do not link if user asks. Replace link:// with file://
      if(source.ReadOnly() && mapped)
        if(mapped_url.Protocol() == "link")
          mapped_url.ChangeProtocol("file");
      DataHandle mapped_h(mapped_url);
      DataPoint& mapped_p(*mapped_h);
      if(mapped_h) {
        mapped_p.SetSecure(force_secure);
        mapped_p.Passive(force_passive);
      }
      /* Try to initiate cache (if needed) */
      if(cacheable) {
        res = DataStatus::Success;
        for(;;) {/* cycle for outdated cache files */
          bool is_in_cache = false;
          if(!cache.start(source.GetURL(), is_in_cache)) {
            cacheable = false;
            logger.msg(INFO, "Failed to initiate cache");
            break;
          }
          if(is_in_cache) {/* just need to check permissions */
            logger.msg(INFO, "File is cached - checking permissions");
            if(!source.Check()) {
              logger.msg(ERROR, "Permission checking failed: %s",
                         source.str().c_str());
              cache.stop(DataCache::file_download_failed |
                         DataCache::file_keep);
              source.NextLocation(); /* try another source */
              logger.msg(DEBUG, "source.next_location");
              res = DataStatus::ReadStartError;
              break;
            }
            logger.msg(DEBUG, "Permission checking passed");
            /* check if file is fresh enough */
            bool outdated = true;
            if(source.CheckCreated() && cache.CheckCreated()) {
              if(source.GetCreated() <= cache.GetCreated())
                outdated = false;
            }
            else if(cache.CheckValid()) {
              if(cache.GetValid() > Time())
                outdated = false;
            }
            if(outdated) {
              cache.stop(DataCache::file_not_valid);
              logger.msg(INFO, "Cached file is outdated");
              continue;
            }
            if(source.ReadOnly()) {
              logger.msg(DEBUG, "Linking/copying cached file");
              if(!cache.link(destination.CurrentLocation().Path())) {
                /* failed cache link is unhandable */
                cache.stop(DataCache::file_download_failed |
                           DataCache::file_keep);
                return DataStatus::CacheError;
              }
            }
            else {
              logger.msg(DEBUG, "Copying cached file");
              if(!cache.copy(destination.CurrentLocation().Path())) {
                /* failed cache copy is unhandable */
                cache.stop(DataCache::file_download_failed |
                           DataCache::file_keep);
                return DataStatus::CacheError;
              }
            }
            cache.stop();
            return DataStatus::Success; // Leave here. Rest of code below is for transfer.
          }
          break;
        }
        if(cacheable)
          if(res != DataStatus::Success) continue;
      }
      if(mapped) {
        if((mapped_url.Protocol() == "link") ||
           (mapped_url.Protocol() == "file")) {
          /* check permissions first */
          logger.msg(INFO, "URL is mapped to local access - "
                     "checking permissions on original URL");
          if(!source.Check()) {
            logger.msg(ERROR,
                       "Permission checking on original URL failed: %s",
                       source.str().c_str());
            source.NextLocation(); /* try another source */
            logger.msg(DEBUG, "source.next_location");
            res = DataStatus::ReadStartError;
            if(cacheable)
              cache.stop(DataCache::file_download_failed |
                         DataCache::file_keep);
            continue;
          }
          logger.msg(DEBUG, "Permission checking passed");
          if(mapped_url.Protocol() == "link") {
            logger.msg(DEBUG, "Linking local file");
            const std::string& file_name = mapped_url.Path();
            const std::string& link_name =
              destination.CurrentLocation().Path();
            // create directory structure for link_name
            {
              Arc::User user;
              std::string dirpath = link_name;
              std::string::size_type n = dirpath.rfind('/');
              if(n == std::string::npos) n = 0;
              if(n == 0)
                dirpath = "/";
              else
                dirpath.resize(n);
              if(mkdir_recursive(NULL, dirpath.c_str(), S_IRWXU, user)
                 != 0) {
                if(errno != EEXIST) {
                  char *err;
                  char errbuf[BUFLEN];
#ifdef STRERROR_R_CHAR_P
                  err = strerror_r(errno, errbuf, sizeof(errbuf));
#else
                  errbuf[0] = 0;
                  err = errbuf;
                  strerror_r(errno, errbuf, sizeof(errbuf));
#endif
                  logger.msg(ERROR, "Failed to create/find directory %s : %s",
                             dirpath.c_str(), err);
                  source.NextLocation(); /* try another source */
                  logger.msg(DEBUG, "source.next_location");
                  res = DataStatus::ReadStartError;
                  if(cacheable)
                    cache.stop(DataCache::file_download_failed |
                               DataCache::file_keep);
                  continue;
                }
              }
            }
            // make link
            if(symlink(file_name.c_str(), link_name.c_str()) == -1) {
              char *err;
              char errbuf[BUFLEN];
#ifdef STRERROR_R_CHAR_P
              err = strerror_r(errno, errbuf, sizeof(errbuf));
#else
              errbuf[0] = 0;
              err = errbuf;
              strerror_r(errno, errbuf, sizeof(errbuf));
#endif
              logger.msg(ERROR, "Failed to make symbolic link %s to %s : %s",
                         link_name.c_str(), file_name.c_str(), err);
              source.NextLocation(); /* try another source */
              logger.msg(DEBUG, "source.next_location");
              res = DataStatus::ReadStartError;
              if(cacheable)
                cache.stop(DataCache::file_download_failed |
                           DataCache::file_keep);
              continue;
            }
            Arc::User user;
            (lchown(link_name.c_str(), user.get_uid(), user.get_gid()) != 0);
            if(cacheable)
              cache.stop();
            return DataStatus::Success; // Leave after making a link. Rest moves data.
          }
        }
      }
      URL churl;
      if(cacheable) {
        /* create new destination for cache file */
        churl = cache.file();
        logger.msg(INFO, "cache file: %s", churl.Path().c_str());
      }
      DataHandle chdest_h(churl);
      DataPoint& chdest(*chdest_h);
      if(chdest_h) {
        chdest.SetSecure(force_secure);
        chdest.Passive(force_passive);
        chdest.SetAdditionalChecks(do_checks);
        chdest.SetMeta(destination); // share metadata
      }
      DataPoint& source_url = mapped ? mapped_p : source;
      DataPoint& destination_url = cacheable ? chdest : destination;
      // Disable checks meant to provide meta-information if not needed
      source_url.SetAdditionalChecks(do_checks & (checks_required | cacheable));
      if(!(res = source_url.StartReading(buffer))) {
        logger.msg(ERROR, "Failed to start reading from source: %s",
                   source_url.str().c_str());
        /* try another source */
        if(source.NextLocation())
          logger.msg(DEBUG, "(Re)Trying next source");
        if(cacheable)
          cache.stop(DataCache::file_download_failed | DataCache::file_keep);
        continue;
      }
      if(mapped)
        destination.SetMeta(mapped_p);
      if(force_registration && destination.IsIndex()) {
        // at least compare metadata
        if(!destination.CompareMeta(source)) {
          logger.msg(ERROR,
                     "Metadata of source and destination are different");
          source.NextLocation(); /* not exactly sure if this would help */
          res = DataStatus::PreRegisterError;
          if(cacheable)
            cache.stop(DataCache::file_download_failed);
          continue;
        }
      }
      destination.SetMeta(source);
      // pass metadata gathered during start_reading()
      // from source to destination
      if(destination.CheckSize())
        buffer.speed.set_max_data(destination.GetSize());
      if(!destination.PreRegister(replication, force_registration)) {
        logger.msg(ERROR, "Failed to preregister destination: %s",
                   destination.str().c_str());
        destination.NextLocation(); /* not exactly sure if this would help */
        logger.msg(DEBUG, "destination.next_location");
        res = DataStatus::PreRegisterError;
        // Normally remote destination is not cached. But who knows.
        if(cacheable)
          cache.stop(DataCache::file_download_failed | DataCache::file_keep);
        continue;
      }
      buffer.speed.reset();
      DataStatus read_failure = DataStatus::Success;
      DataStatus write_failure = DataStatus::Success;
      if(!cacheable) {
        if(!(res = destination.StartWriting(buffer))) {
          logger.msg(ERROR, "Failed to start writing to destination: %s",
                     destination.str().c_str());
          source_url.StopReading();
          if(!destination.PreUnregister(replication ||
                                        destination_meta_initially_stored))
            logger.msg(ERROR, "Failed to unregister preregistered lfn, "
                       "You may need to unregister it manually: %s",
                       destination.str().c_str());
          if(destination.NextLocation())
            logger.msg(DEBUG, "(Re)Trying next destination");
          continue;
        }
      }
      else {
        if(!chdest.StartWriting(buffer, &cache)) {
          logger.msg(ERROR, "Failed to start writing to cache");
          source_url.StopReading();
          // hope there will be more space next time
          cache.stop(DataCache::file_download_failed | DataCache::file_keep);
          if(!destination.PreUnregister(replication ||
               destination_meta_initially_stored))
            logger.msg(ERROR, "Failed to unregister preregistered lfn, "
                       "You may need to unregister it manually");
          return DataStatus::CacheError; // repeating won't help here
        }
      }
      logger.msg(DEBUG, "Waiting for buffer");
      for(; (!buffer.eof_read() || !buffer.eof_write()) && !buffer.error();)
        buffer.wait();
      logger.msg(INFO, "buffer: read eof : %i", (int)buffer.eof_read());
      logger.msg(INFO, "buffer: write eof: %i", (int)buffer.eof_write());
      logger.msg(INFO, "buffer: error    : %i", (int)buffer.error());
      logger.msg(DEBUG, "Closing read channel");
      read_failure = source_url.StopReading();
      if(cacheable && mapped)
        source.SetMeta(mapped_p); // pass more metadata (checksum)
      logger.msg(DEBUG, "Closing write channel");
      write_failure = destination_url.StopWriting();
      if(cacheable) {
        bool download_error = buffer.error();
        if(!download_error) {
          if(source.CheckCreated())
            cache.SetCreated(source.GetCreated());
          if(source.CheckValid())
            cache.SetValid(source.GetValid());
          logger.msg(DEBUG, "Linking/copying cached file");
          if(!cache.link(destination.CurrentLocation().Path())) {
            buffer.error_write(true);
            cache.stop(DataCache::file_download_failed | DataCache::file_keep);
            if(!destination.PreUnregister(replication ||
                                          destination_meta_initially_stored))
              logger.msg(ERROR, "Failed to unregister preregistered lfn, "
                         "You may need to unregister it manually");
            return DataStatus::CacheError;/* retry won't help */
          }
          cache.stop();
        }
        else {
          cache.stop(DataCache::file_download_failed | DataCache::file_keep);
          // keep for retries
        }
      }
      if(buffer.error()) {
        if(!destination.PreUnregister(replication ||
                                      destination_meta_initially_stored))
          logger.msg(ERROR, "Failed to unregister preregistered lfn, "
                     "You may need to unregister it manually");
        // Analyze errors
        // Easy part first - if either read or write part report error
        // go to next endpoint.
        if(buffer.error_read()) {
          if(source.NextLocation())
            logger.msg(DEBUG, "(Re)Trying next source");
          res = read_failure;
        }
        else if(buffer.error_write()) {
          if(destination.NextLocation())
            logger.msg(DEBUG, "(Re)Trying next destination");
          res = write_failure;
        }
        else if(buffer.error_transfer()) {
          // Here is more complicated case - operation timeout
          // Let's first check if buffer was full
          res = DataStatus::TransferError;
          if(!buffer.for_read()) {
            // No free buffers for 'read' side. Buffer must be full.
            if(destination.NextLocation())
              logger.msg(DEBUG, "(Re)Trying next destination");
          }
          else if(!buffer.for_write()) {
            // Buffer is empty
            if(source.NextLocation())
              logger.msg(DEBUG, "(Re)Trying next source");
          }
          else {
            // Both endpoints were very slow? Choose randomly.
            logger.msg(DEBUG, "Cause of failure unclear - choosing randomly");
            if(random() < (RAND_MAX / 2)) {
              if(source.NextLocation())
                logger.msg(DEBUG, "(Re)Trying next source");
            }
            else {
              if(destination.NextLocation())
                logger.msg(DEBUG, "(Re)Trying next destination");
            }
          }
        }
        continue;
      }
      if(crc) {
        if(buffer.checksum_valid()) {
          // source.meta_checksum(crc.end());
          char buf[100];
          crc.print(buf, 100);
          source.SetCheckSum(buf);
          logger.msg(DEBUG, "DataMover::Transfer: have valid checksum");
        }
      }
      destination.SetMeta(source); // pass more metadata (checksum)
      if(!destination.PostRegister(replication)) {
        logger.msg(ERROR, "Failed to postregister destination %s",
                   destination.str().c_str());
        if(!destination.PreUnregister(replication ||
             destination_meta_initially_stored)) {
          logger.msg(ERROR, "Failed to unregister preregistered lfn, "
                     "You may need to unregister it manually %s",
                     destination.str().c_str());
        }
        destination.NextLocation(); /* not sure if this can help */
        logger.msg(DEBUG, "destination.next_location");
        res = DataStatus::PostRegisterError;
        continue;
      }
      if(buffer.error())
        continue; // should never happen - keep just in case
      break;
    }
    return DataStatus::Success;
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
