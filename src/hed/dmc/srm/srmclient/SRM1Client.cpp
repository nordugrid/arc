// -*- indent-tabs-mode: nil -*-

#ifdef WIN32 
#include <arc/win32.h>
#endif

#include "SRM1Client.h"

#include <arc/StringConv.h>

namespace Arc {

  SRM1Client::SRM1Client(const UserConfig& usercfg, const SRMURL& url)
    : SRMClient(usercfg, url) {
    version = "v1";
    ns["SRMv1Type"] =
      "http://www.themindelectric.com/package/diskCacheV111.srm/";
    ns["SRMv1Meth"] =
      "http://tempuri.org/diskCacheV111.srm.server.SRMServerV1";
  }

  SRM1Client::~SRM1Client() {}

  SRMReturnCode SRM1Client::getTURLs(SRMClientRequest& creq,
                                     std::list<std::string>& urls) {
    SRMURL srmurl(creq.surls().front());
    std::list<int> file_ids;

    PayloadSOAP request(ns);
    XMLNode method = request.NewChild("SRMv1Meth:get");
    // Source file names
    XMLNode arg0node = method.NewChild("arg0");
    arg0node.NewAttribute("SOAP-ENC:arrayType") = "xsd:string[1]";
    arg0node.NewChild("item") = srmurl.FullURL();
    // Protocols
    XMLNode arg1node = method.NewChild("arg1");
    arg1node.NewAttribute("SOAP-ENC:arrayType") = "xsd:string[5]";
    arg1node.NewChild("item") = "gsiftp";
    arg1node.NewChild("item") = "https";
    arg1node.NewChild("item") = "httpg";
    arg1node.NewChild("item") = "http";
    arg1node.NewChild("item") = "ftp";

    PayloadSOAP *response = NULL;
    SRMReturnCode status = process(&request, &response);
    if (status != SRM_OK)
      return status;

    XMLNode result = (*response)["getResponse"]["Result"];
    if (!result) {
      logger.msg(INFO, "SRM did not return any information");
      delete response;
      return SRM_ERROR_OTHER;
    }

    std::string request_state = (std::string)result["state"];
    creq.request_id(result["requestId"]);
    time_t t_start = time(NULL);

    for (;;) {
      for (XMLNode n = result["fileStatuses"]["item"]; n; ++n)
        if (strcasecmp(((std::string)n["state"]).c_str(), "ready") == 0)
          if (n["TURL"]) {
            urls.push_back(n["TURL"]);
            file_ids.push_back(stringtoi(n["fileId"]));
          }
      if (!urls.empty())         // Have requested data
        break;
      if (request_state.empty()) // No data and no state - fishy
        break;
      if (strcasecmp(request_state.c_str(), "pending") != 0)
        break;
      if ((time(NULL) - t_start) > creq.request_timeout())
        break;

      int retryDeltaTime = stringtoi(result["retryDeltaTime"]);
      if (retryDeltaTime < 1)
        retryDeltaTime = 1;
      if (retryDeltaTime > 10)
        retryDeltaTime = 10;
      sleep(retryDeltaTime);

      PayloadSOAP request(ns);
      request.NewChild("SRMv1Meth:getRequestStatus").NewChild("arg0") =
        tostring(creq.request_id());

      delete response;
      response = NULL;
      status = process(&request, &response);
      if (status != SRM_OK)
        return status;

      result = (*response)["getRequestStatusResponse"]["Result"];
      if (!result) {
        logger.msg(INFO, "SRM did not return any information");
        delete response;
        return SRM_ERROR_OTHER;
      }

      request_state = (std::string)result["state"];
    }

    creq.file_ids(file_ids);
    delete response;
    if (urls.empty())
      return SRM_ERROR_OTHER;
    return acquire(creq, urls);
  }

