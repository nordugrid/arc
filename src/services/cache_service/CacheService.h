#ifndef CACHESERVICE_H_
#define CACHESERVICE_H_


#include <arc/infosys/RegisteredService.h>
#include <arc/message/Message.h>
#include <arc/Logger.h>
#include <arc/XMLNode.h>
#include <string>

#include "../a-rex/grid-manager/jobs/users.h"

namespace Cache {

/**
 * CacheService provides functionality for A-REX cache operations that can be
 * performed by remote clients. It currently consists of two operations:
 * CacheCheck - allows querying of the cache for the presence of files.
 * CacheLink - enables a running job to dynamically request cache files to
 * be linked to its working (session) directory. This is especially useful
 * in the case of pilot job workflows where job submission does not follow
 * the usual ARC workflow. In order for input files to be available to jobs,
 * the pilot job can call the cache service to prepare them. If requested files
 * are not present in the cache, they may be downloaded by the cache service
 * depending on configuration settings. (TODO: how exactly download will be
 * done).
 */
class CacheService: public Arc::RegisteredService {

 private:
  /** Return codes of cache link */
  enum CacheLinkReturnCode {
    Success,                 // everything went ok
    NotAvailable,            // cache file doesn't exist and dostage is false
    Locked,                  // cache file is locked (being downloaded by other process)
    CacheError,              // error with cache (configuration, filesystem etc)
    PermissionError,         // user doesn't have permission on original source
    LinkError,               // error while linking to session dir
    DownloadError,           // error downloading cache file
    TooManyDownloadsError    // current number of downloads exceeds limit
  };
  /** Construct a SOAP error message with optional extra reason string */
  Arc::MCC_Status make_soap_fault(Arc::Message& outmsg, const std::string& reason = "");
  /** CacheService namespace */
  Arc::NS ns;
  /** Download limit read from cache service configuration */
  unsigned int max_downloads;
  /** Current downloads - may need a lock around it */
  unsigned int current_downloads;
  /** Users read from A-REX configuration */
  JobUsers* users;
  /** Holds environment state, eg config files etc */
  GMEnvironment* gm_env;
  /** Flag to say whether CacheService is valid */
  bool valid;
  /** Logger object */
  static Arc::Logger logger;

 protected:
  /* Cache operations */
  /**
   * Check whether the URLs supplied in the input are present in any cache.
   * Returns in the out message for each file true or false, and if true,
   * the size of the file on cache disk.
   * @param user A-REX user configuration for the mapped user
   */
  Arc::MCC_Status CacheCheck(Arc::XMLNode in, Arc::XMLNode out, const JobUser& user);
  /**
   * This method is used to link cache files to the session dir. A list of
   * URLs is supplied and if they are present in the cache and the user
   * calling the service has permission to access them, then they are linked
   * to the given session directory.
   * TODO: What to do when files are missing
   * @param user A-REX user configuration for the mapped user
   * @param mapped_user The local user to which the client DN was mapped
   */
  Arc::MCC_Status CacheLink(Arc::XMLNode in, Arc::XMLNode out,
                            const JobUser& user, const Arc::User& mapped_user);

 public:
  /**
   * Make a new CacheService. Reads the configuration and determines
   * the validity of the service.
   */
  CacheService(Arc::Config *cfg);
  /**
   * Destroy the CacheService
   */
  virtual ~CacheService(void);
  /**
   * Main method called by HED when CacheService is invoked. Directs call
   * to appropriate CacheService method.
   * */
  virtual Arc::MCC_Status process(Arc::Message &inmsg, Arc::Message &outmsg);
  /**
   * Supplies information on the service for use in the information system.
   */
  bool RegistrationCollector(Arc::XMLNode &doc);
  /** Returns true if the CacheService is valid. */
  operator bool() { return valid; };
  /** Returns true if the CacheService is not valid. */
  bool operator!() { return !valid; };
};

} // namespace Cache

#endif /* CACHESERVICE_H_ */
