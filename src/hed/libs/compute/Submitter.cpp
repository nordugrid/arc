// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/compute/Broker.h>
#include <arc/compute/ComputingServiceRetriever.h>
#include <arc/compute/SubmissionStatus.h>
#include <arc/compute/Submitter.h>

namespace Arc {

  SubmitterPluginLoader Submitter::loader;
  Logger Submitter::logger(Logger::getRootLogger(), "Submitter");

  std::string EndpointSubmissionStatus::str(EndpointSubmissionStatusType s) {
    if      (s == UNKNOWN)     return "UNKNOWN";
    else if (s == NOPLUGIN)    return "NOPLUGIN";
    else if (s == SUCCESSFUL)  return "SUCCESSFUL";
    else                       return ""; // There should be no other alternative!
  }

  void Submitter::removeConsumer(EntityConsumer<Job>& jc) {
    std::list<EntityConsumer<Job>*>::iterator it = std::find(consumers.begin(), consumers.end(), &jc);
    if (it != consumers.end()) {
      consumers.erase(it);
    }
  }
  
  class JobConsumerSingle : public EntityConsumer<Job> {
  public:
    JobConsumerSingle(Job& j) : _j(j) {}
    void addEntity(const Job& j) { _j = j; }
  private:
    Job& _j;
  };

  class JobConsumerList : public EntityConsumer<Job> {
  public:
    JobConsumerList(std::list<Job>& joblist) : joblist(joblist) {}
    void addEntity(const Job& j) { joblist.push_back(j); }
  private:
    std::list<Job>& joblist;
  };

  SubmissionStatus Submitter::Submit(const Endpoint& endpoint, const JobDescription& desc, Job& job) {
    JobConsumerSingle jcs(job);
    addConsumer(jcs);
    SubmissionStatus ok = Submit(endpoint, std::list<JobDescription>(1, desc));
    removeConsumer(jcs);
    return ok;
  }

  SubmissionStatus Submitter::Submit(const Endpoint& endpoint, const std::list<JobDescription>& descs, std::list<Job>& jobs) {
    JobConsumerList jcl(jobs);
    addConsumer(jcl);
    SubmissionStatus ok = Submit(endpoint, descs);
    removeConsumer(jcl);
    return ok;
  }

  SubmissionStatus Submitter::Submit(const Endpoint& endpoint, const std::list<JobDescription>& descs) {
    ClearAll();
    return SubmitNoClear(endpoint, descs);
  }