  SRMReturnCode SRM1Client::putTURLs(SRMClientRequest& creq,
                                     std::list<std::string>& urls) {
    SRMURL srmurl(creq.surls().front());
    std::list<int> file_ids;

    PayloadSOAP request(ns);
    XMLNode method = request.NewChild("SRMv1Meth:put");
    // Source file names
    XMLNode arg0node = method.NewChild("arg0");
    arg0node.NewAttribute("SOAP-ENC:arrayType") = "xsd:string[1]";
    arg0node.NewChild("item") = srmurl.FullURL();
    // Destination file names
    XMLNode arg1node = method.NewChild("arg1");
    arg1node.NewAttribute("SOAP-ENC:arrayType") = "xsd:string[1]";
    arg1node.NewChild("item") = srmurl.FullURL();
    // Sizes
    XMLNode arg2node = method.NewChild("arg2");
    arg2node.NewAttribute("SOAP-ENC:arrayType") = "xsd:string[1]";
    arg2node.NewChild("item") = tostring(creq.total_size());
    // Want Permanent
    XMLNode arg3node = method.NewChild("arg3");
    arg3node.NewAttribute("SOAP-ENC:arrayType") = "xsd:string[1]";
    arg3node.NewChild("item") = "true";
    // Protocols
    XMLNode arg4node = method.NewChild("arg4");
    arg4node.NewAttribute("SOAP-ENC:arrayType") = "xsd:string[5]";
    arg4node.NewChild("item") = "gsiftp";
    arg4node.NewChild("item") = "https";
    arg4node.NewChild("item") = "httpg";
    arg4node.NewChild("item") = "http";
    arg4node.NewChild("item") = "ftp";

    PayloadSOAP *response = NULL;
    SRMReturnCode status = process(&request, &response);
    if (status != SRM_OK)
      return status;

    XMLNode result = (*response)["putResponse"]["Result"];
    if (!result) {
      logger.msg(INFO, "SRM did not return any information");
      delete response;
      return SRM_ERROR_OTHER;
    }

    std::string request_state = (std::string)result["state"];
    creq.request_id(result["requestId"]);
    time_t t_start = time(NULL);

    for (;;) {
      for (XMLNode n = result["fileStatuses"]["item"]; n; ++n)
        if (strcasecmp(((std::string)n["state"]).c_str(), "ready") == 0)
          if (n["TURL"]) {
            urls.push_back(n["TURL"]);
            file_ids.push_back(stringtoi(n["fileId"]));
          }
      if (!urls.empty())         // Have requested data
        break;
      if (request_state.empty()) // No data and no state - fishy
        break;
      if (strcasecmp(request_state.c_str(), "pending") != 0)
        break;
      if ((time(NULL) - t_start) > creq.request_timeout())
        break;

      int retryDeltaTime = stringtoi(result["retryDeltaTime"]);
      if (retryDeltaTime < 1)
        retryDeltaTime = 1;
      if (retryDeltaTime > 10)
        retryDeltaTime = 10;
      sleep(retryDeltaTime);

      PayloadSOAP request(ns);
      request.NewChild("SRMv1Meth:getRequestStatus").NewChild("arg0") =
        tostring(creq.request_id());

      delete response;
      response = NULL;
      status = process(&request, &response);
      if (status != SRM_OK)
        return status;

      result = (*response)["getRequestStatusResponse"]["Result"];
      if (!result) {
        logger.msg(INFO, "SRM did not return any information");
        delete response;
        return SRM_ERROR_OTHER;
      }

      request_state = (std::string)result["state"];
    }

    creq.file_ids(file_ids);
    delete response;
    if (urls.empty())
      return SRM_ERROR_OTHER;
    return acquire(creq, urls);
  }

