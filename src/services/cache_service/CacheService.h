#ifndef CACHESERVICE_H_
#define CACHESERVICE_H_


#include <arc/infosys/RegisteredService.h>
#include <arc/message/Message.h>
#include <arc/Logger.h>
#include <arc/XMLNode.h>
#include <string>

namespace Cache {

/**
 * CacheService provides functionality for A-REX cache operations that can be
 * performed by remote clients.
 */
class CacheService: public Arc::RegisteredService {

 private:
  /** Construct a SOAP error message */
  Arc::MCC_Status make_soap_fault(Arc::Message& outmsg);
  /** CacheService namespace */
  Arc::NS ns;
  /** Caches as taken from the configuration */
  std::vector<std::string> caches;
  /** FileCache object */
  Arc::FileCache* cache;
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
   */
  Arc::MCC_Status CacheCheck(Arc::XMLNode in,Arc::XMLNode out);
  /**
   * This method is used to link cache files to the session dir. A list of
   * URLs is supplied and if they are present in the cache and the user
   * calling the service has permission to access them, then they are linked
   * to the given session directory.
   * TODO: What to do when files are missing
   */
  Arc::MCC_Status CacheLink(Arc::XMLNode in,Arc::XMLNode out);

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
