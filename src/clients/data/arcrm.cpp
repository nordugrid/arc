#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string>
#include <list>

#include <arc/data/DataHandle.h>
#include <arc/Logger.h>
#include <arc/StringConv.h>
#include <arc/URL.h>

static Arc::Logger logger(Arc::Logger::getRootLogger(), "arcrm");

void arcrm(const Arc::URL& file_url,
	   bool errcont,
	   int timeout) {
  if (file_url.Protocol() == "filelist") {
    std::list<Arc::URL> files = Arc::ReadURLList(file_url);
    if (files.size() == 0) {
      logger.msg(Arc::ERROR, "Can't read list of locations from file %s",
		 file_url.Path());
      return;
    }
    for (std::list<Arc::URL>::iterator file = files.begin();
	 file != files.end(); file++)
      arcrm(*file, errcont, timeout);
    return;
  }

  Arc::DataHandle url(file_url);
  if (!url) {
    logger.msg(Arc::ERROR, "Unsupported url given");
    return;
  }
  bool remove_lfn = true;
  if (file_url.Locations().size() != 0)
    remove_lfn = false;
  if (!url->Resolve(true))
    if (remove_lfn)
      logger.msg(Arc::INFO,
		 "No locations found - probably no more physical instances");
  /* go through all locations and try to remove files
     physically and their metadata */
  std::list<Arc::URL> removed_urls; // list of physical removed urls
  if (url->HaveLocations())
    for (; url->LocationValid();) {
      logger.msg(Arc::INFO, "Removing %s", url->CurrentLocation().str());
      // It can happen that after resolving list contains duplicated
      // physical locations obtained from different meta-data-services.
      // Because not all locations can reliably say if files does not exist
      // or access is not allowed, avoid duplicated delete attempts.
      bool url_was_deleted = false;
      for (std::list<Arc::URL>::iterator u = removed_urls.begin();
	   u != removed_urls.end(); u++)
	if (url->CurrentLocation() == (*u)) {
	  url_was_deleted = true;
	  break;
	}
      if (url_was_deleted)
	logger.msg(Arc::ERROR, "This instance was already deleted");
      else {
	url->SetSecure(false);
	if (!url->Remove()) {
	  logger.msg(Arc::ERROR, "Failed to delete physical file");
	  if (!errcont) {
	    url->NextLocation();
	    continue;
	  }
	}
	else
	  removed_urls.push_back(url->CurrentLocation());
      }
      if (!url->IsIndex())
	url->RemoveLocation();
      else {
	logger.msg(Arc::INFO, "Removing metadata in %s",
		   url->CurrentLocationMetadata());
	if (!url->Unregister(false)) {
	  logger.msg(Arc::ERROR, "Failed to delete meta-information");
	  url->NextLocation();
	}
      }
    }
  if (url->HaveLocations()) {
    logger.msg(Arc::ERROR, "Failed to remove all physical instances");
    return;
  }
  if (url->IsIndex())
    if (remove_lfn) {
      logger.msg(Arc::ERROR, "Removing logical file from metadata %s",
		 url->str());
      if (!url->Unregister(true)) {
	logger.msg(Arc::ERROR, "Failed to delete logical file");
	return;
      }
    }
  return;
}

#define ARCRM
#include "arccli.cpp"
