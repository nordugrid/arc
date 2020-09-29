#include <arc/communication/ClientInterface.h>
#include <arc/message/MCC.h>
#include <arc/message/PayloadRaw.h>
#include <arc/credential/VOMSUtil.h>

#include <arc/external/cJSON/cJSON.h>

#include "DataPointRucio.h"

namespace ArcDMCRucio {

  using namespace Arc;

  Arc::Logger DataPointRucio::logger(Arc::Logger::getRootLogger(), "DataPoint.Rucio");
  RucioTokenStore DataPointRucio::tokens;
  Glib::Mutex DataPointRucio::lock;
  const Period DataPointRucio::token_validity(3600); // token lifetime is 1h
  Arc::Logger RucioTokenStore::logger(Arc::Logger::getRootLogger(), "DataPoint.RucioTokenStore");

  void RucioTokenStore::AddToken(const std::string& account, const Time& expirytime, const std::string& token) {
    // Replace any existing token
    if (tokens.find(account) != tokens.end()) {
      logger.msg(VERBOSE, "Replacing existing token for %s in Rucio token cache", account);
    }
    // Create new token
    RucioToken t;
    t.expirytime = expirytime;
    t.token = token;
    tokens[account] = t;
  }

  std::string RucioTokenStore::GetToken(const std::string& account) {
    // Search for account in list
    std::string token;
    if (tokens.find(account) != tokens.end()) {
      logger.msg(VERBOSE, "Found existing token for %s in Rucio token cache with expiry time %s", account, tokens[account].expirytime.str());
      // If 5 mins left until expiry time, get new token
      if (tokens[account].expirytime <= Time()+300) {
        logger.msg(VERBOSE, "Rucio token for %s has expired or is about to expire", account);
      } else {
        token = tokens[account].token;
      }
    }
    return token;
  }

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

  DataPointRucio::DataPointRucio(const URL& url, const UserConfig& usercfg, PluginArgument* parg)
    : DataPointIndex(url, usercfg, parg) {
    // Get RUCIO_ACCOUNT, try in order:
    // - rucioaccount URL option
    // - RUCIO_ACCOUNT environment variable
    // - nickname extracted from VOMS proxy
    account = url.Option("rucioaccount");
    if (account.empty()) {
      account = Arc::GetEnv("RUCIO_ACCOUNT");
    }
    if (account.empty()) {
      // Extract nickname from voms credential
      Credential cred(usercfg);
      account = getCredentialProperty(cred, "voms:nickname");
      logger.msg(VERBOSE, "Extracted nickname %s from credentials to use for RUCIO_ACCOUNT", account);
    }
    if (account.empty()) {
      logger.msg(WARNING, "Failed to extract VOMS nickname from proxy");
    }
    logger.msg(VERBOSE, "Using Rucio account %s", account);
    // Take auth url from env var if available, otherwise use hard-coded one
    // TODO: specify through url option instead?
    std::string rucio_auth_url(Arc::GetEnv("RUCIO_AUTH_URL"));
    if (rucio_auth_url.empty()) {
      rucio_auth_url = "https://voatlasrucio-auth-prod.cern.ch/auth/x509_proxy";
    }
    auth_url = URL(rucio_auth_url);
  }

  DataPointRucio::~DataPointRucio() {}

  Plugin* DataPointRucio::Instance(PluginArgument *arg) {
    DataPointPluginArgument *dmcarg =
      dynamic_cast<DataPointPluginArgument*>(arg);
    if (!dmcarg)
      return NULL;
    if (((const URL&)(*dmcarg)).Protocol() != "rucio")
      return NULL;
    return new DataPointRucio(*dmcarg, *dmcarg, arg);
  }

  DataStatus DataPointRucio::Check(bool check_meta) {
    // Simply check that the file can be resolved
    DataStatus r = Resolve(true);
    if (r) return r;
    return DataStatus(DataStatus::CheckError, r.GetErrno(), r.GetDesc());
  }

