// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/FileUtils.h>
#include <arc/Logger.h>
#include <arc/StringConv.h>
#include <arc/Utils.h>

#include "JobInformationStorageBDB.h"

namespace Arc {

  Logger JobInformationStorageBDB::logger(Logger::getRootLogger(), "JobInformationStorageBDB");

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

  static void* parse_string(int& val, const void* buf, uint32_t& size) {
    std::string str;
    void* r = parse_string(str, buf, size);
    stringto(str, val);
    return r;
  }
  
  static void* parse_string(URL& url, const void* buf, uint32_t& size) {
    std::string str;
    void* r = parse_string(str, buf, size);
    url = URL(str);
    return r;
  }
  
  static void* parse_string(std::list<std::string>& strs, const void* buf, uint32_t& size) {
    std::string str;
    void* r = parse_string(str, buf, size);
    if(!str.empty()) strs.push_back(str);
    return r;
  }

  static void* parse_string(JobState& state, const void* buf, uint32_t& size) {
    std::string str;
    void* r = parse_string(str, buf, size);
    state = JobState(str);
    return r;
  }
  
  static void* parse_string(Time& val, const void* buf, uint32_t& size) {
    std::string str;
    void* r = parse_string(str, buf, size);
    val.SetTime(stringtoi(str));
    return r;
  }

  static void* parse_string(Period& val, const void* buf, uint32_t& size) {
    std::string str;
    void* r = parse_string(str, buf, size);
    val = Period(str);
    return r;
  }

