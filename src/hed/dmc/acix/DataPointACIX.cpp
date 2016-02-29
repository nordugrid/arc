#include <arc/communication/ClientInterface.h>
#include <arc/message/MCC.h>
#include <arc/message/PayloadRaw.h>

#include <arc/external/cJSON/cJSON.h>

#include "DataPointACIX.h"

namespace ArcDMCACIX {

  using namespace Arc;

  Arc::Logger DataPointACIX::logger(Arc::Logger::getRootLogger(), "DataPoint.ACIX");

  // Copied from DataPointHTTP. Should be put in common place
  static int http2errno(int http_code) {
    // Codes taken from RFC 2616 section 10. Only 4xx and 5xx are treated as errors
    switch(http_code) {
      case 400:
      case 405:
      case 411:
      case 413:
      case 414:
      case 415:
      case 416:
      case 417:
        return EINVAL;
      case 401:
      case 403:
      case 407:
        return EACCES;
      case 404:
      case 410:
        return ENOENT;
      case 406:
      case 412:
        return EARCRESINVAL;
      case 408:
        return ETIMEDOUT;
      case 409: // Conflict. Not sure about this one.
      case 500:
      case 502:
      case 503:
      case 504:
        return EARCSVCTMP;
      case 501:
      case 505:
        return EOPNOTSUPP;

      default:
        return EARCOTHER;
    }
  }

  DataPointACIX::DataPointACIX(const URL& url, const UserConfig& usercfg, PluginArgument* parg)
    : DataPointIndex(url, usercfg, parg), original_location_resolved(false) {}

  DataPointACIX::~DataPointACIX() {}

  Plugin* DataPointACIX::Instance(PluginArgument *arg) {
    DataPointPluginArgument *dmcarg =
      dynamic_cast<DataPointPluginArgument*>(arg);
    if (!dmcarg)
      return NULL;
    if (((const URL&)(*dmcarg)).Protocol() != "acix")
      return NULL;
    // Change URL protocol to https and reconstruct URL so HTTP options are parsed
    std::string acix_url(((const URL&)(*dmcarg)).fullstr());
    acix_url.replace(0, 4, "https");
    return new DataPointACIX(URL(acix_url), *dmcarg, arg);
  }

  DataStatus DataPointACIX::Check(bool check_meta) {
    // If original location is set, check that
    if (original_location) {
      DataHandle h(original_location, usercfg);
      DataStatus r = h->Check(check_meta);
      if (!r) return r;
      SetMeta(*h);
      return DataStatus::Success;
    }
    // If not simply check that the file can be resolved
    DataStatus r = Resolve(true);
    if (r) return r;
    return DataStatus(DataStatus::CheckError, r.GetErrno(), r.GetDesc());
  }

  DataStatus DataPointACIX::Resolve(bool source) {
    std::list<DataPoint*> urls(1, const_cast<DataPointACIX*> (this));
    DataStatus r = Resolve(source, urls);
    if (!r) return r;
    if (!HaveLocations()) {
      logger.msg(VERBOSE, "No locations found for %s", url.str());
      return DataStatus(DataStatus::ReadResolveError, ENOENT, "No valid locations found");
    }
    return DataStatus::Success;
  }