  DataStatus DataPointRucio::Resolve(bool source) {

    // Check token and get new one if necessary
    std::string token;
    DataStatus r = checkToken(token);
    if (!r) return r;

    bool osresolve = (url.Path().find("/objectstores/") != std::string::npos);

    // Check if Rucio path is ok: read/write to objectstores and read only from replicas
    if (!osresolve && !(source && url.Path().find("/replicas/") != std::string::npos)) {
      logger.msg(ERROR, "Bad path for %s: Rucio supports read/write at /objectstores and read-only at /replicas", url.str());
      return DataStatus(source ? DataStatus::ReadResolveError : DataStatus::WriteResolveError, EINVAL, "Bad path for Rucio");
    }

    // Call Rucio to get a signed URL for the location

    std::string content;
    r = queryRucio(content, token);
    if (!r) return r;
    if (!osresolve) {
      return parseLocations(content);
    }

    // content should be a signed URL
    URL osurl(content, true);
    if (!osurl) {
      logger.msg(ERROR, "Can't handle URL %s", osurl.str());
      return DataStatus(source ? DataStatus::ReadResolveError : DataStatus::WriteResolveError, EINVAL, "Bad signed URL returned from Rucio");
    }
    // Add URL options to replicas
    for (std::map<std::string, std::string>::const_iterator opt = url.CommonLocOptions().begin();
         opt != url.CommonLocOptions().end(); opt++)
      osurl.AddOption(opt->first, opt->second, false);
    for (std::map<std::string, std::string>::const_iterator opt = url.Options().begin();
         opt != url.Options().end(); opt++)
      osurl.AddOption(opt->first, opt->second, false);
    // OS doesn't accept absolute URIs
    osurl.AddOption("relativeuri=yes");

    AddLocation(osurl, osurl.Host());
    return DataStatus::Success;
  }

  DataStatus DataPointRucio::Resolve(bool source, const std::list<DataPoint*>& urls) {

    if (!source) return DataStatus(DataStatus::WriteResolveError, ENOTSUP, "Writing to Rucio is not supported");
    if (urls.empty()) return DataStatus(DataStatus::ReadResolveError, ENOTSUP, "Bulk resolving is not supported");

    // No bulk yet so query in series
    for (std::list<DataPoint*>::const_iterator i = urls.begin(); i != urls.end(); ++i) {
      DataStatus r = (*i)->Resolve(source);
      if (!r) return r;
    }
    return DataStatus::Success;
  }

  DataStatus DataPointRucio::Stat(FileInfo& file, DataPoint::DataPointInfoType verb) {
    std::list<FileInfo> files;
    std::list<DataPoint*> urls(1, this);
    DataStatus r = Stat(files, urls, verb);
    if (!r) {
      return r;
    }
    if (files.empty()) {
      return DataStatus(DataStatus::StatError, EARCRESINVAL, "No results returned");
    }
    if (!HaveLocations()) {
      return DataStatus(DataStatus::StatError, ENOENT);
    }
    file = files.front();
    return DataStatus::Success;
  }

  DataStatus DataPointRucio::Stat(std::list<FileInfo>& files,
                                  const std::list<DataPoint*>& urls,
                                  DataPointInfoType verb) {
    files.clear();
    DataStatus r = Resolve(true, urls);
    if (!r) {
      return DataStatus(DataStatus::StatError, r.GetErrno(), r.GetDesc());
    }
    for (std::list<DataPoint*>::const_iterator f = urls.begin(); f != urls.end(); ++f) {
      FileInfo info;
      if (!(*f)->HaveLocations()) {
        logger.msg(ERROR, "No locations found for %s", (*f)->GetURL().str());
      } else {
        info.SetName((*f)->GetURL().Path().substr((*f)->GetURL().Path().rfind('/')+1));
        info.SetType(FileInfo::file_type_file);
        info.SetSize((*f)->GetSize());
        info.SetCheckSum((*f)->GetCheckSum());
        for (; (*f)->LocationValid(); (*f)->NextLocation()) {
          info.AddURL((*f)->CurrentLocation());
        }
      }
      files.push_back(info);
    }
    return DataStatus::Success;
  }

