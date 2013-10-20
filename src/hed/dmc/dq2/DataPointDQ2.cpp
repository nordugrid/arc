#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <openssl/evp.h> // For rucio hashing algorithm

#include <arc/FileUtils.h>
#include <arc/communication/ClientInterface.h>
#include <arc/data/DataBuffer.h>
#include <arc/message/MCC.h>
#include <arc/message/PayloadRaw.h>

#include "cJSON/cJSON.h"

#include "DataPointDQ2.h"

namespace ArcDMCDQ2 {

  using namespace Arc;

  Logger DataPointDQ2::logger(Logger::getRootLogger(), "DataPoint.DQ2");
  DataPointDQ2::DQ2Cache DataPointDQ2::dq2_cache;
  Glib::Mutex DataPointDQ2::dq2_cache_lock;

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

  /// Parse the response from MCC process()
  static DataStatus parseHTTPResponse(std::string& content,
                                      const MCC_Status& status,
                                      const HTTPClientInfo& info,
                                      PayloadRawInterface *response) {
    if (!status) {
      return DataStatus(DataStatus::ReadResolveError, "Failed to contact server: " + status.getExplanation());
    }
    if (info.code != 200) {
      return DataStatus(DataStatus::ReadResolveError, http2errno(info.code), "HTTP error when contacting server: %s" + info.reason);
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
    return DataStatus::Success;
  }

  DataPointDQ2::DataPointDQ2(const URL& url, const UserConfig& usercfg, PluginArgument* parg)
    : DataPointIndex(url, usercfg, parg) {
    // Connection URL is DQ2 catalog host:port
    catalog = "http://"+url.Host()+':'+tostring(url.Port())+'/';
    // Dataset is first part of path
    dataset = url.Path().substr(1, url.Path().find('/', 1)-1);
    // Scope is first part of dataset
    std::list<std::string> dataset_parts;
    Arc::tokenize(dataset, dataset_parts, ".");
    if (dataset_parts.size() < 3) {
      logger.msg(ERROR, "Invalid dataset name: %s", dataset);
      return;
    }
    scope = dataset_parts.front();
    // user and group have the second part in the scope
    if (scope == "user" || scope == "group") {
      dataset_parts.pop_front();
      scope += "." + dataset_parts.front();
    }
    // LFN is last part of path
    lfn = url.Path().substr(url.Path().rfind('/')+1);
    // Check lifetime of cached DQ2 info
    Glib::Mutex::Lock l(dq2_cache_lock);
    if (Arc::Time() >= dq2_cache.location_expiration) {
      dq2_cache.dataset_locations.clear();
      dq2_cache.location_expiration = Arc::Time() + dq2_cache.location_validity;
    }
  }

  DataPointDQ2::~DataPointDQ2() {}

  Plugin* DataPointDQ2::Instance(PluginArgument *arg) {
    DataPointPluginArgument *dmcarg = dynamic_cast<DataPointPluginArgument*>(arg);
    if (!dmcarg)
      return NULL;
    if (((const URL&)(*dmcarg)).Protocol() != "dq2")
      return NULL;
    // path must contain dataset/LFN
    if (((const URL&)(*dmcarg)).Path().find('/', 1) == std::string::npos) {
      logger.msg(ERROR, "Invalid DQ2 URL %s", ((const URL&)(*dmcarg)).str());
      return NULL;
    }
    return new DataPointDQ2(*dmcarg, *dmcarg, dmcarg);
  }
  

  DataStatus DataPointDQ2::Resolve(bool source) {

    // TODO: extract nickname from proxy to use as account parameter in requests
    std::list<std::string> sites;
    DataStatus res = resolveLocations(sites);
    if (!res) return res;
    if (sites.empty()) return DataStatus(DataStatus::ReadResolveError, ENOENT, "Dataset has no locations");

    // Get endpoints
    AGISInfo* agis = AGISInfo::getInstance(usercfg.Timeout(), "/tmp/agis-info");
    if (!agis) {
      logger.msg(ERROR, "Could not obtain information from AGIS");
      return DataStatus(DataStatus::ReadResolveError, "Could not obtain information from AGIS");
    }
    std::list<std::string> endpoints = agis->getStorageEndpoints(sites);

    if (endpoints.empty()) {
      logger.msg(ERROR, "No suitable endpoints found in AGIS");
      return DataStatus(DataStatus::ReadResolveError, ENOENT, "No suitable endpoints found in AGIS");
    }

    // Make deterministic physical names
    makePaths(endpoints);

    return DataStatus::Success;
  }

  DataStatus DataPointDQ2::Resolve(bool source, const std::list<DataPoint*>& urls) {

    if (!source) return DataStatus(DataStatus::WriteResolveError, ENOTSUP);
    // Just call resolve for each url. Results are cached so DQ2 should be
    // called only once per dataset.
    for (std::list<DataPoint*>::const_iterator dp = urls.begin(); dp != urls.end(); ++dp) {
      DataStatus res = (*dp)->Resolve(source);
      if (!res) return res;
    }
    return DataStatus::Success;
  }

  DataStatus DataPointDQ2::Check(bool check_meta) {
    // TODO: look up file size and checksum
    // TODO: Check permissions by looking for atlas VOMS extension?
    return DataStatus::Success;
  }

  DataStatus DataPointDQ2::Stat(FileInfo& file, DataPoint::DataPointInfoType verb) {
    // TODO: look up file size and checksum
    file.SetName(lfn);
    return DataStatus::Success;
  }

  DataStatus DataPointDQ2::Stat(std::list<FileInfo>& files,
                                const std::list<DataPoint*>& urls,
                                DataPoint::DataPointInfoType verb) {

    std::list<FileInfo>::iterator f = files.begin();
    for (std::list<DataPoint*>::const_iterator dp = urls.begin(); dp != urls.end(); ++dp, ++f) {
      DataStatus res = (*dp)->Stat(*f, verb);
      if (!res) return res;
    }
    return DataStatus::Success;
  }

  DataStatus DataPointDQ2::resolveLocations(std::list<std::string>& sites) {

    // Check if locations are cached
    {
      Glib::Mutex::Lock l(dq2_cache_lock);
      if (dq2_cache.dataset_locations.find(dataset) != dq2_cache.dataset_locations.end()) {
        logger.msg(DEBUG, "Locations of dataset %s are cached", dataset);
        sites = dq2_cache.dataset_locations[dataset];
        return DataStatus::Success;
      }
    }
    std::string duid;
    // Get duid from dataset name if not cached
    if (dq2_cache.dataset_duid.find(dataset) == dq2_cache.dataset_duid.end()) {
      // Query dataset to get duid
      std::string path("/dq2/ws_repository/rpc?operation=queryDatasetByName&dsn="+dataset+"&API=0_3_0&appid=arc");
      std::string content;
      DataStatus res = queryDQ2(content, "GET", path);
      if (!res) return res;

      cJSON *root = cJSON_Parse(content.c_str());
      if (!root) {
        logger.msg(ERROR, "Failed to parse DQ2 response: %s", content);
        return DataStatus(DataStatus::ReadResolveError, "Failed to parse DQ2 response");
      }
      cJSON *datasetinfo = cJSON_GetObjectItem(root, dataset.c_str());
      if (!datasetinfo) {
        logger.msg(ERROR, "No such dataset: %s", dataset);
        return DataStatus(DataStatus::ReadResolveError, ENOENT, "No such dataset");
      }
      cJSON *duidinfo = cJSON_GetObjectItem(datasetinfo, "duid");
      if (!duidinfo) {
        logger.msg(ERROR, "Malformed DQ2 response: %s", content);
        return DataStatus(DataStatus::ReadResolveError, "DQ2 returned a malformed response");
      }
      duid = duidinfo->valuestring;
      cJSON_Delete(root);
      dq2_cache.dataset_duid[dataset] = duid;
    } else {
      duid = dq2_cache.dataset_duid[dataset];
    }
    logger.msg(DEBUG, "Dataset %s: DUID %s", dataset, duid);

    // Query duid locations (POST)
    std::string path("/dq2/ws_location/rpc");
    std::string post_data("operation=listDatasetReplicas&API=0_3_0&appid=arc&duid="+duid);
    std::string content;
    DataStatus res = queryDQ2(content, "POST", path, post_data);

    cJSON *root = cJSON_Parse(content.c_str());
    if (!root) {
      logger.msg(ERROR, "Failed to parse DQ2 response: %s", content);
      return DataStatus(DataStatus::ReadResolveError, "Failed to parse DQ2 response");
    }
    // Locations are root children
    cJSON *subitem=root->child;
    while (subitem) {
      sites.push_back(subitem->string);
      logger.msg(DEBUG, "Location: %s", sites.back());
      subitem = subitem->next;
    }
    cJSON_Delete(root);

    // Add to location cache
    Glib::Mutex::Lock l(dq2_cache_lock);
    dq2_cache.dataset_locations[dataset] = sites;
    return DataStatus::Success;
  }

  DataStatus DataPointDQ2::queryDQ2(std::string& content,
                                    const std::string& method,
                                    const std::string& path,
                                    const std::string& post_data) const {

    MCCConfig cfg;
    usercfg.ApplyToConfig(cfg);
    ClientHTTP client(cfg, catalog, usercfg.Timeout());

    // Must set User-Agent: dqcurl header to get json output
    std::multimap<std::string, std::string> attrmap;
    attrmap.insert(std::pair<std::string, std::string>("User-Agent", "dqcurl"));
    ClientHTTPAttributes attrs(method, path, attrmap);
    HTTPClientInfo transfer_info;
    PayloadRaw request;
    PayloadRawInterface *response = NULL;

    // POST data if applicable
    if (method == "POST" && !post_data.empty()) request.Insert(post_data.c_str());

    MCC_Status r = client.process(attrs, &request, &transfer_info, &response);
    DataStatus ds = parseHTTPResponse(content, r, transfer_info, response);
    if (!ds) return ds;

    // cJSON parser expects strings are enclosed in double quotes and cannot handle None
    std::replace(content.begin(), content.end(), '\'', '"');
    while (content.find("None") != std::string::npos) {
      content.replace(content.find("None"), 4, "");
    }

    logger.msg(DEBUG, "DQ2 returned %s", content);
    return DataStatus::Success;
  }

  void DataPointDQ2::makePaths(const std::list<std::string>& endpoints) {

    // Construct path in Rucio convention explained here
    // https://twiki.cern.ch/twiki/bin/viewauth/AtlasComputing/DDMRucioPhysicalFileName
    // rucio/scope/L1/L2/LFN where L1/L2 are the first 2 bytes of MD5(scope:LFN)

    std::string path("rucio/" + scope + "/");
    std::string hash(scope + ":" + lfn);

    // Taken from FileCacheHash and changed to use md5
    EVP_MD_CTX mdctx;
    const EVP_MD *md = EVP_md5(); // change to EVP_sha1() for sha1 hashes
    char *mess1 = (char*)hash.c_str();
    unsigned char md_value[EVP_MAX_MD_SIZE];
    unsigned int md_len;

    EVP_MD_CTX_init(&mdctx);
    EVP_DigestInit_ex(&mdctx, md, NULL);
    EVP_DigestUpdate(&mdctx, mess1, strlen(mess1));
    EVP_DigestFinal_ex(&mdctx, md_value, &md_len);
    EVP_MD_CTX_cleanup(&mdctx);

    char result[3];
    snprintf(result, 3, "%02x", md_value[0]);
    logger.msg(DEBUG, result);
    path.append(result);
    path += "/";
    snprintf(result, 3, "%02x", md_value[1]);
    logger.msg(DEBUG, result);
    path.append(result);
    path += "/" + lfn;

    for (std::list<std::string>::const_iterator endpoint = endpoints.begin();
         endpoint != endpoints.end(); ++endpoint) {
      std::string location(*endpoint + path);
      logger.msg(DEBUG, "%s: Adding location %s", lfn, location);
      if (AddLocation(URL(location), url.ConnectionURL()) == DataStatus::LocationAlreadyExistsError) {
        logger.msg(WARNING, "Duplicate location of file %s", lfn);
      }
    }

    // TODO: get meta information (size, checksum) for validation with replicas
  }


  AGISInfo * AGISInfo::instance = NULL;
  Glib::Mutex AGISInfo::lock;
  Logger AGISInfo::logger(Logger::getRootLogger(), "DataPoint.DQ2.AGISInfo");
  const Arc::Period AGISInfo::info_lifetime(360000); // 1 hour lifetime hard-coded

  AGISInfo::AGISInfo(int timeout, const std::string& cache_file)
    : cache_file(cache_file),
      expiry_time(Arc::Time() + info_lifetime),
      timeout(timeout) {
    valid = getAGISInfo();
  }

  AGISInfo::~AGISInfo() {
    if (instance) delete instance;
  }

  AGISInfo * AGISInfo::getInstance(int timeout, const std::string& cache_file) {
    Glib::Mutex::Lock l(lock);
    if (instance) {
      if (Arc::Time() > instance->expiry_time) {
        // refresh info
        instance->getAGISInfo();
      }
    } else {
      instance = new AGISInfo(timeout, cache_file);
      if (!instance->valid) {
        delete instance;
        instance = NULL;
      }
    }
    return instance;
  }

  std::list<std::string> AGISInfo::getStorageEndpoints(const std::list<std::string>& sites) {
    Glib::Mutex::Lock l(lock);
    std::list<std::string> endpoints;
    for (std::list<std::string>::const_iterator site = sites.begin(); site != sites.end(); ++site) {
      if (endpoint_info.find(*site) != endpoint_info.end()) {
        endpoints.push_back(endpoint_info[*site]);
      } else if (std::find(nondeterministic_sites.begin(), nondeterministic_sites.end(), *site) != nondeterministic_sites.end()) {
        logger.msg(WARNING, "Site %s is not deterministic and cannot be used", *site);
      } else {
        logger.msg(WARNING, "Site %s not found in AGIS info", *site);
      }
    }
    return endpoints;
  }

  bool AGISInfo::getAGISInfo() {
    // Get from cached file if available
    if (!cache_file.empty()) {
      std::string data;
      logger.msg(VERBOSE, "Reading cached AGIS data from %s", cache_file);
      if (FileRead(cache_file, data)) {
        return parseAGISInfo(data);
      }
      logger.msg(VERBOSE, "Cannot read cached AGIS info from %s, will re-download", cache_file);
    }
    return parseAGISInfo(downloadAGISInfo());
  }

  std::string AGISInfo::downloadAGISInfo() {

    std::string content;
    std::string agis_url("http://atlas-agis-api.cern.ch/request/ddmendpoint/query/list/?json");
    MCCConfig cfg;
    ClientHTTP client(cfg, agis_url, timeout);

    HTTPClientInfo transfer_info;
    PayloadRaw request;
    PayloadRawInterface *response = NULL;

    MCC_Status r = client.process("GET", &request, &transfer_info, &response);
    DataStatus ds = parseHTTPResponse(content, r, transfer_info, response);
    if (!ds) {
      if (!endpoint_info.empty()) {
        logger.msg(WARNING, "Could not refresh AGIS info, cached version will be used: %s", ds.GetDesc());
      } else {
        logger.msg(ERROR, "Could not download AGIS info: %s", ds.GetDesc());
      }
      return content;
    }

    // AGIS behaves and returns proper json
    logger.msg(DEBUG, "AGIS returned %s", content);

    // Write to file if necessary - FileLock is not necessary because threads
    // are already locked here
    if (!cache_file.empty()) {
      if (!FileCreate(cache_file, content)) logger.msg(WARNING, "Could not create file %s", cache_file);
    }

    return content;
  }

  bool AGISInfo::parseAGISInfo(const std::string& content) {

    cJSON *root = cJSON_Parse(content.c_str());
    if (!root) {
      logger.msg(ERROR, "Failed to parse AGIS response, error at %s: %s", cJSON_GetErrorPtr(), content);
      return DataStatus(DataStatus::ReadResolveError, "Failed to parse AGIS response");
    }
    // schema is [{"name": sitename, "aprotocols:{"r": [ [endpoint, prio, path], ]}},]
    cJSON *subitem=root->child;
    while (subitem) {
      cJSON *sitename = cJSON_GetObjectItem(subitem, "name");
      if (!sitename) {
        logger.msg(WARNING, "Badly formatted output from AGIS");
        subitem = subitem->next;
        continue;
      }
      std::string site = sitename->valuestring;
      cJSON *protocols = cJSON_GetObjectItem(subitem, "aprotocols");
      if (!protocols) {
        logger.msg(WARNING, "Badly formatted output from AGIS");
        subitem = subitem->next;
        continue;
      }
      cJSON *readprotocols = cJSON_GetObjectItem(protocols, "r");
      if (!readprotocols) {
        logger.msg(WARNING, "Badly formatted output from AGIS");
        subitem = subitem->next;
        continue;
      }
      cJSON *protocol = readprotocols->child;
      std::string endpoint;
      int highest_prority = 0;
      while (protocol) {
        // should be 3 children: str, int, str
        cJSON *endpoint_start = protocol->child;
        cJSON *priority = endpoint_start->next;
        cJSON *endpoint_end = priority->next;
        if (!endpoint_start || !priority || !endpoint_end) {
          logger.msg(WARNING, "Badly formatted output from AGIS");
          protocol = protocol->next;
          continue;
        }
        if (priority->valueint > highest_prority) {
          highest_prority = priority->valueint;
          endpoint = std::string(endpoint_start->valuestring) +
                     std::string(endpoint_end->valuestring);
        }
        protocol = protocol->next;
      }
      if (!endpoint.empty()) {
        endpoint_info[site] = endpoint;
        logger.msg(DEBUG, "%s -> %s", site, endpoint);
      }
      subitem = subitem->next;
    }
    cJSON_Delete(root);

    return true;
  }

} // namespace ArcDMCDQ2

Arc::PluginDescriptor ARC_PLUGINS_TABLE_NAME[] = {
  { "dq2", "HED:DMC", "DQ2: ATLAS Data Management System", 0, &ArcDMCDQ2::DataPointDQ2::Instance },
  { NULL, NULL, NULL, 0, NULL }
};
