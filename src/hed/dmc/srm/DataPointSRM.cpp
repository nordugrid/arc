// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <cstdlib>

#include <glibmm/fileutils.h>

#include <arc/Thread.h>
#include <arc/StringConv.h>
#include <arc/Logger.h>
#include <arc/URL.h>
#include <arc/UserConfig.h>
#include <arc/data/DataBuffer.h>
#include <arc/data/DataCallback.h>
#include <arc/data/CheckSum.h>

#include "DataPointSRM.h"

namespace Arc {

  Logger DataPointSRM::logger(Logger::getRootLogger(), "DataPoint.SRM");

  DataPointSRM::DataPointSRM(const URL& url, const UserConfig& usercfg)
    : DataPointDirect(url, usercfg),
      srm_request(NULL),
      r_handle(NULL),
      reading(false),
      writing(false) {
    valid_url_options.push_back("protocol");
    valid_url_options.push_back("spacetoken");
  }

  DataPointSRM::~DataPointSRM() {
    delete r_handle;
    delete srm_request;
  }

  Plugin* DataPointSRM::Instance(PluginArgument *arg) {
    DataPointPluginArgument *dmcarg = dynamic_cast<DataPointPluginArgument*>(arg);
    if (!dmcarg)
      return NULL;
    if (((const URL&)(*dmcarg)).Protocol() != "srm")
      return NULL;
    return new DataPointSRM(*dmcarg, *dmcarg);
  }

  DataStatus DataPointSRM::Check() {

    bool timedout;
    SRMClient *client = SRMClient::getInstance(usercfg, url.fullstr(), timedout);
    if (!client) {
      if (timedout)
        return DataStatus::CheckErrorRetryable;
      return DataStatus::CheckError;
    }

    SRMClientRequest srm_request_tmp(
      (url.HTTPOption("SFN", "") == "")?
      url.str():(url.Protocol() + "://" + url.Host() + "/" + url.HTTPOption("SFN"))
    );
    
    // first check permissions
    SRMReturnCode res = client->checkPermissions(srm_request_tmp);

    if (res != SRM_OK && res != SRM_ERROR_NOT_SUPPORTED) {
      delete client;
      if (res == SRM_ERROR_TEMPORARY) return DataStatus::CheckErrorRetryable;
      return DataStatus::CheckError;
    }

    logger.msg(VERBOSE, "Check: looking for metadata: %s", CurrentLocation().str());
    srm_request_tmp.long_list(true);
    std::list<struct SRMFileMetaData> metadata;

    res = client->info(srm_request_tmp, metadata);
    delete client;
    client = NULL;

    if (res != SRM_OK) {
      if (res == SRM_ERROR_TEMPORARY) return DataStatus::CheckErrorRetryable;
      return DataStatus::CheckError;
    }

    if (metadata.empty()) return DataStatus::CheckError;
    if (metadata.front().size > 0) {
      logger.msg(INFO, "Check: obtained size: %lli", metadata.front().size);
      SetSize(metadata.front().size);
    }
    if (metadata.front().checkSumValue.length() > 0 &&
        metadata.front().checkSumType.length() > 0) {
      std::string csum(metadata.front().checkSumType + ":" + metadata.front().checkSumValue);
      logger.msg(INFO, "Check: obtained checksum: %s", csum);
      SetCheckSum(csum);
    }
    if (metadata.front().createdAtTime > 0) {
      logger.msg(INFO, "Check: obtained creation date: %s", Time(metadata.front().createdAtTime).str());
      SetCreated(Time(metadata.front().createdAtTime));
    }
    if (metadata.front().fileLocality == SRM_ONLINE) {
      logger.msg(INFO, "Check: obtained access latency: low (ONLINE)");
      SetAccessLatency(ACCESS_LATENCY_SMALL);
    }
    else if (metadata.front().fileLocality == SRM_NEARLINE) {
      logger.msg(INFO, "Check: obtained access latency: high (NEARLINE)");
      SetAccessLatency(ACCESS_LATENCY_LARGE);
    }

    return DataStatus::Success;
  }

