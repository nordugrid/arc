// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdexcept>
#include <sys/stat.h>

#include <arc/credential/Credential.h>
#include <arc/credential/VOMSUtil.h>
#include <arc/FileUtils.h>
#include <arc/ArcLocation.h>
#include <arc/message/MCCLoader.h>

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

    if(!SetAndLoadConfig()){
      logger.msg(Arc::ERROR,"Failed to load grid-manager configfile");
      return;
    }

    if(!SetEndPoint()){
      logger.msg(Arc::ERROR,"Failed to set INTERNAL endpoint");
      return;
    }

    MapLocalUser();
    PrepareARexConfig();

  };


  INTERNALClient::INTERNALClient(const Arc::UserConfig& usercfg)
  :usercfg(usercfg),
   config(NULL), arexconfig(NULL) {

    if(!SetAndLoadConfig()){
      logger.msg(Arc::ERROR,"Failed to load grid-manager configfile");
      return;
    }

    if(!SetEndPoint()){
      logger.msg(Arc::ERROR,"Failed to set INTERNAL endpoint");
      return;
    }

    MapLocalUser();
    PrepareARexConfig();

  };


  //using this one from submitterpluginlocal
  INTERNALClient::INTERNALClient(const Arc::URL& url, const Arc::UserConfig& usercfg)
    :ce(url),
     usercfg(usercfg),
     config(NULL), arexconfig(NULL) {

    if(!SetAndLoadConfig()){
      logger.msg(Arc::ERROR,"Failed to load grid-manager configfile");
      return;
    }

    if(!SetEndPoint()){
      logger.msg(Arc::ERROR,"Failed to set INTERNAL endpoint");
      return;
    }

    MapLocalUser();
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




  bool INTERNALClient::SetEndPoint(){
    endpoint = config->ControlDir();
    return true;
  }

  
  
  bool INTERNALClient::SetAndLoadConfig(){
    cfgfile = ARex::GMConfig::GuessConfigFile();
    if (cfgfile.empty()) {
      logger.msg(Arc::ERROR,"Failed to identify grid-manager config file");
      return false;
    }

    // Push configuration through pre-parser in order to setup default values.
    // We are only interested in pidfile location because this is where 
    // fully pre-processed configuration file resides.
    std::list<std::string> parser_args;
    parser_args.push_back(Arc::ArcLocation::GetToolsDir() + "/arcconfig-parser");
    parser_args.push_back("--config");
    parser_args.push_back(cfgfile);
    parser_args.push_back("-b");
    parser_args.push_back("arex");
    parser_args.push_back("-o");
    parser_args.push_back("pidfile");
    Arc::Run parser(parser_args);
    std::string pidfile;
    parser.AssignStdout(pidfile);
    if((!parser.Start()) || (!parser.Wait())) {
      logger.msg(Arc::ERROR,"Failed to run configuration parser at %s.", parser_args.front());
      return false;
    }
    if(parser.Result() != 0) {
      logger.msg(Arc::ERROR,"Parser failed with error code %i.", (int)parser.Result());
      return false;
    }
    pidfile = Arc::trim(pidfile, "\r\n"); // parser adds EOLs
    struct stat st;
    if(!FileStat(pidfile, &st, true)) {
      logger.msg(Arc::ERROR,"No pid file is found at '%s'. Probably A-REX is not running.", pidfile);
      return false;
    }

    // Actual config file location
    cfgfile = pidfile;
    std::string::size_type dot_pos = cfgfile.find_last_of("./");
    if((dot_pos != std::string::npos) && (cfgfile[dot_pos] == '.'))
      cfgfile.resize(dot_pos);
    cfgfile += ".cfg";

    config = new ARex::GMConfig(cfgfile);
    config->SetDelegations(&deleg_stores);
    
    if(!config->Load()){
      logger.msg(Arc::ERROR,"Failed to load grid-manager config file from %s", cfgfile);
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
  

  // Security attribute simulating information pulled from TLS layer.
  class TLSSecAttr: public SecAttr {
  public:
    TLSSecAttr(Arc::UserConfig& usercfg) {
      Arc::Credential cred(usercfg);
      identity_ = cred.GetIdentityName();

      Arc::VOMSTrustList trust_list;
      trust_list.AddRegex("^.*$");
      std::vector<VOMSACInfo> voms;
      if(parseVOMSAC(cred, usercfg.CACertificatesDirectory(), usercfg.CACertificatePath(), usercfg.VOMSESPath()/*?*/, trust_list, voms, true, true)) {
        for(std::vector<VOMSACInfo>::const_iterator v = voms.begin(); v != voms.end();++v) { 
          if(!(v->status & VOMSACInfo::Error)) {
            for(std::vector<std::string>::const_iterator a = v->attributes.begin(); a != v->attributes.end();++a) {
              voms_.push_back(VOMSFQANToFull(v->voname,*a));
            };
          };
        };
      };
    }

    virtual ~TLSSecAttr(void) {
    }

    virtual operator bool(void) const {
      return true;
    }

    virtual bool Export(SecAttrFormat format,XMLNode &val) const {
      return false;
    }

    virtual std::string get(const std::string& id) const {
      if(id == "IDENTITY") return identity_;
      std::list<std::string> items = getAll(id);
      if(!items.empty()) return *items.begin();
      return "";
    }

    virtual std::list<std::string> getAll(const std::string& id) const {
      if(id == "VOMS") {
        return voms_;
      };
      return SecAttr::getAll(id);
    }

    std::string const& Identity() const {
      return identity_;
    }

  protected:
    std::string identity_; // Subject of last non-proxy certificate
    std::list<std::string> voms_; // VOMS attributes from the VOMS extension of proxy

    virtual bool equal(const SecAttr &b) const {
      return false;
    }

  };



  bool INTERNALClient::MapLocalUser(){
    if(!arexconfig) {
      logger.msg(Arc::ERROR, "INTERNALClient is not initialized");
      return false;
    }

    Arc::Credential cred(usercfg);

    // Here we need to simulate message going though chain of plugins.
    // Luckily we only need these SecHandler plugins: legacy.handler and legacy.map.
    // And as source of information "TLS" Security Attribute must be supplied following
    // information items: IDENTITY (user subject) and VOMS (VOMS FQANs).


    // Load plugins

    Config factory_cfg; //("<ArcConfig xmlns=\"http://www.nordugrid.org/schemas/ArcConfig/2007\"/>");
    MCCLoader loader(factory_cfg);
    //factory_cfg.NewChild("ModuleManager").NewChild("Path") = Arc::ArcLocation::Get()+"/lib/arc";
    //factory_cfg.NewChild("Plugins").NewChild("Name") = "arcshclegacy";
    //PluginsFactory factory(factory_cfg);
    ChainContext& context(*static_cast<ChainContext*>(loader));
    PluginsFactory& factory(*static_cast<PluginsFactory*>(context));
    factory.load("arcshc");
    factory.load("arcshclegacy");
    factory.load("identitymap");
    
    //Arc::ChainContext context(MCCLoader& loader);
    ArcSec::SecHandler* gridmapper(NULL);
    ArcSec::SecHandler* handler(NULL);
    ArcSec::SecHandler* mapper(NULL);

    {
      ArcSec::SecHandlerConfig xcfg("identity.map", "incoming");
      Config cfg(xcfg /*, cfg.getFileName()*/);
      XMLNode pdp1 = cfg.NewChild("PDP");
      pdp1.NewAttribute("name") = "allow.pdp";
      pdp1.NewChild("LocalList") = "/etc/grid-security/grid-mapfile";
      //XMLNode pdp2 = cfg.NewChild("PDP");
      //pdp2.NewAttribute("allow.pdp");
      //pdp2.NewChild("LocalName") = "nobody";
      ArcSec::SecHandlerPluginArgument arg(&cfg, &context);
      Plugin* plugin = factory.get_instance(SecHandlerPluginKind, "identity.map", &arg);
      gridmapper = plugin?dynamic_cast<ArcSec::SecHandler*>(plugin):NULL;
    }

    {
      ArcSec::SecHandlerConfig xcfg("arclegacy.handler", "incoming");
      Config cfg(xcfg /*, cfg.getFileName()*/);
      cfg.NewChild("ConfigFile") = config->ConfigFile();
      ArcSec::SecHandlerPluginArgument arg(&cfg, &context);
      Plugin* plugin = factory.get_instance(SecHandlerPluginKind, "arclegacy.handler", &arg);
      handler = plugin?dynamic_cast<ArcSec::SecHandler*>(plugin):NULL;
    };

    {
      ArcSec::SecHandlerConfig xcfg("arclegacy.map", "incoming");
      Config cfg(xcfg /*, cfg.getFileName()*/);
      XMLNode block = cfg.NewChild("ConfigBlock");
      block.NewChild("ConfigFile") = config->ConfigFile();
      block.NewChild("BlockName")  = "mapping";
      ArcSec::SecHandlerPluginArgument arg(&cfg, &context);
      Plugin* plugin = factory.get_instance(SecHandlerPluginKind, "arclegacy.map", &arg);
      mapper = plugin?dynamic_cast<ArcSec::SecHandler*>(plugin):NULL;
    };

    bool result = false;
    if(gridmapper && handler && mapper) {
      // Prepare information source
      TLSSecAttr* sec_attr = new TLSSecAttr(usercfg);

      // Setup fake mesage to be used as container for information being processed
      Arc::Message msg;
      msg.Auth()->set("TLS", sec_attr); // Message takes ownership of the sec_attr
      // Some plugins fetch user DN from message attributes
      msg.Attributes()->set("TLS:IDENTITYDN", sec_attr->Identity());

      // Process collected information
      if((gridmapper->Handle(&msg)) && (handler->Handle(&msg)) && (mapper->Handle(&msg))) {
        // Result of mapping is stored in message attribute - fetch it
        std::string uname = msg.Attributes()->get("SEC:LOCALID");
        if(!uname.empty()) {
          user = Arc::User(uname);
          result = true;
        }
      }
    }

    delete gridmapper;
    delete handler;
    delete mapper;
    return result;
  }


  bool INTERNALClient::PrepareARexConfig(){

    Arc::Credential cred(usercfg);
    std::string gridname = cred.GetIdentityName();
    arexconfig = new ARex::ARexGMConfig(*config,user.Name(),gridname,endpoint);
    return true;
  }

  


  bool INTERNALClient::CreateDelegation(std::string& deleg_id){
    if(!arexconfig) {
      logger.msg(Arc::ERROR, "INTERNALClient is not initialized");
      return false;
    }

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
    if(!arexconfig) {
      logger.msg(Arc::ERROR, "INTERNALClient is not initialized");
      return false;
    }

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
    return "";
  }
 
  
  bool INTERNALClient::submit(const std::list<Arc::JobDescription>& jobdescs,std::list<INTERNALJob>& localjobs, const std::string delegation_id) {
    if(!arexconfig) {
      logger.msg(Arc::ERROR, "INTERNALClient is not initialized");
      return false;
    }
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
        Arc::XMLNode adl(jobdesc_str);
        ARex::JobIDGeneratorINTERNAL idgenerator(endpoint);
        const std::string dummy = "";



        ARex::ARexJob arexjob(adl,*arexconfig,delegation_id,dummy,logger,idgenerator);
        
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
    if(!arexconfig) {
      logger.msg(Arc::ERROR, "INTERNALClient is not initialized");
      return false;
    }

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
    if(localjobs.empty())
      return false;

    localjob = localjobs.back();

    return true;

  }



  bool INTERNALClient::info(std::list<INTERNALJob>& jobs, std::list<INTERNALJob>& jobids_found){
    if(!arexconfig) {
      logger.msg(Arc::ERROR, "INTERNALClient is not initialized");
      return false;
    }

    //at the moment called by JobListretrieverPluginINTERNAL Query

    for(std::list<INTERNALJob>::iterator job = jobs.begin(); job!= jobs.end(); job++){
      ARex::ARexJob arexjob(job->id,*arexconfig,logger);
         std::string state = arexjob.State();
      if (state != "UNDEFINED") jobids_found.push_back(*job);
    }
   
    return true;
  }


  bool INTERNALClient::info(INTERNALJob& localjob, Arc::Job& arcjob){
    if(!arexconfig) {
      logger.msg(Arc::ERROR, "INTERNALClient is not initialized");
      return false;
    }
    //Called from (at least) JobControllerPluginINTERNAL
    //Called for stagein/out/sessionodir if url of either is not known
    //Extracts information about current arcjob from arexjob and job.jobid.description file and updates/populates the localjob and arcjob with this info, and fills a localjob with the information

    std::vector<std::string> tokens;
    Arc::tokenize(arcjob.JobID, tokens, "/"); 
    if(tokens.empty())
      return false;
    //NB! Add control that the arcjob.jobID is in correct format
    localjob.id = tokens.back();
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
    if(!arexconfig) {
      logger.msg(Arc::ERROR, "INTERNALClient is not initialized");
      return false;
    }

    //TO-DO Need to lock info.xml during reading?
    std::string fname = config->InformationFile();
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
    if(!arexconfig) {
      logger.msg(Arc::ERROR, "INTERNALClient is not initialized");
      return false;
    }
    //jobid is full url
    std::vector<std::string> tokens;
    Arc::tokenize(jobid, tokens, "/"); 
    if(tokens.empty())
      return false;
    std::string thisid = tokens.back();

    ARex::ARexJob arexjob(thisid,*arexconfig,logger);
    arexjob.Cancel();
    return true;
  }


  bool INTERNALClient::clean(const std::string& jobid){
    if(!arexconfig) {
      logger.msg(Arc::ERROR, "INTERNALClient is not initialized");
      return false;
    }
    //jobid is full url
    std::vector<std::string> tokens;
    Arc::tokenize(jobid, tokens, "/"); 
    if(tokens.empty())
      return false;
    std::string thisid = tokens.back();

    ARex::ARexJob arexjob(thisid,*arexconfig,logger);
    arexjob.Clean();
    return true;
  }

  
  bool INTERNALClient::restart(const std::string& jobid){
    if(!arexconfig) {
      logger.msg(Arc::ERROR, "INTERNALClient is not initialized");
      return false;
    }
    //jobid is full url
    std::vector<std::string> tokens;
    Arc::tokenize(jobid, tokens, "/"); 
    if(tokens.empty())
      return false;
    std::string thisid = tokens.back();

    ARex::ARexJob arexjob(thisid,*arexconfig,logger);
    arexjob.Resume();
    return true;
  }


  bool INTERNALClient::list(std::list<INTERNALJob>& jobs){
    //Populates localjobs containing only jobid
    //how do I want to search for jobs in system? 

    std::string cdir=config->ControlDir();
    Glib::Dir dir(cdir);
    std::string file_name;
    while ((file_name = dir.read_name()) != "") {
      std::vector<std::string> tokens;
      Arc::tokenize(file_name, tokens, "."); // look for job.id.local
      if (tokens.size() == 3 && tokens[0] == "job" && tokens[2] == "local") {
        INTERNALJob job;
        job.id = (std::string)tokens[1];
        jobs.push_back(job);
      };
    }
    dir.close();
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
    if (!stagein.empty())arcjob.StageInDir = stagein.front();
    else arcjob.StageInDir = sessiondir;
    if (!stageout.empty()) arcjob.StageOutDir = stageout.front();
    else  arcjob.StageOutDir = sessiondir;
    if (!session.empty()) arcjob.StageInDir = session.front();
    else arcjob.SessionDir = sessiondir;
   
    //extract info from arexjob
    //extract jobid from arcjob, which is the full jobid url
    std::vector<std::string> tokens;
    Arc::tokenize(arcjob.JobID, tokens, "/"); 
    if(!tokens.empty()) {
      //NB! Add control that the arcjob.jobID is in correct format
      ARex::JobId gm_job_id = tokens.back();
      if(client && client->arexconfig) {
        ARex::ARexJob arexjob(gm_job_id,*(client->arexconfig),client->logger);
        std::string state = arexjob.State();
        arcjob.State = JobStateINTERNAL(state);
      }
    }
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
