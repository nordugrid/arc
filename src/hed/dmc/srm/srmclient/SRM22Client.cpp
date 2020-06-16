// -*- indent-tabs-mode: nil -*-
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "SRM22Client.h"

#include <arc/StringConv.h>
#include <arc/User.h>

#include <unistd.h>

namespace ArcDMCSRM {

  using namespace Arc;

  /**
   * The max number of files returned when listing dirs
   * current limits are 1000 for dcache, 1024 for castor
   * info() will be called multiple times for directories
   * with more entries than max_files_list
   */
  const static unsigned int max_files_list = 999;


  SRM22Client::SRMStatusCode SRM22Client::GetStatus(XMLNode res, std::string& explanation) {
    std::string statuscode = (std::string)res["statusCode"];
    if (res["explanation"]) explanation = (std::string)res["explanation"];

    if (statuscode == "SRM_SUCCESS")                 return SRM_SUCCESS;
    if (statuscode == "SRM_FAILURE")                 return SRM_FAILURE;
    if (statuscode == "SRM_AUTHENTICATION_FAILURE")  return SRM_AUTHENTICATION_FAILURE;
    if (statuscode == "SRM_AUTHORIZATION_FAILURE")   return SRM_AUTHORIZATION_FAILURE;
    if (statuscode == "SRM_INVALID_REQUEST")         return SRM_INVALID_REQUEST;
    if (statuscode == "SRM_INVALID_PATH")            return SRM_INVALID_PATH;
    if (statuscode == "SRM_FILE_LIFETIME_EXPIRED")   return SRM_FILE_LIFETIME_EXPIRED;
    if (statuscode == "SRM_SPACE_LIFETIME_EXPIRED")  return SRM_SPACE_LIFETIME_EXPIRED;
    if (statuscode == "SRM_EXCEED_ALLOCATION")       return SRM_EXCEED_ALLOCATION;
    if (statuscode == "SRM_NO_USER_SPACE")           return SRM_NO_USER_SPACE;
    if (statuscode == "SRM_NO_FREE_SPACE")           return SRM_NO_FREE_SPACE;
    if (statuscode == "SRM_DUPLICATION_ERROR")       return SRM_DUPLICATION_ERROR;
    if (statuscode == "SRM_NON_EMPTY_DIRECTORY")     return SRM_NON_EMPTY_DIRECTORY;
    if (statuscode == "SRM_TOO_MANY_RESULTS")        return SRM_TOO_MANY_RESULTS;
    if (statuscode == "SRM_INTERNAL_ERROR")          return SRM_INTERNAL_ERROR;
    if (statuscode == "SRM_FATAL_INTERNAL_ERROR")    return SRM_FATAL_INTERNAL_ERROR;
    if (statuscode == "SRM_NOT_SUPPORTED")           return SRM_NOT_SUPPORTED;
    if (statuscode == "SRM_REQUEST_QUEUED")          return SRM_REQUEST_QUEUED;
    if (statuscode == "SRM_REQUEST_INPROGRESS")      return SRM_REQUEST_INPROGRESS;
    if (statuscode == "SRM_REQUEST_SUSPENDED")       return SRM_REQUEST_SUSPENDED;
    if (statuscode == "SRM_ABORTED")                 return SRM_ABORTED;
    if (statuscode == "SRM_RELEASED")                return SRM_RELEASED;
    if (statuscode == "SRM_FILE_PINNED")             return SRM_FILE_PINNED;
    if (statuscode == "SRM_FILE_IN_CACHE")           return SRM_FILE_IN_CACHE;
    if (statuscode == "SRM_SPACE_AVAILABLE")         return SRM_SPACE_AVAILABLE;
    if (statuscode == "SRM_LOWER_SPACE_GRANTED")     return SRM_LOWER_SPACE_GRANTED;
    if (statuscode == "SRM_DONE")                    return SRM_DONE;
    if (statuscode == "SRM_PARTIAL_SUCCESS")         return SRM_PARTIAL_SUCCESS;
    if (statuscode == "SRM_REQUEST_TIMED_OUT")       return SRM_REQUEST_TIMED_OUT;
    if (statuscode == "SRM_LAST_COPY")               return SRM_LAST_COPY;
    if (statuscode == "SRM_FILE_BUSY")               return SRM_FILE_BUSY;
    if (statuscode == "SRM_FILE_LOST")               return SRM_FILE_LOST;
    if (statuscode == "SRM_FILE_UNAVAILABLE")        return SRM_FILE_UNAVAILABLE;
    if (statuscode == "SRM_CUSTOM_STATUS")           return SRM_CUSTOM_STATUS;
    // fallback - should not happen
    return SRM_FAILURE;
  }

  SRM22Client::SRM22Client(const UserConfig& usercfg, const SRMURL& url) 
    : SRMClient(usercfg, url) {
    version = "v2.2";
    ns["SRMv2"] = "http://srm.lbl.gov/StorageResourceManager";
  }

  SRM22Client::~SRM22Client() {}

  DataStatus SRM22Client::ping(std::string& version) {

    PayloadSOAP request(ns);
    request.NewChild("SRMv2:srmPing").NewChild("srmPingRequest");

    PayloadSOAP *response = NULL;
    DataStatus status = process("", &request, &response);
    if (!status) return status;

    XMLNode res = (*response)["srmPingResponse"]["srmPingResponse"];

    if (!res) {
      logger.msg(VERBOSE, "Could not determine version of server");
      delete response;
      return DataStatus(DataStatus::GenericError, EARCRESINVAL, "Could not determine version of server");
    }

    version = (std::string)res["versionInfo"];
    logger.msg(VERBOSE, "Server SRM version: %s", version);

    for (XMLNode n = res["otherInfo"]["extraInfoArray"]; n; ++n) {
      if ((std::string)n["key"] == "backend_type") {
        std::string value = (std::string)n["value"];
        logger.msg(VERBOSE, "Server implementation: %s", value);
        if (value == "dCache") implementation = SRM_IMPLEMENTATION_DCACHE;
        else if (value == "CASTOR") implementation = SRM_IMPLEMENTATION_CASTOR;
        else if (value == "DPM") implementation = SRM_IMPLEMENTATION_DPM;
        else if (value == "StoRM") implementation = SRM_IMPLEMENTATION_STORM;
      }
    }

    delete response;
    return DataStatus::Success;
  }

  DataStatus SRM22Client::getSpaceTokens(std::list<std::string>& tokens,
                                         const std::string& description) {
    PayloadSOAP request(ns);
    XMLNode req = request.NewChild("SRMv2:srmGetSpaceTokens")
                  .NewChild("srmGetSpaceTokensRequest");
    if (!description.empty()) req.NewChild("userSpaceTokenDescription") = description;

    PayloadSOAP *response = NULL;
    DataStatus status = process("", &request, &response);
    if (!status) return status;

    XMLNode res = (*response)["srmGetSpaceTokensResponse"]
                  ["srmGetSpaceTokensResponse"];

    std::string explanation;
    SRMStatusCode statuscode = GetStatus(res["returnStatus"], explanation);

    if (statuscode != SRM_SUCCESS) {
      logger.msg(VERBOSE, "%s", explanation);
      delete response;
      return DataStatus(DataStatus::WritePrepareError, srm2errno(statuscode), explanation);
    }

    for (XMLNode n = res["arrayOfSpaceTokens"]["stringArray"]; n; ++n) {
      std::string token = (std::string)n;
      logger.msg(VERBOSE, "Adding space token %s", token);
      tokens.push_back(token);
    }

    delete response;
    return DataStatus::Success;
  }