  DataStatus DataPointSRM::Remove() {

    bool timedout;
    SRMClient *client = SRMClient::getInstance(usercfg, url.fullstr(), timedout);
    if (!client) {
      if (timedout) return DataStatus::DeleteErrorRetryable;
      return DataStatus::DeleteError;
    }

    // take out options in srm url
    std::string canonic_url;
    if (!url.HTTPOption("SFN").empty())
      canonic_url = url.Protocol() + "://" + url.Host() + "/" + url.HTTPOption("SFN");
    else
      canonic_url = url.Protocol() + "://" + url.Host() + url.Path();

    SRMClientRequest srm_request_tmp(canonic_url);

    logger.msg(VERBOSE, "Remove: deleting: %s", CurrentLocation().str());

    SRMReturnCode res = client->remove(srm_request_tmp);
    delete client;
    client = NULL;

    if (res != SRM_OK) {
      if (res == SRM_ERROR_TEMPORARY) return DataStatus::DeleteErrorRetryable;              
      return DataStatus::DeleteError;
    }

    return DataStatus::Success;
  }

  DataStatus DataPointSRM::PrepareReading(unsigned int stage_timeout,
                                          unsigned int& wait_time,
                                          const std::list<std::string>& requested_protocols) {
    if (writing) return DataStatus::IsWritingError;
    if (reading && r_handle) return DataStatus::IsReadingError;

    reading = true;
    turls.clear();
    std::list<std::string> transport_urls;
    SRMReturnCode res;
    bool timedout;

    std::list<std::string> transport_protocols(requested_protocols);
    if (transport_protocols.empty()) {
      transport_protocols.push_back("gsiftp");
      transport_protocols.push_back("http");
      transport_protocols.push_back("https");
      transport_protocols.push_back("httpg");
      transport_protocols.push_back("ftp");
    }

    // If the file is NEARLINE (on tape) bringOnline is called
    // Whether or not to do this should eventually be specified by the user
    if (access_latency == ACCESS_LATENCY_LARGE) {
      if (srm_request) {
        if (srm_request->status() != SRM_REQUEST_ONGOING) {
          // error, querying a request that was already prepared
          logger.msg(ERROR, "Calling PrepareReading when request was already prepared!");
          reading = false;
          return DataStatus::ReadPrepareError;
        }
        SRMClient *client = SRMClient::getInstance(usercfg, url.fullstr(), timedout);
        if (!client) {
          reading = false;
          if (timedout) return DataStatus::ReadPrepareErrorRetryable;
          return DataStatus::ReadPrepareError;
        }
        res = client->requestBringOnlineStatus(*srm_request);
        delete client;
      }
      // if no existing request, make a new request
      else {
        SRMClient* client = SRMClient::getInstance(usercfg, url.fullstr(), timedout);
        if (!client) {
          if (timedout)
            return DataStatus::ReadPrepareErrorRetryable;
          return DataStatus::ReadPrepareError;
        }

        // take out options in srm url
        std::string canonic_url;
        if (!url.HTTPOption("SFN").empty())
          canonic_url = url.Protocol() + "://" + url.Host() + "/" + url.HTTPOption("SFN");
        else
          canonic_url = url.Protocol() + "://" + url.Host() + url.Path();

        delete srm_request;
        srm_request = new SRMClientRequest(canonic_url);
        logger.msg(INFO, "File %s is NEARLINE, will make request to bring online", canonic_url);
        srm_request->request_timeout(stage_timeout);
        res = client->requestBringOnline(*srm_request);
        delete client;
      }
      if (res != SRM_OK) {
        if (res == SRM_ERROR_TEMPORARY) return DataStatus::ReadPrepareErrorRetryable;
        return DataStatus::ReadPrepareError;
      }
      if (srm_request->status() == SRM_REQUEST_ONGOING) {
        // request is not finished yet
        wait_time = srm_request->waiting_time();
        logger.msg(INFO, "Bring online request %s is still in queue, should wait", srm_request->request_token());
        return DataStatus::ReadPrepareWait;
      }
      else if (srm_request->status() == SRM_REQUEST_FINISHED_SUCCESS) {
        // file is staged so go to next step to get TURLs
        logger.msg(INFO, "Bring online request %s finished successfully, file is now ONLINE", srm_request->request_token());
        access_latency = ACCESS_LATENCY_SMALL;
        delete srm_request;
        srm_request = NULL;
      }
      else {
        // bad logic - SRM_OK returned but request is not finished or on going
        logger.msg(ERROR, "Bad logic for %s - bringOnline returned ok but SRM request is not finished successfully or on going", url.str());
        return DataStatus::ReadPrepareError;
      }
    }

    // Here we assume the file is in an ONLINE state
    // If a request already exists, query status
    if (srm_request) {
      if (srm_request->status() != SRM_REQUEST_ONGOING) {
        // error, querying a request that was already prepared
        logger.msg(ERROR, "Calling PrepareReading when request was already prepared!");
        return DataStatus::ReadPrepareError;
      }
      SRMClient *client = SRMClient::getInstance(usercfg, url.fullstr(), timedout);
      if (!client) {
        if (timedout) return DataStatus::ReadPrepareErrorRetryable;
        return DataStatus::ReadPrepareError;
      }
      res = client->getTURLsStatus(*srm_request, transport_urls);
      delete client;
    }
    // if no existing request, make a new request
    else {
      SRMClient* client = SRMClient::getInstance(usercfg, url.fullstr(), timedout);
      if (!client) {
        if (timedout) return DataStatus::ReadPrepareErrorRetryable;
        return DataStatus::ReadPrepareError;
      }

      // take out options in srm url
      std::string canonic_url;
      if (!url.HTTPOption("SFN").empty())
        canonic_url = url.Protocol() + "://" + url.Host() + "/" + url.HTTPOption("SFN");
      else
        canonic_url = url.Protocol() + "://" + url.Host() + url.Path();

      delete srm_request;

      CheckProtocols(transport_protocols);
      if (transport_protocols.empty()) {
        logger.msg(ERROR, "None of the requested transport protocols are supported");
        delete client;
        return DataStatus::ReadPrepareError;
      }
      srm_request = new SRMClientRequest(canonic_url);
      srm_request->request_timeout(stage_timeout);
      srm_request->transport_protocols(transport_protocols);
      res = client->getTURLs(*srm_request, transport_urls);
      delete client;
    }
    if (res != SRM_OK) {
      if (res == SRM_ERROR_TEMPORARY) return DataStatus::ReadPrepareErrorRetryable;
      return DataStatus::ReadPrepareError;
    }
    if (srm_request->status() == SRM_REQUEST_ONGOING) {
      // request is not finished yet
      wait_time = srm_request->waiting_time();
      logger.msg(INFO, "Get request %s is still in queue, should wait %i seconds", srm_request->request_token(), wait_time);
      return DataStatus::ReadPrepareWait;
    }
    else if (srm_request->status() == SRM_REQUEST_FINISHED_SUCCESS) {
      // request finished - deal with TURLs
      // Add all valid TURLs to list
      for (std::list<std::string>::iterator i = transport_urls.begin(); i != transport_urls.end(); ++i) {
        // Avoid redirection to SRM
        logger.msg(VERBOSE, "Checking URL returned by SRM: %s", *i);
        if (strncasecmp(i->c_str(), "srm://", 6) == 0) continue;
        // Try to use this TURL + old options
        URL redirected_url(*i);
        {
          std::map<std::string, std::string> options = url.Options();
          if (!options.empty())
            for (std::map<std::string, std::string>::iterator oi = options.begin(); oi != options.end(); ++oi)
              redirected_url.AddOption((*oi).first, (*oi).second);
        }
        DataHandle redirected_handle(redirected_url, usercfg);
        // check if url can be handled
        if (!redirected_handle) continue;
        if (redirected_handle->IsIndex()) continue;
        turls.push_back(redirected_url);
      }

      if (turls.empty()) {
        logger.msg(ERROR, "SRM returned no useful Transfer URLs: %s", url.str());
        srm_request->finished_abort();
        return DataStatus::ReadPrepareError;
      }
    }
    else {
      // bad logic - SRM_OK returned but request is not finished or on going
      logger.msg(ERROR, "Bad logic for %s - getTURLs returned ok but SRM request is not finished successfully or on going", url.str());
      return DataStatus::ReadPrepareError;
    }
    return DataStatus::Success;
  }

