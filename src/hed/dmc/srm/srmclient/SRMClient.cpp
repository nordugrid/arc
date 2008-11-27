#include "SRMClient.h"
#include "SRM1Client.h"
#include "SRM22Client.h"

//namespace Arc {
  
Arc::Logger SRMClient::logger(Arc::Logger::getRootLogger(), "SRMClient");

  time_t SRMClient::request_timeout=300;
  
  SRMClient* SRMClient::getInstance(std::string url,
                                    time_t timeout,
                                    SRMVersion srm_version) {
  
    request_timeout = timeout;
  
    // if version is defined, return the appropriate client
    if (srm_version == SRM_V1) return new SRM1Client(url);
    if (srm_version == SRM_V2_2) return new SRM22Client(url);
  
    // try to do srmPing with the 2.2 client - if this returns ok
    // use v2.2, else use v1, or error, depending on the return value
  
    SRMClient * client = new SRM22Client(url);
    std::string version;
  
    SRMReturnCode srm_error;
    if ((srm_error = client->ping(version, false)) == SRM_OK) {
      if (version.compare("v2.2") == 0) {
        logger.msg(Arc::DEBUG, "srmPing gives v2.2, instantiating v2.2 client");
        return client;
      };
    };
  
    // if soap error probably means v2 not supported
    if (srm_error == SRM_ERROR_SOAP) {
      logger.msg(Arc::DEBUG, "SOAP error with srmPing, instantiating v1 client");
      return new SRM1Client(url);
    };
  
    // any other error probably means service is down - return error
    // TODO - proper exceptions!
    //throw new SRMServiceError;
    logger.msg(Arc::ERROR, "Service error, cannot instantiate SRM client");
    return NULL;
  
  };

//} // namespace Arc