  DataStatus DataPointRucio::CompareLocationMetadata() const {
    if (CurrentLocationHandle() && CurrentLocationHandle()->GetURL().HTTPOption("xrdcl.unzip", "") == "") {
      return DataPointIndex::CompareLocationMetadata();
    }
    return DataStatus::Success;
  }

  DataStatus DataPointRucio::PreRegister(bool replication, bool force) {
    if (url.Path().find("/objectstores/") == 0) return DataStatus::Success;
    return DataStatus(DataStatus::PreRegisterError, ENOTSUP, "Writing to Rucio is not supported");
  }

  DataStatus DataPointRucio::PostRegister(bool replication) {
    if (url.Path().find("/objectstores/") == 0) return DataStatus::Success;
    return DataStatus(DataStatus::PostRegisterError, ENOTSUP, "Writing to Rucio is not supported");
  }

  DataStatus DataPointRucio::PreUnregister(bool replication) {
    if (url.Path().find("/objectstores/") == 0) return DataStatus::Success;
    return DataStatus(DataStatus::UnregisterError, ENOTSUP, "Deleting from Rucio is not supported");
  }

  DataStatus DataPointRucio::Unregister(bool all) {
    return DataStatus(DataStatus::UnregisterError, ENOTSUP, "Deleting from Rucio is not supported");
  }

  DataStatus DataPointRucio::List(std::list<FileInfo>& files, DataPoint::DataPointInfoType verb) {
    return DataStatus(DataStatus::ListError, ENOTSUP, "Listing in Rucio is not supported");
  }

  DataStatus DataPointRucio::CreateDirectory(bool with_parents) {
    return DataStatus(DataStatus::CreateDirectoryError, ENOTSUP, "Creating directories in Rucio is not supported");
  }

  DataStatus DataPointRucio::Rename(const URL& newurl) {
    return DataStatus(DataStatus::RenameError, ENOTSUP, "Renaming in Rucio is not supported");
  }

  DataStatus DataPointRucio::checkToken(std::string& token) {

    // Locking the entire method prevents multiple concurrent calls to get tokens
    Glib::Mutex::Lock l(lock);
    std::string t = tokens.GetToken(account);
    if (!t.empty()) {
      token = t;
      return DataStatus::Success;
    }
    // Get a new token
    MCCConfig cfg;
    usercfg.ApplyToConfig(cfg);
    ClientHTTP client(cfg, auth_url, usercfg.Timeout());

    std::multimap<std::string, std::string> attrmap;
    std::string method("GET");
    attrmap.insert(std::pair<std::string, std::string>("X-Rucio-Account", account));
    ClientHTTPAttributes attrs(method, auth_url.Path(), attrmap);

    HTTPClientInfo transfer_info;
    PayloadRaw request;
    PayloadRawInterface *response = NULL;

    MCC_Status r = client.process(attrs, &request, &transfer_info, &response);

    if (!r) {
      return DataStatus(DataStatus::ReadResolveError, "Failed to contact auth server: " + r.getExplanation());
    }
    if (transfer_info.code != 200) {
      return DataStatus(DataStatus::ReadResolveError, http2errno(transfer_info.code), "HTTP error when contacting auth server: " + transfer_info.reason);
    }
    // Get auth token from header
    if (transfer_info.headers.find("HTTP:x-rucio-auth-token") == transfer_info.headers.end()) {
      return DataStatus(DataStatus::ReadResolveError, "Failed to obtain auth token");
    }
    token = transfer_info.headers.find("HTTP:x-rucio-auth-token")->second;
    tokens.AddToken(account, Time()+token_validity, token);
    logger.msg(DEBUG, "Acquired auth token for %s: %s", account, token);
    return DataStatus::Success;
  }