  SubmissionStatus Submitter::SubmitNoClear(const Endpoint& endpoint, const std::list<JobDescription>& descs) {
    ConsumerWrapper cw(*this);

    logger.msg(DEBUG, "Trying to submit directly to endpoint (%s)", endpoint.URLString);

    SubmissionStatus retval;
    
    if (!endpoint.InterfaceName.empty()) {
      logger.msg(DEBUG, "Interface (%s) specified, submitting only to that interface", endpoint.InterfaceName);
      
      SubmitterPlugin *sp = loader.loadByInterfaceName(endpoint.InterfaceName, uc);
      
      if (sp == NULL)  {
        submissionStatusMap[endpoint] = EndpointSubmissionStatus::NOPLUGIN;
        retval |= SubmissionStatus::SUBMITTER_PLUGIN_NOT_LOADED;
        retval |= SubmissionStatus::DESCRIPTION_NOT_SUBMITTED;
        for (std::list<JobDescription>::const_iterator itJ = descs.begin();
             itJ != descs.end(); ++itJ) {
          notsubmitted.push_back(&*itJ);
        }
        return retval;
      }

      return sp->Submit(descs, endpoint.URLString, cw, notsubmitted);
    }
    
    logger.msg(DEBUG, "Trying all available interfaces", endpoint.URLString);
    // InterfaceName is empty -> Try all interfaces.
    loader.initialiseInterfacePluginMap(uc);
    const std::map<std::string, std::string>& interfacePluginMap = loader.getInterfacePluginMap();
    for (std::map<std::string, std::string>::const_iterator it = interfacePluginMap.begin();
         it != interfacePluginMap.end(); ++it) {
      logger.msg(DEBUG, "Trying to submit endpoint (%s) using interface (%s) with plugin (%s).", endpoint.URLString, it->first, it->second);
      SubmitterPlugin *sp = loader.load(it->second, uc);
      
      if (sp == NULL) {
        logger.msg(DEBUG, "Unable to load plugin (%s) for interface (%s) when trying to submit job description.", it->second, it->first);
        continue;
      }

      std::list<JobDescription const *> isNotSubmitted;
      retval = sp->Submit(descs, endpoint.URLString, cw, isNotSubmitted);
      /* If submission attempt above managed to submit one or multiple
       * descriptions then dont try other plugins.
       */
      if (retval || (retval.isSet(SubmissionStatus::DESCRIPTION_NOT_SUBMITTED) && descs.size() != isNotSubmitted.size())) {
        notsubmitted.insert(notsubmitted.end(), isNotSubmitted.begin(), isNotSubmitted.end());
        return retval;
      }
    }
    logger.msg(DEBUG, "No more interfaces to try.", endpoint.URLString);

    for (std::list<JobDescription>::const_iterator it = descs.begin();
         it != descs.end(); ++it) {
      notsubmitted.push_back(&*it);
    }
    if (!notsubmitted.empty()) {
      retval |= SubmissionStatus::DESCRIPTION_NOT_SUBMITTED;
    }
    
    return retval;
  }

  SubmissionStatus Submitter::Submit(const std::list<Endpoint>& endpoints, const std::list<JobDescription>& descs, std::list<Job>& jobs) {
    JobConsumerList jcl(jobs);
    addConsumer(jcl);
    SubmissionStatus ok = Submit(endpoints, descs);
    removeConsumer(jcl);
    return ok;
  }

  SubmissionStatus Submitter::Submit(const std::list<Endpoint>& endpoints, const std::list<JobDescription>& descs) {
    ClearAll();
    
    std::list<JobDescription> descs_to_submit = descs;
    
    SubmissionStatus ok;
    for (std::list<Endpoint>::const_iterator it = endpoints.begin();
         it != endpoints.end(); ++it) {
      ClearNotSubmittedDescriptions();
      ok |= Submit(*it, descs_to_submit);
      if (!ok.isSet(SubmissionStatus::DESCRIPTION_NOT_SUBMITTED)) {
        return ok;
      }
      
      /* Remove DESCRIPTION_NOT_SUBMITTED status from ok, since we are trying
       * multiple endpoints. Set DESCRIPTION_NOT_SUBMITTED status if some
       * descriptions was not submitted at end of loop.
       */
      ok.unset(SubmissionStatus::DESCRIPTION_NOT_SUBMITTED);
      
      /* The private member notsubmitted should contain pointers to the
       * descriptions not submitted. So now the descriptions which _was_
       * submitted should be removed from the descs_to_submit list. The not
       * submitted ones could just be appended to the descs_to_submit list
       * (that involves copying) and then remove original part of the
       * descs_to_submit entries. Another way is to go through each entry and
       * check whether it was submitted or not, and then only keep the submitted
       * ones. The latter algorithm is implemented below.
       */
      for (std::list<JobDescription>::iterator itJ = descs_to_submit.begin();
           itJ != descs_to_submit.end();) {
        std::list<JobDescription const *>::iterator itNotS = notsubmitted.begin();
        for (; itNotS != notsubmitted.end(); ++itNotS) if (*itNotS == &*itJ) break;

        if (itNotS == notsubmitted.end()) { // No match found - job description was submitted.
          // Remove entry from descs_to_submit list, now that it was submitted.
          itJ = descs_to_submit.erase(itJ);
        }
        else { // Match found - job not submitted.
          // Remove entry from notsubmitted list, now that entry was found.
          notsubmitted.erase(itNotS);
          ++itJ;
        }
      }
    }
    
    if (!notsubmitted.empty()) {
      ok |= SubmissionStatus::DESCRIPTION_NOT_SUBMITTED;
    }
    
    return ok;
  }