  DataStatus DataPointSRM::StartReading(DataBuffer& buf) {

    logger.msg(VERBOSE, "StartReading");
    if (!reading || turls.empty() || !srm_request || r_handle) {
      logger.msg(ERROR, "StartReading: File was not prepared properly");
      return DataStatus::ReadStartError;
    }

    buffer = &buf;

    // Choose TURL randomly (validity of TURLs was already checked in Prepare)
    std::srand(time(NULL));
    int n = (int)((std::rand() * ((double)(turls.size() - 1))) / RAND_MAX + 0.25);
    r_url = turls.at(n);
    r_handle = new DataHandle(r_url, usercfg);
    // check if url can be handled
    if (!(*r_handle)) {
      logger.msg(ERROR, "TURL %s cannot be handled", r_url.str());
      return DataStatus::ReadStartError;
    }

    (*r_handle)->SetAdditionalChecks(false); // checks at higher levels are always done on SRM metadata
    (*r_handle)->SetSecure(force_secure);
    (*r_handle)->Passive(force_passive);

    logger.msg(INFO, "Redirecting to new URL: %s", (*r_handle)->CurrentLocation().str());
    if (!(*r_handle)->StartReading(buf)) {
      return DataStatus::ReadStartError;
    }
    return DataStatus::Success;
  }