  DataStatus SRM22Client::getRequestTokens(std::list<std::string>& tokens,
                                           const std::string& description) {
    PayloadSOAP request(ns);
    XMLNode req = request.NewChild("SRMv2:srmGetRequestTokens")
                  .NewChild("srmGetRequestTokensRequest");
    if (!description.empty())
      req.NewChild("userRequestDescription") = description;

    PayloadSOAP *response = NULL;
    DataStatus status = process("", &request, &response);
    if (!status) return status;

    XMLNode res = (*response)["srmGetRequestTokensResponse"]
                  ["srmGetRequestTokensResponse"];

    std::string explanation;
    SRMStatusCode statuscode = GetStatus(res["returnStatus"], explanation);

    if (statuscode == SRM_INVALID_REQUEST) {
      logger.msg(VERBOSE, "No request tokens found");
      delete response;
      return DataStatus::Success;
    }

    if (statuscode != SRM_SUCCESS) {
      logger.msg(VERBOSE, "%s", explanation);
      delete response;
      return DataStatus(DataStatus::GenericError, srm2errno(statuscode), explanation);
    }

    for (XMLNode n = res["arrayOfRequestTokens"]["tokenArray"]; n; ++n) {
      std::string token = (std::string)n["requestToken"];
      logger.msg(VERBOSE, "Adding request token %s", token);
      tokens.push_back(token);
    }

    delete response;
    return DataStatus::Success;
  }

  DataStatus SRM22Client::getTURLs(SRMClientRequest& creq,
                                   std::list<std::string>& urls) {

    // only one file requested at a time
    PayloadSOAP request(ns);
    XMLNode req = request.NewChild("SRMv2:srmPrepareToGet")
                  .NewChild("srmPrepareToGetRequest");
    req.NewChild("arrayOfFileRequests").NewChild("requestArray")
      .NewChild("sourceSURL") = creq.surl();
    XMLNode protocols = req.NewChild("transferParameters")
                        .NewChild("arrayOfTransferProtocols");
    std::list<std::string> transport_protocols(creq.transport_protocols());
    for (std::list<std::string>::iterator prot = transport_protocols.begin();
         prot != transport_protocols.end(); ++prot) {
      protocols.NewChild("stringArray") = *prot;
    }

    PayloadSOAP *response = NULL;
    DataStatus status = process("", &request, &response);
    if (!status) {
      creq.finished_error();
      return status;
    }

    XMLNode res = (*response)["srmPrepareToGetResponse"]
                  ["srmPrepareToGetResponse"];

    std::string explanation;
    SRMStatusCode statuscode = GetStatus(res["returnStatus"], explanation);

    // store the request token in the request object
    if (res["requestToken"]) creq.request_token(res["requestToken"]);

    if (statuscode == SRM_REQUEST_QUEUED ||
        statuscode == SRM_REQUEST_INPROGRESS) {
      // file is queued - if asynchronous need to wait and query with returned request token
      unsigned int sleeptime = 1;
      if (res["arrayOfFileStatuses"]["statusArray"]["estimatedWaitTime"]) {
        sleeptime = stringtoi(res["arrayOfFileStatuses"]["statusArray"]
                              ["estimatedWaitTime"]);
      }
      if (creq.request_timeout() == 0) {
        creq.wait(sleeptime);
        delete response;
        return DataStatus::Success;
      }
      unsigned int request_time = 0;
      while (request_time < creq.request_timeout()) {
  
        // sleep for recommended time (within limits)
        sleeptime = (sleeptime < 1) ? 1 : sleeptime;
        sleeptime = (sleeptime > creq.request_timeout() - request_time) ?
                    creq.request_timeout() - request_time : sleeptime;
        logger.msg(VERBOSE, "%s: File request %s in SRM queue. Sleeping for %i seconds",
                   creq.surl(), creq.request_token(), sleeptime);
        sleep(sleeptime);
        request_time += sleeptime;
  
        // get status of request
        DataStatus status_res = getTURLsStatus(creq, urls);
        if (creq.status() != SRM_REQUEST_ONGOING) {
          delete response;
          return status_res;
        }

        sleeptime = creq.waiting_time();
      }

      // if we get here it means a timeout occurred
      std::string err_msg("PrepareToGet request timed out after " + tostring(creq.request_timeout()) + " seconds");
      logger.msg(VERBOSE, err_msg);
      creq.finished_abort();
      delete response;
      return DataStatus(DataStatus::ReadPrepareError, EARCREQUESTTIMEOUT, err_msg);
  
    } // if file queued
  
    else if (statuscode != SRM_SUCCESS) {
      // any other return code is a failure
      std::string file_explanation;
      SRMStatusCode file_statuscode = GetStatus(res["arrayOfFileStatuses"]["statusArray"]["status"], file_explanation);
      if (explanation.empty()) explanation = file_explanation;
      else if (!file_explanation.empty()) explanation += ": " + file_explanation;
      logger.msg(VERBOSE, explanation);
      creq.finished_error();
      delete response;
      return DataStatus(DataStatus::ReadPrepareError, srm2errno(statuscode, file_statuscode), explanation);
    }

    // the file is ready and pinned - we can get the TURL
    std::string turl = (std::string)res["arrayOfFileStatuses"]["statusArray"]["transferURL"];
    logger.msg(VERBOSE, "File is ready! TURL is %s", turl);

    urls.push_back(turl);
    creq.finished_success();
    delete response;
    return DataStatus::Success;
  }
  
  DataStatus SRM22Client::getTURLsStatus(SRMClientRequest& creq,
                                         std::list<std::string>& urls) {

    PayloadSOAP request(ns);
    XMLNode req = request.NewChild("SRMv2:srmStatusOfGetRequest")
                  .NewChild("srmStatusOfGetRequestRequest");
    req.NewChild("requestToken") = creq.request_token();

    PayloadSOAP *response = NULL;
    DataStatus status = process("", &request, &response);
    if (!status) {
      creq.finished_abort();
      return status;
    }

    XMLNode res = (*response)["srmStatusOfGetRequestResponse"]
          ["srmStatusOfGetRequestResponse"];

    std::string explanation;
    SRMStatusCode statuscode = GetStatus(res["returnStatus"], explanation);

    if (statuscode == SRM_REQUEST_QUEUED ||
        statuscode == SRM_REQUEST_INPROGRESS) {
      // still queued - keep waiting
      int sleeptime = 1;
      if (res["arrayOfFileStatuses"]["statusArray"]["estimatedWaitTime"]) {
        sleeptime = stringtoi(res["arrayOfFileStatuses"]["statusArray"]["estimatedWaitTime"]);
      }
      creq.wait(sleeptime);
    }
    else if (statuscode != SRM_SUCCESS) {
      // error
      std::string file_explanation;
      SRMStatusCode file_statuscode = GetStatus(res["arrayOfFileStatuses"]["statusArray"]["status"], file_explanation);
      if (explanation.empty()) explanation = file_explanation;
      else if (!file_explanation.empty()) explanation += ": " + file_explanation;
      logger.msg(VERBOSE, explanation);
      creq.finished_error();
      delete response;
      return DataStatus(DataStatus::ReadPrepareError, srm2errno(statuscode, file_statuscode), explanation);
    }
    else {
      // success, TURL is ready
      std::string turl = (std::string)res["arrayOfFileStatuses"]["statusArray"]["transferURL"];
      logger.msg(VERBOSE, "File is ready! TURL is %s", turl);
      urls.push_back(std::string(turl));
      creq.finished_success();
    }
    delete response;
    return DataStatus::Success;
  }

