// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/FileLock.h>
#include <arc/Logger.h>
#include <arc/StringConv.h>
#include <arc/Utils.h>

#include "JobInformationStorage.h"

namespace Arc {

  Logger JobInformationStorageXML::logger(Logger::getRootLogger(), "JobInformationStorageXML");

  JobInformationStorageXML::JobInformationStorageXML(const std::string& name, unsigned nTries, unsigned tryInterval)
      : JobInformationStorage(name, nTries, tryInterval) {
    isValid = false;
    isStorageExisting = false;

    if (!Glib::file_test(name, Glib::FILE_TEST_EXISTS)) {
      const std::string joblistdir = Glib::path_get_dirname(name);
      // Check if the parent directory exist.
      if (!Glib::file_test(joblistdir, Glib::FILE_TEST_EXISTS)) {
        logger.msg(ERROR, "Job list file cannot be created: The parent directory (%s) doesn't exist.", joblistdir);
        return;
      }
      else if (!Glib::file_test(joblistdir, Glib::FILE_TEST_IS_DIR)) {
        logger.msg(ERROR, "Job list file cannot be created: %s is not a directory", joblistdir);
        return;
      }
      isValid = true;
      return;
    }
    else if (!Glib::file_test(name, Glib::FILE_TEST_IS_REGULAR)) {
      logger.msg(ERROR, "Job list file (%s) is not a regular file", name);
      return;
    }

    FileLock lock(name);
    for (int tries = (int)nTries; tries > 0; --tries) {
      if (lock.acquire()) {
        isStorageExisting = isValid = jobstorage.ReadFromFile(name);
        lock.release();
        break;
      }

      if (tries == 6) {
        logger.msg(VERBOSE, "Waiting for lock on job list file %s", name);
      }

      Glib::usleep(tryInterval);
    }
  }

  bool JobInformationStorageXML::ReadAll(std::list<Job>& jobs, const std::list<std::string>& rEndpoints) {
    if (!isValid) {
      return false;
    }

    jobs.clear();
    
    XMLNodeList xmljobs = jobstorage.Path("Job");
    for (XMLNodeList::iterator xit = xmljobs.begin(); xit != xmljobs.end(); ++xit) {
      jobs.push_back(*xit);
      for (std::list<std::string>::const_iterator rEIt = rEndpoints.begin();
           rEIt != rEndpoints.end(); ++rEIt) {
        if (jobs.back().JobManagementURL.StringMatches(*rEIt)) {
          jobs.pop_back();
          break;
        }
      }
    }

    return true;
  }

  bool JobInformationStorageXML::Read(std::list<Job>& jobs, std::list<std::string>& jobIdentifiers, const std::list<std::string>& endpoints, const std::list<std::string>& rEndpoints) {
    if (!ReadAll(jobs, rEndpoints)) { return false; }

    std::list<std::string> jobIdentifiersCopy = jobIdentifiers;
    for (std::list<Job>::iterator itJ = jobs.begin();
         itJ != jobs.end();) {
      // Check if the job (itJ) is selected by the job identifies, either by job ID or Name.
      std::list<std::string>::iterator itJIdentifier = jobIdentifiers.begin();
      for (;itJIdentifier != jobIdentifiers.end(); ++itJIdentifier) {
        if ((!itJ->Name.empty() && itJ->Name == *itJIdentifier) ||
            (itJ->JobID == *itJIdentifier)) {
          break;
        }
      }
      if (itJIdentifier != jobIdentifiers.end()) {
        // Job explicitly specified. Remove id from the copy list, in order to keep track of used identifiers.
        std::list<std::string>::iterator itJIdentifierCopy;
        while ((itJIdentifierCopy = std::find(jobIdentifiersCopy.begin(), jobIdentifiersCopy.end(), *itJIdentifier))
               != jobIdentifiersCopy.end()) {
          jobIdentifiersCopy.erase(itJIdentifierCopy);
        }
        ++itJ;
        continue;
      }

      // Check if the job (itJ) is selected by endpoints.
      std::list<std::string>::const_iterator itC = endpoints.begin();
      for (; itC != endpoints.end(); ++itC) {
        if (itJ->JobManagementURL.StringMatches(*itC)) {
          break;
        }
      }
      if (itC != endpoints.end()) {
        // Cluster on which job reside is explicitly specified.
        ++itJ;
        continue;
      }

      // Job is not selected - remove it.
      itJ = jobs.erase(itJ);
    }

    jobIdentifiers = jobIdentifiersCopy;

    return true;
  }