  DataStatus DataPointRucio::queryRucio(std::string& content,
                                        const std::string& token) const {

    // SSL error happens if client certificate is specified, so only set CA dir
    MCCConfig cfg;
    cfg.AddCADir(usercfg.CACertificatesDirectory());
    // Switch rucio protocol to http(s) for lookup
    URL rucio_url(url);
    rucio_url.ChangeProtocol(url.Port() == 80 ? "http" : "https");
    if (rucio_url.Port() == -1) {
      rucio_url.ChangePort(url.Protocol() == "http" ? 80 : 443);
    }
    ClientHTTP client(cfg, rucio_url, usercfg.Timeout());

    std::multimap<std::string, std::string> attrmap;
    std::string method("GET");
    attrmap.insert(std::pair<std::string, std::string>("X-Rucio-Auth-Token", token));
    // Adding the line below makes rucio return a metalink xml
    //attrmap.insert(std::pair<std::string, std::string>("Accept", "application/metalink4+xml"));
    ClientHTTPAttributes attrs(method, url.Path(), attrmap);

    HTTPClientInfo transfer_info;
    PayloadRaw request;
    PayloadRawInterface *response = NULL;

    MCC_Status r = client.process(attrs, &request, &transfer_info, &response);

    if (!r) {
      delete response; response = NULL;
      return DataStatus(DataStatus::ReadResolveError, "Failed to contact server: " + r.getExplanation());
    }
    if (transfer_info.code != 200) {
      std::string errormsg(transfer_info.reason);
      // Extract Rucio exception if any
      if (transfer_info.headers.find("HTTP:exceptionmessage") != transfer_info.headers.end()) {
        errormsg += ": " + transfer_info.headers.find("HTTP:exceptionmessage")->second;
      }
      return DataStatus(DataStatus::ReadResolveError, http2errno(transfer_info.code), "HTTP error when contacting server: " + errormsg);
    }
    PayloadStreamInterface* instream = NULL;
    try {
      instream = dynamic_cast<PayloadStreamInterface*>(dynamic_cast<MessagePayload*>(response));
    } catch(std::exception& e) {
      delete response; response = NULL;
      return DataStatus(DataStatus::ReadResolveError, "Unexpected response from server");
    }
    if (!instream) {
      delete response; response = NULL;
      return DataStatus(DataStatus::ReadResolveError, "Unexpected response from server");
    }

    std::string buf;
    while (instream->Get(buf)) content += buf;
    logger.msg(DEBUG, "Rucio returned %s", content);
    delete response; response = NULL;
    return DataStatus::Success;
  }