  DataStatus SRM22Client::requestBringOnline(SRMClientRequest& creq) {
    PayloadSOAP request(ns);
    XMLNode req = request.NewChild("SRMv2:srmBringOnline")
                  .NewChild("srmBringOnlineRequest");
    std::list<std::string> surls = creq.surls();
    for (std::list<std::string>::iterator it = surls.begin();
         it != surls.end(); ++it) {
      req.NewChild("arrayOfFileRequests").NewChild("requestArray")
      .NewChild("sourceSURL") = *it;
    }

    // should not be needed but dcache returns NullPointerException if
    // it is not given
    XMLNode protocols = req.NewChild("transferParameters")
                        .NewChild("arrayOfTransferProtocols");
    protocols.NewChild("stringArray") = "gsiftp";
    protocols.NewChild("stringArray") = "https";
    protocols.NewChild("stringArray") = "httpg";
    protocols.NewChild("stringArray") = "http";
    protocols.NewChild("stringArray") = "ftp";
    // store the user id as part of the request, so they can find it later

    std::string user = User().Name();
    if (!user.empty()) {
      logger.msg(VERBOSE, "Setting userRequestDescription to %s", user);
      req.NewChild("userRequestDescription") = user;
    }

    PayloadSOAP *response = NULL;
    DataStatus status = process("", &request, &response);
    if (!status) {
      creq.finished_error();
      return status;
    }

    XMLNode res = (*response)["srmBringOnlineResponse"]
                  ["srmBringOnlineResponse"];

    std::string explanation;
    SRMStatusCode statuscode = GetStatus(res["returnStatus"], explanation);

    // store the request token in the request object
    if (res["requestToken"]) creq.request_token(res["requestToken"]);

    if (statuscode == SRM_SUCCESS) {
      // this means files are all already online
      for (std::list<std::string>::iterator it = surls.begin();
           it != surls.end(); ++it) {
        creq.surl_statuses(*it, SRM_ONLINE);
      }
      creq.finished_success();
      delete response;
      return DataStatus::Success;
    }

    if (statuscode == SRM_REQUEST_QUEUED ||
        statuscode == SRM_REQUEST_INPROGRESS) {
      unsigned int sleeptime = 10;

      if (statuscode == SRM_REQUEST_INPROGRESS && res["arrayOfFileStatuses"]) {
        // some files have been queued and some are online. Check each file
        fileStatus(creq, res["arrayOfFileStatuses"]);
      }
      if (res["arrayOfFileStatuses"]["statusArray"]["estimatedWaitTime"]) {
        sleeptime = stringtoi(res["arrayOfFileStatuses"]["statusArray"]["estimatedWaitTime"]);
      }

      if (creq.request_timeout() == 0) {
        creq.wait(sleeptime);
        delete response;
        return DataStatus::Success;
      }

      unsigned int request_time = 0;
      while (request_time < creq.request_timeout()) {
        // sleep for recommended time (within limits)
        sleeptime = (sleeptime < 1) ? 1 : sleeptime;
        sleeptime = (sleeptime > creq.request_timeout() - request_time) ?
                    creq.request_timeout() - request_time : sleeptime;
        logger.msg(VERBOSE, "%s: Bring online request %s in SRM queue. Sleeping for %i seconds",
                   creq.surl(), creq.request_token(), sleeptime);
        sleep(sleeptime);
        request_time += sleeptime;

        // get status of request
        DataStatus status_res = requestBringOnlineStatus(creq);
        if (creq.status() != SRM_REQUEST_ONGOING) {
          delete response;
          return status_res;
        }
        sleeptime = creq.waiting_time();
      }

      // if we get here it means a timeout occurred
      std::string err_msg("Bring online request timed out after " + tostring(creq.request_timeout()) + " seconds");
      logger.msg(VERBOSE, err_msg);
      creq.finished_abort();
      delete response;
      return DataStatus(DataStatus::ReadPrepareError, EARCREQUESTTIMEOUT, err_msg);
    }

    if (statuscode == SRM_PARTIAL_SUCCESS) {
      // some files are already online, some failed. check each file
      fileStatus(creq, res["arrayOfFileStatuses"]);
      creq.finished_partial_success();
      delete response;
      return DataStatus::Success;
    }

    // here means an error code was returned and all files failed
    std::string file_explanation;
    SRMStatusCode file_statuscode = GetStatus(res["arrayOfFileStatuses"]["statusArray"]["status"], file_explanation);
    if (explanation.empty()) explanation = file_explanation;
    else if (!file_explanation.empty()) explanation += ": " + file_explanation;
    logger.msg(VERBOSE, explanation);
    creq.finished_error();
    delete response;
    return DataStatus(DataStatus::ReadPrepareError, srm2errno(statuscode, file_statuscode), explanation);
  }

  DataStatus SRM22Client::requestBringOnlineStatus(SRMClientRequest& creq) {
    if (creq.request_token().empty()) {
      logger.msg(VERBOSE, "No request token specified!");
      creq.finished_abort();
      return DataStatus(DataStatus::ReadPrepareError, EINVAL, "No request token specified");
    }

    PayloadSOAP request(ns);
    XMLNode req = request.NewChild("SRMv2:srmStatusOfBringOnlineRequest")
                  .NewChild("srmStatusOfBringOnlineRequestRequest");
    req.NewChild("requestToken") = creq.request_token();

    PayloadSOAP *response = NULL;
    DataStatus status = process("", &request, &response);
    if (!status) {
      creq.finished_abort();
      return status;
    }

    XMLNode res = (*response)["srmStatusOfBringOnlineRequestResponse"]
                  ["srmStatusOfBringOnlineRequestResponse"];

    std::string explanation;
    SRMStatusCode statuscode = GetStatus(res["returnStatus"], explanation);

    if (statuscode == SRM_SUCCESS) {
      // this means files are all online
      fileStatus(creq, res["arrayOfFileStatuses"]);
      creq.finished_success();
      delete response;
      return DataStatus::Success;
    }

    if (statuscode == SRM_REQUEST_QUEUED) {
      // all files are in the queue - leave statuses as they are
      int sleeptime = 1;
      if (res["arrayOfFileStatuses"]["statusArray"]["estimatedWaitTime"]) {
        sleeptime = stringtoi(res["arrayOfFileStatuses"]["statusArray"]["estimatedWaitTime"]);
      }
      creq.wait(sleeptime);
      delete response;
      return DataStatus::Success;
    }

    if (statuscode == SRM_REQUEST_INPROGRESS) {
      // some files have been queued and some are online. check each file
      fileStatus(creq, res["arrayOfFileStatuses"]);
      int sleeptime = 1;
      if (res["arrayOfFileStatuses"]["statusArray"]["estimatedWaitTime"]) {
        sleeptime = stringtoi(res["arrayOfFileStatuses"]["statusArray"]["estimatedWaitTime"]);
      }
      creq.wait(sleeptime);
      delete response;
      return DataStatus::Success;
    }

    if (statuscode == SRM_PARTIAL_SUCCESS) {
      // some files are online, some failed. check each file
      fileStatus(creq, res["arrayOfFileStatuses"]);
      creq.finished_partial_success();
      delete response;
      return DataStatus::Success;
    }

    if (statuscode == SRM_ABORTED) {
      // The request was aborted or finished successfully. dCache reports
      // SRM_ABORTED after the first time a successful request is queried
      // so we have to look at the explanation string for the real reason.
      if (explanation.find("All files are done") != std::string::npos) {
        logger.msg(VERBOSE, "Request is reported as ABORTED, but all files are done");
        creq.finished_success();
        delete response;
        return DataStatus::Success;
      }
      else if (explanation.find("Canceled") != std::string::npos) {
        logger.msg(VERBOSE, "Request is reported as ABORTED, since it was cancelled");
        creq.cancelled();
        delete response;
        return DataStatus::Success;
      }
      else {
        logger.msg(VERBOSE, "Request is reported as ABORTED. Reason: %s", explanation);
        creq.finished_error();
        delete response;
        return DataStatus(DataStatus::ReadPrepareError, srm2errno(statuscode), explanation);
      }
    }

    // here means an error code was returned and all files failed
    fileStatus(creq, res["arrayOfFileStatuses"]);
    std::string file_explanation;
    SRMStatusCode file_statuscode = GetStatus(res["arrayOfFileStatuses"]["statusArray"]["status"], file_explanation);
    if (explanation.empty()) explanation = file_explanation;
    else if (!file_explanation.empty()) explanation += ": " + file_explanation;
    logger.msg(VERBOSE, explanation);
    creq.finished_error();
    delete response;
    return DataStatus(DataStatus::ReadPrepareError, srm2errno(statuscode, file_statuscode), explanation);
  }

  void SRM22Client::fileStatus(SRMClientRequest& creq, XMLNode file_statuses) {
    int waittime = 0;

    for (XMLNode n = file_statuses["statusArray"]; n; ++n) {
      std::string surl = (std::string)n["sourceSURL"];
      // store the largest estimated waiting time
      if (n["estimatedWaitTime"]) {
        int estimatedWaitTime = stringtoi(n["estimatedWaitTime"]);
        if (estimatedWaitTime > waittime) waittime = estimatedWaitTime;
      }

      std::string explanation;
      SRMStatusCode filestatus = GetStatus(n["status"], explanation);

      if (filestatus == SRM_SUCCESS || filestatus == SRM_FILE_IN_CACHE) {
        creq.surl_statuses(surl, SRM_ONLINE);
      } else if (filestatus == SRM_REQUEST_QUEUED || filestatus == SRM_REQUEST_INPROGRESS) {
        creq.surl_statuses(surl, SRM_NEARLINE);
      } else {
        creq.surl_statuses(surl, SRM_STAGE_ERROR);
        creq.surl_failures(surl, explanation);
      }
    }

    creq.waiting_time(waittime);
  }