  SRMReturnCode SRM1Client::copy(SRMClientRequest& creq,
                                 const std::string& source) {
    SRMURL srmurl(creq.surls().front());
    std::list<int> file_ids;

    PayloadSOAP request(ns);
    XMLNode method = request.NewChild("SRMv1Meth:copy");
    // Source file names
    XMLNode arg0node = method.NewChild("arg0");
    arg0node.NewAttribute("SOAP-ENC:arrayType") = "xsd:string[1]";
    arg0node.NewChild("item") = source;
    // Destination file names
    XMLNode arg1node = method.NewChild("arg1");
    arg1node.NewAttribute("SOAP-ENC:arrayType") = "xsd:string[1]";
    arg1node.NewChild("item") = srmurl.FullURL();
    // Whatever
    XMLNode arg2node = method.NewChild("arg2");
    arg2node.NewAttribute("SOAP-ENC:arrayType") = "xsd:string[1]";
    arg2node.NewChild("item") = "false";

    PayloadSOAP *response = NULL;
    SRMReturnCode status = process(&request, &response);
    if (status != SRM_OK)
      return status;

    XMLNode result = (*response)["copyResponse"]["Result"];
    if (!result) {
      logger.msg(INFO, "SRM did not return any information");
      delete response;
      return SRM_ERROR_OTHER;
    }

    std::string request_state = (std::string)result["state"];
    creq.request_id(result["requestId"]);
    time_t t_start = time(NULL);

    for (;;) {
      for (XMLNode n = result["fileStatuses"]["item"]; n; ++n)
        if (strcasecmp(((std::string)n["state"]).c_str(), "ready") == 0)
          file_ids.push_back(stringtoi(n["fileId"]));
      if (!file_ids.empty())     // Have requested data
        break;
      if (request_state.empty()) // No data and no state - fishy
        break;
      if ((strcasecmp(request_state.c_str(), "pending") != 0) &&
          (strcasecmp(request_state.c_str(), "active") != 0))
        break;
      if ((time(NULL) - t_start) > creq.request_timeout())
        break;

      int retryDeltaTime = stringtoi(result["retryDeltaTime"]);
      if (retryDeltaTime < 1)
        retryDeltaTime = 1;
      if (retryDeltaTime > 10)
        retryDeltaTime = 10;
      sleep(retryDeltaTime);

      PayloadSOAP request(ns);
      request.NewChild("SRMv1Meth:getRequestStatus").NewChild("arg0") =
        tostring(creq.request_id());

      delete response;
      response = NULL;
      status = process(&request, &response);
      if (status != SRM_OK)
        return status;

      result = (*response)["getRequestStatusResponse"]["Result"];
      if (!result) {
        logger.msg(INFO, "SRM did not return any information");
        delete response;
        return SRM_ERROR_OTHER;
      }

      request_state = (std::string)result["state"];
    }

    delete response;
    if (file_ids.empty())
      return SRM_ERROR_OTHER;
    creq.file_ids(file_ids);
    return release(creq);
  }

  SRMReturnCode SRM1Client::acquire(SRMClientRequest& creq,
                                    std::list<std::string>& urls) {
    std::list<int> file_ids = creq.file_ids();

    // Tell server to move files into "Running" state
    std::list<int>::iterator file_id = file_ids.begin();
    std::list<std::string>::iterator file_url = urls.begin();
    while (file_id != file_ids.end()) {
      PayloadSOAP request(ns);
      XMLNode method = request.NewChild("SRMv1Meth:setFileStatus");
      // Request ID
      XMLNode arg0node = method.NewChild("arg0");
      arg0node.NewAttribute("SOAP-ENC:arrayType") = "xsd:string[1]";
      arg0node.NewChild("item") = tostring(creq.request_id());
      // File ID
      XMLNode arg1node = method.NewChild("arg1");
      arg1node.NewAttribute("SOAP-ENC:arrayType") = "xsd:string[1]";
      arg1node.NewChild("item") = tostring(*file_id);
      // Running
      XMLNode arg2node = method.NewChild("arg2");
      arg2node.NewAttribute("SOAP-ENC:arrayType") = "xsd:string[1]";
      arg2node.NewChild("item") = "Running";

      PayloadSOAP *response = NULL;
      SRMReturnCode status = process(&request, &response);
      if (status != SRM_OK)
        return status;

      XMLNode result = (*response)["setFileStatusResponse"]["Result"];
      if (!result) {
        logger.msg(INFO, "SRM did not return any information");
        delete response;
        return SRM_ERROR_OTHER;
      }

      for (XMLNode n = result["fileStatuses"]["item"]; n; ++n) {
        if (stringtoi(n["fileId"]) != *file_id)
          continue;
        if (strcasecmp(((std::string)n["state"]).c_str(), "running") == 0) {
          ++file_id;
          ++file_url;
        }
        else {
          logger.msg(VERBOSE, "File could not be moved to Running state: %s",
                     *file_url);
          file_id = file_ids.erase(file_id);
          file_url = urls.erase(file_url);
        }
      }

      delete response;
    }

    creq.file_ids(file_ids);
    if (urls.empty())
      return SRM_ERROR_OTHER;
    return SRM_OK;
  }

  SRMReturnCode SRM1Client::remove(SRMClientRequest& creq) {
    SRMURL srmurl(creq.surls().front());

    PayloadSOAP request(ns);
    XMLNode method = request.NewChild("SRMv1Meth:advisoryDelete");
    // File names
    XMLNode arg0node = method.NewChild("arg0");
    arg0node.NewAttribute("SOAP-ENC:arrayType") = "xsd:string[1]";
    arg0node.NewChild("item") = srmurl.FullURL();

    PayloadSOAP *response = NULL;
    SRMReturnCode status = process(&request, &response);
    if (status != SRM_OK)
      return status;

    delete response;
    return SRM_OK;
  }