  DataStatus DataPointACIX::Resolve(bool source, const std::list<DataPoint*>& urls) {
    // Contact ACIX to resolve cached replicas. Also resolve original replica
    // and add those locations to locations.
    if (!source) return DataStatus(DataStatus::WriteResolveError, ENOTSUP, "Writing to ACIX is not supported");
    if (urls.empty()) return DataStatus::Success;

    // Resolving each original source can take a long time and exceed the DTR
    // processor timeout (see bug 3511). As a workaround limit the time spent
    // in this method to 30 mins. TODO: resolve original sources in bulk
    Time start_time;

    // Construct acix query URL, giving all urls as option. Assumes only one
    // URL is specified in each datapoint.
    std::list<std::string> urllist;
    for (std::list<DataPoint*>::const_iterator i = urls.begin(); i != urls.end(); ++i) {

      // This casting is needed to access url directly, as GetURL() returns
      // original_location
      DataPointACIX* dp = dynamic_cast<DataPointACIX*>(*i);
      URL lookupurl(uri_unencode(dp->url.HTTPOption("url")));

      if (!lookupurl || lookupurl.str().find(',') != std::string::npos) {
        logger.msg(ERROR, "Found none or multiple URLs (%s) in ACIX URL: %s", lookupurl.str(), dp->url.str());
        return DataStatus(DataStatus::ReadResolveError, EINVAL, "Invalid URLs specified");
      }
      urllist.push_back(lookupurl.str());

      // Now resolve original replica if any
      if (dp->original_location) {

        DataHandle origdp(dp->original_location, dp->usercfg);
        if (!origdp) {
          logger.msg(ERROR, "Cannot handle URL %s", dp->original_location.str());
          return DataStatus(DataStatus::ReadResolveError, EINVAL, "Invalid URL");
        }
        // If index resolve the location and add replicas to this datapoint
        if (origdp->IsIndex()) {
          // Check we are within the time limit;
          if (Time() > Time(start_time + 1800)) {
            logger.msg(WARNING, "Could not resolve original source of %s: out of time", lookupurl.str());
          } else {
            DataStatus res = origdp->Resolve(true);
            if (!res) {
              // Just log a warning and continue. One of the main use cases of ACIX
              // is as a fallback when the original source is not available
              logger.msg(WARNING, "Could not resolve original source of %s: %s", lookupurl.str(), std::string(res));
            } else {
              // Add replicas found from resolving original replica
              for (; origdp->LocationValid(); origdp->NextLocation()) {
                dp->AddLocation(origdp->CurrentLocation(), origdp->CurrentLocation().ConnectionURL());
              }
            }
          }
        } else {
          dp->AddLocation(dp->original_location, dp->original_location.ConnectionURL());
        }
      }
      dp->original_location_resolved = true;
    }
    URL queryURL(url);
    queryURL.AddHTTPOption("url", Arc::join(urllist, ","), true);
    logger.msg(INFO, "Querying ACIX server at %s", queryURL.ConnectionURL());
    logger.msg(DEBUG, "Calling acix with query %s", queryURL.plainstr());

    std::string content;
    DataStatus res = queryACIX(content, queryURL.FullPath());
    // It should not be an error when ACIX is unavailable
    if (!res) {
      logger.msg(WARNING, "Failed to query ACIX: %s", std::string(res));
    } else {
      res = parseLocations(content, urls);
      if (!res) {
        logger.msg(WARNING, "Failed to parse ACIX response: %s", std::string(res));
      }
    }
    return DataStatus::Success;
  }

  DataStatus DataPointACIX::Stat(FileInfo& file, DataPoint::DataPointInfoType verb) {
    std::list<FileInfo> files;
    std::list<DataPoint*> urls(1, const_cast<DataPointACIX*> (this));
    DataStatus r = Stat(files, urls, verb);
    if (!r) {
      return r;
    }
    if (files.empty() || !files.front()) {
      return DataStatus(DataStatus::StatError, EARCRESINVAL, "No results returned");
    }
    file = files.front();
    return DataStatus::Success;
  }

  DataStatus DataPointACIX::Stat(std::list<FileInfo>& files,
                                const std::list<DataPoint*>& urls,
                                DataPointInfoType verb) {
    files.clear();
    DataStatus r = Resolve(true, urls);
    if (!r) {
      return DataStatus(DataStatus::StatError, r.GetErrno(), r.GetDesc());
    }
    for (std::list<DataPoint*>::const_iterator f = urls.begin(); f != urls.end(); ++f) {
      FileInfo info;
      if ((*f)->HaveLocations()) {
        // Only name and replicas are available
        info.SetName((*f)->GetURL().HTTPOption("url"));
        for (; (*f)->LocationValid(); (*f)->NextLocation()) {
          info.AddURL((*f)->CurrentLocation());
        }
      }
      files.push_back(info);
    }
    return DataStatus::Success;
  }

  DataStatus DataPointACIX::PreRegister(bool replication, bool force) {
    return DataStatus(DataStatus::PreRegisterError, ENOTSUP, "Writing to ACIX is not supported");
  }

  DataStatus DataPointACIX::PostRegister(bool replication) {
    return DataStatus(DataStatus::PostRegisterError, ENOTSUP, "Writing to ACIX is not supported");
  }

  DataStatus DataPointACIX::PreUnregister(bool replication) {
    return DataStatus(DataStatus::UnregisterError, ENOTSUP, "Deleting from ACIX is not supported");
  }

  DataStatus DataPointACIX::Unregister(bool all) {
    return DataStatus(DataStatus::UnregisterError, ENOTSUP, "Deleting from ACIX is not supported");
  }

  DataStatus DataPointACIX::List(std::list<FileInfo>& files, DataPoint::DataPointInfoType verb) {
    return DataStatus(DataStatus::ListError, ENOTSUP, "Listing in ACIX is not supported");
  }

  DataStatus DataPointACIX::CreateDirectory(bool with_parents) {
    return DataStatus(DataStatus::CreateDirectoryError, ENOTSUP, "Creating directories in ACIX is not supported");
  }

  DataStatus DataPointACIX::Rename(const URL& newurl) {
    return DataStatus(DataStatus::RenameError, ENOTSUP, "Renaming in ACIX is not supported");
  }

