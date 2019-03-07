#include <sys/stat.h>
#include <errno.h>
#include <unistd.h>

#include <arc/data/FileCache.h>
#include <arc/data/DataHandle.h>
#include <arc/URL.h>
#include <arc/StringConv.h>
#include <arc/UserConfig.h>
#include <arc/FileAccess.h>
#include <arc/FileUtils.h>
#include <arc/User.h>
#include <arc/Utils.h>

#include <arc/message/MessageAttributes.h>
#include <arc/message/PayloadRaw.h>
#include <arc/message/PayloadStream.h>
#include <arc/message/PayloadSOAP.h>
#include <arc/credential/Credential.h>

#include "CandyPond.h"

namespace CandyPond {

static Arc::Plugin *get_service(Arc::PluginArgument* arg)
{
    Arc::ServicePluginArgument* srvarg =
            arg?dynamic_cast<Arc::ServicePluginArgument*>(arg):NULL;
    if(!srvarg) return NULL;
    CandyPond* s = new CandyPond((Arc::Config*)(*srvarg),arg);
    if (*s)
      return s;
    delete s;
    return NULL;
}

Arc::Logger CandyPond::logger(Arc::Logger::rootLogger, "CandyPond");

CandyPond::CandyPond(Arc::Config *cfg, Arc::PluginArgument* parg) :
                                               Service(cfg,parg),
                                               dtr_generator(NULL) {
  valid = false;
  // read configuration information
  /*
  candypond config specifies A-REX conf file
    <candypond:config>/etc/arc.conf</candypond:config>
  */
  ns["candypond"] = "urn:candypond_config";

  if (!(*cfg)["service"] || !(*cfg)["service"]["config"]) {
    // error - no config defined
    logger.msg(Arc::ERROR, "No A-REX config file found in candypond configuration");
    return;
  }
  std::string arex_config = (std::string)(*cfg)["service"]["config"];
  logger.msg(Arc::INFO, "Using A-REX config file %s", arex_config);

  config.SetConfigFile(arex_config);
  if (!config.Load()) {
    logger.msg(Arc::ERROR, "Failed to process A-REX configuration in %s", arex_config);
    return;
  }
  config.Print();
  if (config.CacheParams().getCacheDirs().empty()) {
    logger.msg(Arc::ERROR, "No caches defined in configuration");
    return;
  }

  // check if we are running along with A-REX or standalone
  bool with_arex = false;
  if ((*cfg)["service"]["witharex"] && (std::string)(*cfg)["service"]["witharex"] == "true") with_arex = true;

  // start Generator for data staging
  dtr_generator = new CandyPondGenerator(config, with_arex);

  valid = true;
}

CandyPond::~CandyPond(void) {
  if (dtr_generator) {
    delete dtr_generator;
    dtr_generator = NULL;
  }
}

Arc::MCC_Status CandyPond::CacheCheck(Arc::XMLNode in, Arc::XMLNode out, const Arc::User& mapped_user) {
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

  // substitute cache paths according to mapped user
  ARex::CacheConfig cache_params(config.CacheParams());
  cache_params.substitute(config, mapped_user);
  Arc::FileCache cache(cache_params.getCacheDirs(), "0", mapped_user.get_uid(), mapped_user.get_gid());
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
    resultelement.NewChild("FileURL") = fileurl;
    bool fileexist = false;
    std::string file_lfn;
    Arc::initializeCredentialsType cred_type(Arc::initializeCredentialsType::SkipCredentials);
    Arc::UserConfig usercfg(cred_type);
    Arc::URL url(fileurl);
    Arc::DataHandle d(url, usercfg);

    if (!d) {
      logger.msg(Arc::ERROR, "Can't handle URL %s", fileurl);
      resultelement.NewChild("ExistInTheCache") = "false";
      resultelement.NewChild("FileSize") = "0";
      continue;
    }

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
      logger.msg(Arc::ERROR, "Problem accessing cache file %s: %s", file_lfn, Arc::StrError(errno));

    resultelement.NewChild("ExistInTheCache") = (fileexist ? "true": "false");
    if (fileexist)
      resultelement.NewChild("FileSize") = Arc::tostring(fileStat.st_size);
    else
      resultelement.NewChild("FileSize") = "0";
  }
  return Arc::MCC_Status(Arc::STATUS_OK);
}

