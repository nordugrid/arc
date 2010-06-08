#include <vector>

#include "SRMClient.h"
#include "SRM1Client.h"
#include "SRM22Client.h"
#include "SRMInfo.h"

//namespace Arc {
  
Arc::Logger SRMClient::logger(Arc::Logger::getRootLogger(), "SRMClient");

  time_t SRMClient::request_timeout=300;
  
  SRMReturnCode SRMClient::connect(void) {
    if(!csoap) return SRM_ERROR_OTHER;
    int r = csoap->connect();
    if (r == 1) return SRM_ERROR_TEMPORARY;
    if (r != 0) return SRM_ERROR_CONNECTION;
    return SRM_OK;
  }

  SRMClient* SRMClient::getInstance(const Arc::UserConfig& usercfg,
                                    std::string url,
                                    bool& timedout,
                                    time_t timeout) {
    request_timeout = timeout;
    SRMURL srm_url(url);
    if (!srm_url) return NULL;
  
    // can't use ping with srmv1 so just return
    if (srm_url.SRMVersion() == SRMURL::SRM_URL_VERSION_1)
      return new SRM1Client(usercfg, srm_url);
  
    if (usercfg.UtilsDirPath().empty()) {
      if (srm_url.SRMVersion() == SRMURL::SRM_URL_VERSION_2_2) return new SRM22Client(usercfg, srm_url);
      return NULL;
    }
      
    SRMReturnCode srm_error;
    std::string version;
  
    SRMInfo info(usercfg.UtilsDirPath());
    SRMFileInfo srm_file_info;
    // lists of ports and protocols in the order to try them
    std::vector<int> ports;
    std::vector<std::string> protocols;

    // take hints from certain keywords in the url
    if (srm_url.Path().find("/dpm/") != std::string::npos) {
      ports.push_back(8446);
      ports.push_back(8443);
      ports.push_back(8444);
    }
    else {
      ports.push_back(srm_url.Port());
      if (srm_url.Port() != 8443) ports.push_back(8443);
      if (srm_url.Port() != 8446) ports.push_back(8446);
      if (srm_url.Port() != 8444) ports.push_back(8444);
    }
    if (!srm_url.GSSAPI() || srm_url.Host().find("dcache") != std::string::npos) {
      protocols.push_back("gsi");
      protocols.push_back("gssapi");
    }
    else {
      protocols.push_back("gssapi");
      protocols.push_back("gsi");
    }    
    
    srm_file_info.host = srm_url.Host();
    srm_file_info.version = srm_url.SRMVersion();
    
    // no info
    if (!info.getSRMFileInfo(srm_file_info)) {
      for (std::vector<std::string>::iterator protocol = protocols.begin(); protocol != protocols.end(); ++protocol) {
        srm_url.GSSAPI((*protocol == "gssapi") ? true : false);
        for (std::vector<int>::iterator port = ports.begin(); port != ports.end(); ++port) {
          logger.msg(Arc::VERBOSE, "Attempting to contact %s on port %i using protocol %s", srm_url.Host(), *port, *protocol);
          srm_url.SetPort(*port);
          SRMClient * client = new SRM22Client(usercfg, srm_url);
      
          if ((srm_error = client->ping(version, false)) == SRM_OK) {
            srm_file_info.port = *port;
            srm_file_info.protocol = *protocol;
            logger.msg(Arc::VERBOSE, "Storing port %i and protocol %s for %s", *port, *protocol, srm_url.Host());
            info.putSRMFileInfo(srm_file_info);
            return client;
          }
          delete client;
          if (srm_error == SRM_ERROR_TEMPORARY) {
            // probably correct port and protocol and service is down
            // but don't want to risk storing incorrect info
            timedout = true;
            return NULL;
          }
        }
      }
      // if we get here no combination has worked
      logger.msg(Arc::VERBOSE, "No combination of port and protocol succeeded for %s", srm_url.Host());
      return NULL;
    }
    // url agrees with file info
    else if (srm_file_info == srm_url) {
      srm_url.SetPort(srm_file_info.port);
      srm_url.GSSAPI((srm_file_info.protocol == "gssapi") ? true : false);
      return new SRM22Client(usercfg, srm_url);
    }
    // url disagrees with file info
    else {
      // ping and if ok, replace file info
      logger.msg(Arc::VERBOSE, "URL %s disagrees with stored SRM info, testing new info", srm_url.ShortURL());
      SRMClient * client = new SRM22Client(usercfg, srm_url);
  
      if ((srm_error = client->ping(version, false)) == SRM_OK) {
        srm_file_info.port = srm_url.Port();
        srm_file_info.protocol = srm_url.GSSAPI()? "gssapi" : "gsi";
        logger.msg(Arc::VERBOSE, "Replacing old SRM info with new for URL %s", srm_url.ShortURL());
        info.putSRMFileInfo(srm_file_info);
        return client;
      }
      delete client;
      if (srm_error == SRM_ERROR_TEMPORARY) {
        // probably correct port and protocol and service is down
        // but don't want to risk storing incorrect info
        timedout = true;
      }
      return NULL;
    }
 }
//} // namespace Arc
