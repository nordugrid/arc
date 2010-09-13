#include <sys/stat.h>
#include <errno.h>

#include <arc/data/FileCache.h>
#include <arc/data/DataHandle.h>
#include <arc/URL.h>
#include <arc/StringConv.h>
#include <arc/UserConfig.h>
#include <arc/FileUtils.h>
#include <arc/User.h>

#include <arc/message/MessageAttributes.h>
#include <arc/message/PayloadRaw.h>
#include <arc/message/PayloadStream.h>
#include <arc/message/PayloadSOAP.h>

#include "CacheService.h"

namespace Cache {

static Arc::Plugin *get_service(Arc::PluginArgument* arg)
{
    Arc::ServicePluginArgument* srvarg =
            arg?dynamic_cast<Arc::ServicePluginArgument*>(arg):NULL;
    if(!srvarg) return NULL;
    CacheService* s = new CacheService((Arc::Config*)(*srvarg));
    if (*s)
      return s;
    delete s;
    return NULL;
}

Arc::Logger CacheService::logger(Arc::Logger::rootLogger, "CacheService");

CacheService::CacheService(Arc::Config *cfg) : RegisteredService(cfg),
                                               valid(false) {
  // read configuration information about caches
  /* conf format is like
  <cacheservice:cache>
    <cacheservice:location>
      <cacheservice:path>/var/cache</cacheservice:path>
      <cacheservice:link>.</cacheservice:link>
    </cacheservice:location>
    <cacheservice:location>
      ...
  </cacheservice:cache>
  */
  ns["cacheservice"] = "urn:cacheservice_config";

  Arc::XMLNode cache_node = (*cfg)["cache"];
  if(cache_node) {
    Arc::XMLNode location_node = cache_node["location"];
    for(;location_node;++location_node) {
      std::string cache_dir = location_node["path"];
      std::string cache_link_dir = location_node["link"];
      if(cache_dir.length() == 0) {
        logger.msg(Arc::ERROR, "Missing path in cache location element");
        return;
      }
      // validation of paths
      while (cache_dir.length() > 1 && cache_dir.rfind("/") == cache_dir.length()-1)
        cache_dir = cache_dir.substr(0, cache_dir.length()-1);
      if (cache_dir[0] != '/') {
        logger.msg(Arc::ERROR, "Cache path must start with '/'");
        return;
      }
      if (cache_dir.find("..") != std::string::npos) {
        logger.msg(Arc::ERROR, "Cache path cannot contain '..'");
        return;
      }
      if (!cache_link_dir.empty() && cache_link_dir != "." && cache_link_dir != "drain") {
        while (cache_link_dir.rfind("/") == cache_link_dir.length()-1)
          cache_link_dir = cache_link_dir.substr(0, cache_link_dir.length()-1);
        if (cache_link_dir[0] != '/') {
          logger.msg(Arc::ERROR, "Cache link path must start with '/'");
          return;
        }
        if (cache_link_dir.find("..") != std::string::npos) {
          logger.msg(Arc::ERROR, "Cache link path cannot contain '..'");
          return;
        }
      }
      // don't use draining dirs
      if (cache_link_dir == "drain")
        continue;

      std::string cache = cache_dir;
      if (!cache_link_dir.empty())
        cache += " "+cache_link_dir;

      logger.msg(Arc::INFO, "Adding cache dir %s", cache);
      caches.push_back(cache);
    }
  }
  else {
    // error - no caches defined
    logger.msg(Arc::ERROR, "No cache information found in configuration");
    return;
  }
  if (caches.empty()) {
    logger.msg(Arc::ERROR, "No valid caches found in configuration");
    return;
  }
  Arc::User user;
  cache = new Arc::FileCache(caches, "0", user.get_uid(), user.get_gid());
  if (!cache || !(*cache)) {
    logger.msg(Arc::ERROR, "Error with cache configuration");
    delete cache;
    return;
  }

  valid = true;
}

CacheService::~CacheService(void) {
  if (cache) {
    delete cache;
    cache = NULL;
  }
}

Arc::MCC_Status CacheService::CacheCheck(Arc::XMLNode in,Arc::XMLNode out) {

  bool fileexist;
  Arc::XMLNode resp = out.NewChild("CacheCheckResponse");
  Arc::XMLNode results = resp.NewChild("CacheCheckResult");

  for(int n = 0;;++n) {
    Arc::XMLNode id = in["CacheCheck"]["TheseFilesNeedToCheck"]["FileURL"][n];

    if (!id) break;

    std::string fileurl = (std::string)in["CacheCheck"]["TheseFilesNeedToCheck"]["FileURL"][n];
    Arc::XMLNode resultelement = results.NewChild("Result");
    fileexist = false;
    std::string file_lfn;
    Arc::UserConfig usercfg(Arc::initializeCredentialsType(Arc::initializeCredentialsType::SkipCredentials));
    Arc::URL url(fileurl);
    Arc::DataHandle d(url, usercfg);

    logger.msg(Arc::INFO, "Looking up URL %s", d->str());

    file_lfn = cache->File(d->str());
    if (file_lfn.empty()) {
      logger.msg(Arc::ERROR, "Empty filename returned from FileCache");
      resultelement.NewChild("ExistInTheCache") = "false";
      resultelement.NewChild("FileSize") = "0";
      continue;
    }
    logger.msg(Arc::INFO, "Cache file is %s", file_lfn);

    struct stat fileStat;

    if (Arc::FileStat(file_lfn, &fileStat, false))
      fileexist = true;
    else if (errno != ENOENT)
      logger.msg(Arc::ERROR, "Problem accessing cache file %s: %s", file_lfn, strerror(errno));

    resultelement.NewChild("FileURL") = fileurl;
    resultelement.NewChild("ExistInTheCache") = (fileexist ? "true": "false");
    if (fileexist)
      resultelement.NewChild("FileSize") = Arc::tostring(fileStat.st_size);
    else
      resultelement.NewChild("FileSize") = "0";
  }
  return Arc::MCC_Status(Arc::STATUS_OK);
}

Arc::MCC_Status CacheService::CacheLink(Arc::XMLNode in,Arc::XMLNode out) {
  return Arc::MCC_Status(Arc::STATUS_OK);
}

Arc::MCC_Status CacheService::process(Arc::Message &inmsg, Arc::Message &outmsg) {

  // Check authorization
  if(!ProcessSecHandlers(inmsg, "incoming")) {
    logger.msg(Arc::ERROR, "CacheService: Unauthorized");
    return Arc::MCC_Status(Arc::GENERIC_ERROR);
  }

  std::string method = inmsg.Attributes()->get("HTTP:METHOD");
  if(method == "POST") {
    logger.msg(Arc::VERBOSE, "process: POST");
    logger.msg(Arc::INFO, "Identity is %s", inmsg.Attributes()->get("TLS:PEERDN"));
    // Both input and output are supposed to be SOAP
    // Extracting payload
    Arc::PayloadSOAP* inpayload = NULL;
    try {
      inpayload = dynamic_cast<Arc::PayloadSOAP*>(inmsg.Payload());
    } catch(std::exception& e) { };
    if(!inpayload) {
      logger.msg(Arc::ERROR, "input is not SOAP");
      return make_soap_fault(outmsg);
    }
    // Applying known namespaces
    inpayload->Namespaces(ns);
    if(logger.getThreshold() <= Arc::VERBOSE) {
        std::string str;
        inpayload->GetDoc(str, true);
        logger.msg(Arc::VERBOSE, "process: request=%s",str);
    }
    // Analyzing request
    Arc::XMLNode op = inpayload->Child(0);
    if(!op) {
      logger.msg(Arc::ERROR, "input does not define operation");
      return make_soap_fault(outmsg);
    }
    logger.msg(Arc::VERBOSE, "process: operation: %s",op.Name());

    Arc::PayloadSOAP* outpayload = new Arc::PayloadSOAP(ns);
    outpayload->Namespaces(ns);
    if (MatchXMLName(op,"CacheCheck")) {
      CacheCheck(*inpayload, *outpayload);
    }
    else if (MatchXMLName(op, "CacheLink")) {
      CacheLink(*inpayload, *outpayload);
    }
    else {
      // unknown operation
      logger.msg(Arc::ERROR, "SOAP operation is not supported: %s", op.Name());
      delete outpayload;
      return make_soap_fault(outmsg);
    }
    if (logger.getThreshold() <= Arc::VERBOSE) {
      std::string str;
      outpayload->GetDoc(str, true);
      logger.msg(Arc::VERBOSE, "process: response=%s", str);
    }
    outmsg.Payload(outpayload);

    if (!ProcessSecHandlers(outmsg,"outgoing")) {
      logger.msg(Arc::ERROR, "Security Handlers processing failed");
      delete outmsg.Payload(NULL);
      return Arc::MCC_Status();
    }
  }
  else {
    // only POST supported
    logger.msg(Arc::ERROR, "Only POST is supported in CacheService");
    return Arc::MCC_Status();
  }
  return Arc::MCC_Status(Arc::STATUS_OK);
}

bool CacheService::RegistrationCollector(Arc::XMLNode &doc) {
  Arc::NS isis_ns; isis_ns["isis"] = "http://www.nordugrid.org/schemas/isis/2008/08";
  Arc::XMLNode regentry(isis_ns, "RegEntry");
  regentry.NewChild("SrcAdv").NewChild("Type") = "org.nordugrid.execution.cacheservice";
  regentry.New(doc);
  return true;
}

Arc::MCC_Status CacheService::make_soap_fault(Arc::Message& outmsg) {
  Arc::PayloadSOAP* outpayload = new Arc::PayloadSOAP(ns,true);
  Arc::SOAPFault* fault = outpayload?outpayload->Fault():NULL;
  if(fault) {
    fault->Code(Arc::SOAPFault::Sender);
    fault->Reason("Failed processing request");
  }
  outmsg.Payload(outpayload);
  return Arc::MCC_Status(Arc::STATUS_OK);
}

} // namespace Cache

Arc::PluginDescriptor PLUGINS_TABLE_NAME[] = {
    { "cacheservice", "HED:SERVICE", 0, &Cache::get_service },
    { NULL, NULL, 0, NULL }
};
