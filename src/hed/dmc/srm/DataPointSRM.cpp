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
#include <arc/CheckSum.h>

#include "DataPointSRM.h"

namespace ArcDMCSRM {

  using namespace Arc;

  Logger DataPointSRM::logger(Logger::getRootLogger(), "DataPoint.SRM");

  DataPointSRM::DataPointSRM(const URL& url, const UserConfig& usercfg, PluginArgument* parg)
    : DataPointDirect(url, usercfg, parg),
      reading(false),
      writing(false) {}

  DataPointSRM::~DataPointSRM() {
  }

  Plugin* DataPointSRM::Instance(PluginArgument *arg) {
    DataPointPluginArgument *dmcarg = dynamic_cast<DataPointPluginArgument*>(arg);
    if (!dmcarg)
      return NULL;
    if (((const URL&)(*dmcarg)).Protocol() != "srm")
      return NULL;
    return new DataPointSRM(*dmcarg, *dmcarg, dmcarg);
  }

  DataStatus DataPointSRM::SetupHandler(DataStatus::DataStatusType base_error) const {
    if (r_handle)
      return DataStatus::Success; // already set, assign r_handle null to change TURL.
    if (turls.empty())
      return DataStatus(base_error, EARCRESINVAL, "No TURLs defined");

    // Choose TURL randomly (validity of TURLs should be already checked)
    std::srand(time(NULL));
    int n = (int)((std::rand() * ((double)(turls.size() - 1))) / RAND_MAX + 0.25);
    URL r_url = turls.at(n);
    r_handle = new DataHandle(r_url, usercfg);
    // check if url can be handled
    if (!(*r_handle)) {
      r_handle = NULL;
      logger.msg(VERBOSE, "TURL %s cannot be handled", r_url.str());
      return DataStatus(base_error, EARCRESINVAL, "Transfer URL cannot be handled");
    }

    (*r_handle)->SetAdditionalChecks(false); // checks at higher levels are always done on SRM metadata
    (*r_handle)->SetSecure(force_secure);
    (*r_handle)->Passive(force_passive);
    return DataStatus::Success;
  }

  DataStatus DataPointSRM::Check(bool check_meta) {

    std::string error;
    AutoPointer<SRMClient> client(SRMClient::getInstance(usercfg, url.fullstr(), error));
    if (!client) {
      return DataStatus(DataStatus::CheckError, ECONNREFUSED, error);
    }

    SRMClientRequest srm_request_tmp(CanonicSRMURL(url));
    
    // first check permissions
    DataStatus res = client->checkPermissions(srm_request_tmp);

    if (!res && res.GetErrno() != EOPNOTSUPP) {
      return res;
    }
    if (check_meta) {
      logger.msg(VERBOSE, "Check: looking for metadata: %s", CurrentLocation().str());
      srm_request_tmp.long_list(true);
      std::list<struct SRMFileMetaData> metadata;

      res = client->info(srm_request_tmp, metadata);
      client = NULL;

      if (!res) return DataStatus(DataStatus::CheckError, res.GetErrno(), res.GetDesc());

      if (metadata.empty()) return DataStatus(DataStatus::CheckError, EARCRESINVAL, "No results returned");
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
      if (metadata.front().lastModificationTime > 0) {
        logger.msg(INFO, "Check: obtained modification date: %s", Time(metadata.front().lastModificationTime).str());
        SetModified(Time(metadata.front().lastModificationTime));
      }
      if (metadata.front().fileLocality == SRM_ONLINE) {
        logger.msg(INFO, "Check: obtained access latency: low (ONLINE)");
        SetAccessLatency(ACCESS_LATENCY_SMALL);
      }
      else if (metadata.front().fileLocality == SRM_NEARLINE) {
        logger.msg(INFO, "Check: obtained access latency: high (NEARLINE)");
        SetAccessLatency(ACCESS_LATENCY_LARGE);
      }
    }

    return DataStatus::Success;
  }

