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

// A-REX includes for configuration
#include "../a-rex/grid-manager/conf/conf_file.h"
#include "../a-rex/grid-manager/log/job_log.h"
#include "../a-rex/grid-manager/jobs/states.h"

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
                                               user(NULL),
                                               valid(false) {
  // read configuration information
  /*
  cacheservice config specifies A-REX conf file
  <cacheservice:config>/etc/arc.conf</cacheservice:config>
  */
  ns["cacheservice"] = "urn:cacheservice_config";

  if (!(*cfg)["cache"] || !(*cfg)["cache"]["config"]) {
    // error - no config defined
    logger.msg(Arc::ERROR, "No A-REX config file found in cache service configuration");
    return;
  }
  std::string arex_config = (*cfg)["cache"]["config"];

  JobLog job_log;
  JobsListConfig jobs_cfg;
  GMEnvironment gm_env(job_log, jobs_cfg);
  gm_env.nordugrid_config_loc(arex_config);
  users = new JobUsers(gm_env);

  // read A-REX config
  // user running this service
  Arc::User u;
  JobUser my_user(gm_env);
  if (!configure_serviced_users(*users, u.get_uid(), u.Name(), my_user)) {
    logger.msg(Arc::ERROR, "Failed to process A-REX configuration in %s", gm_env.nordugrid_config_loc());
    return;
  }
  print_serviced_users(*users);

  valid = true;
}

CacheService::~CacheService(void) {
  if (users) {
    delete users;
    users = NULL;
  }
}

Arc::MCC_Status CacheService::CacheCheck(Arc::XMLNode in, Arc::XMLNode out, const JobUser& user) {

  std::vector<std::string> caches = user.CacheParams()->getCacheDirs();
  if (caches.empty()) {
    logger.msg(Arc::ERROR, "No caches configured for user %s", user.UnixName());
    return Arc::MCC_Status(Arc::GENERIC_ERROR, "CacheCheck", "No caches configured for local user");
  }
  Arc::User u(user.UnixName());

  // create cache
  Arc::FileCache cache(caches, "0", u.get_uid(), u.get_gid());
  if (!cache) {
    logger.msg(Arc::ERROR, "Error creating cache");
    return Arc::MCC_Status(Arc::GENERIC_ERROR, "CacheCheck", "Server error with cache");
  }

  Arc::XMLNode resp = out.NewChild("CacheCheckResponse");
  Arc::XMLNode results = resp.NewChild("CacheCheckResult");

  for(int n = 0;;++n) {
    Arc::XMLNode id = in["CacheCheck"]["TheseFilesNeedToCheck"]["FileURL"][n];

    if (!id) break;

    std::string fileurl = (std::string)in["CacheCheck"]["TheseFilesNeedToCheck"]["FileURL"][n];
    Arc::XMLNode resultelement = results.NewChild("Result");
    bool fileexist = false;
    std::string file_lfn;
    Arc::UserConfig usercfg(Arc::initializeCredentialsType(Arc::initializeCredentialsType::SkipCredentials));
    Arc::URL url(fileurl);
    Arc::DataHandle d(url, usercfg);

    logger.msg(Arc::INFO, "Looking up URL %s", d->str());

    file_lfn = cache.File(d->str());
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

Arc::MCC_Status CacheService::CacheLink(Arc::XMLNode in, Arc::XMLNode out, const JobUser& user) {

  // create new cache for the user calling this method
  return Arc::MCC_Status(Arc::STATUS_OK);
}

Arc::MCC_Status CacheService::process(Arc::Message &inmsg, Arc::Message &outmsg) {

  // Check authorization
  if(!ProcessSecHandlers(inmsg, "incoming")) {
    logger.msg(Arc::ERROR, "CacheService: Unauthorized");
    return make_soap_fault(outmsg, "Authorization failed");
  }

  std::string method = inmsg.Attributes()->get("HTTP:METHOD");

  // find local user
  std::string username = inmsg.Attributes()->get("SEC:LOCALID");
  if (username.empty()) {
    logger.msg(Arc::ERROR, "No local user mapping found");
    return make_soap_fault(outmsg, "No local user mapping found");
  }
  if (users->find(username) != users->end()) {
    user = &(*(users->find(username)));
  } else if (users->find("") != users->end()) {
    user = &(*(users->find("")));
  } else {
    logger.msg(Arc::ERROR, "No configuration found for user %s in A-REX configuration", username);
    return make_soap_fault(outmsg, "Server configuration error");
  }
  if (!user || !user->is_valid()) {
    logger.msg(Arc::ERROR, "No configuration found for user %s in A-REX configuration", username);
    return make_soap_fault(outmsg, "Server configuration error");
  }
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

    Arc::MCC_Status result(Arc::STATUS_OK);
    // choose operation
    if (MatchXMLName(op,"CacheCheck")) {
      result = CacheCheck(*inpayload, *outpayload, *user);
    }
    else if (MatchXMLName(op, "CacheLink")) {
      result = CacheLink(*inpayload, *outpayload, *user);
    }
    else {
      // unknown operation
      logger.msg(Arc::ERROR, "SOAP operation is not supported: %s", op.Name());
      delete outpayload;
      return make_soap_fault(outmsg);
    }

    if (!result)
      return make_soap_fault(outmsg, result.getExplanation());

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

Arc::MCC_Status CacheService::make_soap_fault(Arc::Message& outmsg, const std::string& reason) {
  Arc::PayloadSOAP* outpayload = new Arc::PayloadSOAP(ns,true);
  Arc::SOAPFault* fault = outpayload?outpayload->Fault():NULL;
  if(fault) {
    fault->Code(Arc::SOAPFault::Sender);
    if (reason.empty())
      fault->Reason("Failed processing request");
    else
      fault->Reason("Failed processing request: "+reason);
  }
  outmsg.Payload(outpayload);
  return Arc::MCC_Status(Arc::STATUS_OK);
}

} // namespace Cache

Arc::PluginDescriptor PLUGINS_TABLE_NAME[] = {
    { "cacheservice", "HED:SERVICE", 0, &Cache::get_service },
    { NULL, NULL, 0, NULL }
};
