// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <map>
#include <utility>

#include <glib.h>

#include <arc/StringConv.h>
#include <arc/UserConfig.h>
#include <arc/XMLNode.h>
#include <arc/compute/JobDescription.h>
#include <arc/data/DataMover.h>
#include <arc/data/DataHandle.h>
#include <arc/data/URLMap.h>
#include <arc/message/MCC.h>
#include <arc/Utils.h>


#include "../grid-manager/conf/GMConfig.h"
#include "../grid-manager/files/ControlFileHandling.h"


#include "JobStateINTERNAL.h"
#include "INTERNALClient.h"

#include "JobControllerPluginINTERNAL.h"


using namespace Arc;


namespace ARexINTERNAL {

  Logger JobControllerPluginINTERNAL::logger(Logger::getRootLogger(), "JobControllerPlugin.INTERNAL");

  bool JobControllerPluginINTERNAL::isEndpointNotSupported(const std::string& endpoint) const {
    const std::string::size_type pos = endpoint.find("://");
    return pos != std::string::npos && lower(endpoint.substr(0, pos)) != "file" ;
  }

  void JobControllerPluginINTERNAL::UpdateJobs(std::list<Job*>& jobs, std::list<std::string>& IDsProcessed, std::list<std::string>& IDsNotProcessed, bool isGrouped) const {
    if (jobs.empty()) return;
    
    INTERNALClient ac;
    ARex::GMConfig const *config = ac.GetConfig();
    if(!config){
      logger.msg(Arc::ERROR,"Failed to load grid-manager config file");
      return;
    }

    //Is this method doing what it is supposed to do? I think the main purpose is to get hold of jobids for existing jobs in the system. 
    for(std::list<Job*>::iterator itJ = jobs.begin(); itJ != jobs.end(); itJ++){
      
      //stat the .description file to check whether job is still in the system  
      //(*itJ).JobID is now the global id, tokenize and get hold of just the local jobid
      std::vector<std::string> tokens;
      Arc::tokenize((**itJ).JobID, tokens, "/");
      std::string localid = tokens[tokens.size()-1];
      

      std::string rsl;
      if(!ARex::job_description_read_file(localid, *config, rsl)){
        continue;
      }



      //the job exists, so add it
      INTERNALJob localjob;
      //toJob calls info(job) and populates the arcjob with basic information (id and state). 
      localjob.toJob(&ac,**itJ,logger);

      if (itJ != jobs.end()) {
        IDsProcessed.push_back((**itJ).JobID);
      }
      else{
        IDsNotProcessed.push_back((**itJ).JobID);
      }
    }
  }
  

  bool JobControllerPluginINTERNAL::CleanJobs(const std::list<Job*>& jobs, std::list<std::string>& IDsProcessed, std::list<std::string>& IDsNotProcessed, bool isGrouped) const {

    INTERNALClient ac(*usercfg);
    ARex::GMConfig const * config = ac.GetConfig();
    if(!config){
      logger.msg(Arc::ERROR,"Failed to load grid-manager config file");
      return false;
    }
   
    bool ok = true;
    for (std::list<Job*>::const_iterator it = jobs.begin(); it != jobs.end(); ++it) {
      //Job& job = **it;

      if (!ac.clean((*it)->JobID)) {
        ok = false;
        IDsNotProcessed.push_back((*it)->JobID);
        continue;
      }
      IDsProcessed.push_back((*it)->JobID);
    }

    return ok;
  }

  bool JobControllerPluginINTERNAL::CancelJobs(const std::list<Job*>& jobs, std::list<std::string>& IDsProcessed, std::list<std::string>& IDsNotProcessed, bool isGrouped) const {

    bool ok = true;
    for (std::list<Job*>::const_iterator it = jobs.begin(); it != jobs.end(); ++it) {
      
      INTERNALClient ac(*usercfg);
      ARex::GMConfig const * config = ac.GetConfig();
      if(!config){
        logger.msg(Arc::ERROR,"Failed to load grid-manager config file");
        return false;
      }
      
      if(!ac.kill((*it)->JobID)) {
        ok = false;
        IDsNotProcessed.push_back((*it)->JobID);
        continue;
      }

      (*it)->State = JobStateINTERNAL((std::string)"killed");
      IDsProcessed.push_back((*it)->JobID);
    }

    return ok;
  }

  bool JobControllerPluginINTERNAL::RenewJobs(const std::list<Job*>& jobs, std::list<std::string>& IDsProcessed, std::list<std::string>& IDsNotProcessed, bool isGrouped) const {


    bool ok = true;
    for (std::list<Job*>::const_iterator it = jobs.begin(); it != jobs.end(); ++it) {
      // 1. Fetch/find delegation ids for each job

      INTERNALClient ac;
      ARex::GMConfig const * config = ac.GetConfig();
      if(!config){
        logger.msg(Arc::ERROR,"Failed to load grid-manager config file");
        return false;
      }
      if((*it)->DelegationID.empty()) {
        logger.msg(INFO, "Job %s has no delegation associated. Can't renew such job.", (*it)->JobID);
        IDsNotProcessed.push_back((*it)->JobID);
        continue;
      }

      // 2. Leave only unique IDs - not needed yet because current code uses
      //    different delegations for each job.
      // 3. Renew credentials for every ID
      Job& job = **it;

      
      std::list<std::string>::const_iterator did = (*it)->DelegationID.begin();
      for(;did != (*it)->DelegationID.end();++did) {
        if(!ac.RenewDelegation(*did)) {
          logger.msg(INFO, "Job %s failed to renew delegation %s.", (*it)->JobID/*, *did, ac->failure()*/);
          break;
        }
      }
      if(did != (*it)->DelegationID.end()) {
        IDsNotProcessed.push_back((*it)->JobID);
        continue;
      }
      IDsProcessed.push_back((*it)->JobID);

    }
    return false;
  }