  DataStatus DataPointSRM::StopReading() {
    if (!reading) return DataStatus::Success;
    DataStatus r = DataStatus::Success;
    if (r_handle) {
      r = (*r_handle)->StopReading();
      delete r_handle;
      r_handle = NULL;
    }
    return r;
  }

  DataStatus DataPointSRM::FinishReading(bool error) {
    if (!reading) return DataStatus::Success;
    StopReading();
    reading = false;

    bool timedout;
    if (srm_request) {
      SRMClient *client = SRMClient::getInstance(usercfg, url.fullstr(), timedout);
      // if the request finished with an error there is no need to abort or release request
      if (client && (srm_request->status() != SRM_REQUEST_FINISHED_ERROR)) {
        if (error || srm_request->status() == SRM_REQUEST_SHOULD_ABORT) {
          client->abort(*srm_request);
        } else if (srm_request->status() == SRM_REQUEST_FINISHED_SUCCESS) {
          client->releaseGet(*srm_request);
        }
      }
      delete client;
      delete srm_request;
      srm_request = NULL;
    }
    turls.clear();

    return DataStatus::Success;
  }

  DataStatus DataPointSRM::PrepareWriting(unsigned int stage_timeout,
                                          unsigned int& wait_time,
                                          const std::list<std::string>& requested_protocols) {
    if (reading) return DataStatus::IsReadingError;
    if (writing && r_handle) return DataStatus::IsReadingError;

    writing = true;
    turls.clear();
    std::list<std::string> transport_urls;
    SRMReturnCode res;
    bool timedout;

    std::list<std::string> transport_protocols(requested_protocols);
    if (transport_protocols.empty()) {
      transport_protocols.push_back("gsiftp");
      transport_protocols.push_back("http");
      transport_protocols.push_back("https");
      transport_protocols.push_back("httpg");
      transport_protocols.push_back("ftp");
    }

    // If a request already exists, query status
    if (srm_request) {
      if (srm_request->status() != SRM_REQUEST_ONGOING) {
        // error, querying a request that was already prepared
        logger.msg(ERROR, "Calling PrepareWriting when request was already prepared!");
        return DataStatus::WritePrepareError;
      }
      SRMClient *client = SRMClient::getInstance(usercfg, url.fullstr(), timedout);
      if (!client) {
        if (timedout) return DataStatus::WritePrepareErrorRetryable;
        return DataStatus::WritePrepareError;
      }
      res = client->putTURLsStatus(*srm_request, transport_urls);
      delete client;
    }
    // if no existing request, make a new request
    else {
      SRMClient* client = SRMClient::getInstance(usercfg, url.fullstr(), timedout);
      if (!client) {
        if (timedout) return DataStatus::WritePrepareErrorRetryable;
        return DataStatus::WritePrepareError;
      }

      // take out options in srm url
      std::string canonic_url;
      if (!url.HTTPOption("SFN").empty())
        canonic_url = url.Protocol() + "://" + url.Host() + "/" + url.HTTPOption("SFN");
      else
        canonic_url = url.Protocol() + "://" + url.Host() + url.Path();

      delete srm_request;

      CheckProtocols(transport_protocols);
      if (transport_protocols.empty()) {
        logger.msg(ERROR, "None of the requested transport protocols are supported");
        delete client;
        return DataStatus::WritePrepareError;
      }

      srm_request = new SRMClientRequest(canonic_url);
      // set space token
      std::string space_token = url.Option("spacetoken");
      if (space_token.empty()) {
        if (client->getVersion().compare("v2.2") == 0) {
          // only print message if using v2.2
          logger.msg(VERBOSE, "No space token specified");
        }
      }
      else {
        if (client->getVersion().compare("v2.2") != 0) {
          // print warning if not using srm2.2
          logger.msg(WARNING, "Warning: Using SRM protocol v1 which does not support space tokens");
        }
        else {
          logger.msg(VERBOSE, "Using space token description %s", space_token);
          // get token from SRM that matches description
          std::list<std::string> tokens;
          if (client->getSpaceTokens(tokens, space_token) != SRM_OK) {
            // not critical so log a warning
            logger.msg(WARNING, "Warning: Error looking up space tokens matching description %s. Will copy without using token", space_token);
          }
          else if (tokens.empty()) {
            // not critical so log a warning
            logger.msg(WARNING, "Warning: No space tokens found matching description! Will copy without using token");
          }
          else {
            // take the first one in the list
            logger.msg(VERBOSE, "Using space token %s", tokens.front());
            srm_request->space_token(tokens.front());
          }
        }
      }
      srm_request->request_timeout(stage_timeout);
      if (CheckSize()) srm_request->total_size(GetSize());
      srm_request->transport_protocols(transport_protocols);
      res = client->putTURLs(*srm_request, transport_urls);
      delete client;
    }

    if (res != SRM_OK) {
      if (res == SRM_ERROR_TEMPORARY) return DataStatus::WritePrepareErrorRetryable;
      return DataStatus::WritePrepareError;
    }
    if (srm_request->status() == SRM_REQUEST_ONGOING) {
      // request is not finished yet
      wait_time = srm_request->waiting_time();
      logger.msg(INFO, "Put request %s is still in queue, should wait %i seconds", srm_request->request_token(), wait_time);
      return DataStatus::WritePrepareWait;
    }
    else if (srm_request->status() == SRM_REQUEST_FINISHED_SUCCESS) {
      // request finished - deal with TURLs
      // Add all valid TURLs to list
      for (std::list<std::string>::iterator i = transport_urls.begin(); i != transport_urls.end(); ++i) {
        // Avoid redirection to SRM
        logger.msg(VERBOSE, "Checking URL returned by SRM: %s", *i);
        if (strncasecmp(i->c_str(), "srm://", 6) == 0) continue;
        // Try to use this TURL + old options
        URL redirected_url(*i);
        {
          std::map<std::string, std::string> options = url.Options();
          if (!options.empty())
            for (std::map<std::string, std::string>::iterator oi = options.begin(); oi != options.end(); ++oi)
              redirected_url.AddOption((*oi).first, (*oi).second);
        }
        DataHandle redirected_handle(redirected_url, usercfg);
        // check if url can be handled
        if (!redirected_handle) continue;
        if (redirected_handle->IsIndex()) continue;
        turls.push_back(redirected_url);
      }

      if (turls.empty()) {
        logger.msg(ERROR, "SRM returned no useful Transfer URLs: %s", url.str());
        srm_request->finished_abort();
        return DataStatus::WritePrepareError;
      }
    }
    else {
      // bad logic - SRM_OK returned but request is not finished or on going
      logger.msg(ERROR, "Bad logic for %s - putTURLs returned ok but SRM request is not finished successfully or on going", url.str());
      return DataStatus::WritePrepareError;
    }
    return DataStatus::Success;
  }

