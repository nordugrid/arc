// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_JOBINFORMATIONSTORAGEXML_H__
#define __ARC_JOBINFORMATIONSTORAGEXML_H__

#include <arc/ArcConfig.h>

#include "JobInformationStorage.h"

namespace Arc {

  class JobInformationStorageXML : public JobInformationStorage {
  public:
    JobInformationStorageXML(const std::string& name, unsigned nTries = 10, unsigned tryInterval = 500000);
    virtual ~JobInformationStorageXML() {}
    
    bool ReadAll(std::list<Job>& jobs, const std::list<std::string>& rejectEndpoints = std::list<std::string>());
    bool Read(std::list<Job>& jobs, std::list<std::string>& jobIdentifiers,
                      const std::list<std::string>& endpoints = std::list<std::string>(),
                      const std::list<std::string>& rejectEndpoints = std::list<std::string>());
    bool Write(const std::list<Job>& jobs)  { std::list<const Job*> newJobs; std::set<std::string> prunedServices; return Write(jobs, prunedServices, newJobs); }
    bool Write(const std::list<Job>& jobs, const std::set<std::string>& prunedServices, std::list<const Job*>& newJobs);
    bool Clean();
    bool Remove(const std::list<std::string>& jobids);
    
  private:
    Config jobstorage;
    static Logger logger;
  };

} // namespace Arc

#endif // __ARC_JOBINFORMATIONSTORAGEXML_H__
