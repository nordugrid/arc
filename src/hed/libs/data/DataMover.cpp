#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/DateTime.h>
#include <arc/Logger.h>
#include <arc/URL.h>

#include "CheckSum.h"
#include "DataPoint.h"
#include "DataHandle.h"
#include "DataBufferPar.h"
#include "URLMap.h"
#include "CheckFile.h"
#include "MkDirRecursive.h"

#include "DataMover.h"

#include <cerrno>

#ifndef BUFLEN
#define BUFLEN 1024
#endif

#ifndef HAVE_STRERROR_R
static char * strerror_r (int errnum, char * buf, size_t buflen) {
  char * estring = strerror (errnum);
  strncpy (buf, estring, buflen);
  return buf;
}
#endif

namespace Arc {

Logger DataMover::logger(Logger::getRootLogger(), "DataMover");

DataMover::DataMover() {
  be_verbose=false;
  force_secure=false;
  force_passive=false;
  force_registration=false;
  do_retries=true;
  do_checks=true;
  default_min_speed=0;
  default_min_speed_time=0;
  default_min_average_speed=0;
  default_max_inactivity_time=300;
  show_progress=NULL;
}

DataMover::~DataMover() {
}

bool DataMover::verbose(void) { return be_verbose; };

void DataMover::verbose(bool val) { be_verbose=val; };

void DataMover::verbose(const std::string &prefix) { 
  be_verbose=true; verbose_prefix=prefix;
};

bool DataMover::retry(void) { return do_retries; };

void DataMover::retry(bool val) { do_retries=val; };

bool DataMover::checks(void) { return do_checks; };

void DataMover::checks(bool val) { do_checks=val; };

typedef struct {
  DataPoint *source;
  DataPoint *destination;
  DataCache *cache;
  const URLMap *map;
  unsigned long long int min_speed;
  time_t min_speed_time;
  unsigned long long int min_average_speed;
  time_t max_inactivity_time;
  std::string* failure_description;
  DataMover::callback cb;
  DataMover* it;
  void* arg;
  const char* prefix;
} transfer_struct;

DataMover::result DataMover::Delete(DataPoint &url,bool errcont) {
  bool remove_lfn = !url.have_locations(); // pfn or plain url
  if(!url.meta_resolve(true)) {
    // TODO: Check if error is real or "not exist".
    if(remove_lfn) {
      logger.msg(INFO, "No locations found - probably no more physical instances");
    };
  };
  std::list<URL> removed_urls;
  if(url.have_locations()) for(;url.have_location();) {
    logger.msg(INFO, "Removing %s", url.current_location().str().c_str());
    // It can happen that after resolving list contains duplicated
    // physical locations obtained from different meta-data-services.
    // Because not all locations can reliably say if files does not exist
    // or access is not allowed, avoid duplicated delete attempts.
    bool url_was_deleted = false;
    for(std::list<URL>::iterator u = removed_urls.begin();
                                           u!=removed_urls.end();++u) {
      if(url.current_location() == (*u)) { url_was_deleted=true; break; };
    };
    if(url_was_deleted) {
      logger.msg(VERBOSE, "This instance was already deleted");
    } else {
      url.secure(false);
      if(!url.remove()) {
        logger.msg(INFO, "Failed to delete physical file");
        if(!errcont) {
          url.next_location(); continue;
        };
      } else {
        removed_urls.push_back(url.current_location());
      };
    };
    if(!url.meta()) {
      url.remove_location();
    } else {
      logger.msg(INFO, "Removing metadata in %s",
		 url.current_meta_location().c_str());
      if(!url.meta_unregister(false)) {
        logger.msg(ERROR, "Failed to delete meta-information");
        url.next_location();
      }
      else { url.remove_location(); };
    };
  };
  if(url.have_locations()) {
    logger.msg(ERROR, "Failed to remove all physical instances");
    return DataMover::delete_error;
  };
  if(url.meta()) {
    if(remove_lfn) {
      logger.msg(INFO, "Removing logical file from metadata %s",
		 url.str().c_str());
      if(!url.meta_unregister(true)) {
        logger.msg(ERROR, "Failed to delete logical file");
        return DataMover::unregister_error;
      };
    };
  };
  return DataMover::success;
}

void* transfer_func(void* arg) {
  transfer_struct* param = (transfer_struct*)arg;
  std::string failure_description;
  DataMover::result res = param->it->Transfer(
         *(param->source),*(param->destination),*(param->cache),*(param->map),
         param->min_speed,param->min_speed_time,
         param->min_average_speed,param->max_inactivity_time,
         failure_description,
         NULL,NULL,param->prefix);
  if(param->failure_description) 
    *(param->failure_description)=failure_description;
  (*(param->cb))(param->it,res,failure_description.c_str(),param->arg);
  if(param->prefix) free((void*)(param->prefix));
  delete param->cache;
  free(param);
  return NULL;
}

/* transfer data from source to destination */
DataMover::result DataMover::Transfer(
      DataPoint &source,DataPoint &destination,
      DataCache &cache,const URLMap &map,
      std::string &failure_description,
      DataMover::callback cb,void* arg,const char* prefix) {
  return Transfer(source,destination,cache,map,
           default_min_speed,default_min_speed_time,
           default_min_average_speed,
           default_max_inactivity_time,
           failure_description,cb,arg,prefix);
}

DataMover::result DataMover::Transfer(
      DataPoint &source,DataPoint &destination,
      DataCache &cache,const URLMap &map,
      unsigned long long int min_speed,time_t min_speed_time,
      unsigned long long int min_average_speed,
      time_t max_inactivity_time,
      std::string &failure_description,
      DataMover::callback cb,void* arg,const char* prefix) {
  if(cb != NULL) {
    logger.msg(DEBUG, "DataMover::Transfer : starting new thread");
    pthread_t thread;
    pthread_attr_t thread_attr;
    pthread_attr_init(&thread_attr);
    pthread_attr_setdetachstate(&thread_attr,PTHREAD_CREATE_DETACHED);
    transfer_struct* param = (transfer_struct*)malloc(sizeof(transfer_struct));
    if(param==NULL) {
      pthread_attr_destroy(&thread_attr); return system_error;
    };
    param->source=&source;
    param->destination=&destination;
    param->cache=new DataCache(cache);
    param->map=&map;
    param->min_speed=min_speed;
    param->min_speed_time=min_speed_time;
    param->min_average_speed=min_average_speed;
    param->max_inactivity_time=max_inactivity_time;
    param->failure_description=&failure_description;
    param->cb=cb;
    param->it=this;
    param->arg=arg;
    param->prefix=NULL;
    if(prefix) param->prefix=strdup(prefix);
    if(param->prefix == NULL) param->prefix=strdup(verbose_prefix.c_str());
    if(pthread_create(&thread,&thread_attr,&transfer_func,param) != 0) {
      free(param); pthread_attr_destroy(&thread_attr); return system_error;
    };
    pthread_attr_destroy(&thread_attr);
    return success;
  };
  failure_description="";
  logger.msg(INFO, "Transfer from %s to %s", source.str().c_str(), destination.str().c_str());
  if(!source) {
    logger.msg(ERROR, "Not valid source");
    return read_acquire_error;
  };
  if(!destination) {
    logger.msg(ERROR, "Not valid destination");
    return write_acquire_error;
  };
  for(;;) {
    // if(source.meta_resolve(true,map)) {
    if(source.meta_resolve(true)) {
      if(source.have_locations()) break;
      logger.msg(ERROR, "No locations for source found: %s", source.str().c_str());
    } else logger.msg(ERROR, "Failed to resolve source: %s", source.str().c_str());
    source.next_location(); /* try again */
    if(!do_retries) return read_resolve_error;
    if(!source.have_location()) return read_resolve_error;
  };
  for(;;) {
    // if(destination.meta_resolve(false,URLMap())) {
    if(destination.meta_resolve(false)) {
      if(destination.have_locations()) break;
      logger.msg(ERROR, "No locations for destination found: %s", destination.str().c_str());
    } else logger.msg(ERROR, "Failed to resolve destination: %s", destination.str().c_str());
    destination.next_location(); /* try again */
    if(!do_retries) return write_resolve_error;
    if(!destination.have_location()) return write_resolve_error;
  };
  bool replication=false;
  if(source.meta() && destination.meta()) {
    // check for possible replication
    if(source.base_url() == destination.base_url()) {
      replication=true;
      // we do not want to replicate to same site
      destination.remove_locations(source);
      if(!destination.have_locations()) {
        logger.msg(ERROR, "No locations for destination different from source found: %s", destination.str().c_str());
        return write_resolve_error;
      };
    };
  };
  //  Try to avoid any additional checks meant to provide 
  // meta-information whenever possible
  bool checks_required = destination.accepts_meta() && (!replication);
  bool destination_meta_initially_stored = destination.meta_stored();
  bool destination_overwrite = false;
  if(!replication) {  // overwriting has no sense in case of replication
    std::string value = destination.base_url().Option("overwrite","no");
      if(strcasecmp(value.c_str(),"no") != 0) destination_overwrite=true;
  };
  if(destination_overwrite) {
    if( (destination.meta() && destination_meta_initially_stored) ||
        (!destination.meta()) ) {
      URL del_url = destination.base_url();
      logger.msg(DEBUG, "DataMover::Transfer: trying to destroy/overwrite destination: %s", del_url.str().c_str());
      int try_num = destination.GetTries();
      for(;;) {
        DataHandle del(del_url);
        del->SetTries(1);
        result res = Delete(*del);
        if(res == success) break;
        if(!destination.meta()) break; // pfn has chance to be overwritten directly
        logger.msg(INFO, "Failed to delete %s", del_url.str().c_str());

        destination.next_location(); /* try again */
        if(!do_retries) return res;
        if((--try_num) <= 0) return res;
      };
      if(destination.meta()) {
        for(;;) {
          if(destination.meta_resolve(false)) {
            if(destination.have_locations()) break;
            logger.msg(ERROR, "No locations for destination found: %s", destination.str().c_str());
          } else logger.msg(ERROR, "Failed to resolve destination: %s", destination.str().c_str());
          destination.next_location(); /* try again */
          if(!do_retries) return write_resolve_error;
          if(!destination.have_location()) return write_resolve_error;
        };
        destination_meta_initially_stored=destination.meta_stored();
        if(destination_meta_initially_stored) {
          logger.msg(INFO, "Deleted but still have locations at %s", destination.str().c_str());
          return write_resolve_error;
        };
      };
    };
  };
  result res = transfer_error;
  int try_num;
  for(try_num=0;;try_num++) {  /* cycle for retries */
    logger.msg(DEBUG, "DataMover: cycle");
    if((try_num != 0) && (!do_retries)) {
      logger.msg(DEBUG, "DataMover: no retries requested - exit");
      return res;
    };
    if( (!source.have_location()) || (!destination.have_location()) ) {
      if(!source.have_location()) logger.msg(DEBUG, "DataMover: source out of tries - exit");
      if(!destination.have_location()) logger.msg(DEBUG, "DataMover: destination out of tries - exit");
      /* out of tries */
      return res;
    };
    // By putting DataBufferPar here, one makes sure it will be always
    // destroyed AFTER all DataHandle. This allows for not bothering
    // to call stop_reading/stop_writing because they are called in
    // destructor of DataHandle.
    DataBufferPar buffer;
    logger.msg(INFO, "Real transfer from %s to %s", source.current_location().str().c_str(),destination.current_location().str().c_str());
    /* creating handler for transfer */
    source.secure(force_secure);
    source.passive(force_passive);
    destination.secure(force_secure);
    destination.passive(force_passive);
    destination.additional_checks(do_checks);
    /* take suggestion from DataHandle about buffer, etc. */
    DataPoint::analyze_t hint_read;
    DataPoint::analyze_t hint_write;
    bool cacheable=false;
    long int bufsize;
    int bufnum;
    source.analyze(hint_read);
    destination.analyze(hint_write);
    if(hint_read.cache && hint_write.local) {
      if(cache) cacheable=true;
    };
    /* tune buffers */
    bufsize=65536; /* have reasonable buffer size */
    bool seekable = destination.out_of_order();
    source.out_of_order(seekable);
    bufnum=1;
    if(hint_read.bufsize  > bufsize) bufsize=hint_read.bufsize;
    if(hint_write.bufsize > bufsize) bufsize=hint_write.bufsize;
    if(seekable) {
      if(hint_read.bufnum  > bufnum)  bufnum=hint_read.bufnum;
      if(hint_write.bufnum > bufnum)  bufnum=hint_read.bufnum;
    };
    bufnum=bufnum*2;
    logger.msg(DEBUG, "Creating buffer: %i x %i", bufsize, bufnum);
    /* prepare crc */
    CheckSumAny crc;
    // Shold we trust indexing service or always compute checksum ?
    // Let's trust.
    if(destination.accepts_meta()) { // may need to compute crc
      // Let it be CRC32 by default.
      std::string crc_type = destination.base_url().Option("checksum", "cksum");
      logger.msg(DEBUG, "DataMover::Transfer: checksum type is %s", crc_type.c_str());
      if(!source.CheckCheckSum()) {
        crc=crc_type.c_str();
        logger.msg(DEBUG, "DataMover::Transfer: will try to compute crc");
      } else if(CheckSumAny::Type(crc_type.c_str()) != CheckSumAny::Type(source.GetCheckSum().c_str())) {
        crc=crc_type.c_str();
        logger.msg(DEBUG, "DataMover::Transfer: will try to compute crc");
      };
    };
    /* create buffer and tune speed control */
    buffer.set(&crc,bufsize,bufnum);
    if(!buffer) {
      logger.msg(INFO, "Buffer creation failed !");
    };
    buffer.speed.set_min_speed(min_speed,min_speed_time);
    buffer.speed.set_min_average_speed(min_average_speed);
    buffer.speed.set_max_inactivity_time(max_inactivity_time);
    buffer.speed.verbose(be_verbose);
    if(be_verbose) {
      if(prefix) { buffer.speed.verbose(std::string(prefix)); }
      else { buffer.speed.verbose(verbose_prefix); };
      buffer.speed.set_progress_indicator(show_progress);
    };
    /* checking if current source should be mapped to different location */
    /*
       TODO: make mapped url to be handled by source handle directly
    */
    bool mapped = false;
    URL mapped_url;
    if(hint_write.local) {
      mapped_url = source.current_location();
      mapped=map.map(mapped_url);
      /* TODO: copy options to mapped_url */
      if(!mapped) {
        mapped_url=URL();
      } else {
        logger.msg(DEBUG, "Url is mapped to: %s", mapped_url.str().c_str());
        if((mapped_url.Protocol() == "link") ||
           (mapped_url.Protocol() == "file")) {
          /* can't cache links */
          cacheable=false;
        };
      };
    };
    // Do not link if user asks. Replace link:// with file://
    if(hint_read.readonly && mapped) {
      if(mapped_url.Protocol() == "link") {
	mapped_url.ChangeProtocol("file");
      };
    };
    DataHandle mapped_h(mapped_url);
    DataPoint& mapped_p(*mapped_h);
    if(mapped_h) {
      mapped_p.secure(force_secure);
      mapped_p.passive(force_passive);
    }
    /* Try to initiate cache (if needed) */
    if(cacheable) {
      res=success;
      for(;;) { /* cycle for outdated cache files */
        bool is_in_cache = false;
        if(!cache.start(source.base_url(),is_in_cache)) {
          cacheable=false;
          logger.msg(INFO, "Failed to initiate cache");
          break;
        };
        if(is_in_cache) { /* just need to check permissions */
          logger.msg(INFO, "File is cached - checking permissions");
          if(!source.check()) {
            logger.msg(ERROR, "Permission checking failed: %s", source.str().c_str());
            //cache.stop(true,false);
            cache.stop(DataCache::file_download_failed + DataCache::file_keep);
            source.next_location(); /* try another source */
            logger.msg(DEBUG, "source.next_location");
            res=read_start_error;
            break;
          };
          logger.msg(DEBUG, "Permission checking passed");
          /* check if file is fresh enough */
          bool outdated = true;
          if(source.CheckCreated() && cache.CheckCreated()) {
            if(source.GetCreated() <= cache.GetCreated()) outdated=false;
          }
          else if(cache.CheckValid()) {
            if(cache.GetValid() > Time()) outdated=false;
          };
          if(outdated) {
            //cache.stop(false,true);
            cache.stop(DataCache::file_not_valid);
            logger.msg(INFO, "Cached file is outdated");
            continue;
          };
          if(hint_read.readonly) {
            logger.msg(DEBUG, "Linking/copying cached file");
            if(!cache.link(destination.current_location().Path())){
              /* failed cache link is unhandable */
              //cache.stop(true,false);
              cache.stop(DataCache::file_download_failed | DataCache::file_keep);
              return cache_error;
            };
          } else {
            logger.msg(DEBUG, "Copying cached file");
            if(!cache.copy(destination.current_location().Path())){
              /* failed cache copy is unhandable */
              //cache.stop(true,false);
              cache.stop(DataCache::file_download_failed | DataCache::file_keep);
              return cache_error;
            };
          };
          //cache.stop(false,false);
          cache.stop();
          return success; // Leave here. Rest of code below is for transfer.
        };
        break;
      };
      if(cacheable) {
        if(res != success) continue;
      };
    };
    if(mapped) {
      if((mapped_url.Protocol() == "link") ||
         (mapped_url.Protocol() == "file")) {
        /* check permissions first */
        logger.msg(INFO, "URL is mapped to local access - checking permissions on original URL");
        if(!source.check()) {
          logger.msg(ERROR, "Permission checking on original URL failed: %s", source.str().c_str());
          source.next_location(); /* try another source */
          logger.msg(DEBUG, "source.next_location");
          res=read_start_error;
          //if(cacheable) cache.stop(true,false);
          if(cacheable) cache.stop(DataCache::file_download_failed | DataCache::file_keep);
          continue;
        };
        logger.msg(DEBUG, "Permission checking passed");
        if(mapped_url.Protocol() == "link") {
          logger.msg(DEBUG, "Linking local file");
          const std::string& file_name = mapped_url.Path();
          const std::string& link_name = destination.current_location().Path();
          // create directory structure for link_name
          {
            uid_t uid=get_user_id();
            gid_t gid=get_user_group(uid);
            std::string dirpath = link_name;
            std::string::size_type n = dirpath.rfind('/');
            if(n == std::string::npos) n=0;
            if(n == 0) { dirpath="/"; } else { dirpath.resize(n); };
            if(mkdir_recursive(NULL,dirpath.c_str(),S_IRWXU,uid,gid) != 0) {
              if(errno != EEXIST) {
                char* err;
                char errbuf[BUFLEN];
#ifndef _AIX
                err=strerror_r(errno,errbuf,sizeof(errbuf));
#else
                errbuf[0]=0; err=errbuf;
                strerror_r(errno,errbuf,sizeof(errbuf));
#endif
                logger.msg(ERROR, "Failed to create/find directory %s : %s", dirpath.c_str(), err);
                source.next_location(); /* try another source */
                logger.msg(DEBUG, "source.next_location");
                res=read_start_error;
                //if(cacheable) cache.stop(true,false);
                if(cacheable) cache.stop(DataCache::file_download_failed | DataCache::file_keep);
                continue;
              };
            };
          };
          // make link
          if(symlink(file_name.c_str(),link_name.c_str()) == -1) {
            char* err;
            char errbuf[BUFLEN];
#ifndef _AIX
            err=strerror_r(errno,errbuf,sizeof(errbuf));
#else
            errbuf[0]=0; err=errbuf;
            strerror_r(errno,errbuf,sizeof(errbuf));
#endif
            logger.msg(ERROR, "Failed to make symbolic link %s to %s : %s", link_name.c_str(), file_name.c_str(), err);
            source.next_location(); /* try another source */
	    logger.msg(DEBUG, "source.next_location");
            res=read_start_error;
            //if(cacheable) cache.stop(true,false);
            if(cacheable) cache.stop(DataCache::file_download_failed | DataCache::file_keep);
            continue;
          };
          {
            uid_t uid = get_user_id();
            gid_t gid = get_user_group(uid);
            lchown(link_name.c_str(),uid,gid);
          };
          //if(cacheable) cache.stop(false,false);
          if(cacheable) cache.stop();
          return success; // Leave after making a link. Rest moves data.
        };
      };
    };
    URL churl;
    if(cacheable) {
      /* create new destination for cache file */
      churl=cache.file();
      logger.msg(INFO, "cache file: %s", churl.Path().c_str());
    };
    DataHandle chdest_h(churl);
    DataPoint& chdest(*chdest_h);
    if(chdest_h) {
      chdest.secure(force_secure);
      chdest.passive(force_passive);
      chdest.additional_checks(do_checks);
      chdest.meta(destination); // share metadata
    }
    DataPoint& source_url = mapped?mapped_p:source;
    DataPoint& destination_url = cacheable?chdest:destination;
    // Disable checks meant to provide meta-information if not needed
    source_url.additional_checks(do_checks & (checks_required | cacheable));
    if(!source_url.start_reading(buffer)) {
      logger.msg(ERROR, "Failed to start reading from source: %s", source_url.str().c_str());
      res=read_start_error;
      if(source_url.failure_reason()==
         DataPoint::credentials_expired_failure) res=credentials_expired_error;
      /* try another source */
      if(source.next_location()) logger.msg(DEBUG, "(Re)Trying next source");
      //if(cacheable) cache.stop(true,false);
      if(cacheable) 
        cache.stop(DataCache::file_download_failed | DataCache::file_keep);
      continue;
    };
    if(mapped) destination.meta(mapped_p);
    if(force_registration && destination.meta()) { // at least compare metadata
      if(!destination.meta_compare(source)) {
        logger.msg(ERROR, "Metadata of source and destination are different");
        source.next_location(); /* not exactly sure if this would help */
        res=preregister_error;
        //if(cacheable) cache.stop(true,false);
        if(cacheable) cache.stop(DataCache::file_download_failed);
        continue;
      };
    };
    destination.meta(source); // pass metadata gathered during start_reading()
                              // from source to destination
    if(destination.CheckSize()) {
      buffer.speed.set_max_data(destination.GetSize());
    };
    if(!destination.meta_preregister(replication,force_registration)) {
      logger.msg(ERROR, "Failed to preregister destination: %s", destination.str().c_str());
      destination.next_location(); /* not exactly sure if this would help */
      logger.msg(DEBUG, "destination.next_location");
      res=preregister_error;
      //if(cacheable) cache.stop(true,false);
      // Normally remote destination is not cached. But who knows.
      if(cacheable) cache.stop(DataCache::file_download_failed | DataCache::file_keep);
      continue;
    };
    buffer.speed.reset();
    DataPoint::failure_reason_t read_failure = DataPoint::common_failure;
    DataPoint::failure_reason_t write_failure = DataPoint::common_failure;
    if(!cacheable) {
      if(!destination.start_writing(buffer)) {
        logger.msg(ERROR, "Failed to start writing to destination: %s", destination.str().c_str());
        source_url.stop_reading();
        if(!destination.meta_preunregister(replication ||
                               destination_meta_initially_stored)) {
          logger.msg(ERROR, "Failed to unregister preregistered lfn, You may need to unregister it manually: %s", destination.str().c_str());
        };
        if(destination.next_location())
          logger.msg(DEBUG, "(Re)Trying next destination");
        res=write_start_error;
        if(destination.failure_reason()==
         DataPoint::credentials_expired_failure) res=credentials_expired_error;
        continue;
      };
    } else { 
      if(!chdest.start_writing(buffer,&cache)) {
        logger.msg(ERROR, "Failed to start writing to cache");
        source_url.stop_reading();
        //cache.stop(true,false);
        // hope there will be more space next time
        cache.stop(DataCache::file_download_failed | DataCache::file_keep);
        if(!destination.meta_preunregister(replication || 
                                  destination_meta_initially_stored)) {
          logger.msg(ERROR, "Failed to unregister preregistered lfn, You may need to unregister it manually");
        };
        return cache_error;  /* repeating won't help here */
      };
    };
    logger.msg(DEBUG, "Waiting for buffer");
    for(;(!buffer.eof_read() || !buffer.eof_write()) && !buffer.error();) {
      buffer.wait();
    };
    logger.msg(INFO, "buffer: read eof : %i", buffer.eof_read());
    logger.msg(INFO, "buffer: write eof: %i", buffer.eof_write());
    logger.msg(INFO, "buffer: error    : %s", buffer.error());
    logger.msg(DEBUG, "Closing read channel");
    source_url.stop_reading();
    read_failure=source_url.failure_reason();
    if(cacheable && mapped) {
      source.meta(mapped_p); // pass more metadata (checksum)
    };
    logger.msg(DEBUG, "Closing write channel");
    destination_url.stop_writing();
    write_failure=destination_url.failure_reason();
    // write_failure=destination.failure_reason();
    if(cacheable) {
      bool download_error = buffer.error();
      if(!download_error) {
        if(source.CheckCreated()) cache.SetCreated(source.GetCreated());
        if(source.CheckValid()) cache.SetValid(source.GetValid());
        logger.msg(DEBUG, "Linking/copying cached file");
        if(!cache.link(destination.current_location().Path())) {
          buffer.error_write(true);
          //cache.stop(download_error,false);
          cache.stop(DataCache::file_download_failed | DataCache::file_keep);
          if(!destination.meta_preunregister(replication || destination_meta_initially_stored)) {
            logger.msg(ERROR, "Failed to unregister preregistered lfn, You may need to unregister it manually");
          };
          return cache_error;  /* retry won't help */
        };
        cache.stop();
      } else {
        cache.stop(DataCache::file_download_failed | DataCache::file_keep); // keep for retries 
      };
      //cache.stop(download_error,false);
    };
    if(buffer.error()) {
      if(!destination.meta_preunregister(replication || destination_meta_initially_stored)) {
        logger.msg(ERROR, "Failed to unregister preregistered lfn, You may need to unregister it manually");
      };
      // Analyze errors
      // Easy part first - if either read or write part report error
      // go to next endpoint.
      if(buffer.error_read())  {
        failure_description=source_url.failure_text();
        if(source.next_location())
          logger.msg(DEBUG, "(Re)Trying next source");
        if(read_failure == DataPoint::credentials_expired_failure) {
          res=credentials_expired_error;
        } else { res=read_error; };
      } else if(buffer.error_write()) {
        failure_description=destination_url.failure_text();
        if(destination.next_location())
          logger.msg(DEBUG, "(Re)Trying next destination");
        if(write_failure == DataPoint::credentials_expired_failure) {
          res=credentials_expired_error;
        } else { res=write_error; };
      } else if(buffer.error_transfer()) {
        // Here is more complicated case - operation timeout
        // Let's first check if buffer was full
        res=transfer_error;
        if(!buffer.for_read()) {
          // No free buffers for 'read' side. Buffer must be full.
          failure_description=destination_url.failure_text();
          if(destination.next_location())
            logger.msg(DEBUG, "(Re)Trying next destination");
        } else if(!buffer.for_write()) {
          // Buffer is empty 
          failure_description=source_url.failure_text();
          if(source.next_location())
            logger.msg(DEBUG, "(Re)Trying next source");
        } else {
          // Both endpoints were very slow? Choose randomly.
          logger.msg(DEBUG, "Cause of failure unclear - choosing randomly");
          if(random() < (RAND_MAX/2)) {
            failure_description=source_url.failure_text();
            if(source.next_location())
              logger.msg(DEBUG, "(Re)Trying next source");
          } else {
            failure_description=destination_url.failure_text();
            if(destination.next_location())
              logger.msg(DEBUG, "(Re)Trying next destination");
          };
        };
      };
      continue;
    };
    if(crc) {
      if(buffer.checksum_valid()) {
        // source.meta_checksum(crc.end());
        char buf[100];
        crc.print(buf,100);
        source.SetCheckSum(buf);
        logger.msg(DEBUG, "DataMover::Transfer: have valid checksum");
      };
    };
    destination.meta(source); // pass more metadata (checksum)
    if(!destination.meta_postregister(replication)) {
      logger.msg(ERROR, "Failed to postregister destination %s", destination.str().c_str());
      if(!destination.meta_preunregister(replication || destination_meta_initially_stored)) {
        logger.msg(ERROR, "Failed to unregister preregistered lfn, You may need to unregister it manually %s", destination.str().c_str());
      };
      destination.next_location(); /* not sure if this can help */
      logger.msg(DEBUG, "destination.next_location");
      res=postregister_error;
      continue;
    };
    if(buffer.error()) continue; // should never happen - keep just in case
    break;
  };
  return success;
}

void DataMover::secure(bool val) {
  force_secure=val;
}

void DataMover::passive(bool val) {
  force_passive=val;
}

void DataMover::force_to_meta(bool val) {
  force_registration=val;
}


static const char* result_string[] = {
    "success",  // 0
    "bad source URL", // 1
    "bad destination URL", // 2
    "failed to resolve source locations", // 3
    "failed to resolve destination locations", // 4
    "failed to register new destination file", // 5
    "can't start reading from source", // 6
    "can't start writing to destination", // 7
    "can't read from source", // 8
    "can't write to destination", // 9
    "data transfer was too slow", // 10
    "failed while closing connection to source", // 11
    "failed while closing connection to destination", // 12
    "failed to register new location", // 13
    "can't use local cache", // 14
    "system error", // 15
    "delegated credentials expired" // 16
};

const char* DataMover::get_result_string(result r) {
  if(r == undefined_error) return "unexpected error";
  if((r<0) || (r>16)) return "unknown error";
  return result_string[r];
}

} // namespace Arc