  DataStatus DataPointSRM::StartWriting(DataBuffer& buf,
                                        DataCallback *space_cb) {
    logger.msg(VERBOSE, "StartWriting");
    if (!writing || turls.empty() || !srm_request || r_handle) {
      logger.msg(ERROR, "StartWriting: File was not prepared properly");
      return DataStatus::WriteStartError;
    }

    buffer = &buf;

    // Choose TURL randomly (validity of TURLs was already checked in Prepare)
    std::srand(time(NULL));
    int n = (int)((std::rand() * ((double)(turls.size() - 1))) / RAND_MAX + 0.25);
    r_url = turls.at(n);
    r_handle = new DataHandle(r_url, usercfg);
    // check if url can be handled
    if (!(*r_handle)) {
      logger.msg(ERROR, "TURL %s cannot be handled", r_url.str());
      return DataStatus::WriteStartError;
    }

    (*r_handle)->SetAdditionalChecks(false); // checks at higher levels are always done on SRM metadata
    (*r_handle)->SetSecure(force_secure);
    (*r_handle)->Passive(force_passive);

    logger.msg(INFO, "Redirecting to new URL: %s", (*r_handle)->CurrentLocation().str());
    if (!(*r_handle)->StartWriting(buf)) {
      return DataStatus::WriteStartError;
    }
    return DataStatus::Success;
  }

