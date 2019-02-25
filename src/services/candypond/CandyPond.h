#ifndef CANDYPONDSERVICE_H_
#define CANDYPONDSERVICE_H_


#include <arc/message/Service.h>
#include <arc/message/Message.h>
#include <arc/Logger.h>
#include <arc/XMLNode.h>
#include <string>

// A-REX includes for GM configuration and delegation
#include "../a-rex/grid-manager/conf/GMConfig.h"
#include "../a-rex/grid-manager/files/ControlFileContent.h"
#include "../a-rex/grid-manager/files/ControlFileHandling.h"
#include "../a-rex/delegation/DelegationStore.h"

#include "CandyPondGenerator.h"

namespace CandyPond {

/**
 * CandyPond provides functionality for A-REX cache operations that can be
 * performed by remote clients. It currently consists of three operations:
 * CacheCheck - allows querying of the cache for the presence of files.
 * CacheLink - enables a running job to dynamically request cache files to
 * be linked to its working (session) directory.
 * CacheLinkQuery - query the status of a transfer initiated by CacheLink.
 * This service is especially useful in the case of pilot job workflows where
 * job submission does not follow the usual ARC workflow. In order for input
 * files to be available to jobs, the pilot job can call CandyPond to
 * prepare them. If requested files are not present in the cache, they can be
 * downloaded by CandyPond if requested, using the DTR data staging
 * framework.
 */
class CandyPond: public Arc::Service {

 private:
  /** Return codes of cache link */
  enum CacheLinkReturnCode {
    Success,                 // everything went ok
    Staging,                 // files are still in the middle of downloading
    NotAvailable,            // cache file doesn't exist and dostage is false
    Locked,                  // cache file is locked (being downloaded by other process)
    CacheError,              // error with cache (configuration, filesystem etc)
    PermissionError,         // user doesn't have permission on original source
    LinkError,               // error while linking to session dir
    DownloadError,           // error downloading cache file
    BadURLError,             // A bad URL was supplied which could not be handled
  };
  /** Construct a SOAP error message with optional extra reason string */
  Arc::MCC_Status make_soap_fault(Arc::Message& outmsg, const std::string& reason = "");
  /** CandyPond namespace */
  Arc::NS ns;
  /** A-REX configuration */
  ARex::GMConfig config;
  /** Generator to handle data staging */
  CandyPondGenerator* dtr_generator;
  /** Logger object */
  static Arc::Logger logger;

 protected:
  /* Cache operations */
  /**
   * Check whether the URLs supplied in the input are present in any cache.
   * Returns in the out message for each file true or false, and if true,
   * the size of the file on cache disk.
   * @param mapped_user The local user to which the client DN was mapped
   */
  Arc::MCC_Status CacheCheck(Arc::XMLNode in, Arc::XMLNode out, const Arc::User& mapped_user);
  /**
   * This method is used to link cache files to the session dir. A list of
   * URLs is supplied and if they are present in the cache and the user
   * calling the service has permission to access them, then they are linked
   * to the given session directory. If the user requests that missing files
   * be staged, then data staging requests are entered. The user should then
   * use CacheLinkQuery to poll the status of the requests.
   * @param mapped_user The local user to which the client DN was mapped
   */
  Arc::MCC_Status CacheLink(Arc::XMLNode in, Arc::XMLNode out, const Arc::User& mapped_user);

  /**
   * Query the status of data staging for a given job ID.
   */
  Arc::MCC_Status CacheLinkQuery(Arc::XMLNode in, Arc::XMLNode out);

 public:
  /**
   * Make a new CandyPond. Reads the configuration and determines
   * the validity of the service.
   */
  CandyPond(Arc::Config *cfg, Arc::PluginArgument* parg);
  /**
   * Destroy the CandyPond
   */
  virtual ~CandyPond(void);
  /**
   * Main method called by HED when CandyPond is invoked. Directs call
   * to appropriate CandyPond method.
   */
  virtual Arc::MCC_Status process(Arc::Message &inmsg, Arc::Message &outmsg);
  /** Returns true if the CandyPond is valid. */
  operator bool() { return valid; };
  /** Returns true if the CandyPond is not valid. */
  bool operator!() { return !valid; };
};

} // namespace CandyPond

#endif /* CANDYPONDSERVICE_H_ */