  bool JobControllerPluginINTERNAL::ResumeJobs(const std::list<Job*>& jobs, std::list<std::string>& IDsProcessed, std::list<std::string>& IDsNotProcessed, bool isGrouped) const {


    bool ok = true;
    for (std::list<Job*>::const_iterator it = jobs.begin(); it != jobs.end(); ++it) {
    
      INTERNALClient ac;
      ARex::GMConfig const * config = ac.GetConfig();
      if(!config){
        logger.msg(Arc::ERROR,"Failed to load grid-manager config file");
        return false;
      }
    

      Job& job = **it;
      if (!job.RestartState) {
        logger.msg(INFO, "Job %s does not report a resumable state", job.JobID);
        ok = false;
        IDsNotProcessed.push_back(job.JobID);
        continue;
      }
      logger.msg(VERBOSE, "Resuming job: %s at state: %s (%s)", job.JobID, job.RestartState.GetGeneralState(), job.RestartState());
      
      if(!ac.restart((*it)->JobID)) {
        ok = false;
        IDsNotProcessed.push_back((*it)->JobID);
        continue;
      }
      
      IDsProcessed.push_back((*it)->JobID);
      logger.msg(VERBOSE, "Job resuming successful");
    }
    return ok;
  }


  bool JobControllerPluginINTERNAL::GetURLToJobResource(const Job& job, Job::ResourceType resource, URL& url) const {
    
    if (resource == Job::JOBDESCRIPTION) {
      return false;
    }

    // Obtain information about staging urls
    INTERNALJob ljob;
    ljob = job;
    URL stagein;
    URL stageout;
    URL session;
    // TODO: currently using first valid URL. Need support for multiple.
    for(std::list<URL>::const_iterator s = ljob.GetStagein().begin();s!=ljob.GetStagein().end();++s) {
      if(*s) { stagein = *s; break; }
    }
    for(std::list<URL>::const_iterator s = ljob.GetStageout().begin();s!=ljob.GetStageout().end();++s) {
      if(*s) { stageout = *s; break; }
    }
    for(std::list<URL>::const_iterator s = ljob.GetSession().begin();s!=ljob.GetSession().end();++s) {
      if(*s) { session = *s; break; }
    }



    if ((resource != Job::STAGEINDIR  || !stagein)  &&
        (resource != Job::STAGEOUTDIR || !stageout) &&
        (resource != Job::SESSIONDIR  || !session)) {
      // If there is no needed URL provided try to fetch it from server
     
      Job tjob;
      tjob.JobID = job.JobID;
      INTERNALClient ac;
      ARex::GMConfig const * config = ac.GetConfig();
      if(!config){
        logger.msg(Arc::ERROR,"Failed to load grid-manager config file");
        return false;
      }
    
      if (!ac.info(ljob, tjob)) {
        logger.msg(INFO, "Failed retrieving information for job: %s", job.JobID);
        return false;
      }
      for(std::list<URL>::const_iterator s = ljob.GetStagein().begin();s!=ljob.GetStagein().end();++s) {
        if(*s) { stagein = *s; break; }
      }
      for(std::list<URL>::const_iterator s = ljob.GetStageout().begin();s!=ljob.GetStageout().end();++s) {
        if(*s) { stageout = *s; break; }
      }
      for(std::list<URL>::const_iterator s = ljob.GetSession().begin();s!=ljob.GetSession().end();++s) {
        if(*s) { session = *s; break; }
      }
      // Choose url by state
      // TODO: For INTERNAL submission plugin the url is the same for all, although not reflected here
      // TODO: maybe this method should somehow know what is purpose of URL
      // TODO: state attributes would be more suitable
      // TODO: library need to be etended to allow for multiple URLs
      if((tjob.State == JobState::ACCEPTED) ||
         (tjob.State == JobState::PREPARING)) {
        url = stagein;
      } else if((tjob.State == JobState::DELETED) ||
                (tjob.State == JobState::FAILED) ||
                (tjob.State == JobState::KILLED) ||
                (tjob.State == JobState::FINISHED) ||
                (tjob.State == JobState::FINISHING)) {
        url = stageout;
      } else {
        url = session;
      }
      // If no url found by state still try to get something
      if(!url) {
        if(session)  url = session;
        if(stagein)  url = stagein;
        if(stageout) url = stageout;
      }
    }

    switch (resource) {
    case Job::STDIN:
      url.ChangePath(url.Path() + '/' + job.StdIn);
      break;
    case Job::STDOUT:
      url.ChangePath(url.Path() + '/' + job.StdOut);
      break;
    case Job::STDERR:
      url.ChangePath(url.Path() + '/' + job.StdErr);
      break;
    case Job::JOBLOG:
      url.ChangePath(url.Path() + "/" + job.LogDir + "/errors");
      break;
    case Job::STAGEINDIR:
      if(stagein) url = stagein;
      break;
    case Job::STAGEOUTDIR:
      if(stageout) url = stageout;
      break;
    case Job::SESSIONDIR:
      if(session) url = session;
      break;
    default:
      break;
    }
    if(url && ((url.Protocol() == "file"))) {
      //To-do - is this relevant for INTERNAL plugin?
      url.AddOption("threads=2",false);
      url.AddOption("encryption=optional",false);
      // url.AddOption("httpputpartial=yes",false); - TODO: use for A-REX
    }

    return true;
  }

  bool JobControllerPluginINTERNAL::GetJobDescription(const Job& /* job */, std::string& /* desc_str */) const {
    logger.msg(INFO, "Retrieving job description of INTERNAL jobs is not supported");
    return false;
  }

} // namespace Arc