  DataStatus DataPointSRM::StopWriting() {
    if (!writing) return DataStatus::Success;
    DataStatus r = DataStatus::Success;
    if (r_handle) {
      r = (*r_handle)->StopWriting();
      // check if checksum was verified at lower level
      if ((*r_handle)->CheckCheckSum()) SetCheckSum((*r_handle)->GetCheckSum());
      delete r_handle;
      r_handle = NULL;
    }
    return r;
  }

  DataStatus DataPointSRM::FinishWriting(bool error) {
    if (!writing) return DataStatus::Success;
    StopWriting();
    writing = false;

    DataStatus r = DataStatus::Success;
    bool timedout;

    // if the request finished with an error there is no need to abort or release request
    if (srm_request) {
      SRMClient * client = SRMClient::getInstance(usercfg, url.fullstr(), timedout);
      if (client && (srm_request->status() != SRM_REQUEST_FINISHED_ERROR)) {
        // call abort if failure, or releasePut on success
        if (error || srm_request->status() == SRM_REQUEST_SHOULD_ABORT) {
           client->abort(*srm_request);
           client->remove(*srm_request);
        }
        else {
          // checksum verification - if requested and not already done at lower level
          if (srm_request->status() == SRM_REQUEST_FINISHED_SUCCESS && additional_checks && !CheckCheckSum()) {
            const CheckSum * calc_sum = buffer->checksum_object();
            if (calc_sum && *calc_sum && buffer->checksum_valid()) {
              char buf[100];
              calc_sum->print(buf,100);
              std::string csum(buf);
              if (!csum.empty() && csum.find(':') != std::string::npos) {
                // get checksum info for checksum verification
                logger.msg(VERBOSE, "FinishWriting: looking for metadata: %s", url.str());
                // create a new request
                SRMClientRequest list_request(srm_request->surls());
                list_request.long_list(true);
                std::list<struct SRMFileMetaData> metadata;
                SRMReturnCode res = client->info(list_request,metadata);
                if (res != SRM_OK) {
                  client->abort(*srm_request); // if we can't list then we can't remove either
                  delete client;
                  delete srm_request;
                  srm_request = NULL;
                  if (res == SRM_ERROR_TEMPORARY) return DataStatus::WriteFinishErrorRetryable;
                  return DataStatus::WriteFinishError;
                }
                if (!metadata.empty()) {
                  if (metadata.front().checkSumValue.length() > 0 &&
                     metadata.front().checkSumType.length() > 0) {
                    std::string servercsum(metadata.front().checkSumType+":"+metadata.front().checkSumValue);
                    logger.msg(INFO, "FinishWriting: obtained checksum: %s", servercsum);
                    if (csum.substr(0, csum.find(':')) == metadata.front().checkSumType) {
                      if (csum.substr(csum.find(':')+1) == metadata.front().checkSumValue) {
                        logger.msg(INFO, "Calculated/supplied transfer checksum %s matches checksum reported by SRM destination %s", csum, servercsum);
                      }
                      else {
                        logger.msg(ERROR, "Checksum mismatch between calculated/supplied checksum (%s) and checksum reported by SRM destination (%s)", csum, servercsum);
                        r = DataStatus::WriteFinishErrorRetryable;
                      }
                    } else logger.msg(WARNING, "Checksum type of SRM (%s) and calculated/supplied checksum (%s) differ, cannot compare", servercsum, csum);
                  } else logger.msg(WARNING, "No checksum information from server");
                } else logger.msg(WARNING, "No checksum information from server");
              } else logger.msg(INFO, "No checksum verification possible");
            } else logger.msg(INFO, "No checksum verification possible");
          }
          if (r.Passed()) {
            if (srm_request->status() == SRM_REQUEST_FINISHED_SUCCESS)
              if (client->releasePut(*srm_request) != SRM_OK) {
                logger.msg(ERROR, "Failed to release completed request");
                r = DataStatus::WriteFinishError;
              }
          } else {
            client->abort(*srm_request);
            client->remove(*srm_request);
          }
        }
      }
      delete client;
      delete srm_request;
      srm_request = NULL;
    }
    return r;
  }

