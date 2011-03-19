// -*- indent-tabs-mode: nil -*-

#include "SRMClient.h"
#include "SRM1Client.h"
#include "SRM22Client.h"
#include "SRMInfo.h"

namespace Arc {

  Logger SRMClient::logger(Logger::getRootLogger(), "SRMClient");

  SRMClient::SRMClient(const UserConfig& usercfg, const SRMURL& url)
    : service_endpoint(url.ContactURL()),
      implementation(SRM_IMPLEMENTATION_UNKNOWN),
      user_timeout(usercfg.Timeout()) {
    usercfg.ApplyToConfig(cfg);
    client = new ClientSOAP(cfg, service_endpoint, usercfg.Timeout());
  }

  SRMClient::~SRMClient() {
    if (client)
      delete client;
  }

  SRMClient* SRMClient::getInstance(const UserConfig& usercfg,
                                    const std::string& url,
                                    bool& timedout) {
    SRMURL srm_url(url);
    if (!srm_url) return NULL;

    // can't use ping with srmv1 so just return
    if (srm_url.SRMVersion() == SRMURL::SRM_URL_VERSION_1) {
      return new SRM1Client(usercfg, srm_url);
    }

    if (usercfg.UtilsDirPath().empty()) {
      if (srm_url.SRMVersion() == SRMURL::SRM_URL_VERSION_2_2) {
        return new SRM22Client(usercfg, srm_url);
      }
      return NULL;
    }

    SRMReturnCode srm_error;
    std::string version;

    SRMInfo info(usercfg.UtilsDirPath());
    SRMFileInfo srm_file_info;
    // lists of ports in the order to try them
    std::list<int> ports;

    // if port is specified then only try that one
    if (srm_url.PortDefined()) {
      ports.push_back(srm_url.Port());
    }
    // take hints from certain keywords in the url
    else if (srm_url.Path().find("/dpm/") != std::string::npos) {
      ports.push_back(8446);
      ports.push_back(8443);
      ports.push_back(8444);
    }
    else {
      ports.push_back(8443);
      ports.push_back(8446);
      ports.push_back(8444);
    }

    srm_file_info.host = srm_url.Host();
    srm_file_info.version = srm_url.SRMVersion();

    // no info
    if (!info.getSRMFileInfo(srm_file_info)) {
      for (std::list<int>::iterator port = ports.begin();
           port != ports.end(); ++port) {
        logger.msg(VERBOSE, "Attempting to contact %s on port %i",
                   srm_url.Host(), *port);
        srm_url.SetPort(*port);
        SRMClient *client = new SRM22Client(usercfg, srm_url);

        if ((srm_error = client->ping(version, false)) == SRM_OK) {
          srm_file_info.port = *port;
          logger.msg(VERBOSE, "Storing port %i for %s", *port, srm_url.Host());
          info.putSRMFileInfo(srm_file_info);
          return client;
        }
        delete client;
        if (srm_error == SRM_ERROR_TEMPORARY) {
          // probably correct port and service is down
          // but don't want to risk storing incorrect info
          timedout = true;
          return NULL;
        }
      }
      // if we get here no port has worked
      logger.msg(VERBOSE, "No port succeeded for %s", srm_url.Host());
    }
    // url agrees with file info
    else if (srm_file_info == srm_url) {
      srm_url.SetPort(srm_file_info.port);
      return new SRM22Client(usercfg, srm_url);
    }
    // url disagrees with file info
    else {
      // ping and if ok, replace file info
      logger.msg(VERBOSE,
                 "URL %s disagrees with stored SRM info, testing new info",
                 srm_url.ShortURL());
      SRMClient *client = new SRM22Client(usercfg, srm_url);

      if ((srm_error = client->ping(version, false)) == SRM_OK) {
        srm_file_info.port = srm_url.Port();
        logger.msg(VERBOSE, "Replacing old SRM info with new for URL %s",
                   srm_url.ShortURL());
        info.putSRMFileInfo(srm_file_info);
        return client;
      }
      delete client;
      if (srm_error == SRM_ERROR_TEMPORARY) {
        // probably correct port and service is down
        // but don't want to risk storing incorrect info
        timedout = true;
      }
    }
    return NULL;
  }

  SRMReturnCode SRMClient::process(PayloadSOAP *request,
                                   PayloadSOAP **response) {

    if (logger.getThreshold() <= DEBUG) {
      std::string xml;
      request->GetXML(xml, true);
      logger.msg(DEBUG, "SOAP request: %s", xml);
    }

    MCC_Status status = client->process(request, response);

    // Try to reconnect in case of failure
    if (*response && (*response)->IsFault()) {
      logger.msg(DEBUG, "SOAP fault: %s", (*response)->Fault()->Reason());
      logger.msg(DEBUG, "Reconnecting");
      delete *response; *response = NULL;
      delete client;
      client = new ClientSOAP(cfg, service_endpoint, user_timeout);
      status = client->process(request, response);
    }

    if (!status) {
      logger.msg(VERBOSE, "SRM Client status: %s", (std::string)status);
      if (*response) delete *response;
      *response = NULL;
      return SRM_ERROR_SOAP;
    }
    if (!(*response)) {
      logger.msg(VERBOSE, "No SOAP response");
      return SRM_ERROR_SOAP;
    }

    if (logger.getThreshold() <= DEBUG) {
      std::string xml;
      (*response)->GetXML(xml, true);
      logger.msg(DEBUG, "SOAP response: %s", xml);
    }

    if ((*response)->IsFault()) {
      logger.msg(VERBOSE, "SOAP fault: %s", (*response)->Fault()->Reason());
      delete *response;
      *response = NULL;
      return SRM_ERROR_SOAP;
    }

    return SRM_OK;
  }

} // namespace Arc