Arc::MCC_Status CandyPond::CacheLink(Arc::XMLNode in, Arc::XMLNode out, const Arc::User& mapped_user) {
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
     <Priority>90</Priority>
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

  int priority = 50;
  Arc::XMLNode prioritynode = in["CacheLink"]["Priority"];
  if (prioritynode) {
    if (!Arc::stringto((std::string)prioritynode, priority)) {
      logger.msg(Arc::ERROR, "Bad number in priority element: %s", (std::string)prioritynode);
      return Arc::MCC_Status(Arc::GENERIC_ERROR, "CacheLink", "Bad input (bad number in Priority)");
    }
    if (priority <= 0) priority = 1;
    if (priority > 100) priority = 100;
  }

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
  // substitute session dirs and use tmp configuration to find the one for this job
  std::vector<std::string> sessions = config.SessionRoots();
  for (std::vector<std::string>::iterator session = sessions.begin(); session != sessions.end(); ++session) {
    config.Substitute(*session, mapped_user);
  }
  ARex::GMConfig tmp_config;
  tmp_config.SetSessionRoot(sessions);
  std::string session_root = tmp_config.SessionRoot(jobid);
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
  // TODO: use credentials of caller of this service. For now ask the
  // delegation store for the proxy of the job.

  ARex::DelegationStore::DbType deleg_db_type = ARex::DelegationStore::DbBerkeley;
  switch (config.DelegationDBType()) {
   case ARex::GMConfig::deleg_db_bdb:
    deleg_db_type = ARex::DelegationStore::DbBerkeley;
    break;
   case ARex::GMConfig::deleg_db_sqlite:
    deleg_db_type = ARex::DelegationStore::DbSQLite;
    break;
  }

  ARex::DelegationStore dstore(config.DelegationDir(), deleg_db_type, false);
  std::string proxy_path;
  // Read job's local file to extract delegation id
  ARex::JobLocalDescription job_desc;
  if (job_local_read_file(jobid, config, job_desc) && !job_desc.delegationid.empty()) {
     proxy_path = dstore.FindCred(job_desc.delegationid, job_desc.DN);
  }

  if (proxy_path.empty() || !Arc::FileStat(proxy_path, &fileStat, true)) {
    logger.msg(Arc::ERROR, "Failed to access proxy of given job id %s at %s", jobid, proxy_path);
    return Arc::MCC_Status(Arc::GENERIC_ERROR, "CacheLink", "Failed to access proxy");
  }

  Arc::UserConfig usercfg;
  usercfg.UtilsDirPath(config.ControlDir());
  usercfg.ProxyPath(proxy_path);
  usercfg.InitializeCredentials(Arc::initializeCredentialsType::NotTryCredentials);
  std::string dn;
  Arc::Time exp_time;
  try {
    Arc::Credential ci(usercfg.ProxyPath(), usercfg.ProxyPath(), usercfg.CACertificatesDirectory(), "");
    dn = ci.GetIdentityName();
    exp_time = ci.GetEndTime();
  } catch (Arc::CredentialError& e) {
    logger.msg(Arc::ERROR, "Couldn't handle certificate: %s", e.what());
    return Arc::MCC_Status(Arc::GENERIC_ERROR, "CacheLink", std::string("Error with proxy at "+proxy_path));
  }
  logger.msg(Arc::INFO, "DN is %s", dn);

  // create cache
  // substitute cache paths according to mapped user
  ARex::CacheConfig cache_params(config.CacheParams());
  cache_params.substitute(config, mapped_user);
  Arc::FileCache cache(cache_params.getCacheDirs(), jobid, mapped_user.get_uid(), mapped_user.get_gid());
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
    std::string session_file = session_dir + '/' + filename;

    Arc::XMLNode resultelement = results.NewChild("Result");

    logger.msg(Arc::INFO, "Looking up URL %s", fileurl);
    resultelement.NewChild("FileURL") = fileurl;

    Arc::URL u(fileurl);
    Arc::DataHandle d(u, usercfg);
    if (!d) {
      logger.msg(Arc::ERROR, "Can't handle URL %s", fileurl);
      resultelement.NewChild("ReturnCode") = Arc::tostring(CandyPond::BadURLError);
      resultelement.NewChild("ReturnCodeExplanation") = "Could not hande input URL";
      error_happened = true;
      continue;
    }

    d->SetSecure(false);
    // the actual url used with the cache
    std::string url = d->str();

    bool available = false;
    bool is_locked = false;

    if (!cache.Start(url, available, is_locked)) {
      if (is_locked) {
        resultelement.NewChild("ReturnCode") = Arc::tostring(CandyPond::Locked);
        resultelement.NewChild("ReturnCodeExplanation") = "File is locked";
      } else {
        resultelement.NewChild("ReturnCode") = Arc::tostring(CandyPond::CacheError);
        resultelement.NewChild("ReturnCodeExplanation") = "Error starting cache";
      }
      error_happened = true;
      continue;
    }
    if (!available) {
      cache.Stop(url);
      // file not in cache - the result status for these files will be set later
      to_download[fileurl] = session_file;
      continue;
    }
    // file is in cache - check permissions
    if (!cache.CheckDN(url, dn)) {
      Arc::DataStatus res = d->Check(false);
      if (!res.Passed()) {
        logger.msg(Arc::ERROR, "Permission checking failed: %s", url);
        resultelement.NewChild("ReturnCode") = Arc::tostring(CandyPond::PermissionError);
        resultelement.NewChild("ReturnCodeExplanation") = "Permission denied";
        error_happened = true;
        continue;
      }
      cache.AddDN(url, dn, exp_time);
      logger.msg(Arc::VERBOSE, "Permission checking passed for url %s", url);
    }

    // link file
    bool try_again = false;
    // TODO add executable and copy flags to request
    if (!cache.Link(session_file, url, false, false, false, try_again)) {
      // If locked, send to DTR and let it deal with the retry strategy
      if (try_again) {
        to_download[fileurl] = session_file;
        continue;
      }
      // failed to link - report as if not there
      resultelement.NewChild("ReturnCode") = Arc::tostring(CandyPond::LinkError);
      resultelement.NewChild("ReturnCodeExplanation") = "Failed to link to session dir";
      error_happened = true;
      continue;
    }
    // Successfully linked to session - move to scratch if necessary
    if (!config.ScratchDir().empty()) {
      std::string scratch_file(config.ScratchDir()+'/'+jobid+'/'+filename);
      // Access session and scratch under mapped uid
      Arc::FileAccess fa;
      if (!fa.fa_setuid(mapped_user.get_uid(), mapped_user.get_gid()) ||
          !fa.fa_rename(session_file, scratch_file)) {
        logger.msg(Arc::ERROR, "Failed to move %s to %s: %s", session_file, scratch_file, Arc::StrError(errno));
        resultelement.NewChild("ReturnCode") = Arc::tostring(CandyPond::LinkError);
        resultelement.NewChild("ReturnCodeExplanation") = "Failed to link to move file from session dir to scratch";
        error_happened = true;
        continue;
      }
    }

    // everything went ok so report success
    resultelement.NewChild("ReturnCode") = Arc::tostring(CandyPond::Success);
    resultelement.NewChild("ReturnCodeExplanation") = "Success";
  }

  // check for any downloads to perform, only if requested and there were no previous errors
  if (to_download.empty() || error_happened || !dostage) {
    for (std::map<std::string, std::string>::iterator i = to_download.begin(); i != to_download.end(); ++i) {
      Arc::XMLNode resultelement = results.NewChild("Result");
      resultelement.NewChild("FileURL") = i->first;
      resultelement.NewChild("ReturnCode") = Arc::tostring(CandyPond::NotAvailable);
      resultelement.NewChild("ReturnCodeExplanation") = "File not available";
    }
    return Arc::MCC_Status(Arc::STATUS_OK);
  }

  bool stage_start_error = false;

  // Loop through files to download and start a DTR for each one
  for (std::map<std::string, std::string>::iterator i = to_download.begin(); i != to_download.end(); ++i) {
    Arc::XMLNode resultelement = results.NewChild("Result");
    resultelement.NewChild("FileURL") = i->first;

    // if one DTR failed to start then don't start any more
    // TODO cancel others already started
    if (stage_start_error) {
      resultelement.NewChild("ReturnCode") = Arc::tostring(CandyPond::DownloadError);
      resultelement.NewChild("ReturnCodeExplanation") = "Failed to start data staging";
      continue;
    }

    logger.msg(Arc::VERBOSE, "Starting new DTR for %s", i->first);
    if (!dtr_generator->addNewRequest(mapped_user, i->first, i->second, usercfg, jobid, priority)) {
      logger.msg(Arc::ERROR, "Failed to start new DTR for %s", i->first);
      resultelement.NewChild("ReturnCode") = Arc::tostring(CandyPond::DownloadError);
      resultelement.NewChild("ReturnCodeExplanation") = "Failed to start data staging";
      stage_start_error = true;
    }
    else {
      resultelement.NewChild("ReturnCode") = Arc::tostring(CandyPond::Staging);
      resultelement.NewChild("ReturnCodeExplanation") = "Staging started";
    }
  }
  return Arc::MCC_Status(Arc::STATUS_OK);
}

