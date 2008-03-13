#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string>
#include <list>

#include <arc/data/DataCache.h>
#include <arc/data/DataHandle.h>
#include <arc/data/DataMover.h>
#include <arc/data/URLMap.h>
#include <arc/Logger.h>
#include <arc/StringConv.h>
#include <arc/URL.h>

static Arc::Logger logger(Arc::Logger::getRootLogger(), "arccp");

static void progress(FILE* o,const char*,unsigned int,
       unsigned long long int all,unsigned long long int max,
       double,double) {
  static int rs = 0;
  const char rs_[4] = { '|','/','-','\\' };
  if(max) {
    fprintf(o,"\r|");
    unsigned int l = (74*all+37)/max; if(l>74) l=74;
    unsigned int i = 0;
    for(;i<l;i++) fprintf(o,"=");
    fprintf(o,"%c",rs_[rs++]); if(rs>3) rs=0;
    for(;i<74;i++) fprintf(o," ");
    fprintf(o,"|\r"); fflush(o);
    return;
  }
  fprintf(o,"\r%llu kB                    \r",all/1024);
}

void arcregister (
    const Arc::URL& source_url,
    const Arc::URL& destination_url,
    bool secure,
    bool passive,
    bool force_meta,
    int timeout
) {
  if(source_url.Protocol() == "urllist" &&
     destination_url.Protocol() == "urllist") {
    std::list<Arc::URL> sources = Arc::ReadURLList(source_url);
    std::list<Arc::URL> destinations = Arc::ReadURLList(destination_url);
    if(sources.size() == 0) {
      logger.msg(Arc::ERROR, "Can't read list of sources from file %s",
                 source_url.Path().c_str());
      return;
    }
    if(destinations.size() == 0) {
      logger.msg(Arc::ERROR, "Can't read list of destinations from file %s",
                 destination_url.Path().c_str());
      return;
    }
    if(sources.size() != destinations.size()) {
      logger.msg(Arc::ERROR,
                 "Numbers of sources and destinations do not match");
      return;
    }
    for(std::list<Arc::URL>::iterator source = sources.begin(),
          destination = destinations.begin();
        (source!=sources.end()) && (destination!=destinations.end());
        source++, destination++)
      arcregister(*source,*destination,secure,passive,force_meta,timeout);
    return;
  }
  if(source_url.Protocol() == "urllist") {
    std::list<Arc::URL> sources = Arc::ReadURLList(source_url);
    if(sources.size() == 0) {
      logger.msg(Arc::ERROR, "Can't read list of sources from file %s",
                 source_url.Path().c_str());
      return;
    }
    for(std::list<Arc::URL>::iterator source = sources.begin();
        source != sources.end(); source++)
      arcregister(*source,destination_url,secure,passive,force_meta,timeout);
    return;
  }
  if(destination_url.Protocol() == "urllist") {
    std::list<Arc::URL> destinations = Arc::ReadURLList(destination_url);
    if(destinations.size() == 0) {
      logger.msg(Arc::ERROR, "Can't read list of destinations from file %s",
                 destination_url.Path().c_str());
      return;
    }
    for(std::list<Arc::URL>::iterator destination = destinations.begin();
        destination != destinations.end(); destination++)
      arcregister(source_url,*destination,secure,passive,force_meta,timeout);
    return;
  }

  if(destination_url.Path()[destination_url.Path().length()-1] == '/') {
    logger.msg(Arc::ERROR, "Fileset registration is not supported yet");
    return;
  }
  Arc::DataHandle source(source_url);
  Arc::DataHandle destination(destination_url);
  if(!source) {
    logger.msg(Arc::ERROR, "Unsupported source url: %s",
               source_url.str().c_str());
    return;
  }
  if(!destination) {
    logger.msg(Arc::ERROR, "Unsupported destination url: %s",
               destination_url.str().c_str());
    return;
  }
  if(source->IsIndex() || !destination->IsIndex()) {
    logger.msg(Arc::ERROR, "For registration source must be ordinary URL"
	                   " and destination must be indexing service");
    return;
  }
  // Obtain meta-information about source
  if(!source->Check()) {
    logger.msg(Arc::ERROR, "Source probably does not exist");
    return;
  }
  // add new location
  if(!destination->Resolve(false)) {
    logger.msg(Arc::ERROR, "Problems resolving destination");
    return;
  }
  bool replication = destination->Registered();
  destination->SetMeta(*source); // pass metadata
  std::string metaname;
  // look for similar destination
  for(destination->SetTries(1); destination->LocationValid();
      destination->NextLocation()) {
    const Arc::URL& loc_url = destination->CurrentLocation();
    if(loc_url == source_url) {
      metaname = destination->CurrentLocationMetadata();
      break;
    }
  }
  // remove locations if exist
  for(destination->SetTries(1); destination->RemoveLocation();) { }
  // add new location
  if(metaname.empty()) {
    metaname = source_url.ConnectionURL();
  }
  if(!destination->AddLocation(source_url, metaname)) {
    destination->PreUnregister(replication);
    logger.msg(Arc::ERROR, "Failed to accept new file/destination");
    return;
  }
  destination->SetTries(1);
  if(!destination->PreRegister(replication, force_meta)) {
    logger.msg(Arc::ERROR, "Failed to register new file/destination");
    return;
  }
  if(!destination->PostRegister(replication)) {
    destination->PreUnregister(replication);
    logger.msg(Arc::ERROR, "Failed to register new file/destination");
    return;
  }
  return;
}

