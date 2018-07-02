// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdexcept>
#include <sys/stat.h>

#include <arc/credential/Credential.h>
#include <arc/FileUtils.h>

#include "../grid-manager/jobs/CommFIFO.h"
#include "../grid-manager/jobs/JobDescriptionHandler.h"
#include "../grid-manager/conf/GMConfig.h"
#include "../grid-manager/files/ControlFileHandling.h"

#include "JobStateINTERNAL.h"
#include "INTERNALClient.h"

//#include "../job.cpp"

using namespace Arc;

namespace ARexINTERNAL {


  Arc::Logger INTERNALClient::logger(Arc::Logger::rootLogger, "INTERNAL Client"); 


  INTERNALClient::INTERNALClient(void) : config(NULL), arexconfig(NULL) {

    logger.msg(Arc::DEBUG,"Default INTERNAL client contructor");

    if(!SetAndLoadConfig(config, cfgfile)){
      logger.msg(Arc::ERROR,"Failed to load grid-manager configfile");
      return;
    }

    if(!SetEndPoint(config)){
      logger.msg(Arc::ERROR,"Failed to set INTERNAL endpoint");
      return;
    }

    user = Arc::User();
    PrepareARexConfig();

  };


  INTERNALClient::INTERNALClient(const Arc::UserConfig& usercfg)
  :usercfg(usercfg),
   config(NULL), arexconfig(NULL) {

    if(!SetAndLoadConfig(config, cfgfile)){
      logger.msg(Arc::ERROR,"Failed to load grid-manager configfile");
      return;
    }

    if(!SetEndPoint(config)){
      logger.msg(Arc::ERROR,"Failed to set INTERNAL endpoint");
      return;
    }

    user = Arc::User();
    PrepareARexConfig();

  };


  //using this one from submitterpluginlocal
  INTERNALClient::INTERNALClient(const Arc::URL& url, const Arc::UserConfig& usercfg)
    :ce(url),
     usercfg(usercfg),
     config(NULL), arexconfig(NULL) {

    if(!SetAndLoadConfig(config, cfgfile)){
      logger.msg(Arc::ERROR,"Failed to load grid-manager configfile");
      return;
    }

    if(!SetEndPoint(config)){
      logger.msg(Arc::ERROR,"Failed to set INTERNAL endpoint");
      return;
    }

    user = Arc::User();
    PrepareARexConfig();

  };


  

  INTERNALClient::~INTERNALClient() {
   delete config;
   delete arexconfig;
  }

 

  INTERNALJob::INTERNALJob(/*const */ARex::ARexJob& _arexjob, const ARex::GMConfig& config, std::string const& _deleg_id)
    :id(_arexjob.ID()),
     state((std::string)_arexjob.State()),
     sessiondir(_arexjob.SessionDir()),
     controldir(config.ControlDir()),
     delegation_id(_deleg_id)
  {
    stageout.push_back(_arexjob.SessionDir());
    stagein.push_back(_arexjob.SessionDir());
  }




  bool INTERNALClient::SetEndPoint(ARex::GMConfig*& config){

    endpoint = config->ControlDir();
    return true;
  }

  
  
  bool INTERNALClient::SetAndLoadConfig(ARex::GMConfig*& config, std::string cfgfile){

    struct stat st;
    config = new ARex::GMConfig();
    config->SetDelegations(&deleg_stores);
    
    if(!config->Load()){
      logger.msg(Arc::ERROR,"Failed to load grid-manager config file");
      return false;
    }

    ARex::DelegationStore::DbType deleg_db_type = ARex::DelegationStore::DbBerkeley;
    switch(config->DelegationDBType()) {
      case ARex::GMConfig::deleg_db_bdb:
        deleg_db_type = ARex::DelegationStore::DbBerkeley;
        break;
      case ARex::GMConfig::deleg_db_sqlite:
        deleg_db_type = ARex::DelegationStore::DbSQLite;
        break;
    };
    deleg_stores.SetDbType(deleg_db_type);

    config->Print();
    return true;
  }
  

  bool INTERNALClient::PrepareARexConfig(){

    Arc::Credential cred(usercfg);
    std::string gridname = cred.GetIdentityName();
     arexconfig = new ARex::ARexGMConfig(*config,user.Name(),gridname,endpoint);
     return true;
  }

  