  SRMReturnCode SRM1Client::info(SRMClientRequest& creq,
                                 std::list<struct SRMFileMetaData>& metadata,
                                 const int /* recursive */,
                                 bool report_error) {
    SRMURL srmurl(creq.surls().front());

    PayloadSOAP request(ns);
    XMLNode method = request.NewChild("SRMv1Meth:getFileMetaData");
    // File names
    XMLNode arg0node = method.NewChild("arg0");
    arg0node.NewAttribute("SOAP-ENC:arrayType") = "xsd:string[1]";
    arg0node.NewChild("item") = srmurl.FullURL();

    PayloadSOAP *response = NULL;
    SRMReturnCode status = process(&request, &response);
    if (status != SRM_OK)
      return status;

    XMLNode result = (*response)["getFileMetaDataResponse"]["Result"];
    if (!result) {
      logger.msg(report_error ? INFO : DEBUG,
                 "SRM did not return any information");
      delete response;
      return SRM_ERROR_OTHER;
    }

    XMLNode mdata = result["item"];
    if (!mdata) {
      logger.msg(report_error ? INFO : DEBUG,
                 "SRM did not return any useful information");
      delete response;
      return SRM_ERROR_OTHER;
    }

    struct SRMFileMetaData md;
    md.path = srmurl.FileName();
    // tidy up path
    std::string::size_type i = md.path.find("//");
    while (i != std::string::npos) {
      md.path.erase(i, 1);
      i = md.path.find("//", i);
    }
    if (md.path.find("/") != 0)
      md.path = "/" + md.path;
    // date, type and locality not supported in v1
    md.createdAtTime = (time_t)0;
    md.fileType = SRM_FILE_TYPE_UNKNOWN;
    md.fileLocality = SRM_UNKNOWN;
    md.size = stringtoull(mdata["size"]);
    if (mdata["checksumType"])
      md.checkSumType = (std::string)mdata["checksumType"];
    if (mdata["checksumValue"])
      md.checkSumValue = (std::string)mdata["checksumValue"];
    metadata.push_back(md);

    delete response;
    return SRM_OK;
  }

  SRMReturnCode SRM1Client::release(SRMClientRequest& creq) {
    std::list<int> file_ids = creq.file_ids();

    // Tell server to move files into "Done" state
    std::list<int>::iterator file_id = file_ids.begin();
    while (file_id != file_ids.end()) {
      PayloadSOAP request(ns);
      XMLNode method = request.NewChild("SRMv1Meth:setFileStatus");
      // Request ID
      XMLNode arg0node = method.NewChild("arg0");
      arg0node.NewAttribute("SOAP-ENC:arrayType") = "xsd:string[1]";
      arg0node.NewChild("item") = tostring(creq.request_id());
      // File ID
      XMLNode arg1node = method.NewChild("arg1");
      arg1node.NewAttribute("SOAP-ENC:arrayType") = "xsd:string[1]";
      arg1node.NewChild("item") = tostring(*file_id);
      // Done
      XMLNode arg2node = method.NewChild("arg2");
      arg2node.NewAttribute("SOAP-ENC:arrayType") = "xsd:string[1]";
      arg2node.NewChild("item") = "Done";

      PayloadSOAP *response = NULL;
      SRMReturnCode status = process(&request, &response);
      if (status != SRM_OK)
        return status;

      XMLNode result = (*response)["setFileStatusResponse"]["Result"];
      if (!result) {
        logger.msg(INFO, "SRM did not return any information");
        delete response;
        return SRM_ERROR_OTHER;
      }

      for (XMLNode n = result["fileStatuses"]["item"]; n; ++n) {
        if (stringtoi(n["fileId"]) != *file_id)
          continue;
        if (strcasecmp(((std::string)n["state"]).c_str(), "done") == 0)
          ++file_id;
        else {
          logger.msg(VERBOSE, "File could not be moved to Done state");
          file_id = file_ids.erase(file_id);
        }
      }

      delete response;
    }

    creq.file_ids(file_ids);
    return SRM_OK;
  }

  SRMReturnCode SRM1Client::releaseGet(SRMClientRequest& creq) {
    return release(creq);
  }

  SRMReturnCode SRM1Client::releasePut(SRMClientRequest& creq) {
    return release(creq);
  }

  SRMReturnCode SRM1Client::abort(SRMClientRequest& creq) {
    return release(creq);
  }

} // namespace Arc
