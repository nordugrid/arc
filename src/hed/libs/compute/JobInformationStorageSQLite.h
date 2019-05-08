// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_JOBINFORMATIONSTORAGESQLITE_H__
#define __ARC_JOBINFORMATIONSTORAGESQLITE_H__

#include <sqlite3.h>

#include "JobInformationStorage.h"

namespace Arc {
  
  class JobInformationStorageSQLite : public JobInformationStorage {
  public:
    JobInformationStorageSQLite(const std::string& name, unsigned nTries = 10, unsigned tryInterval = 500000);
    virtual ~JobInformationStorageSQLite() {}

    static JobInformationStorage* Instance(const std::string& name) { return new JobInformationStorageSQLite(name); }
    
    bool ReadAll(std::list<Job>& jobs, const std::list<std::string>& rejectEndpoints = std::list<std::string>());
    bool Read(std::list<Job>& jobs, std::list<std::string>& jobIdentifiers,
                      const std::list<std::string>& endpoints = std::list<std::string>(),
                      const std::list<std::string>& rejectEndpoints = std::list<std::string>());
    bool Write(const std::list<Job>& jobs)  { std::list<const Job*> newJobs; std::set<std::string> prunedServices; return Write(jobs, prunedServices, newJobs); }
    bool Write(const std::list<Job>& jobs, const std::set<std::string>& prunedServices, std::list<const Job*>& newJobs);
    bool Clean();
    bool Remove(const std::list<std::string>& jobids);

  private:
    static void logErrorMessage(int err);
  
    static Logger logger;
    
    class JobDB {
    public:
      JobDB(const std::string& name, bool create = false);

      ~JobDB();
      
      sqlite3* handle() { return jobDB; }

    private:
      void tearDown();

      void handleError(const char* errpfx, int err);

      sqlite3* jobDB;

    };
    
    class SQLiteException {
    public:
      SQLiteException(const std::string& msg, int ret, bool writeLogMessage = true) throw();
      ~SQLiteException() throw() {}
      const std::string& getMessage() const throw()  { return message; }
      int getReturnValue() const throw() { return returnvalue; }

    private:
      std::string message;
      int returnvalue;
    };
  };

} // namespace Arc

#endif // __ARC_JOBINFORMATIONSTORAGESQLITE_H__