  DataStatus DataPointSRM::Remove() {

    std::string error;
    AutoPointer<SRMClient> client(SRMClient::getInstance(usercfg, url.fullstr(), error));
    if (!client) {
      return DataStatus(DataStatus::DeleteError, ECONNREFUSED, error);
    }

    // take out options in srm url and encode path
    SRMClientRequest srm_request_tmp(CanonicSRMURL(url));

    logger.msg(VERBOSE, "Remove: deleting: %s", CurrentLocation().str());

    DataStatus res = client->remove(srm_request_tmp);

    return res;
  }

  DataStatus DataPointSRM::CreateDirectory(bool with_parents) {

    std::string error;
    AutoPointer<SRMClient> client(SRMClient::getInstance(usercfg, url.fullstr(), error));
    if (!client) {
      return DataStatus(DataStatus::CreateDirectoryError, ECONNREFUSED, error);
    }

    // take out options in srm url and encode path
    SRMClientRequest request(CanonicSRMURL(url));

    logger.msg(VERBOSE, "Creating directory: %s", CanonicSRMURL(url));

    DataStatus res = client->mkDir(request);

    return res;
  }

  DataStatus DataPointSRM::Rename(const URL& newurl) {

    std::string error;
    AutoPointer<SRMClient> client(SRMClient::getInstance(usercfg, url.fullstr(), error));
    if (!client) {
      return DataStatus(DataStatus::RenameError, ECONNREFUSED, error);
    }

    // take out options in srm urls and encode paths
    SRMClientRequest request(CanonicSRMURL(url));
    URL canonic_newurl(CanonicSRMURL(newurl));

    logger.msg(VERBOSE, "Renaming %s to %s", CanonicSRMURL(url), canonic_newurl.str());

    DataStatus res = client->rename(request, canonic_newurl);

    return res;
  }

  DataStatus DataPointSRM::PrepareReading(unsigned int stage_timeout,
                                          unsigned int& wait_time) {
    if (writing) return DataStatus(DataStatus::IsWritingError, EARCLOGIC, "Already writing");
    if (reading && r_handle) return DataStatus(DataStatus::IsReadingError, EARCLOGIC, "Already reading");
    if (reading && !turls.empty()) return DataStatus::Success; // Already prepared

    reading = true;
    turls.clear();
    std::list<std::string> transport_urls;
    DataStatus res;
    std::string error;

    // choose transfer procotols
    std::list<std::string> transport_protocols;
    ChooseTransferProtocols(transport_protocols);

    // If the file is NEARLINE (on tape) bringOnline is called
    // Whether or not to do this should eventually be specified by the user
    if (access_latency == ACCESS_LATENCY_LARGE) {
      if (srm_request) {
        if (srm_request->status() != SRM_REQUEST_ONGOING) {
          // error, querying a request that was already prepared
          logger.msg(VERBOSE, "Calling PrepareReading when request was already prepared!");
          reading = false;
          return DataStatus(DataStatus::ReadPrepareError, EARCLOGIC, "File is already prepared");
        }
        AutoPointer<SRMClient> client(SRMClient::getInstance(usercfg, url.fullstr(), error));
        if (!client) {
          reading = false;
          return DataStatus(DataStatus::ReadPrepareError, ECONNREFUSED, error);
        }
        res = client->requestBringOnlineStatus(*srm_request);
      }
      // if no existing request, make a new request
      else {
        AutoPointer<SRMClient> client(SRMClient::getInstance(usercfg, url.fullstr(), error));
        if (!client) {
          return DataStatus(DataStatus::ReadPrepareError, ECONNREFUSED, error);
        }

        // take out options in srm url and encode path
        srm_request = new SRMClientRequest(CanonicSRMURL(url));
        logger.msg(INFO, "File %s is NEARLINE, will make request to bring online", CanonicSRMURL(url));
        srm_request->request_timeout(stage_timeout);
        res = client->requestBringOnline(*srm_request);
      }
      if (!res) return res;

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
        srm_request = NULL;
      }
      else {
        // bad logic - SRM_OK returned but request is not finished or on going
        logger.msg(VERBOSE, "Bad logic for %s - bringOnline returned ok but SRM request is not finished successfully or on going", url.str());
        return DataStatus(DataStatus::ReadPrepareError, EARCLOGIC, "Inconsistent status code from SRM");
      }
    }
    // Here we assume the file is in an ONLINE state
    // If a request already exists, query status
    if (srm_request) {
      if (srm_request->status() != SRM_REQUEST_ONGOING) {
        // error, querying a request that was already prepared
        logger.msg(VERBOSE, "Calling PrepareReading when request was already prepared!");
        return DataStatus(DataStatus::ReadPrepareError, EARCLOGIC, "File is already prepared");
      }
      AutoPointer<SRMClient> client(SRMClient::getInstance(usercfg, url.fullstr(), error));
      if (!client) {
        return DataStatus(DataStatus::ReadPrepareError, ECONNREFUSED, error);
      }
      res = client->getTURLsStatus(*srm_request, transport_urls);
    }
    // if no existing request, make a new request
    else {
      AutoPointer<SRMClient> client(SRMClient::getInstance(usercfg, url.fullstr(), error));
      if (!client) {
        return DataStatus(DataStatus::ReadPrepareError, ECONNREFUSED, error);
      }
      srm_request = NULL;

      CheckProtocols(transport_protocols);
      if (transport_protocols.empty()) {
        logger.msg(VERBOSE, "None of the requested transfer protocols are supported");
        return DataStatus(DataStatus::ReadPrepareError, EOPNOTSUPP, "None of the requested transfer protocols are supported");
      }
      srm_request = new SRMClientRequest(CanonicSRMURL(url));
      srm_request->request_timeout(stage_timeout);
      srm_request->transport_protocols(transport_protocols);
      res = client->getTURLs(*srm_request, transport_urls);
    }
    if (!res) return res;

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
        DataHandle redirected_handle(redirected_url, usercfg);

