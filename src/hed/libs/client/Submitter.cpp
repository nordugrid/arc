// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/client/Broker.h>
#include <arc/client/ComputingServiceRetriever.h>
#include <arc/client/Submitter.h>

namespace Arc {

  SubmitterPluginLoader Submitter::loader;
  Logger Submitter::logger(Logger::getRootLogger(), "Submitter");

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

  bool Submitter::Submit(const ExecutionTarget& et, const JobDescription& desc, Job& job) {
    JobConsumerSingle jcs(job);
    addConsumer(jcs);
    bool ok = Submit(et, std::list<JobDescription>(1, desc));
    removeConsumer(jcs);
    return ok;
  }

  bool Submitter::Submit(const ExecutionTarget& et, const std::list<JobDescription>& descs, std::list<Job>& jobs) {
    JobConsumerList jcl(jobs);
    addConsumer(jcl);
    bool ok = Submit(et, descs);
    removeConsumer(jcl);
    return ok;
  }

  bool Submitter::Submit(const ExecutionTarget& et, const std::list<JobDescription>& descs) {
    ClearAll();

    ConsumerWrapper cw(*this);

    SubmitterPlugin *sp = loader.loadByInterfaceName(et.ComputingEndpoint->InterfaceName, uc);
    
    if (sp == NULL) {
      for (std::list<JobDescription>::const_iterator it = descs.begin(); it != descs.end(); ++it) {
        notsubmitted.push_back(&*it);
      }
      return false;
    }
    
    return sp->Submit(descs, et, cw, notsubmitted);
  }


  SubmissionStatus Submitter::BrokeredSubmit(const std::list<std::string>& endpoints, const std::list<JobDescription>& descs, std::list<Job> jobs, const std::list<std::string>& requestedSubmissionInterfaces) {
    JobConsumerList jcl(jobs);
    addConsumer(jcl);
    SubmissionStatus ok = BrokeredSubmit(endpoints, descs);
    removeConsumer(jcl);
    return ok;
  }
  
  SubmissionStatus Submitter::BrokeredSubmit(const std::list<std::string>& endpoints, const std::list<JobDescription>& descs, const std::list<std::string>& requestedSubmissionInterfaces) {
    std::list<Endpoint> endpointObjects;
    for (std::list<std::string>::const_iterator it = endpoints.begin(); it != endpoints.end(); ++it) {
      endpointObjects.push_back(Endpoint(*it, Endpoint::JOBSUBMIT));
    }
    return BrokeredSubmit(endpointObjects, descs, requestedSubmissionInterfaces);
  }

  SubmissionStatus Submitter::BrokeredSubmit(const std::list<Endpoint>& endpoints, const std::list<JobDescription>& descs, std::list<Job> jobs, const std::list<std::string>& requestedSubmissionInterfaces) {
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

    std::list<std::string> preferredInterfaceNames;
    if (uc.InfoInterface().empty()) {
      // Maybe defaults should be moved somewhere else.
      preferredInterfaceNames.push_back("org.nordugrid.ldapglue2");
      preferredInterfaceNames.push_back("org.ogf.emies");
    } else {
      preferredInterfaceNames.push_back(uc.InfoInterface());
    }

    ComputingServiceRetriever csr(uc, endpoints, uc.RejectDiscoveryURLs(), preferredInterfaceNames);
    csr.wait();

    queryingStatusMap = csr.getAllStatuses();
  
    if (csr.empty()) {
      for (std::list<JobDescription>::const_iterator it = descs.begin();
           it != descs.end(); ++it) {
        notsubmitted.push_back(&*it);
      }
      return (SubmissionStatus::NO_SERVICES | SubmissionStatus::DESCRIPTION_NOT_SUBMITTED);
    }
  
    Broker broker(uc, uc.Broker().first);
    if (!broker.isValid()) {
      for (std::list<JobDescription>::const_iterator it = descs.begin();
           it != descs.end(); ++it) {
        notsubmitted.push_back(&*it);
      }
      return (SubmissionStatus::BROKER_PLUGIN_NOT_LOADED | SubmissionStatus::DESCRIPTION_NOT_SUBMITTED);
    }

    SubmissionStatus retval;
    ConsumerWrapper cw(*this);
    for (std::list<JobDescription>::const_iterator itJ = descs.begin();
         itJ != descs.end(); ++itJ) {
      bool descriptionSubmitted = false;
      broker.set(*itJ);
      ExecutionTargetSet etSet(broker, csr);
      for (ExecutionTargetSet::iterator itET = etSet.begin(); itET != etSet.end(); ++itET) {
        if(!match_submission_interface(*itET, requestedSubmissionInterfaces)) continue;
        SubmitterPlugin *sp = loader.loadByInterfaceName(itET->ComputingEndpoint->InterfaceName, uc);
        if (sp == NULL) {
          submissionStatusMap[Endpoint(*itET)] = EndpointSubmissionStatus(EndpointSubmissionStatus::NOPLUGIN);
          retval |= SubmissionStatus::SUBMITTER_PLUGIN_NOT_LOADED;
          continue;
        }
        bool submitStatus = sp->Submit(*itJ, *itET, cw);
        if (submitStatus) {
          submissionStatusMap[Endpoint(*itET)] = EndpointSubmissionStatus(EndpointSubmissionStatus::SUCCESSFUL);
  
          descriptionSubmitted = true;
          itET->RegisterJobSubmission(*itJ);
          break;
        }
        /* TODO: Set detailed status of endpoint, in case a general error is
         * encountered, i.e. not specific to the job description, so subsequent
         * submissions of job descriptions can check if a particular endpoint
         * should be avoided.
         *submissionStatusMap[Endpoint(*itET)] = submitStatus; // Currently 'submitStatus' is only a bool, improving the detail level of it would be helpful at this point.
         */
      }
      if (!descriptionSubmitted && itJ->HasAlternatives()) {
        // TODO: Deal with alternative job descriptions more effective.
        for (std::list<JobDescription>::const_iterator itJAlt = itJ->GetAlternatives().begin();
             itJAlt != itJ->GetAlternatives().end(); ++itJAlt) {
          broker.set(*itJAlt);
          ExecutionTargetSet etSetAlt(broker, csr);
          for (ExecutionTargetSet::iterator itET = etSetAlt.begin(); itET != etSetAlt.end(); ++itET) {
            if(!match_submission_interface(*itET, requestedSubmissionInterfaces)) continue;
            SubmitterPlugin *sp = loader.loadByInterfaceName(itET->ComputingEndpoint->InterfaceName, uc);
            if (sp == NULL) {
              submissionStatusMap[Endpoint(*itET)] = EndpointSubmissionStatus(EndpointSubmissionStatus::NOPLUGIN);
              retval |= SubmissionStatus::SUBMITTER_PLUGIN_NOT_LOADED;
              continue;
            }
            bool submitStatus = sp->Submit(*itJAlt, *itET, cw);
            if (submitStatus) {
              itET->RegisterJobSubmission(*itJAlt);
              descriptionSubmitted = true;
              break;
            }
            // TODO: See comment above, in the other job description loop.
          }
          if (descriptionSubmitted) {
            break;
          }
        }
      }
      if (!descriptionSubmitted) {
        notsubmitted.push_back(&*itJ);
        retval |= SubmissionStatus::DESCRIPTION_NOT_SUBMITTED;
      }
    }
  
    return retval;
  }
} // namespace Arc