  bool INTERNALClient::CreateDelegation(std::string& deleg_id){
    // Create new delegation slot in delegation store and
    // generate or apply delegation id.

    Arc::Credential cred(usercfg);
    std::string gridname = cred.GetIdentityName();

    std::string proxy_data;
    std::string proxy_part1;
    std::string proxy_part2;
    std::string proxy_part3;
    cred.OutputCertificate(proxy_part1);
    cred.OutputPrivatekey(proxy_part2);
    cred.OutputCertificateChain(proxy_part3);
    proxy_data = proxy_part1 + proxy_part2 + proxy_part3;

    
    ARex::DelegationStore& deleg = deleg_stores[config->DelegationDir()];
    if(!deleg.AddCred(deleg_id, gridname, proxy_data)) {
      error_description="Failed to store delegation.";
      logger.msg(Arc::ERROR, "%s", error_description);
      return false;
    }

    return true;
  }


  bool INTERNALClient::RenewDelegation(std::string const& deleg_id) {
    // Create new delegation in already assigned slot 
    if(deleg_id.empty()) return false;

    Arc::Credential cred(usercfg);
    std::string gridname = cred.GetIdentityName();

    std::string proxy_data;
    //std::string proxy_key;
    //cred.OutputCertificateChain(proxy_data);
    //cred.OutputPrivatekey(proxy_key);
    //proxy_data = proxy_key + proxy_data;

    //usercfg.CredentialString(proxy_data);


    std::string proxy_part1;
    std::string proxy_part2;
    std::string proxy_part3;
    cred.OutputCertificate(proxy_part1);
    cred.OutputPrivatekey(proxy_part2);
    cred.OutputCertificateChain(proxy_part3);
    proxy_data = proxy_part1 + proxy_part2 + proxy_part3;
    
    ARex::DelegationStore& deleg = deleg_stores[config->DelegationDir()];
    if(!deleg.PutCred(deleg_id, gridname, proxy_data)) {
      error_description="Failed to store delegation.";
      logger.msg(Arc::ERROR, "%s", error_description);
      return false;
    }

    return true;
  }


  std::string INTERNALClient::get_error_description() const {
    if (!error_description.empty()) return error_description;
  }
 
  
  bool INTERNALClient::submit(const std::list<Arc::JobDescription>& jobdescs,std::list<INTERNALJob>& localjobs, const std::string delegation_id) {
    //called by SubmitterPluginINTERNAL ac->submit(..)

    logger.msg(Arc::VERBOSE, "Submitting job ");


    bool noFailures = true;
    int limit = 1000000; // 1 M - Safety
    std::list<Arc::JobDescription>::const_iterator itSubmit = jobdescs.begin(), itLastProcessedEnd = jobdescs.begin();
    while (itSubmit != jobdescs.end() && limit > 0) {
      for (int i = 0; itSubmit != jobdescs.end() && i < limit; ++itSubmit, ++i) {



        INTERNALJob localjob;
        //set some additional parameters
        if(config->DefaultQueue().empty() && (config->Queues().size() == 1)) {
          config->SetDefaultQueue(*(config->Queues().begin()));
        }


        ARex::JobDescriptionHandler job_desc_handler(*config);
        ARex::JobLocalDescription job_desc;
        std::string jobdesc_str;
        Arc::JobDescriptionResult ures = (*itSubmit).UnParse(jobdesc_str,"emies:adl");
        Arc::XMLNode jsdl(jobdesc_str);
        ARex::JobIDGeneratorINTERNAL idgenerator(endpoint);
        const std::string dummy = "";



        ARex::ARexJob arexjob(jsdl,*arexconfig,delegation_id,dummy,logger,idgenerator);
        
        if(!arexjob){
          logger.msg(Arc::ERROR, "%s",arexjob.Failure());
          return false;
        }
        else{
          //make localjob for internal handling
          INTERNALJob localjob(arexjob,*config,delegation_id);
          localjobs.push_back(localjob);
        }
      }
      itLastProcessedEnd = itSubmit;
    }

    return noFailures;
  }


  bool INTERNALClient::putFiles(INTERNALJob const& localjob, std::list<std::string> const& sources, std::list<std::string> const& destinations) {
    ARex::GMJob gmjob(localjob.id, user, localjob.sessiondir, ARex::JOB_STATE_ACCEPTED);
    //Fix-me removed cbegin and cend from sources and destination. Either fix compiler, or rewrite to be const. 
    for(std::list<std::string>::const_iterator source = sources.begin(), destination = destinations.begin();
               source != sources.end() && destination != destinations.end(); ++source, ++destination) {
      std::string path = localjob.sessiondir + "/" + *destination;
      std::string fn = "/" + *destination;
      // TODO: direct copy will not work if session is on NFS
      if(!FileCopy(*source, path)) {
        logger.msg(Arc::ERROR, "Failed to copy input file: %s to path: %s",path);
        return false;
      }
      
      if((!ARex::fix_file_permissions(path,false)) || // executable flags is handled by A-Rex
         (!ARex::fix_file_owner(path,gmjob))) {
        logger.msg(Arc::ERROR, "Failed to set permissions on: %s",path);
        //clean job here? At the moment job is left in limbo in control and sessiondir
        clean(localjob.id);
        return false;
      }
      ARex::job_input_status_add_file(gmjob,*config,fn);
    }
    (void)ARex::CommFIFO::Signal(config->ControlDir(), localjob.id);
    return true;
  }