  DataStatus DataPointACIX::AddLocation(const URL& urlloc,
                                        const std::string& meta) {
    if (!original_location && !original_location_resolved) {
      original_location = URLLocation(urlloc);
      // Add any URL options to the acix URL
      for (std::map<std::string, std::string>::const_iterator opt = original_location.Options().begin();
           opt != original_location.Options().end(); ++opt) {
        url.AddOption(opt->first, opt->second);
      }
      return DataStatus::Success;
    }
    return DataPointIndex::AddLocation(urlloc, meta);
  }

  const URL& DataPointACIX::GetURL() const {
    if (original_location) return original_location;
    return url;
  }

  std::string DataPointACIX::str() const {
    if (original_location) return original_location.str();
    return url.str();
  }

  DataStatus DataPointACIX::queryACIX(std::string& content,
                                      const std::string& path) const {

    MCCConfig cfg;
    usercfg.ApplyToConfig(cfg);
    ClientHTTP client(cfg, url, usercfg.Timeout());
    client.RelativeURI(true); // twisted (ACIX server) doesn't support GET with full URL

    HTTPClientInfo transfer_info;
    PayloadRaw request;
    PayloadRawInterface *response = NULL;

    MCC_Status r = client.process("GET", path, &request, &transfer_info, &response);

    if (!r) {
      return DataStatus(DataStatus::ReadResolveError, "Failed to contact server: " + r.getExplanation());
    }
    if (transfer_info.code != 200) {
      return DataStatus(DataStatus::ReadResolveError, http2errno(transfer_info.code), "HTTP error when contacting server: %s" + transfer_info.reason);
    }
    PayloadStreamInterface* instream = NULL;
    try {
      instream = dynamic_cast<PayloadStreamInterface*>(dynamic_cast<MessagePayload*>(response));
    } catch(std::exception& e) {
      return DataStatus(DataStatus::ReadResolveError, "Unexpected response from server");
    }
    if (!instream) {
      return DataStatus(DataStatus::ReadResolveError, "Unexpected response from server");
    }
    content.clear();
    std::string buf;
    while (instream->Get(buf)) content += buf;

    logger.msg(DEBUG, "ACIX returned %s", content);
    return DataStatus::Success;
  }

  DataStatus DataPointACIX::parseLocations(const std::string& content,
                                           const std::list<DataPoint*>& urls) const {

    // parse JSON {url: [loc1, loc2,...]}
    cJSON *root = cJSON_Parse(content.c_str());
    if (!root) {
      logger.msg(ERROR, "Failed to parse ACIX response: %s", content);
      return DataStatus(DataStatus::ReadResolveError, "Failed to parse ACIX response");
    }
    for (std::list<DataPoint*>::const_iterator i = urls.begin(); i != urls.end(); ++i) {
      // This casting is needed to access url directly, as GetURL() returns
      // original_location
      DataPointACIX* dp = dynamic_cast<DataPointACIX*>(*i);
      std::string urlstr = URL(uri_unencode(dp->url.HTTPOption("url"))).str();

      cJSON *urlinfo = cJSON_GetObjectItem(root, urlstr.c_str());
      if (!urlinfo) {
        logger.msg(WARNING, "No locations for %s", urlstr);
        continue;
      }
      cJSON *locinfo = urlinfo->child;
      while (locinfo) {
        std::string loc = std::string(locinfo->valuestring);
        logger.msg(INFO, "%s: ACIX Location: %s", urlstr, loc);
        if (loc.find("://") == std::string::npos) {
          logger.msg(INFO, "%s: Location %s not accessible remotely, skipping", urlstr, loc);
        } else {
          URL fullloc(loc + '/' + urlstr);
          // Add URL options to replicas
          for (std::map<std::string, std::string>::const_iterator opt = dp->url.CommonLocOptions().begin();
               opt != dp->url.CommonLocOptions().end(); opt++)
            fullloc.AddOption(opt->first, opt->second, false);
          for (std::map<std::string, std::string>::const_iterator opt = dp->url.Options().begin();
               opt != dp->url.Options().end(); opt++)
            fullloc.AddOption(opt->first, opt->second, false);
          dp->AddLocation(fullloc, loc);
        }
        locinfo = locinfo->next;
      }
      if (!dp->HaveLocations()) {
        logger.msg(WARNING, "No locations found for %s", dp->url.str());
      }
    }
    cJSON_Delete(root);
    return DataStatus::Success;
  }

} // namespace ArcDMCACIX

extern Arc::PluginDescriptor const ARC_PLUGINS_TABLE_NAME[] = {
  { "acix", "HED:DMC", "ARC Cache Index", 0, &ArcDMCACIX::DataPointACIX::Instance },
  { NULL, NULL, NULL, 0, NULL }
};