  static void serialiseJob(const Job& j, Dbt& data) {
    const std::string version = "3.0.2";
    const std::string empty_string;
    const std::string dataItems[] =
      {version, j.IDFromEndpoint, j.Name,
       j.JobStatusInterfaceName, j.JobStatusURL.fullstr(),
       j.JobManagementInterfaceName, j.JobManagementURL.fullstr(),
       j.ServiceInformationInterfaceName, j.ServiceInformationURL.fullstr(),
       j.SessionDir.fullstr(), j.StageInDir.fullstr(), j.StageOutDir.fullstr(),
       j.JobDescriptionDocument, tostring(j.LocalSubmissionTime.GetTime()),
       // 3.0.1
       j.DelegationID.size()>0?*j.DelegationID.begin():empty_string,
       // 3.0.2
       j.Type,
       j.LocalIDFromManager,
       j.JobDescription,
       j.State.GetGeneralState(),
       j.RestartState.GetGeneralState(),
       Arc::tostring(j.ExitCode),
       j.ComputingManagerExitCode,
       j.Error.size()>0?*j.Error.begin():empty_string,
       Arc::tostring(j.WaitingPosition),
       j.UserDomain,
       j.Owner,
       j.LocalOwner,
       j.RequestedTotalWallTime,
       j.RequestedTotalCPUTime,
       Arc::tostring(j.RequestedSlots),
       j.RequestedApplicationEnvironment.size()>0?*j.RequestedApplicationEnvironment.begin():empty_string,
       j.StdIn,
       j.StdOut,
       j.StdErr,
       j.LogDir,
       j.ExecutionNode.size()>0?*j.ExecutionNode.begin():empty_string,
       j.Queue,
       j.UsedTotalWallTime,
       j.UsedTotalCPUTime,
       Arc::tostring(j.UsedMainMemory),
       tostring(j.SubmissionTime.GetTime()),
       tostring(j.ComputingManagerSubmissionTime.GetTime()),
       tostring(j.StartTime.GetTime()),
       tostring(j.ComputingManagerEndTime.GetTime()),
       tostring(j.EndTime.GetTime()),
       tostring(j.WorkingAreaEraseTime.GetTime()),
       tostring(j.ProxyExpirationTime.GetTime()),
       j.SubmissionHost,
       j.SubmissionClientName,
       j.OtherMessages.size()>0?*j.OtherMessages.begin():empty_string,
       j.ActivityOldID.size()>0?*j.ActivityOldID.begin():empty_string
      };
    const unsigned nItems = sizeof(dataItems)/sizeof(dataItems[0]);
      
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
    if ((version == "3.0.0") || (version == "3.0.1") || (version == "3.0.2")) {
      /* Order of items in record. Version 3.0.0
          {version, j.IDFromEndpoint, j.Name,
           j.JobStatusInterfaceName, j.JobStatusURL.fullstr(),
           j.JobManagementInterfaceName, j.JobManagementURL.fullstr(),
           j.ServiceInformationInterfaceName, j.ServiceInformationURL.fullstr(),
           j.SessionDir.fullstr(), j.StageInDir.fullstr(), j.StageOutDir.fullstr(),
           j.JobDescriptionDocument, tostring(j.LocalSubmissionTime.GetTime())};
         Version 3.0.1
           ..., j.DelegationID}
         Version 3.0.2
             j.Type,
             j.LocalIDFromManager,
             j.JobDescription,
             j.State.GetGeneralState(),
             j.RestartState.GetGeneralState(),
             j.ExitCode,
             j.ComputingManagerExitCode,
             j.Error,
             j.WaitingPosition,
             j.UserDomain,
             j.Owner,
             j.LocalOwner,
             j.RequestedTotalWallTime,
             j.RequestedTotalCPUTime,
             j.RequestedSlots,
             j.RequestedApplicationEnvironment,
             j.StdIn,
             j.StdOut,
             j.StdErr,
             j.LogDir,
             j.ExecutionNode,
             j.Queue,
             j.UsedTotalWallTime,
             j.UsedTotalCPUTime,
             j.UsedMainMemory,
             j.SubmissionTime,
             j.ComputingManagerSubmissionTime,
             j.StartTime,
             j.ComputingManagerEndTime,
             j.EndTime,
             j.WorkingAreaEraseTime,
             j.ProxyExpirationTime,
             j.SubmissionHost,
             j.SubmissionClientName,
             j.OtherMessages,
             j.ActivityOldID}
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
      d = parse_string(j.LocalSubmissionTime, d, size);
      j.DelegationID.clear();
      if ((version == "3.0.1") || (version == "3.0.2")) {
        d = parse_string(s, d, size);
        if(!s.empty()) j.DelegationID.push_back(s);
        if (version == "3.0.2") {
          d = parse_string(j.Type, d, size);
          d = parse_string(j.LocalIDFromManager, d, size);
          d = parse_string(j.JobDescription, d, size);
          d = parse_string(j.State, d, size);
          d = parse_string(j.RestartState, d, size);
          d = parse_string(j.ExitCode, d, size);
          d = parse_string(j.ComputingManagerExitCode, d, size);
          d = parse_string(j.Error, d, size);
          d = parse_string(j.WaitingPosition, d, size);
          d = parse_string(j.UserDomain, d, size);
          d = parse_string(j.Owner, d, size);
          d = parse_string(j.LocalOwner, d, size);
          d = parse_string(j.RequestedTotalWallTime, d, size);
          d = parse_string(j.RequestedTotalCPUTime, d, size);
          d = parse_string(j.RequestedSlots, d, size);
          d = parse_string(j.RequestedApplicationEnvironment, d, size);
          d = parse_string(j.StdIn, d, size);
          d = parse_string(j.StdOut, d, size);
          d = parse_string(j.StdErr, d, size);
          d = parse_string(j.LogDir, d, size);
          d = parse_string(j.ExecutionNode, d, size);
          d = parse_string(j.Queue, d, size);
          d = parse_string(j.UsedTotalWallTime, d, size);
          d = parse_string(j.UsedTotalCPUTime, d, size);
          d = parse_string(j.UsedMainMemory, d, size);
          d = parse_string(j.SubmissionTime, d, size);
          d = parse_string(j.ComputingManagerSubmissionTime, d, size);
          d = parse_string(j.StartTime, d, size);
          d = parse_string(j.ComputingManagerEndTime, d, size);
          d = parse_string(j.EndTime, d, size);
          d = parse_string(j.WorkingAreaEraseTime, d, size);
          d = parse_string(j.ProxyExpirationTime, d, size);
          d = parse_string(j.SubmissionHost, d, size);
          d = parse_string(j.SubmissionClientName, d, size);
          d = parse_string(j.OtherMessages, d, size);
          d = parse_string(j.ActivityOldID, d, size);
        }
      }
    }
  }
  
  static void deserialiseNthJobAttribute(std::string& attr, const Dbt& data, unsigned n) {
    uint32_t size = 0;
    void* d = NULL;

    d = (void*)data.get_data();
    size = (uint32_t)data.get_size();
    
    std::string version;
    d = parse_string(version, d, size);
    if ((version == "3.0.0") || (version == "3.0.1")) {
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
  
  JobInformationStorageBDB::JobDB::JobDB(const std::string& name, u_int32_t flags)
    : dbEnv(NULL), jobDB(NULL), endpointSecondaryKeyDB(NULL), nameSecondaryKeyDB(NULL), serviceInfoSecondaryKeyDB(NULL)
  {
    int ret;
    const DBTYPE type = (flags == DB_CREATE ? DB_BTREE : DB_UNKNOWN);
    std::string basepath = "";
    
    if (!TmpDirCreate(tmpdir)) {
      throw BDBException(IString("Unable to create temporary directory").str(), 1);
    }

    dbEnv = new DbEnv(DB_CXX_NO_EXCEPTIONS);
    dbEnv->set_errcall(&handleError);

    if ((ret = dbEnv->open(tmpdir.c_str(), DB_CREATE | DB_INIT_CDB | DB_INIT_MPOOL, 0)) != 0) {
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

    std::string absPathToDB = URL(name).Path();
    if ((ret = jobDB->open(NULL, absPathToDB.c_str(), "job_records", type, flags, 0)) != 0) {
      tearDown();
      throw BDBException(IString("Unable to create job database (%s)", name).str(), ret);
    }
    if ((ret = nameSecondaryKeyDB->open(NULL, absPathToDB.c_str(), "name_keys", type, flags, 0)) != 0) {
      tearDown();
      throw BDBException(IString("Unable to create DB for secondary name keys (%s)", name).str(), ret);
    }
    if ((ret = endpointSecondaryKeyDB->open(NULL, absPathToDB.c_str(), "endpoint_keys", type, flags, 0)) != 0) {
      tearDown();
      throw BDBException(IString("Unable to create DB for secondary endpoint keys (%s)", name).str(), ret);
    }
    if ((ret = serviceInfoSecondaryKeyDB->open(NULL, absPathToDB.c_str(), "serviceinfo_keys", type, flags, 0)) != 0) {
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
    dbEnv->remove(tmpdir.c_str(), 0);
    DirDelete(tmpdir, true);
    delete dbEnv; dbEnv = NULL;
  }

  JobInformationStorageBDB::JobDB::~JobDB() {
    tearDown();
  }

#if ((DB_VERSION_MAJOR > 4)||(DB_VERSION_MAJOR == 4 && DB_VERSION_MINOR >= 3))
  void JobInformationStorageBDB::JobDB::handleError(const DbEnv *dbenv, const char *errpfx, const char *msg) {
#else
  void JobInformationStorageBDB::JobDB::handleError(const char *errpfx, char *msg) {
#endif
    if (errpfx) {
      JobInformationStorageBDB::logger.msg(DEBUG, "Error from BDB: %s: %s", errpfx, msg);
    }
    else {
      JobInformationStorageBDB::logger.msg(DEBUG, "Error from BDB: %s", msg);
    }
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
} // namespace Arc