  DataStatus SRM22Client::putTURLs(SRMClientRequest& creq,
                                   std::list<std::string>& urls) {
    // only one file requested at a time
    PayloadSOAP request(ns);
    XMLNode req = request.NewChild("SRMv2:srmPrepareToPut")
                  .NewChild("srmPrepareToPutRequest");
    XMLNode reqarray = req.NewChild("arrayOfFileRequests")
                       .NewChild("requestArray");
    reqarray.NewChild("targetSURL") = creq.surl();
    reqarray.NewChild("expectedFileSize") = tostring(creq.total_size());
    req.NewChild("desiredFileStorageType") = "PERMANENT";
    XMLNode protocols = req.NewChild("transferParameters")
                        .NewChild("arrayOfTransferProtocols");

    std::list<std::string> transport_protocols(creq.transport_protocols());
    for (std::list<std::string>::iterator prot = transport_protocols.begin();
         prot != transport_protocols.end(); ++prot) {
      protocols.NewChild("stringArray") = *prot;
    }

    // set space token if supplied
    if (!creq.space_token().empty()) req.NewChild("targetSpaceToken") = creq.space_token();

    PayloadSOAP *response = NULL;
    DataStatus status = process("", &request, &response);
    if (!status) {
      creq.finished_error();
      return status;
    }

    XMLNode res = (*response)["srmPrepareToPutResponse"]
                  ["srmPrepareToPutResponse"];

    std::string explanation;
    SRMStatusCode statuscode = GetStatus(res["returnStatus"], explanation);

    // store the request token in the request object
    if (res["requestToken"]) creq.request_token(res["requestToken"]);

    if (statuscode == SRM_REQUEST_QUEUED ||
        statuscode == SRM_REQUEST_INPROGRESS) {
      // file is queued - need to wait and query with returned request token
      unsigned int sleeptime = 1;
      if (res["arrayOfFileStatuses"]["statusArray"]["estimatedWaitTime"]) {
        sleeptime = stringtoi(res["arrayOfFileStatuses"]["statusArray"]["estimatedWaitTime"]);
      }
      unsigned int request_time = 0;

      if (creq.request_timeout() == 0) {
        creq.wait(sleeptime);
        delete response;
        return DataStatus::Success;
      };

      while (request_time < creq.request_timeout()) {

        // sleep for recommended time (within limits)
        sleeptime = (sleeptime < 1) ? 1 : sleeptime;
        sleeptime = (sleeptime > creq.request_timeout() - request_time) ?
                    creq.request_timeout() - request_time : sleeptime;
        logger.msg(VERBOSE, "%s: File request %s in SRM queue. Sleeping for %i seconds",
                   creq.surl(), creq.request_token(), sleeptime);

        sleep(sleeptime);
        request_time += sleeptime;

        // get status of request
        DataStatus status_res = putTURLsStatus(creq, urls);
        if (creq.status() != SRM_REQUEST_ONGOING) {
          delete response;
          return status_res;
        }
        sleeptime = creq.waiting_time();
      }

      // if we get here it means a timeout occurred
      std::string err_msg("PrepareToPut request timed out after " + tostring(creq.request_timeout()) + " seconds");
      logger.msg(VERBOSE, err_msg);
      creq.finished_abort();
      delete response;
      return DataStatus(DataStatus::WritePrepareError, EARCREQUESTTIMEOUT, err_msg);

    } // if file queued
  
    else if (statuscode != SRM_SUCCESS) {
      std::string file_explanation;
      SRMStatusCode file_status = GetStatus(res["arrayOfFileStatuses"]
                                            ["statusArray"]["status"],
                                            file_explanation);
      if (file_status == SRM_INVALID_PATH) {
        // make directories
        logger.msg(VERBOSE, "Path %s is invalid, creating required directories", creq.surl());
        DataStatus mkdirres = mkDir(creq);
        delete response;
        if (mkdirres.Passed()) return putTURLs(creq, urls);

        logger.msg(VERBOSE, "Error creating required directories for %s", creq.surl());
        creq.finished_error();
        return mkdirres;
      }

      if (explanation.empty()) explanation = file_explanation;
      else if (!file_explanation.empty()) explanation += ": " + file_explanation;
      logger.msg(VERBOSE, explanation);

      if (file_status == SRM_FILE_BUSY) {
        // TODO: a previous upload failed to be aborted so kill it
      }

      creq.finished_error();
      delete response;
      return DataStatus(DataStatus::WritePrepareError, srm2errno(statuscode, file_status), explanation);
    }

    // the file is ready and pinned - we can get the TURL
    std::string turl = (std::string)res["arrayOfFileStatuses"]["statusArray"]["transferURL"];
    logger.msg(VERBOSE, "File is ready! TURL is %s", turl);

    urls.push_back(turl);
    creq.finished_success();
    delete response;
    return DataStatus::Success;
  }
  
  DataStatus SRM22Client::putTURLsStatus(SRMClientRequest& creq,
                                         std::list<std::string>& urls) {

    PayloadSOAP request(ns);
    XMLNode req = request.NewChild("SRMv2:srmStatusOfPutRequest")
                  .NewChild("srmStatusOfPutRequestRequest");
    req.NewChild("requestToken") = creq.request_token();

    PayloadSOAP *response = NULL;
    DataStatus status = process("", &request, &response);
    if (!status) {
      creq.finished_abort();
      return status;
    }

    XMLNode res = (*response)["srmStatusOfPutRequestResponse"]
          ["srmStatusOfPutRequestResponse"];

    std::string explanation;
    SRMStatusCode statuscode = GetStatus(res["returnStatus"], explanation);

    if (statuscode == SRM_REQUEST_QUEUED ||
        statuscode == SRM_REQUEST_INPROGRESS) {
      // still queued - keep waiting
      int sleeptime = 1;
      if (res["arrayOfFileStatuses"]["statusArray"]["estimatedWaitTime"]) {
        sleeptime = stringtoi(res["arrayOfFileStatuses"]["statusArray"]["estimatedWaitTime"]);
      }
      creq.wait(sleeptime);
    }
    else if (statuscode != SRM_SUCCESS) {
      // error
      // check individual file statuses
      std::string file_explanation;
      SRMStatusCode file_status = GetStatus(res["arrayOfFileStatuses"]
                                            ["statusArray"]["status"],
                                            file_explanation);
      if (file_status == SRM_INVALID_PATH) {
        // make directories
        logger.msg(VERBOSE, "Path %s is invalid, creating required directories", creq.surl());
        DataStatus mkdirres = mkDir(creq);
        delete response;
        if (mkdirres.Passed()) return putTURLs(creq, urls);

        logger.msg(VERBOSE, "Error creating required directories for %s", creq.surl());
        creq.finished_error();
        return mkdirres;
      }
      if (explanation.empty()) explanation = file_explanation;
      else if (!file_explanation.empty()) explanation += ": " + file_explanation;
      logger.msg(VERBOSE, explanation);
      creq.finished_error();
      delete response;
      return DataStatus(DataStatus::WritePrepareError, srm2errno(statuscode, file_status), explanation);
    }
    else {
      // success, TURL is ready
      std::string turl = (std::string)res["arrayOfFileStatuses"]["statusArray"]["transferURL"];
      logger.msg(VERBOSE, "File is ready! TURL is %s", turl);
      urls.push_back(std::string(turl));

      creq.finished_success();
    }
    delete response;
    return DataStatus::Success;
  }

  DataStatus SRM22Client::info(SRMClientRequest& req,
                               std::list<struct SRMFileMetaData>& metadata) {

    std::map<std::string, std::list<struct SRMFileMetaData> > metadata_map;
    DataStatus res = info(req, metadata_map);
    if (!res || metadata_map.find(req.surl()) == metadata_map.end()) return res;
    metadata = metadata_map[req.surl()];
    return DataStatus::Success;
  }

