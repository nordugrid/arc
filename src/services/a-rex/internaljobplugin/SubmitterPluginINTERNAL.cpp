// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <unistd.h>
#include <string>
#include <sstream>

#include <glibmm.h>

#include <arc/StringConv.h>
#include <arc/UserConfig.h>
#include <arc/compute/ExecutionTarget.h>
#include <arc/compute/Job.h>
#include <arc/compute/JobDescription.h>
#include <arc/compute/SubmissionStatus.h>
#include <arc/message/MCC.h>

//#include "JobStateINTERNAL.h"

#include "SubmitterPluginINTERNAL.h"



using namespace Arc;


namespace ARexINTERNAL {

  
  bool SubmitterPluginINTERNAL::isEndpointNotSupported(const std::string& endpoint) const {
    const std::string::size_type pos = endpoint.find("://");
    return pos != std::string::npos && lower(endpoint.substr(0, pos)) != "file";
  }
  

  bool SubmitterPluginINTERNAL::getDelegationID(const URL& durl, std::string& delegation_id) {
    if(!durl) {
      logger.msg(INFO, "Failed to delegate credentials to server - no delegation interface found");
      return false;
    }
    
    INTERNALClient ac(durl,*usercfg);
    
    if(!ac.CreateDelegation(delegation_id)) {
      logger.msg(INFO, "Failed to delegate credentials to server - %s",ac.failure());
      return false;
    }

    return true;
  }



  Arc::SubmissionStatus SubmitterPluginINTERNAL::Submit(const std::list<JobDescription>& jobdescs, const ExecutionTarget& et, EntityConsumer<Job>& jc, std::list<const JobDescription*>& notSubmitted){
 

    Arc::SubmissionStatus retval; 
    std::string endpoint = et.ComputingEndpoint->URLString;
    retval = Submit(jobdescs, endpoint, jc, notSubmitted);

    return retval;
  }


  Arc::SubmissionStatus SubmitterPluginINTERNAL::Submit(const std::list<JobDescription>& jobdescs, const std::string& endpoint, EntityConsumer<Job>& jc, std::list<const JobDescription*>& notSubmitted) {
    //jobdescs as passed down from the client
    // TODO: this is multi step process. So having retries would be nice.
    // TODO: If delegation interface is not on same endpoint as submission interface this method is faulty.

      
    URL url((endpoint.find("://") == std::string::npos ? "file://" : "") + endpoint, false);

    /*for accessing jobs*/
    /*Preparation of jobdescription*/
    Arc::SubmissionStatus retval;
    std::string delegation_id;
    INTERNALClient ac(url,*usercfg);

    for (std::list<JobDescription>::const_iterator itJ = jobdescs.begin(); itJ != jobdescs.end(); ++itJ) {

      //Calls JobDescription.Prepare, which Check for identical file names. and if executable and input is contained in the file list.
      JobDescription preparedjobdesc(*itJ);
      if (!preparedjobdesc.Prepare()) {
        logger.msg(INFO, "Failed preparing job description");
        notSubmitted.push_back(&*itJ);
        retval |= Arc::SubmissionStatus::DESCRIPTION_NOT_SUBMITTED;
        continue;
      }

      bool need_delegation = false;
      std::list<std::string> upload_sources;
      std::list<std::string> upload_destinations;
      /*Preparation of input files and outputfiles */
      for(std::list<InputFileType>::const_iterator itIF = preparedjobdesc.DataStaging.InputFiles.begin();
          itIF != preparedjobdesc.DataStaging.InputFiles.end(); ++itIF) {
        if(!itIF->Sources.empty()) {
          if(itIF->Sources.front().Protocol() == "file") {
            upload_sources.push_back(itIF->Sources.front().Path());
            upload_destinations.push_back(itIF->Name);
          } else {
            need_delegation = true;
          }
        }
      }

      for(std::list<OutputFileType>::const_iterator itOF = itJ->DataStaging.OutputFiles.begin();
          itOF != itJ->DataStaging.OutputFiles.end() && !need_delegation; ++itOF) {
        if((!itOF->Targets.empty()) || 
           (itOF->Name[0] == '@')) { // ARC specific - dynamic list of output files
          need_delegation = true;
        }
      }
      /*end preparation of input and output files */


      if (need_delegation && delegation_id.empty()) {
        if (!getDelegationID(url, delegation_id)) {
          notSubmitted.push_back(&*itJ);
          retval |= Arc::SubmissionStatus::DESCRIPTION_NOT_SUBMITTED;
          continue;
        }
      }
      std::list<INTERNALJob> localjobs;   
      std::list<JobDescription> preparedjobdescs;
      preparedjobdescs.push_back(preparedjobdesc);
      if((!ac.submit(preparedjobdescs, localjobs, delegation_id)) || (localjobs.empty())) {
        logger.msg(INFO, "Failed submitting job description");
        notSubmitted.push_back(&*itJ);
        retval |= Arc::SubmissionStatus::DESCRIPTION_NOT_SUBMITTED;
        continue;
      }
      if(!upload_sources.empty()) {

        if(!ac.putFiles(localjobs.front(), upload_sources, upload_destinations)) {
        notSubmitted.push_back(&*itJ);
        retval |= Arc::SubmissionStatus::DESCRIPTION_NOT_SUBMITTED;
        continue;
        }
      }

      Arc::Job job;
      localjobs.front().toJob(&ac,&(localjobs.front()),job);
      AddJobDetails(preparedjobdesc, job);
      jc.addEntity(job);
    }//end loop over jobdescriptions
    
    return retval;
  }




} // namespace ARexINTERNAL
