// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <errno.h>

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


#ifdef DBJSTORE_ENABLED
  static void* store_string(const std::string& str, void* buf) {
    uint32_t l = str.length();
    unsigned char* p = (unsigned char*)buf;
    *p = (unsigned char)l; l >>= 8; ++p;
    *p = (unsigned char)l; l >>= 8; ++p;
    *p = (unsigned char)l; l >>= 8; ++p;
    *p = (unsigned char)l; l >>= 8; ++p;
    ::memcpy(p,str.c_str(),str.length());
    p += str.length();
    return (void*)p;
  }

  static void* parse_string(std::string& str, const void* buf, uint32_t& size) {
    uint32_t l = 0;
    const unsigned char* p = (unsigned char*)buf;
    if(size < 4) { p += size; size = 0; return (void*)p; };
    l |= ((uint32_t)(*p)) << 0; ++p; --size;
    l |= ((uint32_t)(*p)) << 8; ++p; --size;
    l |= ((uint32_t)(*p)) << 16; ++p; --size;
    l |= ((uint32_t)(*p)) << 24; ++p; --size;
    if(l > size) l = size;
    // TODO: sanity check
    str.assign((const char*)p,l);
    p += l; size -= l;
    return (void*)p;
  }
  
  static void serialiseJob(const Job& j, Dbt& data) {
    const std::string version = "3.0.0";
    const unsigned nItems = 14;
    const std::string dataItems[] =
      {version, j.IDFromEndpoint, j.Name,
       j.JobStatusInterfaceName, j.JobStatusURL.fullstr(),
       j.JobManagementInterfaceName, j.JobManagementURL.fullstr(),
       j.ServiceInformationInterfaceName, j.ServiceInformationURL.fullstr(),
       j.SessionDir.fullstr(), j.StageInDir.fullstr(), j.StageOutDir.fullstr(),
       j.JobDescriptionDocument, tostring(j.LocalSubmissionTime.GetTime())};
      
    data.set_data(NULL); data.set_size(0);
    uint32_t l = 0;
    for (unsigned i = 0; i < nItems; ++i) l += 4 + dataItems[i].length();
    void* d = (void*)::malloc(l);
    if(!d) return;
    data.set_data(d); data.set_size(l);
    
    for (unsigned i = 0; i < nItems; ++i) d = store_string(dataItems[i], d);
  }
 
  static void deserialiseJob(Job& j, const Dbt& data) {
    uint32_t size = 0;
    void* d = NULL;

    d = (void*)data.get_data();
    size = (uint32_t)data.get_size();
    
    std::string version;
    d = parse_string(version, d, size);
    if (version == "3.0.0") {
      /* Order of items in record. Version 3.0.0
          {version, j.IDFromEndpoint, j.Name,
           j.JobStatusInterfaceName, j.JobStatusURL.fullstr(),
           j.JobManagementInterfaceName, j.JobManagementURL.fullstr(),
           j.ServiceInformationInterfaceName, j.ServiceInformationURL.fullstr(),
           j.SessionDir.fullstr(), j.StageInDir.fullstr(), j.StageOutDir.fullstr(),
           j.JobDescriptionDocument, tostring(j.LocalSubmissionTime.GetTime())};
       */
      std::string s;
      d = parse_string(j.IDFromEndpoint, d, size);
      d = parse_string(j.Name, d, size);
      d = parse_string(j.JobStatusInterfaceName, d, size);
      d = parse_string(s, d, size); j.JobStatusURL = URL(s);
      d = parse_string(j.JobManagementInterfaceName, d, size);
      d = parse_string(s, d, size); j.JobManagementURL = URL(s);
      d = parse_string(j.ServiceInformationInterfaceName, d, size);
      d = parse_string(s, d, size); j.ServiceInformationURL = URL(s);
      d = parse_string(s, d, size); j.SessionDir = URL(s);
      d = parse_string(s, d, size); j.StageInDir = URL(s);
      d = parse_string(s, d, size); j.StageOutDir = URL(s);
      d = parse_string(j.JobDescriptionDocument, d, size);
      d = parse_string(s, d, size); j.LocalSubmissionTime.SetTime(stringtoi(s));
    }
  }
  
  static void deserialiseNthJobAttribute(std::string& attr, const Dbt& data, unsigned n) {
    uint32_t size = 0;
    void* d = NULL;

    d = (void*)data.get_data();
    size = (uint32_t)data.get_size();
    
    std::string version;
    d = parse_string(version, d, size);
    if (version == "3.0.0") {
      for (unsigned i = 0; i < n-1; ++i) {
        d = parse_string(attr, d, size);
      }
    }
  }
  
  static int getNameKey(Db *secondary, const Dbt *key, const Dbt *data, Dbt *result) {
    std::string name;
    // 3rd attribute in job record is job name.
    deserialiseNthJobAttribute(name, *data, 3);
    result->set_flags(DB_DBT_APPMALLOC);
    result->set_size(name.size());
    result->set_data(strdup(name.c_str()));
    return 0;
  }
  
  static int getEndpointKey(Db *secondary, const Dbt *key, const Dbt *data, Dbt *result) {
    std::string endpointS;
    // 7th attribute in job record is job management URL.
    deserialiseNthJobAttribute(endpointS, *data, 7);
    endpointS = URL(endpointS).Host();
    if (endpointS.empty()) {
      return DB_DONOTINDEX;
    }
    result->set_flags(DB_DBT_APPMALLOC);
    result->set_size(endpointS.size());
    result->set_data(strdup(endpointS.c_str()));
    return 0;
  }
  
  static int getServiceInfoHostnameKey(Db *secondary, const Dbt *key, const Dbt *data, Dbt *result) {
    std::string endpointS;
    // 9th attribute in job record is service information URL.
    deserialiseNthJobAttribute(endpointS, *data, 9);
    endpointS = URL(endpointS).Host();
    if (endpointS.empty()) {
      return DB_DONOTINDEX;
    }
    result->set_flags(DB_DBT_APPMALLOC);
    result->set_size(endpointS.size());
    result->set_data(strdup(endpointS.c_str()));
    return 0;
  }
  
  Logger JobInformationStorageBDB::logger(Logger::getRootLogger(), "JobInformationStorageBDB");

  JobInformationStorageBDB::JobDB::JobDB(const std::string& name, u_int32_t flags)
    : dbEnv(NULL), jobDB(NULL), endpointSecondaryKeyDB(NULL), nameSecondaryKeyDB(NULL), serviceInfoSecondaryKeyDB(NULL)
  {
    int ret;
    const DBTYPE type = (flags == DB_CREATE ? DB_BTREE : DB_UNKNOWN);
    std::string basepath = "";
    
    dbEnv = new DbEnv(DB_CXX_NO_EXCEPTIONS);
    if ((ret = dbEnv->open(NULL, DB_CREATE | DB_INIT_CDB | DB_INIT_MPOOL, 0)) != 0) {
      tearDown();
      throw BDBException(IString("Unable to create data base environment (%s)", name).str(), ret);
    }

    jobDB = new Db(dbEnv, DB_CXX_NO_EXCEPTIONS);
    nameSecondaryKeyDB = new Db(dbEnv, DB_CXX_NO_EXCEPTIONS);
    endpointSecondaryKeyDB = new Db(dbEnv, DB_CXX_NO_EXCEPTIONS);
    serviceInfoSecondaryKeyDB = new Db(dbEnv, DB_CXX_NO_EXCEPTIONS);

    if ((ret = nameSecondaryKeyDB->set_flags(DB_DUPSORT)) != 0) {
      tearDown();
      throw BDBException(IString("Unable to set duplicate flags for secondary key DB (%s)", name).str(), ret);
    }
    if ((ret = endpointSecondaryKeyDB->set_flags(DB_DUPSORT)) != 0) {
      tearDown();
      throw BDBException(IString("Unable to set duplicate flags for secondary key DB (%s)", name).str(), ret);
    }
    if ((ret = serviceInfoSecondaryKeyDB->set_flags(DB_DUPSORT)) != 0) {
      tearDown();
      throw BDBException(IString("Unable to set duplicate flags for secondary key DB (%s)", name).str(), ret);
    }

    if ((ret = jobDB->open(NULL, name.c_str(), "job_records", type, flags, 0)) != 0) {
      tearDown();
      throw BDBException(IString("Unable to create job database (%s)", name).str(), ret);
    }
    if ((ret = nameSecondaryKeyDB->open(NULL, name.c_str(), "name_keys", type, flags, 0)) != 0) {
      tearDown();
      throw BDBException(IString("Unable to create DB for secondary name keys (%s)", name).str(), ret);
    }
    if ((ret = endpointSecondaryKeyDB->open(NULL, name.c_str(), "endpoint_keys", type, flags, 0)) != 0) {
      tearDown();
      throw BDBException(IString("Unable to create DB for secondary endpoint keys (%s)", name).str(), ret);
    }
    if ((ret = serviceInfoSecondaryKeyDB->open(NULL, name.c_str(), "serviceinfo_keys", type, flags, 0)) != 0) {
      tearDown();
      throw BDBException(IString("Unable to create DB for secondary service info keys (%s)", name).str(), ret);
    }

    if ((ret = jobDB->associate(NULL, nameSecondaryKeyDB, (flags != DB_RDONLY ? getNameKey : NULL), 0)) != 0) {
      tearDown();
      throw BDBException(IString("Unable to associate secondary DB with primary DB (%s)", name).str(), ret);
    }
    if ((ret = jobDB->associate(NULL, endpointSecondaryKeyDB, (flags != DB_RDONLY ? getEndpointKey : NULL), 0)) != 0) {
      tearDown();
      throw BDBException(IString("Unable to associate secondary DB with primary DB (%s)", name).str(), ret);
    }
    if ((ret = jobDB->associate(NULL, serviceInfoSecondaryKeyDB, (flags != DB_RDONLY ? getServiceInfoHostnameKey : NULL), 0)) != 0) {
      tearDown();
      throw BDBException(IString("Unable to associate secondary DB with primary DB (%s)", name).str(), ret);
    }

    JobInformationStorageBDB::logger.msg(DEBUG, "Job database created successfully (%s)", name);
  }

  void JobInformationStorageBDB::JobDB::tearDown() {
    if (nameSecondaryKeyDB) {
      nameSecondaryKeyDB->close(0);
    }
    if (endpointSecondaryKeyDB) {
      endpointSecondaryKeyDB->close(0);
    }
    if (serviceInfoSecondaryKeyDB) {
      serviceInfoSecondaryKeyDB->close(0);
    }
    if (jobDB) {
      jobDB->close(0);
    }
    if (dbEnv) {
      dbEnv->close(0);
    }

    delete endpointSecondaryKeyDB; endpointSecondaryKeyDB = NULL; 
    delete nameSecondaryKeyDB; nameSecondaryKeyDB = NULL;
    delete serviceInfoSecondaryKeyDB; serviceInfoSecondaryKeyDB = NULL;
    delete jobDB; jobDB = NULL;
    delete dbEnv; dbEnv = NULL;

    dbEnv = new DbEnv(DB_CXX_NO_EXCEPTIONS);
    dbEnv->remove(NULL, 0);
    delete dbEnv; dbEnv = NULL;
  }

  JobInformationStorageBDB::JobDB::~JobDB() {
    tearDown();
  }

  JobInformationStorageBDB::BDBException::BDBException(const std::string& msg, int ret, bool writeLogMessage) throw() : message(msg), returnvalue(ret) {
    if (writeLogMessage) {
      JobInformationStorageBDB::logger.msg(VERBOSE, msg);
      JobInformationStorageBDB::logErrorMessage(ret);
    }
  }

  JobInformationStorageBDB::JobInformationStorageBDB(const std::string& name, unsigned nTries, unsigned tryInterval)
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

    try {
      JobDB db(name);
    } catch (const BDBException& e) {
      isValid = false;
      return;
    }
    isStorageExisting = isValid = true;
  }

  bool JobInformationStorageBDB::Write(const std::list<Job>& jobs) {
    if (!isValid) {
      return false;
    }
    if (jobs.empty()) return true;
    
    try {
      JobDB db(name, DB_CREATE);
      int ret;
      std::list<Job>::const_iterator it = jobs.begin();
      void* pdata = NULL;
      Dbt key, data;
      {
        InterruptGuard guard;
        do {
          ::free(pdata);
          key.set_size(it->JobID.size());
          key.set_data((char*)it->JobID.c_str());
          serialiseJob(*it, data);
          pdata = data.get_data();
        } while ((ret = db->put(NULL, &key, &data, 0)) == 0 && ++it != jobs.end());
        ::free(pdata);
      };
      
      if (ret != 0) {
        logger.msg(VERBOSE, "Unable to write key/value pair to job database (%s): Key \"%s\"", name, (char*)key.get_data());
        logErrorMessage(ret);
        return false;
      }
    } catch (const BDBException& e) {
      return false;
    }

    return true;
  }

  bool JobInformationStorageBDB::Write(const std::list<Job>& jobs, const std::set<std::string>& prunedServices, std::list<const Job*>& newJobs) {
    if (!isValid) {
      return false;
    }
    if (jobs.empty()) return true;
    
    try {
      JobDB db(name, DB_CREATE);
      int ret = 0;
      
      std::set<std::string> idsOfPrunedJobs;
      Dbc *cursor = NULL;
      if ((ret = db.viaServiceInfoKeys()->cursor(NULL, &cursor, DB_WRITECURSOR)) != 0) return false;
      for (std::set<std::string>::const_iterator itPruned = prunedServices.begin();
           itPruned != prunedServices.end(); ++itPruned) {
        Dbt key((char *)itPruned->c_str(), itPruned->size()), pkey, data;
        if (cursor->pget(&key, &pkey, &data, DB_SET) != 0) continue;
        do {
          idsOfPrunedJobs.insert(std::string((char *)pkey.get_data(), pkey.get_size()));
          cursor->del(0);
        } while (cursor->pget(&key, &pkey, &data, DB_NEXT_DUP) == 0);
      }
      cursor->close();
      
      std::list<Job>::const_iterator it = jobs.begin();
      void* pdata = NULL;
      Dbt key, data;
      bool jobWasPruned;
      {
        InterruptGuard guard;
        do {
          ::free(pdata);
          key.set_size(it->JobID.size());
          key.set_data((char*)it->JobID.c_str());
          serialiseJob(*it, data);
          pdata = data.get_data();
          jobWasPruned = (idsOfPrunedJobs.count(it->JobID) != 0);
          if (!jobWasPruned) { // Check if job already exist.
            Dbt existingData;
            if (db->get(NULL, &key, &existingData, 0) == DB_NOTFOUND) {
              newJobs.push_back(&*it);
            }
          }
        } while (((ret = db->put(NULL, &key, &data, 0)) == 0 && ++it != jobs.end()));
        ::free(pdata);
      };
      
      if (ret != 0) {
        logger.msg(VERBOSE, "Unable to write key/value pair to job database (%s): Key \"%s\"", name, (char*)key.get_data());
        logErrorMessage(ret);
        return false;
      }
    } catch (const BDBException& e) {
      return false;
    }

    return true;
  }

  bool JobInformationStorageBDB::ReadAll(std::list<Job>& jobs, const std::list<std::string>& rejectEndpoints) {
    if (!isValid) {
      return false;
    }
    jobs.clear();

    try {
      int ret;
      JobDB db(name);
  
      Dbc *cursor;
      if ((ret = db->cursor(NULL, &cursor, 0)) != 0) {
        //dbp->err(dbp, ret, "DB->cursor");
        return false;
      }
      
      Dbt key, data;
      while ((ret = cursor->get(&key, &data, DB_NEXT)) == 0) {
        jobs.push_back(Job());
        jobs.back().JobID = std::string((char *)key.get_data(), key.get_size());
        deserialiseJob(jobs.back(), data);
        for (std::list<std::string>::const_iterator it = rejectEndpoints.begin();
             it != rejectEndpoints.end(); ++it) {
          if (jobs.back().JobManagementURL.StringMatches(*it)) {
            jobs.pop_back();
            break;
          }
        }
      }
  
      cursor->close();
      
      if (ret != DB_NOTFOUND) {
        //dbp->err(dbp, ret, "DBcursor->get");
        return false;
      }
    } catch (const BDBException& e) {
      return false;
    }
    
    return true;
  }
  
  static void addJobFromDB(const Dbt& key, const Dbt& data, std::list<Job>& jobs, std::set<std::string>& idsOfAddedJobs, const std::list<std::string>& rejectEndpoints) {
    jobs.push_back(Job());
    jobs.back().JobID.assign((char *)key.get_data(), key.get_size());
    deserialiseJob(jobs.back(), data);
    if (idsOfAddedJobs.count(jobs.back().JobID) != 0) { // Look for duplicates and remove them.
      jobs.pop_back();
      return;
    }
    idsOfAddedJobs.insert(jobs.back().JobID);
    for (std::list<std::string>::const_iterator it = rejectEndpoints.begin();
         it != rejectEndpoints.end(); ++it) {
      if (jobs.back().JobManagementURL.StringMatches(*it)) {
        idsOfAddedJobs.erase(jobs.back().JobID);
        jobs.pop_back();
        return;
      }
    }
  }

  static bool addJobsFromDuplicateKeys(Db& db, Dbt& key, std::list<Job>& jobs, std::set<std::string>& idsOfAddedJobs, const std::list<std::string>& rejectEndpoints) {
    int ret;
    Dbt pkey, data;
    Dbc *cursor;
    if ((ret = db.cursor(NULL, &cursor, 0)) != 0) {
      //dbp->err(dbp, ret, "DB->cursor");
      return false;
    }
    ret = cursor->pget(&key, &pkey, &data, DB_SET);
    if (ret != 0) return false;
    
    addJobFromDB(pkey, data, jobs, idsOfAddedJobs, rejectEndpoints);
    while ((ret = cursor->pget(&key, &pkey, &data, DB_NEXT_DUP)) == 0) {
       addJobFromDB(pkey, data, jobs, idsOfAddedJobs, rejectEndpoints);
    }
    return true;
  }
  
  bool JobInformationStorageBDB::Read(std::list<Job>& jobs, std::list<std::string>& jobIdentifiers,
                                      const std::list<std::string>& endpoints,
                                      const std::list<std::string>& rejectEndpoints) {
    if (!isValid) {
      return false;
    }
    jobs.clear();
    
    try {
      JobDB db(name);
      int ret;
      std::set<std::string> idsOfAddedJobs;
      for (std::list<std::string>::iterator it = jobIdentifiers.begin();
           it != jobIdentifiers.end();) {
        if (it->empty()) continue;
        
        Dbt key((char *)it->c_str(), it->size()), data;
        ret = db->get(NULL, &key, &data, 0);
        if (ret == DB_NOTFOUND) {
          if (addJobsFromDuplicateKeys(*db.viaNameKeys(), key, jobs, idsOfAddedJobs, rejectEndpoints)) {
            it = jobIdentifiers.erase(it);
          }
          else {
            ++it;
          }
          continue;
        }
        addJobFromDB(key, data, jobs, idsOfAddedJobs, rejectEndpoints);
        it = jobIdentifiers.erase(it);
      }
      if (endpoints.empty()) return true;
      
      Dbc *cursor;
      if ((ret = db.viaEndpointKeys()->cursor(NULL, &cursor, 0)) != 0) return false;
      for (std::list<std::string>::const_iterator it = endpoints.begin();
           it != endpoints.end(); ++it) {
        // Extract hostname from iterator.
        URL u(*it);
        if (u.Protocol() == "file") {
          u = URL("http://" + *it); // Only need to extract hostname. Prefix with "http://".
        }
        if (u.Host().empty()) continue;

        Dbt key((char *)u.Host().c_str(), u.Host().size()), pkey, data;
        ret = cursor->pget(&key, &pkey, &data, DB_SET);
        if (ret != 0) {
          continue;
        }
        std::string tmpEndpoint;
        deserialiseNthJobAttribute(tmpEndpoint, data, 7);
        URL jobManagementURL(tmpEndpoint);
        if (jobManagementURL.StringMatches(*it)) {
          addJobFromDB(pkey, data, jobs, idsOfAddedJobs, rejectEndpoints);
        }
        while ((ret = cursor->pget(&key, &pkey, &data, DB_NEXT_DUP)) == 0) {
          deserialiseNthJobAttribute(tmpEndpoint, data, 7);
          URL jobManagementURL(tmpEndpoint);
          if (jobManagementURL.StringMatches(*it)) {
           addJobFromDB(pkey, data, jobs, idsOfAddedJobs, rejectEndpoints);
         }
        }
      }
    } catch (const BDBException& e) {
      return false;
    }

    return true;
  }
  
  bool JobInformationStorageBDB::Clean() {
    if (!isValid) {
      return false;
    }

    if (remove(name.c_str()) != 0) {
      if (errno == ENOENT) return true; // No such file. DB already cleaned.
      logger.msg(VERBOSE, "Unable to truncate job database (%s)", name);
      perror("Error");
      return false;
    }
    
    return true;
  }
  
  bool JobInformationStorageBDB::Remove(const std::list<std::string>& jobids) {
    if (!isValid) {
      return false;
    }

    try {
      InterruptGuard guard;
      JobDB db(name, DB_CREATE);
      for (std::list<std::string>::const_iterator it = jobids.begin();
           it != jobids.end(); ++it) {
        Dbt key((char *)it->c_str(), it->size());
        db->del(NULL, &key, 0);
      }
    } catch (const BDBException& e) {
      return false;
    }
    
    return true;
  }
  
  void JobInformationStorageBDB::logErrorMessage(int err) {
    switch (err) {
    case ENOENT:
      logger.msg(DEBUG, "ENOENT: The file or directory does not exist, Or a nonexistent re_source file was specified.");
      break;
    case DB_OLD_VERSION:
      logger.msg(DEBUG, "DB_OLD_VERSION: The database cannot be opened without being first upgraded.");
      break;
    case EEXIST:
      logger.msg(DEBUG, "EEXIST: DB_CREATE and DB_EXCL were specified and the database exists.");
    case EINVAL:
      logger.msg(DEBUG, "EINVAL");
      break;
    default:
      logger.msg(DEBUG, "Unable to determine error (%d)", err);
    }
  }
#endif

} // namespace Arc