  DataStatus SRM22Client::info(SRMClientRequest& creq,
                               std::map<std::string, std::list<struct SRMFileMetaData> >& metadata) {

    PayloadSOAP request(ns);
    XMLNode req = request.NewChild("SRMv2:srmLs").NewChild("srmLsRequest");
    XMLNode surl_array = req.NewChild("arrayOfSURLs");

    std::list<std::string> surls = creq.surls();
    for (std::list<std::string>::const_iterator surl = surls.begin(); surl != surls.end(); ++surl) {
      surl_array.NewChild("urlArray") = *surl;
    }
    // 0 corresponds to list the directory entry not the files in it
    // 1 corresponds to list the files in a directory - this is the desired
    // behaviour of arcls with no recursion, so we add 1 to the -r value
    req.NewChild("numOfLevels") = tostring(creq.recursion() + 1);
    // add count and offset options, if set
    if (creq.offset() != 0) req.NewChild("offset") = tostring(creq.offset());
    if (creq.count() != 0) req.NewChild("count") = tostring(creq.count());
    if (creq.long_list()) req.NewChild("fullDetailedList") = "true";

    PayloadSOAP *response = NULL;
    DataStatus status = process("", &request, &response);
    if (!status) return status;

    XMLNode res = (*response)["srmLsResponse"]["srmLsResponse"];

    std::string explanation;
    SRMStatusCode statuscode = GetStatus(res["returnStatus"], explanation);

    // store the request token in the request object
    if (res["requestToken"]) creq.request_token(res["requestToken"]);

    if (statuscode == SRM_SUCCESS ||
        statuscode == SRM_TOO_MANY_RESULTS ||
        statuscode == SRM_PARTIAL_SUCCESS) {
      // request is finished - we can get all the details
    }
    else if (statuscode == SRM_REQUEST_QUEUED ||
             statuscode == SRM_REQUEST_INPROGRESS) {
      // file is queued - need to wait and query with returned request token
      int sleeptime = 1;
      unsigned int request_time = 0;

      while (statuscode != SRM_SUCCESS && statuscode != SRM_PARTIAL_SUCCESS && request_time < creq.request_timeout()) {
        // sleep for some time (no estimated time is given by the server)
        logger.msg(VERBOSE, "%s: File request %s in SRM queue. Sleeping for %i seconds",
                   creq.surl(), creq.request_token(), sleeptime);
        sleep(sleeptime);
        request_time += sleeptime;

        PayloadSOAP request(ns);
        XMLNode req = request.NewChild("SRMv2:srmStatusOfLsRequest")
                      .NewChild("srmStatusOfLsRequestRequest");
        req.NewChild("requestToken") = creq.request_token();

        delete response;
        response = NULL;
        status = process("", &request, &response);
        if (!status) return status;

        res = (*response)["srmStatusOfLsRequestResponse"]
              ["srmStatusOfLsRequestResponse"];

        statuscode = GetStatus(res["returnStatus"], explanation);

        if (statuscode == SRM_TOO_MANY_RESULTS) {
          // we can only handle too many results if a single directory was listed
          if (surls.size() > 1) {
            logger.msg(VERBOSE, "Too many files in one request - please try again with fewer files");
            return DataStatus(DataStatus::ListError, EARCRESINVAL, "Too many files in one request");
          }
          break;
        }

        // loop will exit on success or return false on error
        if (statuscode != SRM_SUCCESS &&
            statuscode != SRM_PARTIAL_SUCCESS &&
            statuscode != SRM_REQUEST_QUEUED &&
            statuscode != SRM_REQUEST_INPROGRESS) {

          // check if individual file status gives more info
          std::string file_explanation;
          SRMStatusCode file_statuscode = GetStatus(res["details"]["pathDetailArray"]["status"], file_explanation);
          if (explanation.empty()) explanation = file_explanation;
          else if (!file_explanation.empty()) explanation += ": " + file_explanation;
          logger.msg(VERBOSE, explanation);
          delete response;
          return DataStatus(DataStatus::ListError, srm2errno(statuscode, file_statuscode), explanation);
        }
      }

      // check for timeout
      if (request_time >= creq.request_timeout()) {
        std::string err_msg("Ls request timed out after " + tostring(creq.request_timeout()) + " seconds");
        logger.msg(VERBOSE, err_msg);
        abort(creq, true);
        delete response;
        return DataStatus(DataStatus::ListError, EARCREQUESTTIMEOUT, err_msg);
      }
    }
    else {
      // check if individual file status gives more info
      std::string file_explanation;
      SRMStatusCode file_statuscode = GetStatus(res["details"]["pathDetailArray"]["status"], file_explanation);
      if (explanation.empty()) explanation = file_explanation;
      else if (!file_explanation.empty()) explanation += ": " + file_explanation;
      logger.msg(VERBOSE, explanation);
      delete response;
      return DataStatus(DataStatus::ListError, srm2errno(statuscode, file_statuscode), explanation);
    }

    // the request is ready - collect the details
    // assuming the result is in the same order as the surls in the request
    std::list<std::string>::const_iterator surl = surls.begin();

    if (statuscode == SRM_TOO_MANY_RESULTS) {
      logger.msg(INFO, "Directory size is too large to list in one call, will"
                 " have to call multiple times");
      delete response;
      std::list<struct SRMFileMetaData> md;
      std::list<struct SRMFileMetaData> list_metadata;

      for (int list_no = 0; list_no == 0 || list_metadata.size() == max_files_list; ++list_no) {
        list_metadata.clear();
        // set up new request with offset and count set
        SRMClientRequest list_req(creq.surl());
        list_req.long_list(creq.long_list());
        list_req.offset(max_files_list * list_no);
        list_req.count(max_files_list);

        DataStatus infores = info(list_req, list_metadata);
        if (!infores) return infores;

        // append to metadata
        for (std::list<SRMFileMetaData>::iterator it = list_metadata.begin();
             it != list_metadata.end(); ++it) {
          md.push_back(*it);
        }
      }
      // add to the map
      metadata[*surl] = md;
      return DataStatus::Success;
    }

    for (XMLNode details = res["details"]["pathDetailArray"]; details; ++details, ++surl) {

      // With Storm and offset and count set files are reported at this level
      // rather than in arrayOfSubPaths, so check whether we are really listing
      // a large number of surls
      if (surls.size() < creq.offset() && surl != surls.begin()) --surl;

      if (surl == surls.end()) {
        // Something bad happened with the parsing
        logger.msg(WARNING, "Failure in parsing response from server - "
                   "some information may be inaccurate");
        break;
      }
      SRMStatusCode filestatuscode = GetStatus(details["status"], explanation);
      if (filestatuscode != SRM_SUCCESS && filestatuscode != SRM_FILE_BUSY) {
        logger.msg(VERBOSE, "%s: %s", *surl, explanation);
        continue;
      }
      std::list<struct SRMFileMetaData> md;

      // if we think this entry is a file or we don't want recursion, add it
      if (!details["type"] || details["type"] != "DIRECTORY" || creq.recursion() < 0) {
        // it can happen that with multiple calls to info() for large dirs the
        // last call returns one directory. In this case we want to list it
        // without the directory structure.
        if (creq.count() == 0) md.push_back(fillDetails(details, false));
        else md.push_back(fillDetails(details, true));
      }

      // look for sub paths (files in a directory)
      XMLNode subpath = details["arrayOfSubPaths"]["pathDetailArray"];

      // if no subpaths or we are not doing recursion, go to next surl
      if (creq.recursion() < 0 || !subpath) {
        metadata[*surl] = md;
        continue;
      }

      // sometimes we don't know if we have a file or dir so take out the
      // entry added above if offset is 0
      if (creq.offset() == 0) md.clear();

      for (unsigned int i = 0; subpath; ++subpath, ++i) {
        // some older implementations would return a truncated list rather
        // than SRM_TOO_MANY_RESULTS. So to be safe, if there are more entries
        // than max_files_list we call info() multiple times, setting offset
        // and count. TODO: investigate if this is still necessary
        if (i == max_files_list) {
          logger.msg(INFO, "Directory size is larger than %i files, will have "
                     "to call multiple times", max_files_list);
          std::list<SRMFileMetaData> list_metadata;
          // if too many results return code, start from 0 again
          int list_no = 1;
          do {
            list_metadata.clear();
            SRMClientRequest list_req(creq.surl());
            list_req.long_list(creq.long_list());
            list_req.offset(max_files_list * list_no);
            list_req.count(max_files_list);
            list_req.recursion(creq.recursion());
            DataStatus res = info(list_req, list_metadata);
            if (!res) {
              delete response;
              return res;
            }
            list_no++;
            // append to metadata
            for (std::list<SRMFileMetaData>::iterator it = list_metadata.begin();
                 it != list_metadata.end(); ++it) {
              md.push_back(*it);
            }
          } while (list_metadata.size() == max_files_list);
          break;
        }
        md.push_back(fillDetails(subpath, true));
      }
      // add to the map
      metadata[*surl] = md;
    }
    delete response;
    return DataStatus::Success;
  }