  DataStatus DataPointSRM::Stat(FileInfo& file, DataPointInfoType verb) {
    std::list<FileInfo> files;
    DataStatus r = ListFiles(files,verb,-1);
    if(files.size() != 1) return DataStatus::StatError;
    file = *(files.begin());
    if(r == DataStatus::ListErrorRetryable) r = DataStatus::StatErrorRetryable;
    if(r == DataStatus::ListError) r = DataStatus::StatError;
    return r;
  }

  DataStatus DataPointSRM::List(std::list<FileInfo>& files, DataPointInfoType verb) {
    return ListFiles(files,verb,0);
  }

  DataStatus DataPointSRM::ListFiles(std::list<FileInfo>& files, DataPointInfoType verb, int recursion) {
    // This method does not use any dynamic members of this object. Hence
    // it can be executed even while reading or writing
    if(reading || writing) return DataStatus::ListErrorRetryable;

    bool timedout;
    SRMClient * client = SRMClient::getInstance(usercfg, url.fullstr(), timedout);
    if(!client) {
      if (timedout) return DataStatus::ListErrorRetryable;
      return DataStatus::ListError;
    }
    
    std::string canonic_url;
    std::string sfn_path = url.HTTPOption("SFN");
    if (!sfn_path.empty()) {
      while (sfn_path[0] == '/') sfn_path.erase(0,1);
      canonic_url = url.Protocol() + "://" + url.Host() + "/" + sfn_path;
    } else {
      canonic_url = url.Protocol() + "://" + url.Host() + url.Path();
    }

    SRMClientRequest srm_request_tmp(canonic_url);
    logger.msg(VERBOSE, "ListFiles: looking for metadata: %s", CurrentLocation().str());
    if ((verb | INFO_TYPE_NAME) != INFO_TYPE_NAME) srm_request_tmp.long_list(true);
    std::list<struct SRMFileMetaData> srm_metadata;

    // get info from SRM
    SRMReturnCode res = client->info(srm_request_tmp, srm_metadata, recursion);
    delete client;
    if (res != SRM_OK) {
      if (res == SRM_ERROR_TEMPORARY) return DataStatus::ListErrorRetryable;   
      return DataStatus::ListError;
    }

    if (srm_metadata.empty()) {
      return DataStatus::Success;
    }
    // set URL attributes for surl requested (file or dir)
    if(srm_metadata.front().size > 0) {
      SetSize(srm_metadata.front().size);
    }
    if(srm_metadata.front().checkSumType.length() > 0 &&
       srm_metadata.front().checkSumValue.length() > 0) {
      std::string csum(srm_metadata.front().checkSumType+":"+srm_metadata.front().checkSumValue);
      SetCheckSum(csum);
    }
    if(srm_metadata.front().createdAtTime > 0) {
      SetCreated(Time(srm_metadata.front().createdAtTime));
    }

    // set FileInfo attributes for surl requested and any files within a dir
    for (std::list<struct SRMFileMetaData>::iterator i = srm_metadata.begin();
         i != srm_metadata.end();
         ++i) {

      std::list<FileInfo>::iterator f =
        files.insert(files.end(), FileInfo(i->path));
      f->SetMetaData("path", i->path);
      
      if (i->fileType == SRM_FILE) {
        f->SetType(FileInfo::file_type_file);
        f->SetMetaData("type", "file");
      }
      else if (i->fileType == SRM_DIRECTORY) {
        f->SetType(FileInfo::file_type_dir);
        f->SetMetaData("type", "dir");
      }

      if (i->size >= 0) {
        f->SetSize(i->size);
        f->SetMetaData("size", tostring(i->size));
      }
      if (i->createdAtTime > 0) {
        f->SetCreated(Time(i->createdAtTime));
        f->SetMetaData("ctime", (Time(i->createdAtTime)).str());
      }
      if (i->checkSumType.length() > 0 &&
          i->checkSumValue.length() > 0) {
        std::string csum(i->checkSumType + ":" + i->checkSumValue);
        f->SetCheckSum(csum);
        f->SetMetaData("checksum", csum);
      }
      if (i->fileLocality == SRM_ONLINE) {
        f->SetLatency("ONLINE");
        f->SetMetaData("latency", "ONLINE");
      }
      else if (i->fileLocality == SRM_NEARLINE) {
        f->SetLatency("NEARLINE");
        f->SetMetaData("latency", "NEARLINE");
      }
      if (!i->spaceTokens.empty()) {
        std::string spaceTokens;
        for (std::list<std::string>::iterator it = i->spaceTokens.begin();
             it != i->spaceTokens.end(); it++) {
          if (!spaceTokens.empty()) 
            spaceTokens += ',';
          spaceTokens += *it;
        }
        f->SetMetaData("spacetokens", spaceTokens);
      }
      if(!i->owner.empty()) f->SetMetaData("owner", i->owner);
      if(!i->group.empty()) f->SetMetaData("group", i->group);
      if(!i->permission.empty()) f->SetMetaData("accessperm", i->permission);
      if(i->lastModificationTime > 0)
        f->SetMetaData("mtime", (Time(i->lastModificationTime)).str());
      if(i->lifetimeLeft != 0) f->SetMetaData("lifetimeleft", tostring(i->lifetimeLeft));
      if(i->lifetimeAssigned != 0) f->SetMetaData("lifetimeassigned", tostring(i->lifetimeAssigned));
  
      if (i->retentionPolicy == SRM_REPLICA) f->SetMetaData("retentionpolicy", "REPLICA");
      else if (i->retentionPolicy == SRM_OUTPUT) f->SetMetaData("retentionpolicy", "OUTPUT");
      else if (i->retentionPolicy == SRM_CUSTODIAL)  f->SetMetaData("retentionpolicy", "CUSTODIAL");

      if (i->fileStorageType == SRM_VOLATILE) f->SetMetaData("filestoragetype", "VOLATILE");
      else if (i->fileStorageType == SRM_DURABLE) f->SetMetaData("filestoragetype", "DURABLE");
      else if (i->fileStorageType == SRM_PERMANENT) f->SetMetaData("filestoragetype", "PERMANENT"); 

    }
    return DataStatus::Success;
  }

  const std::string DataPointSRM::DefaultCheckSum() const {
    return std::string("adler32");
  }

  bool DataPointSRM::ProvidesMeta() const {
    return true;
  }

  bool DataPointSRM::IsStageable() const {
    return true;
  }

  std::vector<URL> DataPointSRM::TransferLocations() const {
    return turls;
  }

  void DataPointSRM::CheckProtocols(std::list<std::string>& transport_protocols) {
    for (std::list<std::string>::iterator protocol = transport_protocols.begin();
         protocol != transport_protocols.end();) {
      // try to load plugins
      URL url(*protocol+"://host/path");
      DataHandle handle(url, usercfg);
      if (handle) {
        ++protocol;
      } else {
        logger.msg(INFO, "plugin for transport protocol %s is not installed", *protocol);
        protocol = transport_protocols.erase(protocol);
      }
    }
  }

} // namespace Arc

Arc::PluginDescriptor PLUGINS_TABLE_NAME[] = {
  { "srm", "HED:DMC", 0, &Arc::DataPointSRM::Instance },
  { NULL, NULL, 0, NULL }
};