Arc::MCC_Status CandyPond::CacheLinkQuery(Arc::XMLNode in, Arc::XMLNode out) {
  /*
   Accepts:
   <CacheLinkQuery>
     <JobID>123456789</JobID>
   </CacheLinkQuery>

   Returns:
   <CacheLinkQueryResponse>
     <CacheLinkQueryResult>
       <Result>
         <ReturnCode>0</ReturnCode>
         <ReturnExplanation>success</ReturnExplanation>
       </Result>
     </CacheLinkQueryResult>
   </CacheLinkQueryResponse>
   */

  Arc::XMLNode jobidnode = in["CacheLinkQuery"]["JobID"];
  if (!jobidnode) {
    logger.msg(Arc::ERROR, "No job ID supplied");
    return Arc::MCC_Status(Arc::GENERIC_ERROR, "CacheLinkQuery", "Bad input (no JobID specified)");
  }
  std::string jobid = (std::string)jobidnode;

  // set up response structure
  Arc::XMLNode resp = out.NewChild("CacheLinkQueryResponse");
  Arc::XMLNode results = resp.NewChild("CacheLinkQueryResult");
  Arc::XMLNode resultelement = results.NewChild("Result");

  std::string error;
  // query Generator for DTR status
  if (dtr_generator->queryRequestsFinished(jobid, error)) {
    if (error.empty()) {
      logger.msg(Arc::INFO, "Job %s: all files downloaded successfully", jobid);
      resultelement.NewChild("ReturnCode") = Arc::tostring(CandyPond::Success);
      resultelement.NewChild("ReturnCodeExplanation") = "Success";
    }
    else if (error == "Job not found") {
      resultelement.NewChild("ReturnCode") = Arc::tostring(CandyPond::CacheError);
      resultelement.NewChild("ReturnCodeExplanation") = "No such job";
    }
    else {
      logger.msg(Arc::INFO, "Job %s: Some downloads failed", jobid);
      resultelement.NewChild("ReturnCode") = Arc::tostring(CandyPond::DownloadError);
      resultelement.NewChild("ReturnCodeExplanation") = "Download failed: " + error;
    }
  }
  else {
    logger.msg(Arc::VERBOSE, "Job %s: files still downloading", jobid);
    resultelement.NewChild("ReturnCode") = Arc::tostring(CandyPond::Staging);
    resultelement.NewChild("ReturnCodeExplanation") = "Still staging";
  }

  return Arc::MCC_Status(Arc::STATUS_OK);
}


Arc::MCC_Status CandyPond::process(Arc::Message &inmsg, Arc::Message &outmsg) {

  // Check authorization
  if(!ProcessSecHandlers(inmsg, "incoming")) {
    logger.msg(Arc::ERROR, "CandyPond: Unauthorized");
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
      result = CacheCheck(*inpayload, *outpayload, mapped_user);
    }
    else if (MatchXMLName(op, "CacheLink")) {
      result = CacheLink(*inpayload, *outpayload, mapped_user);
    }
    else if (MatchXMLName(op, "CacheLinkQuery")) {
      result = CacheLinkQuery(*inpayload, *outpayload);
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
    logger.msg(Arc::ERROR, "Only POST is supported in CandyPond");
    return Arc::MCC_Status();
  }
  return Arc::MCC_Status(Arc::STATUS_OK);
}

Arc::MCC_Status CandyPond::make_soap_fault(Arc::Message& outmsg, const std::string& reason) {
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

} // namespace CandyPond

extern Arc::PluginDescriptor const ARC_PLUGINS_TABLE_NAME[] = {
    { "candypond", "HED:SERVICE", NULL, 0, &CandyPond::get_service },
    { NULL, NULL, NULL, 0, NULL }
};