  SRMFileMetaData SRM22Client::fillDetails(XMLNode details, bool directory) {
    SRMFileMetaData metadata;
    if (details["path"]) {
      metadata.path = (std::string)details["path"];
      std::string::size_type i = metadata.path.find("//");
      while (i != std::string::npos) {
        metadata.path.erase(i, 1);
        i = metadata.path.find("//", i);
      }
      if (metadata.path.find("/") != 0) metadata.path = "/" + metadata.path;
      // only use the basename of the path if directory
      if (directory) metadata.path = metadata.path.substr(metadata.path.rfind("/") + 1);
    }

    if (details["size"] && !((std::string)details["size"]).empty()) {
      metadata.size = stringtoull(details["size"]);
    } else {
      metadata.size = -1;
    }

    if (details["checkSumType"]) {
      metadata.checkSumType = lower((std::string)details["checkSumType"]);
    } else {
      metadata.checkSumType = "";
    }

    if (details["checkSumValue"]) {
      metadata.checkSumValue = lower((std::string)details["checkSumValue"]);
    } else {
      metadata.checkSumValue = "";
    }

    if (details["createdAtTime"]) {
      std::string created = (std::string)details["createdAtTime"];
      if (!created.empty()) metadata.createdAtTime = created;
      else metadata.createdAtTime = (time_t)0;
    } else {
      metadata.createdAtTime = (time_t)0;
    }

    if (details["type"]) {
      std::string filetype = (std::string)details["type"];
      if (filetype == "FILE") metadata.fileType = SRM_FILE;
      else if (filetype == "DIRECTORY") metadata.fileType = SRM_DIRECTORY;
      else if (filetype == "LINK") metadata.fileType = SRM_LINK;
    } else {
      metadata.fileType = SRM_FILE_TYPE_UNKNOWN;
    }

    if (details["fileLocality"]) {
      std::string filelocality = (std::string)details["fileLocality"];
      if (filelocality == "ONLINE" ||
          filelocality == "ONLINE_AND_NEARLINE")  metadata.fileLocality = SRM_ONLINE;
      else if (filelocality == "NEARLINE") metadata.fileLocality = SRM_NEARLINE;
    } else {
      metadata.fileLocality = SRM_UNKNOWN;
    }

    if (details["arrayOfSpaceTokens"]) {
      for (XMLNode n = details["arrayOfSpaceTokens"]["stringArray"]; n; ++n) {
        metadata.spaceTokens.push_back((std::string)n);
      }
    }

    if (details["ownerPermission"] && details["groupPermission"] && details["otherPermission"]) {
      if (details["ownerPermission"]["userID"]) {
        metadata.owner = (std::string)details["ownerPermission"]["userID"];
      }
      if (details["groupPermission"]["groupID"]) {
        metadata.group = (std::string)details["groupPermission"]["groupID"];
      }
      if (details["ownerPermission"]["mode"] &&
          details["groupPermission"]["mode"] &&
          details["otherPermission"]) {
        std::string perms;
        std::string uperm = (std::string)details["ownerPermission"]["mode"];
        std::string gperm = (std::string)details["groupPermission"]["mode"];
        std::string operm = (std::string)details["otherPermission"];
        if (uperm.find('R') != std::string::npos) perms += 'r';
        else perms += '-';
        if (uperm.find('W') != std::string::npos) perms += 'w';
        else perms += '-';
        if (uperm.find('X') != std::string::npos) perms += 'x';
        else perms += '-';
        if (gperm.find('R') != std::string::npos) perms += 'r';
        else perms += '-';
        if (gperm.find('W') != std::string::npos) perms += 'w';
        else perms += '-';
        if (gperm.find('X') != std::string::npos) perms += 'x';
        else perms += '-';
        if (operm.find('R') != std::string::npos) perms += 'r';
        else perms += '-';
        if (operm.find('W') != std::string::npos) perms += 'w';
        else perms += '-';
        if (operm.find('X') != std::string::npos) perms += 'x';
        else perms += '-';
        metadata.permission = perms;
      }
    }

    if (details["lastModificationTime"]) {
      std::string modified = (std::string)details["lastModificationTime"];
      if (!modified.empty()) metadata.lastModificationTime = modified;
      else metadata.lastModificationTime = (time_t)0;
    } else {
      metadata.lastModificationTime = (time_t)0;
    }

    if (details["lifetimeAssigned"]) {
      std::string lifetimeassigned = (std::string)details["lifetimeAssigned"];
      if (!lifetimeassigned.empty()) metadata.lifetimeAssigned = lifetimeassigned;
      else metadata.lifetimeAssigned = 0;
    } else {
      metadata.lifetimeAssigned = 0;
    }

    if (details["lifetimeLeft"]) {
      std::string lifetimeleft = (std::string)details["lifetimeLeft"];
      if (!lifetimeleft.empty()) metadata.lifetimeLeft = lifetimeleft;
      else metadata.lifetimeLeft = 0;
    } else {
      metadata.lifetimeLeft = 0;
    }

    if (details["retentionPolicyInfo"]) {
      std::string policy = (std::string)details["retentionPolicyInfo"];
      if (policy == "REPLICA") metadata.retentionPolicy = SRM_REPLICA;
      else if (policy == "OUTPUT") metadata.retentionPolicy = SRM_OUTPUT;
      else if (policy == "CUSTODIAL") metadata.retentionPolicy = SRM_CUSTODIAL;
      else metadata.retentionPolicy = SRM_RETENTION_UNKNOWN;
    } else {
      metadata.retentionPolicy = SRM_RETENTION_UNKNOWN;
    }

    if (details["fileStorageType"]) {
      std::string type = (std::string)details["fileStorageType"];
      if (type == "VOLATILE") metadata.fileStorageType = SRM_VOLATILE;
      else if (type == "DURABLE") metadata.fileStorageType = SRM_DURABLE;
      else if (type == "PERMANENT") metadata.fileStorageType = SRM_PERMANENT;
      else metadata.fileStorageType = SRM_FILE_STORAGE_UNKNOWN;
    } else {
      // if any other value, leave undefined
      metadata.fileStorageType = SRM_FILE_STORAGE_UNKNOWN;
    }
    return metadata;
  }

  DataStatus SRM22Client::releaseGet(SRMClientRequest& creq) {
    // Release all pins referred to by the request token in the request object
    if (creq.request_token().empty()) {
      logger.msg(VERBOSE, "No request token specified!");
      return DataStatus(DataStatus::ReadPrepareError, EINVAL, "No request token specified");
    }

    PayloadSOAP request(ns);
    XMLNode req = request.NewChild("SRMv2:srmReleaseFiles")
                  .NewChild("srmReleaseFilesRequest");
    req.NewChild("requestToken") = creq.request_token();

    PayloadSOAP *response = NULL;
    DataStatus status = process("", &request, &response);
    if (!status) return status;

    XMLNode res = (*response)["srmReleaseFilesResponse"]
                  ["srmReleaseFilesResponse"];

    std::string explanation;
    SRMStatusCode statuscode = GetStatus(res["returnStatus"], explanation);

    if (statuscode != SRM_SUCCESS) {
      logger.msg(VERBOSE, explanation);
      delete response;
      return DataStatus(DataStatus::ReadPrepareError, srm2errno(statuscode), explanation);
    }

    logger.msg(VERBOSE, "Files associated with request token %s released successfully", creq.request_token());
    delete response;
    return DataStatus::Success;
  }