        // check if url can be handled
        if (!redirected_handle || !(*redirected_handle)) continue;
        if (redirected_handle->IsIndex()) continue;

        redirected_handle->AddURLOptions(url.Options());
        turls.push_back(redirected_handle->GetURL());
      }

      if (turls.empty()) {
        logger.msg(VERBOSE, "SRM returned no useful Transfer URLs: %s", url.str());
        srm_request->finished_abort();
        return DataStatus(DataStatus::ReadPrepareError, EARCRESINVAL, "No useful transfer URLs returned");
      }
    }
    else {
      // bad logic - SRM_OK returned but request is not finished or on going
      logger.msg(VERBOSE, "Bad logic for %s - getTURLs returned ok but SRM request is not finished successfully or on going", url.str());
      return DataStatus(DataStatus::ReadPrepareError, EARCLOGIC, "Inconsistent status code from SRM");
    }
    return DataStatus::Success;
  }

  DataStatus DataPointSRM::StartReading(DataBuffer& buf) {

    logger.msg(VERBOSE, "StartReading");
    if (!reading || turls.empty() || !srm_request || r_handle) {
      logger.msg(VERBOSE, "StartReading: File was not prepared properly");
      return DataStatus(DataStatus::ReadStartError, EARCLOGIC, "File was not prepared");
    }

    buffer = &buf;

    DataStatus r = SetupHandler(DataStatus::ReadStartError);
    if (!r)
       return r;

    logger.msg(INFO, "Redirecting to new URL: %s", (*r_handle)->CurrentLocation().str());
    r = (*r_handle)->StartReading(buf);
    if(!r) {
      r_handle = NULL;
    }
    return r;
  }

  DataStatus DataPointSRM::StopReading() {
    if (!reading) return DataStatus::Success;
    DataStatus r = DataStatus::Success;
    if (r_handle) {
      r = (*r_handle)->StopReading();
      r_handle = NULL;
    }
    return r;
  }

  DataStatus DataPointSRM::FinishReading(bool error) {
    if (!reading) return DataStatus::Success;
    StopReading();
    reading = false;

    if (srm_request) {
      std::string err;
      AutoPointer<SRMClient> client(SRMClient::getInstance(usercfg, url.fullstr(), err));
      // if the request finished with an error there is no need to abort or release request
      if (client && (srm_request->status() != SRM_REQUEST_FINISHED_ERROR)) {
        if (error || srm_request->status() == SRM_REQUEST_SHOULD_ABORT) {
          client->abort(*srm_request, true);
        } else if (srm_request->status() == SRM_REQUEST_FINISHED_SUCCESS) {
          client->releaseGet(*srm_request);
        }
      }
      srm_request = NULL;
    }
    turls.clear();

    return DataStatus::Success;
  }

  DataStatus DataPointSRM::PrepareWriting(unsigned int stage_timeout,
                                          unsigned int& wait_time) {
    if (reading) return DataStatus(DataStatus::IsReadingError, EARCLOGIC, "Already reading");
    if (writing && r_handle) return DataStatus(DataStatus::IsWritingError, EARCLOGIC, "Already writing");
    if (writing && !turls.empty()) return DataStatus::Success; // Already prepared

    writing = true;
    turls.clear();
    std::list<std::string> transport_urls;
    DataStatus res;
    std::string error;

    // choose transfer procotols
    std::list<std::string> transport_protocols;
    ChooseTransferProtocols(transport_protocols);

    // If a request already exists, query status
    if (srm_request) {
      if (srm_request->status() != SRM_REQUEST_ONGOING) {
        // error, querying a request that was already prepared
        logger.msg(VERBOSE, "Calling PrepareWriting when request was already prepared!");
        return DataStatus(DataStatus::WritePrepareError, EARCLOGIC, "File was already prepared");
      }
      AutoPointer<SRMClient> client(SRMClient::getInstance(usercfg, url.fullstr(), error));
      if (!client) {
        return DataStatus(DataStatus::WritePrepareError, ECONNREFUSED, error);
      }
      res = client->putTURLsStatus(*srm_request, transport_urls);
    }
    // if no existing request, make a new request
    else {
      AutoPointer<SRMClient> client(SRMClient::getInstance(usercfg, url.fullstr(), error));
      if (!client) {
        return DataStatus(DataStatus::WritePrepareError, ECONNREFUSED, error);
      }
      srm_request = NULL;

      CheckProtocols(transport_protocols);
      if (transport_protocols.empty()) {
        logger.msg(VERBOSE, "None of the requested transfer protocols are supported");
        return DataStatus(DataStatus::WritePrepareError, EOPNOTSUPP, "None of the requested transfer protocols are supported");
      }

      srm_request = new SRMClientRequest(CanonicSRMURL(url));
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
          // errors with space tokens now cause the transfer to fail - see bug 2061
          std::list<std::string> tokens;
          DataStatus token_res = client->getSpaceTokens(tokens, space_token);
          if (!token_res) {
            logger.msg(VERBOSE, "Error looking up space tokens matching description %s", space_token);
            return DataStatus(DataStatus::WritePrepareError, token_res.GetErrno(), "Error looking up space tokens matching description");
          }
          if (tokens.empty()) {
            logger.msg(VERBOSE, "No space tokens found matching description %s", space_token);
            srm_request = NULL;
            return DataStatus(DataStatus::WritePrepareError, EARCRESINVAL, "No space tokens found matching description");
          }
          // take the first one in the list
          logger.msg(VERBOSE, "Using space token %s", tokens.front());
          srm_request->space_token(tokens.front());
        }
      }
      srm_request->request_timeout(stage_timeout);
      if (CheckSize()) srm_request->total_size(GetSize());
      srm_request->transport_protocols(transport_protocols);
      res = client->putTURLs(*srm_request, transport_urls);
    }

    if (!res) return res;

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
        DataHandle redirected_handle(redirected_url, usercfg);

        // check if url can be handled
        if (!redirected_handle || !(*redirected_handle)) continue;
        if (redirected_handle->IsIndex()) continue;

        redirected_handle->AddURLOptions(url.Options());
        turls.push_back(redirected_handle->GetURL());
      }

      if (turls.empty()) {
        logger.msg(VERBOSE, "SRM returned no useful Transfer URLs: %s", url.str());
        srm_request->finished_abort();
        return DataStatus(DataStatus::WritePrepareError, EARCRESINVAL, "No useful transfer URLs returned");
      }
    }
    else {
      // bad logic - SRM_OK returned but request is not finished or on going
      logger.msg(VERBOSE, "Bad logic for %s - putTURLs returned ok but SRM request is not finished successfully or on going", url.str());
      return DataStatus(DataStatus::WritePrepareError, EARCLOGIC, "Inconsistent status code from SRM");
    }
    return DataStatus::Success;
  }

  DataStatus DataPointSRM::StartWriting(DataBuffer& buf,
                                        DataCallback *space_cb) {
    logger.msg(VERBOSE, "StartWriting");
    if (!writing || turls.empty() || !srm_request || r_handle) {
      logger.msg(VERBOSE, "StartWriting: File was not prepared properly");
      return DataStatus(DataStatus::WriteStartError, EARCLOGIC, "File was not prepared");
    }

    buffer = &buf;

    DataStatus r = SetupHandler(DataStatus::WriteStartError);
    if (!r)
       return r;

    logger.msg(INFO, "Redirecting to new URL: %s", (*r_handle)->CurrentLocation().str());
    r = (*r_handle)->StartWriting(buf);
    if(!r) {
      r_handle = NULL;
    }
    return r;
  }

  DataStatus DataPointSRM::StopWriting() {
    if (!writing) return DataStatus::Success;
    DataStatus r = DataStatus::Success;
    if (r_handle) {
      r = (*r_handle)->StopWriting();
      // check if checksum was verified at lower level
      if ((*r_handle)->CheckCheckSum()) SetCheckSum((*r_handle)->GetCheckSum());
      r_handle = NULL;
    }
    return r;
  }

  DataStatus DataPointSRM::FinishWriting(bool error) {
    if (!writing) return DataStatus::Success;
    StopWriting();
    writing = false;

    DataStatus r = DataStatus::Success;

    // if the request finished with an error there is no need to abort or release request
    if (srm_request) {
      std::string err;
      AutoPointer<SRMClient> client(SRMClient::getInstance(usercfg, url.fullstr(), err));
      if (client && (srm_request->status() != SRM_REQUEST_FINISHED_ERROR)) {
        // call abort if failure, or releasePut on success
        if (error || srm_request->status() == SRM_REQUEST_SHOULD_ABORT) {
           client->abort(*srm_request, false);
           // according to the spec the SURL may or may not exist after abort
           // so remove may fail, however it is not an error
           client->remove(*srm_request);
        }
        else {
          // checksum verification - if requested and not already done at lower level
          if (srm_request->status() == SRM_REQUEST_FINISHED_SUCCESS && additional_checks && buffer && !CheckCheckSum()) {
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
                DataStatus res = client->info(list_request,metadata);
                if (!res) {
                  client->abort(*srm_request, false); // if we can't list then we can't remove either
                  srm_request = NULL;
                  return DataStatus(DataStatus::WriteFinishError, res.GetErrno(), res.GetDesc());
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
                        logger.msg(VERBOSE, "Checksum mismatch between calculated/supplied checksum (%s) and checksum reported by SRM destination (%s)", csum, servercsum);
                        r = DataStatus(DataStatus::WriteFinishError, EARCCHECKSUM, "Checksum mismatch between calculated/supplied checksum and reported by SRM destination");
                      }
                    } else logger.msg(WARNING, "Checksum type of SRM (%s) and calculated/supplied checksum (%s) differ, cannot compare", servercsum, csum);
                  } else logger.msg(WARNING, "No checksum information from server");
                } else logger.msg(WARNING, "No checksum information from server");
              } else logger.msg(INFO, "No checksum verification possible");
            } else logger.msg(INFO, "No checksum verification possible");
          }
          if (r.Passed()) {
            if (srm_request->status() == SRM_REQUEST_FINISHED_SUCCESS) {
              DataStatus res = client->releasePut(*srm_request);
              if (!res) {
                logger.msg(VERBOSE, "Failed to release completed request");
                r = DataStatus(DataStatus::WriteFinishError, res.GetErrno(), "Failed to release completed request");
              }
            }
          } else {
            client->abort(*srm_request, false);
            // according to the spec the SURL may or may not exist after abort
            // so remove may fail, however it is not an error
            client->remove(*srm_request);
          }
        }
      }
      srm_request = NULL;
    }
    turls.clear();
    return r;
  }

  DataStatus DataPointSRM::Stat(FileInfo& file, DataPointInfoType verb) {
    std::list<FileInfo> files;
    std::list<DataPoint*> urls;
    urls.push_back(const_cast<DataPointSRM*> (this));
    DataStatus r = Stat(files, urls, verb);
    if (!r.Passed()) return r;
    file = files.front();
    return r;
  }

  DataStatus DataPointSRM::Stat(std::list<FileInfo>& files,
                                const std::list<DataPoint*>& urls,
                                DataPointInfoType verb) {

    if (urls.empty()) return DataStatus::Success;

    std::string error;
    AutoPointer<SRMClient> client(SRMClient::getInstance(usercfg, url.fullstr(), error));
    if(!client) {
      return DataStatus(DataStatus::StatError, ECONNREFUSED, error);
    }

    std::list<std::string> surls;
    for (std::list<DataPoint*>::const_iterator i = urls.begin(); i != urls.end(); ++i) {
      surls.push_back(CanonicSRMURL((*i)->CurrentLocation()));
      logger.msg(VERBOSE, "ListFiles: looking for metadata: %s", (*i)->CurrentLocation().str());
    }

    SRMClientRequest srm_request_tmp(surls);
    srm_request_tmp.recursion(-1);
    if ((verb | INFO_TYPE_NAME) != INFO_TYPE_NAME) srm_request_tmp.long_list(true);
    std::map<std::string, std::list<struct SRMFileMetaData> > metadata_map;

    // get info from SRM
    DataStatus res = client->info(srm_request_tmp, metadata_map);
    client = NULL;
    if (!res) return DataStatus(DataStatus::StatError, res.GetErrno(), res.GetDesc());

    for (std::list<DataPoint*>::const_iterator dp = urls.begin(); dp != urls.end(); ++dp) {

      std::string surl = CanonicSRMURL((*dp)->CurrentLocation());
      if (metadata_map.find(surl) == metadata_map.end()) {
        // error
        files.push_back(FileInfo());
        continue;
      }
      if (metadata_map[surl].size() != 1) {
        // error
        files.push_back(FileInfo());
        continue;
      }
      struct SRMFileMetaData srm_metadata = metadata_map[surl].front();

      // set URL attributes for surl requested (file or dir)
      if(srm_metadata.size > 0) {
        (*dp)->SetSize(srm_metadata.size);
      }
      if(srm_metadata.checkSumType.length() > 0 &&
         srm_metadata.checkSumValue.length() > 0) {
        std::string csum(srm_metadata.checkSumType+":"+srm_metadata.checkSumValue);
        (*dp)->SetCheckSum(csum);
      }
      if(srm_metadata.lastModificationTime > 0) {
        (*dp)->SetModified(Time(srm_metadata.lastModificationTime));
      }
      if(srm_metadata.fileLocality == SRM_ONLINE) {
        (*dp)->SetAccessLatency(ACCESS_LATENCY_SMALL);
      } else if(srm_metadata.fileLocality == SRM_NEARLINE) {
        (*dp)->SetAccessLatency(ACCESS_LATENCY_LARGE);
      }
      FillFileInfo(files, srm_metadata);
    }
    return DataStatus::Success;
  }

  DataStatus DataPointSRM::List(std::list<FileInfo>& files, DataPointInfoType verb) {
    return ListFiles(files,verb,0);
  }

  DataStatus DataPointSRM::ListFiles(std::list<FileInfo>& files, DataPointInfoType verb, int recursion) {
    // This method does not use any dynamic members of this object. Hence
    // it can be executed even while reading or writing

    std::string error;
    AutoPointer<SRMClient> client(SRMClient::getInstance(usercfg, url.fullstr(), error));
    if(!client) {
      return DataStatus(DataStatus::ListError, ECONNREFUSED, error);
    }

    SRMClientRequest srm_request_tmp(CanonicSRMURL(url));
    srm_request_tmp.recursion(recursion);

    logger.msg(VERBOSE, "ListFiles: looking for metadata: %s", CurrentLocation().str());
    if ((verb | INFO_TYPE_NAME) != INFO_TYPE_NAME) srm_request_tmp.long_list(true);
    std::list<struct SRMFileMetaData> srm_metadata;

    // get info from SRM
    DataStatus res = client->info(srm_request_tmp, srm_metadata);
    client = NULL;
    if (!res) return res;

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
    if(srm_metadata.front().lastModificationTime > 0) {
      SetModified(Time(srm_metadata.front().lastModificationTime));
    }
    if(srm_metadata.front().fileLocality == SRM_ONLINE) {
      SetAccessLatency(ACCESS_LATENCY_SMALL);
    } else if(srm_metadata.front().fileLocality == SRM_NEARLINE) {
      SetAccessLatency(ACCESS_LATENCY_LARGE);
    }
    // set FileInfo attributes for surl requested and any files within a dir
    for (std::list<struct SRMFileMetaData>::const_iterator i = srm_metadata.begin();
         i != srm_metadata.end(); ++i) {
      FillFileInfo(files, *i);
    }
    return DataStatus::Success;
  }

  const std::string DataPointSRM::DefaultCheckSum() const {
    return std::string("adler32");
  }

  bool DataPointSRM::ProvidesMeta() const {
    return true;
  }

  bool DataPointSRM::AcceptsMeta() const {
    // Handles metadata internally?
    return false;
  }

  bool DataPointSRM::IsStageable() const {
    return true;
  }

  bool DataPointSRM::WriteOutOfOrder() const {
    // Not sure if underlying data destination protocol will support
    // accepting out-of-order data. So simply say no.
    return false;
  }

  std::vector<URL> DataPointSRM::TransferLocations() const {
    return turls;
  }

  void DataPointSRM::ClearTransferLocations() {
    turls.clear();
  }

  bool DataPointSRM::SupportsTransfer() const {
    // Being optimistic. In worst case will return error in Transfer().
    return true;
  }

  DataStatus DataPointSRM::Transfer(const URL& otherendpoint, bool source, TransferCallback callback) {
    if (reading) return DataStatus(DataStatus::IsReadingError, EARCLOGIC, "Already reading");
    if (writing) return DataStatus(DataStatus::IsWritingError, EARCLOGIC, "Already writing");
    // TODO: replace simplified check with proper one
    DataStatus r;
    unsigned int wait_time(0);
    if (turls.empty()) {
      // Not ready for transfers yet.
      // TODO: Replace default timeout constant with proper value.
      if (source) {
        r = PrepareReading(300, wait_time);
      } else {
        r = PrepareWriting(300, wait_time);
      }
      // TODO: Handle case when more calls to *Preparing is needed.
      if (!r)
        return r;
    }
    r = SetupHandler(DataStatus::GenericError);
    if (!r) 
      return DataStatus(DataStatus::UnimplementedError, EOPNOTSUPP);
    bool supportsTransfer = (*r_handle)->SupportsTransfer();
    if (!supportsTransfer) {
      // Drop handle but keep results of *Preparing.
      r_handle = NULL;
      return DataStatus(DataStatus::UnimplementedError, EOPNOTSUPP);
    }
    r = (*r_handle)->Transfer(otherendpoint, source, callback);
    if (source) {
      FinishReading(!r);
    } else {
      FinishWriting(!r);
    }
    return r;
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
        logger.msg(WARNING, "plugin for transport protocol %s is not installed", *protocol);
        protocol = transport_protocols.erase(protocol);
      }
    }
  }

  void DataPointSRM::ChooseTransferProtocols(std::list<std::string>& transport_protocols) {

    std::string option_protocols(url.Option("transferprotocol"));
    if (option_protocols.empty()) {
      transport_protocols.push_back("gsiftp");
      transport_protocols.push_back("http");
      transport_protocols.push_back("https");
      transport_protocols.push_back("httpg");
      transport_protocols.push_back("ftp");
    } else {
      tokenize(option_protocols, transport_protocols, ",");
    }
  }

  std::string DataPointSRM::CanonicSRMURL(const URL& srm_url) {
    std::string canonic_url;
    std::string sfn_path = srm_url.HTTPOption("SFN");
    if (!sfn_path.empty()) {
      while (sfn_path[0] == '/') sfn_path.erase(0,1);
      canonic_url = srm_url.Protocol() + "://" + srm_url.Host() + "/" + uri_encode(sfn_path, false);
    } else {
      // if SFN option is not used, treat everything in the path including
      // options as part of the path and encode it
      canonic_url = srm_url.Protocol() + "://" + srm_url.Host() + uri_encode(srm_url.Path(), false);
      std::string extrapath;
      for (std::map<std::string, std::string>::const_iterator
           it = srm_url.HTTPOptions().begin(); it != srm_url.HTTPOptions().end(); it++) {
        if (it == srm_url.HTTPOptions().begin()) extrapath += '?';
        else extrapath += '&';
        extrapath += it->first;
        if (!it->second.empty()) extrapath += '=' + it->second;
      }
      canonic_url += uri_encode(extrapath, false);
    }
    return canonic_url;
  }

  void DataPointSRM::FillFileInfo(std::list<FileInfo>& files, const struct SRMFileMetaData& srm_metadata) {
    // set FileInfo attributes
    std::list<FileInfo>::iterator f = files.insert(files.end(), FileInfo(srm_metadata.path));

    if (srm_metadata.fileType == SRM_FILE) {
      f->SetType(FileInfo::file_type_file);
    }
    else if (srm_metadata.fileType == SRM_DIRECTORY) {
      f->SetType(FileInfo::file_type_dir);
    }
    if (srm_metadata.size >= 0) {
      f->SetSize(srm_metadata.size);
    }
    if (srm_metadata.lastModificationTime > 0) {
      f->SetModified(Time(srm_metadata.lastModificationTime));
    }
    if (srm_metadata.checkSumType.length() > 0 &&
        srm_metadata.checkSumValue.length() > 0) {
      std::string csum(srm_metadata.checkSumType + ":" + srm_metadata.checkSumValue);
      f->SetCheckSum(csum);
    }
    if (srm_metadata.fileLocality == SRM_ONLINE) {
      f->SetLatency("ONLINE");
    }
    else if (srm_metadata.fileLocality == SRM_NEARLINE) {
      f->SetLatency("NEARLINE");
    }
    if (srm_metadata.createdAtTime > 0) {
      f->SetMetaData("ctime", (Time(srm_metadata.createdAtTime)).str());
    }
    if (!srm_metadata.spaceTokens.empty()) {
      std::string spaceTokens;
      for (std::list<std::string>::const_iterator it = srm_metadata.spaceTokens.begin();
           it != srm_metadata.spaceTokens.end(); it++) {
        if (!spaceTokens.empty()) spaceTokens += ',';
        spaceTokens += *it;
      }
      f->SetMetaData("spacetokens", spaceTokens);
    }
    if (!srm_metadata.owner.empty()) f->SetMetaData("owner", srm_metadata.owner);
    if (!srm_metadata.group.empty()) f->SetMetaData("group", srm_metadata.group);
    if (!srm_metadata.permission.empty()) f->SetMetaData("accessperm", srm_metadata.permission);
    if (srm_metadata.lifetimeLeft != 0) f->SetMetaData("lifetimeleft", tostring(srm_metadata.lifetimeLeft));
    if (srm_metadata.lifetimeAssigned != 0) f->SetMetaData("lifetimeassigned", tostring(srm_metadata.lifetimeAssigned));

    if (srm_metadata.retentionPolicy == SRM_REPLICA) f->SetMetaData("retentionpolicy", "REPLICA");
    else if (srm_metadata.retentionPolicy == SRM_OUTPUT) f->SetMetaData("retentionpolicy", "OUTPUT");
    else if (srm_metadata.retentionPolicy == SRM_CUSTODIAL)  f->SetMetaData("retentionpolicy", "CUSTODIAL");

    if (srm_metadata.fileStorageType == SRM_VOLATILE) f->SetMetaData("filestoragetype", "VOLATILE");
    else if (srm_metadata.fileStorageType == SRM_DURABLE) f->SetMetaData("filestoragetype", "DURABLE");
    else if (srm_metadata.fileStorageType == SRM_PERMANENT) f->SetMetaData("filestoragetype", "PERMANENT");
  }

} // namespace ArcDMCSRM

extern Arc::PluginDescriptor const ARC_PLUGINS_TABLE_NAME[] = {
  { "srm", "HED:DMC", "Storage Resource Manager", 0, &ArcDMCSRM::DataPointSRM::Instance },
  { NULL, NULL, NULL, 0, NULL }
};