void arccp (
    const Arc::URL& source_url_,
    const Arc::URL& destination_url_,
    bool secure,
    bool passive,
    bool force_meta,
    int recursion,
    int tries,
    bool verbose,
    int timeout
) {
  Arc::URL source_url(source_url_);
  Arc::URL destination_url(destination_url_);
  std::string cache_path;
  std::string cache_data_path;
  std::string id = "<ngcp>";
  // bool verbose = false;
  if(timeout <= 0) timeout=300; // 5 minute default
  if(tries < 0) tries=0;
  if(source_url.Protocol() == "urllist" &&
     destination_url.Protocol() == "urllist") {
    std::list<Arc::URL> sources = Arc::ReadURLList(source_url);
    std::list<Arc::URL> destinations = Arc::ReadURLList(destination_url);
    if(sources.size() == 0) {
      logger.msg(Arc::ERROR, "Can't read list of sources from file %s",
                 source_url.Path().c_str());
      return;
    }
    if(destinations.size() == 0) {
      logger.msg(Arc::ERROR, "Can't read list of destinations from file %s",
                 destination_url.Path().c_str());
      return;
    }
    if(sources.size() != destinations.size()) {
      logger.msg(Arc::ERROR,
                 "Numbers of sources and destinations do not match");
      return;
    }
    for(std::list<Arc::URL>::iterator source = sources.begin(),
          destination = destinations.begin();
        (source != sources.end()) && (destination != destinations.end());
        source++, destination++)
      arccp(*source,*destination,
            secure,passive,force_meta,recursion,tries,verbose,timeout);
    return;
  }
  if(source_url.Protocol() == "urllist") {
    std::list<Arc::URL> sources = Arc::ReadURLList(source_url);
    if(sources.size() == 0) {
      logger.msg(Arc::ERROR, "Can't read list of sources from file %s",
                 source_url.Path().c_str());
      return;
    }

    for(std::list<Arc::URL>::iterator source = sources.begin();
        source != sources.end(); source++)
      arccp(*source,destination_url,
            secure,passive,force_meta,recursion,tries,verbose,timeout);
    return;
  }
  if(destination_url.Protocol() == "urllist") {
    std::list<Arc::URL> destinations = Arc::ReadURLList(destination_url);
    if(destinations.size() == 0) {
      logger.msg(Arc::ERROR, "Can't read list of destinations from file %s",
                 destination_url.Path().c_str());
      return;
    }
    for(std::list<Arc::URL>::iterator destination = destinations.begin();
        destination != destinations.end(); destination++)
      arccp(source_url,*destination,
            secure,passive,force_meta,recursion,tries,verbose,timeout);
    return;
  }

  if(destination_url.Path()[destination_url.Path().length()-1] != '/') {
    if(source_url.Path()[source_url.Path().length()-1] == '/') {
      logger.msg(Arc::ERROR,
                 "Fileset copy to single object is not supported yet");
      return;
    }
  } else {
    // Copy TO fileset/directory
    if(source_url.Path()[source_url.Path().length()-1] != '/') {
      // Copy FROM single object
      std::string::size_type p = source_url.Path().rfind('/');
      if(p == std::string::npos) {
        logger.msg(Arc::ERROR, "Can't extract object's name from source url");
        return;
      }
      destination_url.ChangePath(destination_url.Path() +
                                 source_url.Path().substr(p+1));
    } else {
      // Fileset copy
      // Find out if source can be listed (TODO - through datapoint)
      if((source_url.Protocol() != "rc") &&
         (source_url.Protocol() != "rls") &&
         (source_url.Protocol() != "fireman") &&
         (source_url.Protocol() != "file") &&
         (source_url.Protocol() != "se") &&
         (source_url.Protocol() != "gsiftp") &&
         (source_url.Protocol() != "ftp")) {
        logger.msg(Arc::ERROR,
                   "Fileset copy for this kind of source is not supported");
        return;
      }
      Arc::DataHandle source(source_url);
      if(!source) {
        logger.msg(Arc::ERROR, "Unsupported source url: %s",
                   source_url.str().c_str());
        return;
      }
      std::list<Arc::FileInfo> files;
      if(source->IsIndex()) {
        if(!source->ListFiles(files,recursion>0)) {
          logger.msg(Arc::ERROR,"Failed listing metafiles");
          return;
        }
      } else {
        if(!source->ListFiles(files,true)) {
          logger.msg(Arc::ERROR,"Failed listing files");
          return;
        }
      }
      bool failures = false;
      // Handle transfer of files first (treat unknown like files)
      for(std::list<Arc::FileInfo>::iterator i = files.begin();
          i != files.end(); i++) {
        if((i->GetType() != Arc::FileInfo::file_type_unknown) && 
           (i->GetType() != Arc::FileInfo::file_type_file)) continue;
        logger.msg(Arc::INFO,"Name: %s", i->GetName().c_str());
        std::string s_url(source_url.str());
        std::string d_url(destination_url.str());
        s_url+=i->GetName();
        d_url+=i->GetName();
        logger.msg(Arc::INFO,"Source: %s", s_url.c_str());
        logger.msg(Arc::INFO,"Destination: %s", d_url.c_str());
        Arc::DataHandle source(s_url);
        Arc::DataHandle destination(d_url);
        if(!source) {
          logger.msg(Arc::INFO, "Unsupported source url: %s", s_url.c_str());
          continue;
        }
        if(!destination) {
          logger.msg(Arc::INFO, "Unsupported destination url: %s",
                     d_url.c_str());
          continue;
        }
        Arc::DataMover mover;
        mover.secure(secure);
        mover.passive(passive);
        mover.verbose(verbose);
        mover.force_to_meta(force_meta);
        if(tries) {
          mover.retry(true); // go through all locations
          source->SetTries(tries); // try all locations "tries" times
          destination->SetTries(tries);
        }
        Arc::User cache_user;
        Arc::DataCache cache(cache_path,cache_data_path,"",id,
                             cache_user);
        std::string failure;
        if(!mover.Transfer(*source,*destination,cache,Arc::URLMap(),
			   0,0,0,timeout,failure)) {
          if(!failure.empty())
            logger.msg(Arc::INFO,"Current transfer FAILED: %s",
                       failure.c_str());
          else
            logger.msg(Arc::INFO,"Current transfer FAILED.");
          destination->SetTries(1);
          // It is not clear how to clear half-registered file. So remove it 
          // only in case of explicit destination.
          if(!(destination->IsIndex()))
	    mover.Delete(*destination);
          failures=true;
        }
        else
          logger.msg(Arc::INFO, "Current transfer complete.");
      }
      if(failures) {
        logger.msg(Arc::ERROR, "Some transfers failed");
        return;
      }
      // Go deeper if allowed
      if(recursion > 0) {
        // Handle directories recursively
        for(std::list<Arc::FileInfo>::iterator i = files.begin();
            i != files.end(); i++) {
          if(i->GetType() != Arc::FileInfo::file_type_dir) continue;
          if(verbose)
            logger.msg(Arc::INFO,"Directory: %s",i->GetName().c_str());
          std::string s_url(source_url.str());
          std::string d_url(destination_url.str());
          s_url+=i->GetName(); d_url+=i->GetName();
          s_url+="/"; d_url+="/";
          arccp(s_url,d_url,
                secure,passive,force_meta,recursion-1,tries,verbose,timeout);
        }
      }
      return;
    }
  }
  Arc::DataHandle source(source_url);
  Arc::DataHandle destination(destination_url);
  if(!source) {
    logger.msg(Arc::ERROR, "Unsupported source url: %s",
               source_url.str().c_str());
    return;
  }
  if(!destination) {
    logger.msg(Arc::ERROR, "Unsupported destination url: %s",
               destination_url.str().c_str());
    return;
  }
  Arc::DataMover mover;
  mover.secure(secure);
  mover.passive(passive);
  mover.verbose(verbose);
  mover.force_to_meta(force_meta);
  if(tries) { // 0 means default behavior
    mover.retry(true); // go through all locations
    source->SetTries(tries); // try all locations "tries" times
    destination->SetTries(tries);
  }
  Arc::User cache_user;
  Arc::DataCache cache(cache_path,cache_data_path,"",id,cache_user);
  if(verbose) mover.set_progress_indicator(&progress);
  std::string failure;
  if(!mover.Transfer(*source,*destination,cache,Arc::URLMap(),
		     0,0,0,timeout,failure)) {
    if(failure.length()) {
      logger.msg(Arc::ERROR, "Transfer FAILED: %s", failure.c_str());
      return;
    } else {
      logger.msg(Arc::ERROR, "Transfer FAILED.");
      return;
    }
    destination->SetTries(1);
    // It is not clear how to clear half-registered file. So remove it only
    // in case of explicit destination.
    if(!(destination->IsIndex()))
      mover.Delete(*destination);
  }
  logger.msg(Arc::INFO, "Transfer complete.");
  return;
}

#define ARCCP
#include "arccli.cpp"