  DataStatus SRM22Client::releasePut(SRMClientRequest& creq) {
    // Set the files referred to by the request token in the request object
    // which were prepared to put to done
    if (creq.request_token().empty()) {
      logger.msg(VERBOSE, "No request token specified!");
      return DataStatus(DataStatus::WritePrepareError, EINVAL, "No request token specified");
    }

    PayloadSOAP request(ns);
    XMLNode req = request.NewChild("SRMv2:srmPutDone")
                  .NewChild("srmPutDoneRequest");
    req.NewChild("requestToken") = creq.request_token();
    req.NewChild("arrayOfSURLs").NewChild("urlArray") = creq.surl();

    PayloadSOAP *response = NULL;
    DataStatus status = process("", &request, &response);
    if (!status) return status;

    XMLNode res = (*response)["srmPutDoneResponse"]["srmPutDoneResponse"];

    std::string explanation;
    SRMStatusCode statuscode = GetStatus(res["returnStatus"], explanation);

    if (statuscode != SRM_SUCCESS) {
      logger.msg(VERBOSE, "%s", explanation);
      delete response;
      return DataStatus(DataStatus::WritePrepareError, srm2errno(statuscode), explanation);
    }

    logger.msg(VERBOSE, "Files associated with request token %s put done successfully", creq.request_token());
    delete response;
    return DataStatus::Success;
  }

  DataStatus SRM22Client::abort(SRMClientRequest& creq,
                                bool source) {
    // Call srmAbortRequest on the files in the request token
    if (creq.request_token().empty()) {
      logger.msg(VERBOSE, "No request token specified!");
      return DataStatus(source ? DataStatus::ReadFinishError : DataStatus::WriteFinishError,
                        EINVAL, "No request token specified");
    }

    PayloadSOAP request(ns);
    XMLNode req = request.NewChild("SRMv2:srmAbortRequest")
                  .NewChild("srmAbortRequestRequest");
    req.NewChild("requestToken") = creq.request_token();

    PayloadSOAP *response = NULL;
    DataStatus status = process("", &request, &response);
    if (!status) return status;

    XMLNode res = (*response)["srmAbortRequestResponse"]["srmAbortRequestResponse"];

    std::string explanation;
    SRMStatusCode statuscode = GetStatus(res["returnStatus"], explanation);

    if (statuscode != SRM_SUCCESS) {
      logger.msg(VERBOSE, "%s", explanation);
      delete response;
      return DataStatus(source ? DataStatus::ReadFinishError : DataStatus::WriteFinishError,
                        srm2errno(statuscode), explanation);
    }

    logger.msg(VERBOSE, "Files associated with request token %s aborted successfully", creq.request_token());
    delete response;
    return DataStatus::Success;
  }

  DataStatus SRM22Client::remove(SRMClientRequest& creq) {
    // TODO: bulk remove

    // call info() to find out if we are dealing with a file or directory
    SRMClientRequest inforeq(creq.surls());
    // set recursion to -1, meaning don't list entries in a dir
    inforeq.recursion(-1);

    std::list<struct SRMFileMetaData> metadata;
    DataStatus res = info(inforeq, metadata);
    if (!res) {
      logger.msg(VERBOSE,
                 "Failed to find metadata info on %s for determining file or directory delete",
                 inforeq.surl());
      return res;
    }

    if (metadata.front().fileType == SRM_FILE) {
      logger.msg(VERBOSE, "Type is file, calling srmRm");
      return removeFile(creq);
    }
    if (metadata.front().fileType == SRM_DIRECTORY) {
      logger.msg(VERBOSE, "Type is dir, calling srmRmDir");
      return removeDir(creq);
    }

    logger.msg(WARNING, "File type is not available, attempting file delete");
    if (removeFile(creq).Passed()) return DataStatus::Success;

    logger.msg(WARNING, "File delete failed, attempting directory delete");
    return removeDir(creq);
  }

  DataStatus SRM22Client::removeFile(SRMClientRequest& creq) {
    // only one file requested at a time
    PayloadSOAP request(ns);
    XMLNode req = request.NewChild("SRMv2:srmRm").NewChild("srmRmRequest");
    req.NewChild("arrayOfSURLs").NewChild("urlArray") = creq.surl();

    PayloadSOAP *response = NULL;
    DataStatus status = process("", &request, &response);
    if (!status) return status;

    XMLNode res = (*response)["srmRmResponse"]["srmRmResponse"];

    std::string explanation;
    SRMStatusCode statuscode = GetStatus(res["returnStatus"], explanation);

    if (statuscode != SRM_SUCCESS) {
      logger.msg(VERBOSE, explanation);
      delete response;
      return DataStatus(DataStatus::DeleteError, srm2errno(statuscode), explanation);
    }

    logger.msg(VERBOSE, "File %s removed successfully", creq.surl());
    delete response;
    return DataStatus::Success;
  }

  DataStatus SRM22Client::removeDir(SRMClientRequest& creq) {
    // only one file requested at a time
    PayloadSOAP request(ns);
    XMLNode req = request.NewChild("SRMv2:srmRmdir")
                  .NewChild("srmRmdirRequest");
    req.NewChild("SURL") = creq.surl();

    PayloadSOAP *response = NULL;
    DataStatus status = process("", &request, &response);
    if (!status) return status;

    XMLNode res = (*response)["srmRmdirResponse"]["srmRmdirResponse"];

    std::string explanation;
    SRMStatusCode statuscode = GetStatus(res["returnStatus"], explanation);

    if (statuscode != SRM_SUCCESS) {
      logger.msg(VERBOSE, explanation);
      delete response;
      return DataStatus(DataStatus::DeleteError, srm2errno(statuscode), explanation);
    }

    logger.msg(VERBOSE, "Directory %s removed successfully", creq.surl());
    delete response;
    return DataStatus::Success;
  }

  DataStatus SRM22Client::copy(SRMClientRequest& creq,
                               const std::string& source) {
    PayloadSOAP request(ns);
    XMLNode req = request.NewChild("SRMv2:srmCopy").NewChild("srmCopyRequest");
    XMLNode reqarray = req.NewChild("arrayOfFileRequests")
                       .NewChild("requestArray");
    reqarray.NewChild("sourceSURL") = source;
    reqarray.NewChild("targetSURL") = creq.surl();
    if (!creq.space_token().empty()) req.NewChild("targetSpaceToken") = creq.space_token();

    PayloadSOAP *response = NULL;
    DataStatus status = process("", &request, &response);
    if (!status) return status;

    XMLNode res = (*response)["srmCopyResponse"]["srmCopyResponse"];

    std::string explanation;
    SRMStatusCode statuscode = GetStatus(res["returnStatus"], explanation);

    // store the request token in the request object
    if (res["requestToken"]) creq.request_token(res["requestToken"]);

    // set timeout for copying. Since we don't know the progress of the
    // transfer we hard code a value 10 x the request timeout
    time_t copy_timeout = creq.request_timeout() * 10;

    if (statuscode == SRM_REQUEST_QUEUED ||
        statuscode == SRM_REQUEST_INPROGRESS) {
      // request is queued - need to wait and query with returned request token
      int sleeptime = 1;
      if (res["arrayOfFileStatuses"]["statusArray"]["estimatedWaitTime"]) {
        sleeptime = stringtoi(res["arrayOfFileStatuses"]["statusArray"]
                              ["estimatedWaitTime"]);
      }
      int request_time = 0;

      while (statuscode != SRM_SUCCESS && request_time < copy_timeout) {
        // sleep for recommended time (within limits)
        sleeptime = (sleeptime < 1) ? 1 : sleeptime;
        sleeptime = (sleeptime > 10) ? 10 : sleeptime;
        logger.msg(VERBOSE, "%s: File request %s in SRM queue. Sleeping for %i seconds",
                   creq.surl(), creq.request_token(), sleeptime);
        sleep(sleeptime);
        request_time += sleeptime;

        PayloadSOAP request(ns);
        XMLNode req = request.NewChild("SRMv2:srmStatusOfCopyRequest")
                      .NewChild("srmStatusOfCopyRequestRequest");
        req.NewChild("requestToken") = creq.request_token();

        delete response;
        response = NULL;
        status = process("", &request, &response);
        if (!status) return status;

        res = (*response)["srmStatusOfCopyRequestResponse"]
              ["srmStatusOfCopyRequestResponse"];

        statuscode = GetStatus(res["returnStatus"], explanation);

        // loop will exit on success or return false on error
        if (statuscode == SRM_REQUEST_QUEUED ||
            statuscode == SRM_REQUEST_INPROGRESS) {
          // still queued - keep waiting
          if (res["arrayOfFileStatuses"]["statusArray"]["estimatedWaitTime"]) {
            sleeptime = stringtoi(res["arrayOfFileStatuses"]["statusArray"]["estimatedWaitTime"]);
          }
        }
        else if (statuscode != SRM_SUCCESS) {
          logger.msg(VERBOSE, explanation);
          delete response;
          return DataStatus(DataStatus::TransferError, srm2errno(statuscode), explanation);
        }
      }

      // check for timeout
      if (request_time >= copy_timeout) {
        std::string err_msg("copy request timed out after " + tostring(creq.request_timeout()) + " seconds");
        logger.msg(VERBOSE, err_msg);
        creq.finished_abort();
        delete response;
        return DataStatus(DataStatus::TransferError, EARCREQUESTTIMEOUT, err_msg);
      }
    }
    else if (statuscode != SRM_SUCCESS) {
      logger.msg(VERBOSE, "%s", explanation);
      delete response;
      return DataStatus(DataStatus::TransferError, srm2errno(statuscode), explanation);
    }

    creq.finished_success();
    delete response;
    return DataStatus::Success;
  }