  SubmissionStatus Submitter::Submit(const ExecutionTarget& et, const JobDescription& desc, Job& job) {
    JobConsumerSingle jcs(job);
    addConsumer(jcs);
    SubmissionStatus ok = Submit(et, std::list<JobDescription>(1, desc));
    removeConsumer(jcs);
    return ok;
  }

  SubmissionStatus Submitter::Submit(const ExecutionTarget& et, const std::list<JobDescription>& descs, std::list<Job>& jobs) {
    JobConsumerList jcl(jobs);
    addConsumer(jcl);
    SubmissionStatus ok = Submit(et, descs);
    removeConsumer(jcl);
    return ok;
  }

  SubmissionStatus Submitter::Submit(const ExecutionTarget& et, const std::list<JobDescription>& descs) {
    ClearAll();

    ConsumerWrapper cw(*this);

    SubmitterPlugin *sp = loader.loadByInterfaceName(et.ComputingEndpoint->InterfaceName, uc);
    
    if (sp == NULL) {
      for (std::list<JobDescription>::const_iterator it = descs.begin(); it != descs.end(); ++it) {
        notsubmitted.push_back(&*it);
      }
      return SubmissionStatus::SUBMITTER_PLUGIN_NOT_LOADED;
    }
    
    return sp->Submit(descs, et, cw, notsubmitted);
  }


  SubmissionStatus Submitter::BrokeredSubmit(const std::list<std::string>& endpoints, const std::list<JobDescription>& descs, std::list<Job>& jobs, const std::list<std::string>& requestedSubmissionInterfaces) {
    JobConsumerList jcl(jobs);
    addConsumer(jcl);
    SubmissionStatus ok = BrokeredSubmit(endpoints, descs);
    removeConsumer(jcl);
    return ok;
  }
  
  SubmissionStatus Submitter::BrokeredSubmit(const std::list<std::string>& endpoints, const std::list<JobDescription>& descs, const std::list<std::string>& requestedSubmissionInterfaces) {
    std::list<Endpoint> endpointObjects;
    for (std::list<std::string>::const_iterator it = endpoints.begin(); it != endpoints.end(); ++it) {
      endpointObjects.push_back(Endpoint(*it, Endpoint::UNSPECIFIED));
    }
    return BrokeredSubmit(endpointObjects, descs, requestedSubmissionInterfaces);
  }

  SubmissionStatus Submitter::BrokeredSubmit(const std::list<Endpoint>& endpoints, const std::list<JobDescription>& descs, std::list<Job>& jobs, const std::list<std::string>& requestedSubmissionInterfaces) {
    JobConsumerList jcl(jobs);
    addConsumer(jcl);
    SubmissionStatus ok = BrokeredSubmit(endpoints, descs, requestedSubmissionInterfaces);
    removeConsumer(jcl);
    return ok;
  }

  static bool match_submission_interface(const ExecutionTarget& target, const std::list<std::string>& requestedSubmissionInterfaces) {
    if (requestedSubmissionInterfaces.empty()) return true;
    for (std::list<std::string>::const_iterator iname = requestedSubmissionInterfaces.begin();
                  iname != requestedSubmissionInterfaces.end(); ++iname) {
      if (*iname == target.ComputingEndpoint->InterfaceName) return true;
    }
    return false;
  }

