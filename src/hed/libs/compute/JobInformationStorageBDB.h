// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_JOBINFORMATIONSTORAGEBDB_H__
#define __ARC_JOBINFORMATIONSTORAGEBDB_H__

#include <db_cxx.h>

#include "JobInformationStorage.h"

namespace Arc {
  
  class JobInformationStorageBDB : public JobInformationStorage {
  public:
    JobInformationStorageBDB(const std::string& name, unsigned nTries = 10, unsigned tryInterval = 500000);
    virtual ~JobInformationStorageBDB() {}
    
    bool ReadAll(std::list<Job>& jobs, const std::list<std::string>& rejectEndpoints = std::list<std::string>());
    bool Read(std::list<Job>& jobs, std::list<std::string>& jobIdentifiers,
                      const std::list<std::string>& endpoints = std::list<std::string>(),
                      const std::list<std::string>& rejectEndpoints = std::list<std::string>());
    bool Write(const std::list<Job>& jobs);
    bool Write(const std::list<Job>& jobs, const std::set<std::string>& prunedServices, std::list<const Job*>& newJobs);
    bool Clean();
    bool Remove(const std::list<std::string>& jobids);

  private:
    static void logErrorMessage(int err);
  
    static Logger logger;
    
    class JobDB {
    public:
      JobDB(const std::string&, u_int32_t = DB_RDONLY);
      ~JobDB();
      
      void tearDown();

      static void handleError(const DbEnv *dbenv, const char *errpfx, const char *msg);

      Db* operator->() { return jobDB; }
      Db* viaNameKeys() { return nameSecondaryKeyDB; }
      Db* viaEndpointKeys() { return endpointSecondaryKeyDB; }
      Db* viaServiceInfoKeys() { return serviceInfoSecondaryKeyDB; }
      
      DbEnv *dbEnv;
      Db *jobDB;
      Db *endpointSecondaryKeyDB;
      Db *nameSecondaryKeyDB;
      Db *serviceInfoSecondaryKeyDB;
    };
    
    class BDBException {
    public:
      BDBException(const std::string& msg, int ret, bool writeLogMessage = true) throw();
      ~BDBException() throw() {}
      const std::string& getMessage() const throw()  { return message; }
      int getReturnValue() const throw() { return returnvalue; }

    private:
      std::string message;
      int returnvalue;
    };
  };

} // namespace Arc

#endif // __ARC_JOBINFORMATIONSTORAGEBDB_H__