  bool JobInformationStorageXML::Clean() {
    if (!isValid) {
      return false;
    }
    
    if (remove(name.c_str()) != 0) {
      if (errno == ENOENT) {
        jobstorage.Destroy();
        return true; // No such file. DB already cleaned.
      }
      logger.msg(VERBOSE, "Unable to truncate job database (%s)", name);
      perror("Error");
      return false;
    }
    
    jobstorage.Destroy();
    return true;
  }

  bool JobInformationStorageXML::Write(const std::list<Job>& jobs, const std::set<std::string>& prunedServices, std::list<const Job*>& newJobs) {
    if (!isValid) {
      return false;
    }

    if (!jobstorage) {
      Config().Swap(jobstorage);
    }

    // Use std::map to store job IDs to be searched for duplicates.
    std::map<std::string, XMLNode> jobIDXMLMap;
    std::map<std::string, XMLNode> jobsToRemove;
    for (Arc::XMLNode j = jobstorage["Job"]; j; ++j) {
      if (!((std::string)j["JobID"]).empty()) {
        std::string serviceName = URL(j["ServiceInformationURL"]).Host();
        if (!serviceName.empty() && prunedServices.count(serviceName)) {
          logger.msg(DEBUG, "Will remove %s on service %s.",
                     ((std::string)j["JobID"]).c_str(), serviceName);
          jobsToRemove[(std::string)j["JobID"]] = j;
        }
        else {
          jobIDXMLMap[(std::string)j["JobID"]] = j;
        }
      }
    }

    // Remove jobs which belong to our list of endpoints to prune.
    for (std::map<std::string, XMLNode>::iterator it = jobsToRemove.begin();
         it != jobsToRemove.end(); ++it) {
      it->second.Destroy();
    }

    std::map<std::string, const Job*> newJobsMap;
    for (std::list<Job>::const_iterator it = jobs.begin(); it != jobs.end(); ++it) {
      std::map<std::string, XMLNode>::iterator itJobXML = jobIDXMLMap.find(it->JobID);
      if (itJobXML == jobIDXMLMap.end()) {
        XMLNode xJob = jobstorage.NewChild("Job");
        it->ToXML(xJob);
        jobIDXMLMap[it->JobID] = xJob;

        std::map<std::string, XMLNode>::iterator itRemovedJobs = jobsToRemove.find(it->JobID);
        if (itRemovedJobs == jobsToRemove.end()) {
          newJobsMap[it->JobID] = &(*it);
        }
      }
      else {
        // Duplicate found, replace it.
        itJobXML->second.Replace(XMLNode(NS(), "Job"));
        it->ToXML(itJobXML->second);

        // Only add to newJobsMap if this is a new job, i.e. not previous present in jobfile.
        std::map<std::string, const Job*>::iterator itNewJobsMap = newJobsMap.find(it->JobID);
        if (itNewJobsMap != newJobsMap.end()) {
          itNewJobsMap->second = &(*it);
        }
      }
    }

    // Add pointers to new Job objects to the newJobs list.
    for (std::map<std::string, const Job*>::const_iterator it = newJobsMap.begin();
         it != newJobsMap.end(); ++it) {
      newJobs.push_back(it->second);
    }


    FileLock lock(name);
    for (int tries = nTries; tries > 0; --tries) {
      if (lock.acquire()) {
        if (!jobstorage.SaveToFile(name)) {
          lock.release();
          return false;
        }
        lock.release();
        return true;
      }

      if (tries == 6) {
        logger.msg(WARNING, "Waiting for lock on job list file %s", name);
      }

      Glib::usleep(tryInterval);
    }

    return false;
  }

  bool JobInformationStorageXML::Remove(const std::list<std::string>& jobids) {
    if (!isValid) {
      return false;
    }
    
    if (jobids.empty()) {
      return true;
    }

    XMLNodeList xmlJobs = jobstorage.Path("Job");
    for (std::list<std::string>::const_iterator it = jobids.begin(); it != jobids.end(); ++it) {
      for (XMLNodeList::iterator xJIt = xmlJobs.begin(); xJIt != xmlJobs.end(); ++xJIt) {
        if ((*xJIt)["JobID"] == *it ||
            (*xJIt)["IDFromEndpoint"] == *it // Included for backwards compatibility.
            ) {
          xJIt->Destroy(); // Do not break, since for some reason there might be multiple identical jobs in the file.
        }
      }
    }

    FileLock lock(name);
    for (int tries = nTries; tries > 0; --tries) {
      if (lock.acquire()) {
        if (!jobstorage.SaveToFile(name)) {
          lock.release();
          return false;
        }
        lock.release();
        return true;
      }

      if (tries == 6) {
        logger.msg(VERBOSE, "Waiting for lock on job list file %s", name);
      }
      Glib::usleep(tryInterval);
    }

    return false;
  }

} // namespace Arc