  DataStatus DataPointRucio::parseLocations(const std::string& content) {

    // parse JSON:
    // {"adler32": "ffa2c799",
    //  "name": "EVNT.545023._000082.pool.root.1",
    //  "replicas": [],
    //  "rses": {"LRZ-LMU_DATADISK": ["srm://lcg-lrz-srm.grid.lrz.de:8443/srm/managerv2?SFN=/pnfs/lrz-muenchen.de/data/atlas/dq2/atlasdatadisk/rucio/mc11_7TeV/6c/13/EVNT.545023._000082.pool.root.1"],
    //           "INFN-FRASCATI_DATADISK": ["srm://atlasse.lnf.infn.it:8446/srm/managerv2?SFN=/dpm/lnf.infn.it/home/atlas/atlasdatadisk/rucio/mc11_7TeV/6c/13/EVNT.545023._000082.pool.root.1"],
    //           "TAIWAN-LCG2_DATADISK": ["srm://f-dpm001.grid.sinica.edu.tw:8446/srm/managerv2?SFN=/dpm/grid.sinica.edu.tw/home/atlas/atlasdatadisk/rucio/mc11_7TeV/6c/13/EVNT.545023._000082.pool.root.1"]},
    //  "bytes": 69234676,
    //  "space_token": "ATLASDATADISK",
    //  "scope": "mc11_7TeV",
    //  "md5": null}

    if (content.empty()) {
      // empty response means no such file
      return DataStatus(DataStatus::ReadResolveError, ENOENT);
    }

    cJSON *root = cJSON_Parse(content.c_str());
    if (!root) {
      logger.msg(ERROR, "Failed to parse Rucio response: %s", content);
      cJSON_Delete(root);
      return DataStatus(DataStatus::ReadResolveError, EARCRESINVAL, "Failed to parse Rucio response");
    }
    cJSON *name = cJSON_GetObjectItem(root, "name");
    if (!name || name->type != cJSON_String || !name->valuestring) {
      logger.msg(ERROR, "Filename not returned in Rucio response: %s", content);
      cJSON_Delete(root);
      return DataStatus(DataStatus::ReadResolveError, EARCRESINVAL, "Failed to parse Rucio response");
    }
    std::string filename(name->valuestring);
    if (filename != url.Path().substr(url.Path().rfind('/')+1)) {
      logger.msg(ERROR, "Unexpected name returned in Rucio response: %s", content);
      cJSON_Delete(root);
      return DataStatus(DataStatus::ReadResolveError, EARCRESINVAL, "Failed to parse Rucio response");
    }
    cJSON *rses = cJSON_GetObjectItem(root, "rses");
    if (!rses) {
      logger.msg(ERROR, "No RSE information returned in Rucio response: %s", content);
      cJSON_Delete(root);
      return DataStatus(DataStatus::ReadResolveError, EARCRESINVAL, "Failed to parse Rucio response");
    }
    cJSON *rse = rses->child;
    while (rse) {
      cJSON *replicas = rse->child;
      while(replicas) {
        if(replicas->type == cJSON_String || replicas->valuestring) {
          URL loc(std::string(replicas->valuestring));
          if(loc) {
            // Add URL options to replicas
            for (std::map<std::string, std::string>::const_iterator opt = url.CommonLocOptions().begin();
                 opt != url.CommonLocOptions().end(); opt++)
              loc.AddOption(opt->first, opt->second, false);
            for (std::map<std::string, std::string>::const_iterator opt = url.Options().begin();
                 opt != url.Options().end(); opt++)
              loc.AddOption(opt->first, opt->second, false);
            AddLocation(loc, loc.ConnectionURL());
          }
        }
        replicas = replicas->next;
      }
      rse = rse->next;
    }
    cJSON *fsize = cJSON_GetObjectItem(root, "bytes");
    if (!fsize || fsize->type == cJSON_NULL) {
      logger.msg(WARNING, "No filesize information returned in Rucio response for %s", filename);
    } else {
      SetSize((unsigned long long int)fsize->valuedouble);
      logger.msg(DEBUG, "%s: size %llu", filename, GetSize());
    }
    cJSON *csum = cJSON_GetObjectItem(root, "adler32");
    if (!csum || csum->type != cJSON_String || !csum->valuestring) {
      logger.msg(WARNING, "No checksum information returned in Rucio response for %s", filename);
    } else {
      SetCheckSum(std::string("adler32:") + std::string(csum->valuestring));
      logger.msg(DEBUG, "%s: checksum %s", filename, GetCheckSum());
    }
    cJSON_Delete(root);

    if (!HaveLocations()) {
      logger.msg(ERROR, "No locations found for %s", url.str());
      return DataStatus(DataStatus::ReadResolveError, ENOENT);
    }
    return DataStatus::Success;
  }

} // namespace ArcDMCRucio

extern Arc::PluginDescriptor const ARC_PLUGINS_TABLE_NAME[] = {
  { "rucio", "HED:DMC", "ATLAS Data Management System", 0, &ArcDMCRucio::DataPointRucio::Instance },
  { NULL, NULL, NULL, 0, NULL }
};
