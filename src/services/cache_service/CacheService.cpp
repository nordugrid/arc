#include <sys/stat.h>
#include <errno.h>

#include <arc/credential/Credential.h>
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

// A-REX includes for configuration and running downloader
#include "../a-rex/grid-manager/conf/conf_file.h"
#include "../a-rex/grid-manager/log/job_log.h"
#include "../a-rex/grid-manager/jobs/states.h"
#include "../a-rex/grid-manager/files/info_types.h"
#include "../a-rex/grid-manager/files/info_files.h"
#include "../a-rex/grid-manager/run/run_parallel.h"

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
                                               max_downloads(10),
                                               current_downloads(0),
                                               users(NULL),
                                               gm_env(NULL),
                                               jcfg(NULL),
                                               valid(false) {
  // read configuration information
  /*
  cacheservice config specifies A-REX conf file
    <cacheservice:config>/etc/arc.conf</cacheservice:config>
  max simultaneous downloads
    <cacheservice:maxload>10</cacheservice:maxload>
  */
  ns["cacheservice"] = "urn:cacheservice_config";

  if (!(*cfg)["cache"] || !(*cfg)["cache"]["config"]) {
    // error - no config defined
    logger.msg(Arc::ERROR, "No A-REX config file found in cache service configuration");
    return;
  }
  std::string arex_config = (std::string)(*cfg)["cache"]["config"];
  logger.msg(Arc::INFO, "Using A-REX config file %s", arex_config);

  if ((*cfg)["cache"]["maxload"]) {
    std::string maxload = (std::string)(*cfg)["cache"]["maxload"];
    if (maxload.empty() || !Arc::stringto(maxload, max_downloads)) {
      logger.msg(Arc::ERROR, "Error converting maxload parameter %s to integer", maxload);
      return;
    }
  }
  logger.msg(Arc::INFO, "Setting max downloads to %u", max_downloads);

  JobLog job_log;
  jcfg = new JobsListConfig;
  gm_env = new GMEnvironment(job_log, *jcfg);
  gm_env->nordugrid_config_loc(arex_config);
  users = new JobUsers(*gm_env);

  // read A-REX config
  // user running this service
  Arc::User u;
  JobUser my_user(*gm_env);
  if (!configure_serviced_users(*users, u.get_uid(), u.Name(), my_user)) {
    logger.msg(Arc::ERROR, "Failed to process A-REX configuration in %s", gm_env->nordugrid_config_loc());
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
  if (gm_env) {
    delete gm_env;
    gm_env = NULL;
  }
  if (jcfg) {
    delete jcfg;
    jcfg = NULL;
  }
}

int CacheService::Download(const std::map<std::string, std::string>& urls,
                           const JobUser& user,
                           const std::string& job_id,
                           const Arc::User& mapped_user) {
  // create job.id.input file, then run new downloader process
  // wait for result with some timeout
  g_atomic_int_inc(&current_downloads);

  // set up objects for writing .input file
  JobDescription job_desc(job_id, user.SessionRoot(job_id) + "/" + job_id);
  job_desc.set_uid(mapped_user.get_uid(), mapped_user.get_gid());
  std::list<FileData> files;
  for (std::map<std::string, std::string>::const_iterator i = urls.begin(); i != urls.end(); ++i) {
    FileData filedata(i->second.c_str(), i->first.c_str());
    files.push_back(filedata);
  }
  if (!job_input_write_file(job_desc, user, files)) {
    logger.msg(Arc::ERROR, "Failed writing file with inputs");
    return -1;
  }

  // get parameters from JobListConfig
  bool switch_user = false; // download to cache should always be done as root
  std::string cmd = user.Env().nordugrid_libexec_loc()+"/downloader";
  std::string user_id = Arc::tostring(mapped_user.get_uid());
  std::string max_files_s;
  std::string min_speed_s;
  std::string min_time_s;
  std::string min_average_s;
  std::string max_inactivity_time_s;
  int argn=4;
  const char* args[] = {
    cmd.c_str(),
    "-U",
    user_id.c_str(),
    "-f",
    NULL, // -n
    NULL, // (-n)
    NULL, // -c
    NULL, // -p
    NULL, // -l
    NULL, // -s
    NULL, // (-s)
    NULL, // -S
    NULL, // (-S)
    NULL, // -a
    NULL, // (-a)
    NULL, // -i
    NULL, // (-i)
    NULL, // -d
    NULL, // (-d)
    NULL, // -C
    NULL, // (-C)
    NULL, // -r
    NULL, // (-r)
    NULL, // id
    NULL, // control
    NULL, // session
    NULL,
    NULL
  };
  int max_processing, max_processing_emergency, max_down;
  jcfg->GetMaxJobsLoad(max_processing, max_processing_emergency, max_down);
  if (max_down > 0) {
    max_files_s=Arc::tostring(max_down);
    args[argn]="-n"; argn++;
    args[argn]=(char*)(max_files_s.c_str()); argn++;
  };
  if (!jcfg->GetSecureTransfer()) {
    args[argn]="-c"; argn++;
  };
  if (jcfg->GetPassiveTransfer()) {
    args[argn]="-p"; argn++;
  };
  if (jcfg->GetLocalTransfer()) {
    args[argn]="-l"; argn++;
  };
  unsigned long long int min_speed, min_average_speed;
  time_t min_time, max_inactivity_time;
  jcfg->GetSpeedControl(min_speed, min_time, min_average_speed, max_inactivity_time);
  if (min_speed > 0) {
    min_speed_s = Arc::tostring(min_speed);
    min_time_s = Arc::tostring(min_time);
    args[argn]="-s"; argn++;
    args[argn]=(char*)(min_speed_s.c_str()); argn++;
    args[argn]="-S"; argn++;
    args[argn]=(char*)(min_time_s.c_str()); argn++;
  };
  if (min_average_speed > 0) {
    min_average_s = Arc::tostring(min_average_speed);
    args[argn]="-a"; argn++;
    args[argn]=(char*)(min_average_s.c_str()); argn++;
  };
  if (max_inactivity_time > 0) {
    max_inactivity_time_s=Arc::tostring(max_inactivity_time);
    args[argn]="-i"; argn++;
    args[argn]=(char*)(max_inactivity_time_s.c_str()); argn++;
  };
  std::string debug_level = Arc::level_to_string(Arc::Logger::getRootLogger().getThreshold());
  if (!debug_level.empty()) {
    args[argn]="-d"; argn++;
    args[argn]=(char*)(debug_level.c_str()); argn++;
  }
  std::string cfg_path = user.Env().nordugrid_config_loc();
  if (!cfg_path.empty()) {
    args[argn]="-C"; argn++;
    args[argn]=(char*)(cfg_path.c_str()); argn++;
  }
  if (!jcfg->GetPreferredPattern().empty()) {
    args[argn]="-r"; argn++;
    args[argn]=(char*)(jcfg->GetPreferredPattern().c_str()); argn++;
  }
  args[argn]=(char*)(job_id.c_str()); argn++;
  args[argn]=(char*)(user.ControlDir().c_str()); argn++;
  args[argn]=(char*)(job_desc.SessionDir().c_str()); argn++;

  Arc::Run* child = new Arc::Run(cmd);
  logger.msg(Arc::INFO, "Starting child downloader process");
  for (int i = 0; i < argn; ++i)
    logger.msg(Arc::VERBOSE, args[i]);
  if(!RunParallel::run(user, job_desc, (char**)args, &child, switch_user)) {
    logger.msg(Arc::ERROR, "Failed to run downloader process for job id %s", job_id);
    delete child;
    return false;
  }

  // wait for child to finish
  Arc::Time timeout;
  timeout = timeout + 3600; // timeout 1 hour TODO make configurable
  while (child->Running() && (Arc::Time() < timeout)) {
    logger.msg(Arc::DEBUG, "%s: child is running", job_id);
    sleep(1);
  }
  if (Arc::Time() >= timeout) {
    // timeout so kill child process
    delete child;
    logger.msg(Arc::ERROR, "Download process for job %s timed out", job_id);
    return -1;
  }

  // child finished - check exit code
  int result = child->Result();
  logger.msg(Arc::INFO, "Downloader exited with code: %i", result);
  delete child;

  g_atomic_int_dec_and_test(&current_downloads);
  return result;
}

Arc::MCC_Status CacheService::CacheCheck(Arc::XMLNode in, Arc::XMLNode out, const JobUser& user) {
  /*
   Accepts:
   <CacheCheck>
     <TheseFilesNeedToCheck>
       <FileURL>url</FileURL>
       ...
     </TheseFilesNeedToCheck>
   </CacheCheck>

   Returns
   <CacheCheckResponse>
     <CacheCheckResult>
       <Result>
         <FileURL>url</FileURL>
         <ExistInTheCache>true</ExistInTheCache>
         <FileSize>1234</FileSize>
       </Result>
       ...
     </CacheCheckResult>
   </CacheCheckResponse>
   */

  // create cache
  Arc::FileCache cache(user.CacheParams().getCacheDirs(), "0", user.get_uid(), user.get_gid());
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

Arc::MCC_Status CacheService::CacheLink(Arc::XMLNode in, Arc::XMLNode out,
                                        const JobUser& user, const Arc::User& mapped_user) {
  /*
   Accepts:
   <CacheLink>
     <TheseFilesNeedToLink>
       <File>
         <FileURL>url</FileURL>    // remote file
         <FileName>name</FileName> // local file on session dir
       </File>
       ...
     </TheseFilesNeedToLink>
     <Username>uname</Username>
     <JobID>123456789</JobID>
     <Stage>false</Stage>
   </CacheLink>

   Returns:
   <CacheLinkResponse>
     <CacheLinkResult>
       <Result>
         <FileURL>url</FileURL>
         <ReturnCode>0</ReturnCode>
         <ReturnExplanation>success</ReturnExplanation>
       </Result>
       ...
     </CacheLinkResult>
   </CacheLinkResponse>
   */

  // read in inputs
  bool dostage = false;
  if (in["CacheLink"]["Stage"])
    dostage = ((std::string)in["CacheLink"]["Stage"] == "true") ? true : false;

  Arc::XMLNode jobidnode = in["CacheLink"]["JobID"];
  if (!jobidnode) {
    logger.msg(Arc::ERROR, "No job ID supplied");
    return Arc::MCC_Status(Arc::GENERIC_ERROR, "CacheLink", "Bad input (no JobID specified)");
  }
  std::string jobid = (std::string)jobidnode;

  Arc::XMLNode uname = in["CacheLink"]["Username"];
  if (!uname) {
    logger.msg(Arc::ERROR, "No username supplied");
    return Arc::MCC_Status(Arc::GENERIC_ERROR, "CacheLink", "Bad input (no Username specified)");
  }
  std::string username = (std::string)uname;

  // TODO: try to force mapping to supplied user
  if (username != mapped_user.Name()) {
    logger.msg(Arc::ERROR, "Supplied username %s does not match mapped username %s", username, mapped_user.Name());
    return Arc::MCC_Status(Arc::GENERIC_ERROR, "CacheLink", "Supplied username does not match mapped user");
  }

  // check job id and session dir are ok
  std::string session_root = user.SessionRoot(jobid);
  if (session_root.empty()) {
    logger.msg(Arc::ERROR, "No session directory found");
    return Arc::MCC_Status(Arc::GENERIC_ERROR, "CacheLink", "No session directory found for supplied Job ID");
  }
  std::string session_dir = session_root + '/' + jobid;
  logger.msg(Arc::INFO, "Using session dir %s", session_dir);

  struct stat fileStat;
  if (!Arc::FileStat(session_dir, &fileStat, true)) {
    logger.msg(Arc::ERROR, "Failed to stat session dir %s", session_dir);
    return Arc::MCC_Status(Arc::GENERIC_ERROR, "CacheLink", "Failed to access session dir");
  }
  // check permissions - owner must be same as mapped user
  if (fileStat.st_uid != mapped_user.get_uid()) {
    logger.msg(Arc::ERROR, "Session dir %s is owned by %i, but current mapped user is %i", session_dir, fileStat.st_uid, mapped_user.get_uid());
    return Arc::MCC_Status(Arc::GENERIC_ERROR, "CacheLink", "Failed to access session dir");
  }

  // get delegated proxy info to check permission on cached files
  // TODO: use credentials of caller of this service. For now use the
  // proxy of the specified job in the control dir

  std::string proxy_path = user.ControlDir() + "/job." + jobid + ".proxy";
  if (!Arc::FileStat(proxy_path, &fileStat, true)) {
    logger.msg(Arc::ERROR, "Failed to access proxy of given job id at %s", proxy_path);
    return Arc::MCC_Status(Arc::GENERIC_ERROR, "CacheLink", "Failed to access proxy");
  }
  Arc::UserConfig usercfg;
  usercfg.UtilsDirPath(user.ControlDir());
  usercfg.ProxyPath(proxy_path);
  usercfg.InitializeCredentials();
  std::string dn;
  Arc::Time exp_time;
  try {
    Arc::Credential ci(usercfg.ProxyPath(), usercfg.ProxyPath(), usercfg.CACertificatesDirectory(), "");
    dn = ci.GetIdentityName();
    exp_time = ci.GetEndTime();
  } catch (Arc::CredentialError e) {
    logger.msg(Arc::ERROR, "Couldn't handle certificate: %s", e.what());
    return Arc::MCC_Status(Arc::GENERIC_ERROR, "CacheLink", std::string("Error with proxy at "+proxy_path));
  }
  logger.msg(Arc::INFO, "DN is %s", dn);

  // create cache
  Arc::FileCache cache(user.CacheParams().getCacheDirs(), jobid, mapped_user.get_uid(), mapped_user.get_gid());
  if (!cache) {
    logger.msg(Arc::ERROR, "Error with cache configuration");
    return Arc::MCC_Status(Arc::GENERIC_ERROR, "CacheCheck", "Server error with cache");
  }

  // set up response structure
  Arc::XMLNode resp = out.NewChild("CacheLinkResponse");
  Arc::XMLNode results = resp.NewChild("CacheLinkResult");

  std::map<std::string, std::string> to_download; // files not in cache (remote, local)
  bool error_happened = false; // if true then don't bother with downloads at the end

  // loop through all files
  for (int n = 0;;++n) {
    Arc::XMLNode id = in["CacheLink"]["TheseFilesNeedToLink"]["File"][n];
    if (!id) break;

    Arc::XMLNode f_url = id["FileURL"];
    if (!f_url) break;
    Arc::XMLNode f_name = id["FileName"];
    if (!f_name) break;

    std::string fileurl = (std::string)f_url;
    std::string filename = (std::string)f_name;

    logger.msg(Arc::INFO, "Looking up URL %s", fileurl);

    Arc::URL u(fileurl);
    Arc::DataHandle d(u, usercfg);
    d->SetSecure(false);
    // the actual url used with the cache
    std::string url = d->str();

    bool available = false;
    bool is_locked = false;

    if (!cache.Start(url, available, is_locked, true)) {
      Arc::XMLNode resultelement = results.NewChild("Result");
      resultelement.NewChild("FileURL") = fileurl;
      if (is_locked) {
        resultelement.NewChild("ReturnCode") = Arc::tostring(CacheService::Locked);
        resultelement.NewChild("ReturnCodeExplanation") = "File is locked";
      } else {
        resultelement.NewChild("ReturnCode") = Arc::tostring(CacheService::CacheError);
        resultelement.NewChild("ReturnCodeExplanation") = "Error starting cache";
      }
      error_happened = true;
      continue;
    }
    if (!available) {
      cache.Stop(url);
      // file not in cache - the result status for these files will be set later
      to_download[fileurl] = filename;
      continue;
    }
    Arc::XMLNode resultelement = results.NewChild("Result");
    resultelement.NewChild("FileURL") = fileurl;
    // file is in cache - check permissions
    if (!cache.CheckDN(url, dn)) {
      Arc::DataStatus res = d->Check();
      if (!res.Passed()) {
        logger.msg(Arc::ERROR, "Permission checking failed: %s", url);
        cache.Stop(url);
        resultelement.NewChild("ReturnCode") = Arc::tostring(CacheService::PermissionError);
        resultelement.NewChild("ReturnCodeExplanation") = "Permission denied";
        error_happened = true;
        continue;
      }
      cache.AddDN(url, dn, exp_time);
      logger.msg(Arc::VERBOSE, "Permission checking passed for url %s", url);
    }

    // link file
    std::string session_file = session_dir + '/' + filename;
    if (!cache.Link(session_file, url)) {
      // failed to link - report as if not there
      cache.Stop(url);
      resultelement.NewChild("ReturnCode") = Arc::tostring(CacheService::LinkError);
      resultelement.NewChild("ReturnCodeExplanation") = "Failed to link to session dir";
      error_happened = true;
      continue;
    }
    // everything went ok so stop cache and report success
    cache.Stop(url);
    resultelement.NewChild("ReturnCode") = Arc::tostring(CacheService::Success);
    resultelement.NewChild("ReturnCodeExplanation") = "Success";
  }

  // check for any downloads to perform, only if requested and there were no previous errors
  if (to_download.empty() || error_happened || !dostage) {
    for (std::map<std::string, std::string>::iterator i = to_download.begin(); i != to_download.end(); ++i) {
      Arc::XMLNode resultelement = results.NewChild("Result");
      resultelement.NewChild("FileURL") = i->first;
      resultelement.NewChild("ReturnCode") = Arc::tostring(CacheService::NotAvailable);
      resultelement.NewChild("ReturnCodeExplanation") = "File not available";
    }
    return Arc::MCC_Status(Arc::STATUS_OK);
  }

  // check max_downloads
  if (g_atomic_int_get(&current_downloads) >= max_downloads) {
    for (std::map<std::string, std::string>::iterator i = to_download.begin(); i != to_download.end(); ++i) {
      Arc::XMLNode resultelement = results.NewChild("Result");
      resultelement.NewChild("FileURL") = i->first;
      // fill in this status for unavailable files
      resultelement.NewChild("ReturnCode") = Arc::tostring(CacheService::TooManyDownloadsError);
      resultelement.NewChild("ReturnCodeExplanation") = "Currently at maximum limit of downloads";
    }
    return Arc::MCC_Status(Arc::STATUS_OK);
  }

  // launch download and wait for completion
  int result = Download(to_download, user, jobid, mapped_user);

  // if successful return success for each file
  // TODO: retry on cache lock? (result == 4)
  for (std::map<std::string, std::string>::iterator i = to_download.begin(); i != to_download.end(); ++i) {
    Arc::XMLNode resultelement = results.NewChild("Result");
    resultelement.NewChild("FileURL") = i->first;
    if (result == 0) {
      resultelement.NewChild("ReturnCode") = Arc::tostring(CacheService::Success);
      resultelement.NewChild("ReturnCodeExplanation") = "Success";
    }
    else {
      resultelement.NewChild("ReturnCode") = Arc::tostring(CacheService::DownloadError);
      resultelement.NewChild("ReturnCodeExplanation") = "Download failed";
    }
  }
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
  std::string mapped_username = inmsg.Attributes()->get("SEC:LOCALID");
  if (mapped_username.empty()) {
    logger.msg(Arc::ERROR, "No local user mapping found");
    return make_soap_fault(outmsg, "No local user mapping found");
  }
  Arc::User mapped_user(mapped_username);

  // temporary user created for this call
  // we don't care about validity of this user, it is just a place to store
  // config information
  JobUser jobuser(*gm_env);
  if (users->find(mapped_username) != users->end()) {
    jobuser.SetCacheParams(users->find(mapped_username)->CacheParams());
    jobuser.SetControlDir(users->find(mapped_username)->ControlDir());
    jobuser.SetSessionRoot(users->find(mapped_username)->SessionRoots());
  } else if (users->find("") != users->end()) {
    jobuser.SetCacheParams(users->find("")->CacheParams());
    jobuser.SetControlDir(users->find("")->ControlDir());
    jobuser.SetSessionRoot(users->find("")->SessionRoots());
  } else {
    logger.msg(Arc::ERROR, "No configuration found for user %s in A-REX configuration", mapped_username);
    return make_soap_fault(outmsg, "Server configuration error");
  }

  std::vector<std::string> caches = jobuser.CacheParams().getCacheDirs();
  if (caches.empty()) {
    logger.msg(Arc::ERROR, "No caches configured for user %s", jobuser.UnixName());
    return make_soap_fault(outmsg, "No caches configured for local user");
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
      result = CacheCheck(*inpayload, *outpayload, jobuser);
    }
    else if (MatchXMLName(op, "CacheLink")) {
      result = CacheLink(*inpayload, *outpayload, jobuser, mapped_user);
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