  DataStatus SRM22Client::mkDir(SRMClientRequest& creq) {
    std::string surl = creq.surl();
    std::string::size_type slashpos = surl.find('/', 6);
    slashpos = surl.find('/', slashpos + 1); // don't create root dir
    bool keeplisting = true; // whether to keep checking dir exists
    SRMStatusCode parent_status = SRM_SUCCESS; // reason for failing to create parent dirs
    std::string parent_explanation; // detailed reason for failing to create parent dirs

    while (slashpos != std::string::npos) {
      std::string dirname = surl.substr(0, slashpos);
      // list dir to see if it exists
      SRMClientRequest listreq(dirname);
      listreq.recursion(-1);
      std::list<struct SRMFileMetaData> metadata;
      if (keeplisting) {
        logger.msg(VERBOSE, "Checking for existence of %s", dirname);
        if (info(listreq, metadata).Passed()) {
          if (metadata.front().fileType == SRM_FILE) {
            logger.msg(VERBOSE, "File already exists: %s", dirname);
            return DataStatus(DataStatus::CreateDirectoryError, ENOTDIR, "File already exists");
          }
          slashpos = surl.find("/", slashpos + 1);
          continue;
        }
      }

      logger.msg(VERBOSE, "Creating directory %s", dirname);

      PayloadSOAP request(ns);
      XMLNode req = request.NewChild("SRMv2:srmMkdir")
                    .NewChild("srmMkdirRequest");
      req.NewChild("SURL") = dirname;

      PayloadSOAP *response = NULL;
      DataStatus status = process("", &request, &response);
      if (!status) return status;

      XMLNode res = (*response)["srmMkdirResponse"]["srmMkdirResponse"];

      std::string explanation;
      SRMStatusCode statuscode = GetStatus(res["returnStatus"], explanation);

      slashpos = surl.find("/", slashpos + 1);

      // there can be undetectable errors in creating dirs that already exist
      // so only report error on creating the final dir
      if (statuscode == SRM_SUCCESS ||
          statuscode == SRM_DUPLICATION_ERROR) {
        keeplisting = false;
      }
      else if (slashpos == std::string::npos) {
        if (statuscode == SRM_INVALID_PATH && parent_status != SRM_SUCCESS) {
          statuscode = parent_status;
          explanation = parent_explanation;
        }
        logger.msg(VERBOSE, "Error creating directory %s: %s", dirname, explanation);
        delete response;
        return DataStatus(DataStatus::CreateDirectoryError, srm2errno(statuscode), explanation);
      }
      else if (statuscode != SRM_INVALID_PATH) {
        // remember high-level error so as not to report ENOENT when final dir
        // fails due to failing to create parent dirs
        parent_status = statuscode;
        parent_explanation = explanation;
      }

      delete response;
    }

    return DataStatus::Success;
  }

  DataStatus SRM22Client::rename(SRMClientRequest& creq,
                                 const URL& newurl) {
    PayloadSOAP request(ns);
    XMLNode req = request.NewChild("SRMv2:srmMv")
                  .NewChild("srmMvRequest");
    req.NewChild("fromSURL") = creq.surl();
    req.NewChild("toSURL") = newurl.plainstr();

    PayloadSOAP *response = NULL;
    DataStatus status = process("", &request, &response);
    if (!status) return status;

    XMLNode res = (*response)["srmMvResponse"]["srmMvResponse"];

    std::string explanation;
    SRMStatusCode statuscode = GetStatus(res["returnStatus"], explanation);

    if (statuscode == SRM_SUCCESS) return DataStatus::Success;

    logger.msg(VERBOSE, explanation);
    return DataStatus(DataStatus::RenameError, srm2errno(statuscode), explanation);
  }

  DataStatus SRM22Client::checkPermissions(SRMClientRequest& creq) {
    PayloadSOAP request(ns);
    XMLNode req = request.NewChild("SRMv2:srmCheckPermission")
                  .NewChild("srmCheckPermissionRequest");
    req.NewChild("arrayOfSURLs").NewChild("urlArray") = creq.surl();

    PayloadSOAP *response = NULL;
    DataStatus status = process("", &request, &response);
    if (!status) return status;

    XMLNode res = (*response)["srmCheckPermissionResponse"]["srmCheckPermissionResponse"];

    std::string explanation;
    SRMStatusCode statuscode = GetStatus(res["returnStatus"], explanation);

    if (statuscode != SRM_SUCCESS) {
      logger.msg(VERBOSE, explanation);
      delete response;
      return DataStatus(DataStatus::CheckError, srm2errno(statuscode), explanation);
    }
    // check if 'r' bit is set
    if (std::string(res["arrayOfPermissions"]["surlPermissionArray"]["permission"]).find('R') != std::string::npos) {
      delete response;
      return DataStatus::Success;
    }
    return DataStatus(DataStatus::CheckError, EACCES);
  }

  int SRM22Client::srm2errno(SRMStatusCode reqstat, SRMStatusCode filestat) {

    // Try file-level code first, and if no detailed status is given use
    // request-level code
    SRMStatusCode stat = filestat;
    if (stat == SRM_SUCCESS || stat == SRM_FAILURE) stat = reqstat;

    switch(stat) {

      case SRM_INVALID_PATH:
      case SRM_FILE_LOST:
        return ENOENT;

      case SRM_AUTHENTICATION_FAILURE:
      case SRM_AUTHORIZATION_FAILURE:
        return EACCES;

      case SRM_INVALID_REQUEST:
        return EINVAL;

      case SRM_NON_EMPTY_DIRECTORY:
        return ENOTEMPTY;

      case SRM_SPACE_LIFETIME_EXPIRED:
      case SRM_EXCEED_ALLOCATION:
      case SRM_NO_USER_SPACE:
      case SRM_NO_FREE_SPACE:
        return ENOSPC;

      case SRM_DUPLICATION_ERROR:
        return EEXIST;

      case SRM_TOO_MANY_RESULTS:
        return EARCRESINVAL;

      case SRM_INTERNAL_ERROR:
      case SRM_FILE_UNAVAILABLE:
        return EAGAIN;

      case SRM_FILE_BUSY:
        return EBUSY;

      case SRM_FATAL_INTERNAL_ERROR:
        return EARCSVCPERM;

      case SRM_NOT_SUPPORTED:
        return EOPNOTSUPP;

      case SRM_REQUEST_TIMED_OUT:
        return ETIMEDOUT;

      default:
        // other codes are not errors or are generic failure codes
        return EARCOTHER;
    }
  }


} // namespace ArcDMCSRM