  SubmissionStatus Submitter::BrokeredSubmit(const std::list<Endpoint>& endpoints, const std::list<JobDescription>& descs, const std::list<std::string>& requestedSubmissionInterfaces) {
    ClearAll();

    std::set<std::string> preferredInterfaceNames;
    if (uc.InfoInterface().empty()) {
      // Maybe defaults should be moved somewhere else.
      preferredInterfaceNames.insert("org.nordugrid.ldapglue2");
    } else {
      preferredInterfaceNames.insert(uc.InfoInterface());
    }

    Arc::ComputingServiceUniq csu;
    Arc::ComputingServiceRetriever csr(uc, std::list<Arc::Endpoint>(), uc.RejectDiscoveryURLs(), preferredInterfaceNames);
    csr.addConsumer(csu);
    for (std::list<Arc::Endpoint>::const_iterator it = endpoints.begin(); it != endpoints.end(); it++) {
      csr.addEndpoint(*it);
    }
    csr.wait();

    std::list<Arc::ComputingServiceType> services = csu.getServices();

    queryingStatusMap = csr.getAllStatuses();
  
    if (services.empty()) {
      for (std::list<JobDescription>::const_iterator it = descs.begin();
           it != descs.end(); ++it) {
        notsubmitted.push_back(&*it);
      }
      return (SubmissionStatus::NO_SERVICES | SubmissionStatus::DESCRIPTION_NOT_SUBMITTED);
    }
  
    Broker broker(uc, uc.Broker().first);
    if (!broker.isValid(false)) { // Only check if BrokerPlugin was loaded.
      for (std::list<JobDescription>::const_iterator it = descs.begin();
           it != descs.end(); ++it) {
        notsubmitted.push_back(&*it);
      }
      return (SubmissionStatus::BROKER_PLUGIN_NOT_LOADED | SubmissionStatus::DESCRIPTION_NOT_SUBMITTED);
    }

    SubmissionStatus retval;
    ConsumerWrapper cw(*this);
    ExecutionTargetSorter ets(broker, services);
    std::list<JobDescription>::const_iterator itJAlt; // Iterator to use for alternative job descriptions.
    for (std::list<JobDescription>::const_iterator itJ = descs.begin();
         itJ != descs.end(); ++itJ) {
      bool descriptionSubmitted = false;
      const JobDescription* currentJobDesc = &*itJ;
      do {
        ets.set(*currentJobDesc);
        for (; !ets.endOfList(); ets.next()) {
          if(!match_submission_interface(*ets, requestedSubmissionInterfaces)) continue;
          SubmitterPlugin *sp = loader.loadByInterfaceName(ets->ComputingEndpoint->InterfaceName, uc);
          if (sp == NULL) {
            submissionStatusMap[Endpoint(*ets)] = EndpointSubmissionStatus(EndpointSubmissionStatus::NOPLUGIN);
            retval |= SubmissionStatus::SUBMITTER_PLUGIN_NOT_LOADED;
            continue;
          }
          SubmissionStatus submitStatus = sp->Submit(*currentJobDesc, *ets, cw);
          if (submitStatus) {
            submissionStatusMap[Endpoint(*ets)] = EndpointSubmissionStatus(EndpointSubmissionStatus::SUCCESSFUL);
    
            descriptionSubmitted = true;
            ets->RegisterJobSubmission(*currentJobDesc);
            break;
          }
          /* TODO: Set detailed status of endpoint, in case a general error is
           * encountered, i.e. not specific to the job description, so subsequent
           * submissions of job descriptions can check if a particular endpoint
           * should be avoided.
           *submissionStatusMap[Endpoint(*itET)] = submitStatus; // Currently 'submitStatus' is only a bool, improving the detail level of it would be helpful at this point.
           */
        }
        if (!descriptionSubmitted && itJ->HasAlternatives()) { // Alternative job descriptions.
          if (currentJobDesc == &*itJ) {
            itJAlt = itJ->GetAlternatives().begin();
          }
          else {
            ++itJAlt;
          }
          currentJobDesc = &*itJAlt;
        }
      } while (!descriptionSubmitted && itJ->HasAlternatives() && itJAlt != itJ->GetAlternatives().end());
      if (!descriptionSubmitted) {
        notsubmitted.push_back(&*itJ);
        retval |= SubmissionStatus::DESCRIPTION_NOT_SUBMITTED;
      }
    }
  
    return retval;
  }
} // namespace Arc