  bool INTERNALClient::submit(const Arc::JobDescription& jobdesc, INTERNALJob& localjob, const std::string delegation_id) {

    std::list<JobDescription> jobdescs;
    std::list<INTERNALJob> localjobs;

    jobdescs.push_back(jobdesc);

    if(!submit(jobdescs, localjobs, delegation_id))
      return false;

    localjob = localjobs.back();

    return true;

  }



  bool INTERNALClient::info(std::list<INTERNALJob>& jobs, std::list<INTERNALJob>& jobids_found){

    //at the moment called by JobListretrieverPluginINTERNAL Query

    for(std::list<INTERNALJob>::iterator job = jobs.begin(); job!= jobs.end(); job++){
      ARex::ARexJob arexjob(job->id,*arexconfig,logger);
         std::string state = arexjob.State();
      if (state != "UNDEFINED") jobids_found.push_back(*job);
    }
   
    return true;
  }


  bool INTERNALClient::info(INTERNALJob& localjob, Arc::Job& arcjob){
    //Called from (at least) JobControllerPluginINTERNAL
    //Called for stagein/out/sessionodir if url of either is not known
    //Extracts information about current arcjob from arexjob and job.jobid.description file and updates/populates the localjob and arcjob with this info, and fills a localjob with the information

    std::vector<std::string> tokens;
    Arc::tokenize(arcjob.JobID, tokens, "/"); 
    //NB! Add control that the arcjob.jobID is in correct format
    localjob.id = (std::string)tokens.back();
    ARex::JobId gm_job_id = localjob.id;


    ARex::ARexJob arexjob(gm_job_id,*arexconfig,logger);
    arcjob.State = JobStateINTERNAL((std::string)arexjob.State());


    if(!localjob.delegation_id.empty())
      arcjob.DelegationID.push_back(localjob.delegation_id);

    //Get other relevant info from the .info file
    ARex::JobLocalDescription job_desc;
    if(!ARex::job_local_read_file(gm_job_id,*config,job_desc)) {
      error_description="Job is probably corrupted: can't read internal information.";
      logger.msg(Arc::ERROR, "%s", error_description);
      return false;
    };

    
    //JobControllerPluginINTERNAL needs this, so make sure it is set.
    if(localjob.session.empty()){
      localjob.session.push_back((std::string)job_desc.sessiondir);
    }
     if(localjob.stagein.empty()){
      //assume that it is sessiondir
      localjob.stagein.push_back((std::string)job_desc.sessiondir);
    }
    if(localjob.stageout.empty()){
      //assume that it is sessiondir
      localjob.stageout.push_back((std::string)job_desc.sessiondir);
    }
    return true;
  }




  bool INTERNALClient::sstat(Arc::XMLNode& xmldoc) {

    //TO-DO Need to lock info.xml during reading?
    std::string fname = config->ControlDir() + "/" + "info.xml";
    std::string xmlstring;
    
    (void)Arc::FileRead(fname, xmlstring);
    if(xmlstring.empty()){
      error_description="Failed to obtain resource information.";
      logger.msg(Arc::ERROR, "%s", error_description);
      return false;
    }


    XMLNode tmp(xmlstring);
    XMLNode services = tmp["Domains"]["AdminDomain"]["Services"];

    if(!services) {
      lfailure = "Missing Services in response";
      return false;
    }
    services.Move(xmldoc);
    return true;
  }





  bool INTERNALClient::kill(const std::string& jobid){
    //jobid is full url
    std::vector<std::string> tokens;
    Arc::tokenize(jobid, tokens, "/"); 
    std::string thisid = (std::string)tokens.back();

    ARex::ARexJob arexjob(thisid,*arexconfig,logger);
    arexjob.Cancel();
    return true;
  }


  bool INTERNALClient::clean(const std::string& jobid){
    //jobid is full url
    std::vector<std::string> tokens;
    Arc::tokenize(jobid, tokens, "/"); 
    std::string thisid = (std::string)tokens.back();

    ARex::ARexJob arexjob(thisid,*arexconfig,logger);
    arexjob.Clean();
    return true;
  }

  
  bool INTERNALClient::restart(const std::string& jobid){
    //jobid is full url
    std::vector<std::string> tokens;
    Arc::tokenize(jobid, tokens, "/"); 
    std::string thisid = (std::string)tokens.back();

    ARex::ARexJob arexjob(thisid,*arexconfig,logger);
    arexjob.Resume();
    return true;
  }


  bool INTERNALClient::list(std::list<INTERNALJob>& jobs){
    //Populates localjobs containing only jobid
    //how do I want to search for jobs in system? 

    std::string cdir=config->ControlDir();
    Glib::Dir *dir=new Glib::Dir(cdir);
    std::string file_name;
    while ((file_name = dir->read_name()) != "") {
      std::vector<std::string> tokens;
      Arc::tokenize(file_name, tokens, "."); // look for job.id.local
      if (tokens.size() == 3 && tokens[0] == "job" && tokens[2] == "local") {
        INTERNALJob job;
        job.id = (std::string)tokens[1];
        jobs.push_back(job);
      };
      dir->close();
      delete dir;
    }
    return true;
  }
  

 
  INTERNALJob& INTERNALJob::operator=(const Arc::Job& job) {
    //Set localjob attributes from the ARC job
    //Called from JobControllerPlugin

    stagein.clear();
    session.clear();
    stageout.clear();
    if (job.StageInDir) stagein.push_back(job.StageInDir);
    if (job.StageOutDir) stageout.push_back(job.StageOutDir);
    if (job.SessionDir) session.push_back(job.SessionDir);
    id = job.JobID;
    manager = job.JobManagementURL;
    resource = job.ServiceInformationURL;
    delegation_id = job.DelegationID.empty()?std::string(""):*job.DelegationID.begin();
    // State information is not transfered from Job object. Currently not needed.
    return *this;
  }
  
  


  void INTERNALJob::toJob(INTERNALClient* client, INTERNALJob* localjob, Arc::Job& j) const {
    //fills an arcjob from localjob

    //j.JobID = (client->ce).str() + "/" + localjob->id;
    j.JobID = "file://"  + sessiondir;
    j.ServiceInformationURL = client->ce;
    j.ServiceInformationInterfaceName = "org.nordugrid.internal";
    j.JobStatusURL = client->ce;
    j.JobStatusInterfaceName = "org.nordugrid.internal";
    j.JobManagementURL = client->ce;
    j.JobManagementInterfaceName = "org.nordugrid.internal";
    j.IDFromEndpoint = id;
    if (!stagein.empty())j.StageInDir = stagein.front();
    else j.StageInDir = sessiondir;
    if (!stageout.empty())j.StageOutDir = stageout.front();
    else  j.StageOutDir = sessiondir;
    if (!session.empty()) j.SessionDir = session.front();
    else j.SessionDir = sessiondir;

    j.DelegationID.clear();
    if(!(localjob->delegation_id).empty()) j.DelegationID.push_back(localjob->delegation_id);
    
  }


  void INTERNALJob::toJob(INTERNALClient* client, Arc::Job& arcjob, Arc::Logger& logger) const {
    //called from UpdateJobs in JobControllerPluginINTERNAL
    //extract info from arexjob
  
    //extract jobid from arcjob, which is the full jobid url
    std::vector<std::string> tokens;
    ARex::JobId arcjobid = arcjob.JobID;
    Arc::tokenize(arcjob.JobID, tokens, "/"); 
    //NB! Add control that the arcjob.jobID is in correct format
    ARex::JobId gm_job_id = (std::string)tokens.back();


    ARex::ARexJob arexjob(gm_job_id,*(client->arexconfig),client->logger);
    std::string state = arexjob.State();
    arcjob.State = JobStateINTERNAL(state);

    if (!stagein.empty())arcjob.StageInDir = stagein.front();
    else arcjob.StageInDir = sessiondir;
    if (!stageout.empty()) arcjob.StageOutDir = stageout.front();
    else  arcjob.StageOutDir = sessiondir;
    if (!session.empty()) arcjob.StageInDir = session.front();
    else arcjob.SessionDir = sessiondir;
   
  }

 

// -----------------------------------------------------------------------------

  // TODO: does it need locking?

  INTERNALClients::INTERNALClients(const Arc::UserConfig& usercfg):usercfg_(usercfg) {
  }

  INTERNALClients::~INTERNALClients(void) {
    std::multimap<Arc::URL, INTERNALClient*>::iterator it;
    for (it = clients_.begin(); it != clients_.end(); it = clients_.begin()) {
      delete it->second;
    }
  }

}
