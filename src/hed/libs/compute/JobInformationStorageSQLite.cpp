// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/FileUtils.h>
#include <arc/Logger.h>
#include <arc/StringConv.h>
#include <arc/Utils.h>

#include "JobInformationStorageSQLite.h"

namespace Arc {

  Logger JobInformationStorageSQLite::logger(Logger::getRootLogger(), "JobInformationStorageSQLite");

  static const std::string sql_special_chars("'#\r\n\b\0",6);
  static const char sql_escape_char('%');
  static const Arc::escape_type sql_escape_type(Arc::escape_hex);

  inline static std::string sql_escape(const std::string& str) {
    return Arc::escape_chars(str, sql_special_chars, sql_escape_char, false, sql_escape_type);
  }

  inline static std::string sql_escape(const std::list<std::string>& strs) {
    return Arc::escape_chars(!strs.empty()?*strs.begin():"", sql_special_chars, sql_escape_char, false, sql_escape_type);
  }

  inline static std::string sql_escape(int num) {
    return Arc::tostring(num);
  }

  inline static std::string sql_escape(const Arc::Period& val) {
    return Arc::escape_chars((std::string)val, sql_special_chars, sql_escape_char, false, sql_escape_type);
  }

  inline static std::string sql_escape(const Arc::Time& val) {
    if(val.GetTime() == -1) return "";
    return Arc::escape_chars((std::string)val, sql_special_chars, sql_escape_char, false, sql_escape_type);
  }

  inline static std::string sql_unescape(const std::string& str) {
    return Arc::unescape_chars(str, sql_escape_char,sql_escape_type);
  }

  inline static void sql_unescape(const std::string& str, std::string& val) {
    val = Arc::unescape_chars(str, sql_escape_char,sql_escape_type);
  }

  inline static void sql_unescape(const std::string& str, std::list<std::string>& vals) {
    std::string val = Arc::unescape_chars(str, sql_escape_char,sql_escape_type);
    vals.clear();
    if(!val.empty()) vals.push_back(val);
  }

  inline static void sql_unescape(const std::string& str, int& val) {
    (void)Arc::stringto(Arc::unescape_chars(str, sql_escape_char,sql_escape_type), val);
  }

  inline static void sql_unescape(const std::string& str, Arc::Period& val) {
    val = Arc::Period(Arc::unescape_chars(str, sql_escape_char,sql_escape_type));
  }

  inline static void sql_unescape(const std::string& str, Arc::Time& val) {
    if(str.empty()) { val = Arc::Time(); return; }
    val = Arc::Time(Arc::unescape_chars(str, sql_escape_char,sql_escape_type));
  }

  int sqlite3_exec_nobusy(sqlite3* db, const char *sql, int (*callback)(void*,int,char**,char**), void *arg, char **errmsg) {
    int err;
    while((err = sqlite3_exec(db, sql, callback, arg, errmsg)) == SQLITE_BUSY) {
      // Access to database is designed in such way that it should not block for long time.
      // So it should be safe to simply wait for lock to be released without any timeout.
      struct timespec delay = { 0, 10000000 }; // 0.01s - should be enough for most cases
      (void)::nanosleep(&delay, NULL);
    };
    return err;
  }

  #define JOBS_COLUMNS_OLD \
            "id, idfromendpoint, name, statusinterface, statusurl, " \
            "managementinterfacename, managementurl, " \
            "serviceinformationinterfacename, serviceinformationurl, serviceinformationhost, " \
            "sessiondir, stageindir, stageoutdir, " \
            "descriptiondocument, localsubmissiontime, delegationid"

  #define JOBS_COLUMNS \
            "id, idfromendpoint, name, statusinterface, statusurl, " \
            "managementinterfacename, managementurl, " \
            "serviceinformationinterfacename, serviceinformationurl, serviceinformationhost, " \
            "sessiondir, stageindir, stageoutdir, " \
            "descriptiondocument, localsubmissiontime, delegationid, " \
            "type, localidfrommanager, description, state, restartstate, exitcode, computingmanagerexitcode, " \
            "error, waitingposition, userdomain, owner, localowner, " \
            "requestedtotalwallltime, requestedtotalcputime, requestedslots, requestedapplicationenvironment, " \
            "stdin, stdout, stderr, logdir, executionnode, queue, " \
            "usedtotalwallltime, usedtotalcputime, usedmainmemory, " \
            "submissiontime, computingmanagersubmissiontime, starttime, computingmanagerendtime, endtime, " \
            "workingareaerasetime, proxyexpirationtime, submissionhost, submissionclienttime, " \
            "othermessages, activityoldid"

 
  JobInformationStorageSQLite::JobDB::JobDB(const std::string& name, bool create): jobDB(NULL)
  {
    int err;
    int flags = SQLITE_OPEN_READWRITE; // it will open read-only if access is protected
    if(create) flags |= SQLITE_OPEN_CREATE;

    while((err = sqlite3_open_v2(name.c_str(), &jobDB, flags, NULL)) == SQLITE_BUSY) {
      // In case something prevents database from open right now - retry
      if(jobDB) (void)sqlite3_close(jobDB);
      jobDB = NULL;
      struct timespec delay = { 0, 10000000 }; // 0.01s - should be enough for most cases
      (void)::nanosleep(&delay, NULL);
    }
    if(err != SQLITE_OK) {
      handleError(NULL, err);
      tearDown();
      throw SQLiteException(IString("Unable to create data base (%s)", name).str(), err);
    }

    if(create) {
      err = sqlite3_exec_nobusy(jobDB, "CREATE TABLE IF NOT EXISTS jobs(" JOBS_COLUMNS ", UNIQUE(id))", NULL, NULL, NULL);   
      if(err != SQLITE_OK) {
        handleError(NULL, err);
        tearDown();
        throw SQLiteException(IString("Unable to create jobs table in data base (%s)", name).str(), err);
      }
      err = sqlite3_table_column_metadata(jobDB, NULL, "jobs", "activityoldid", NULL, NULL, NULL, NULL, NULL);
      if(err != SQLITE_OK) {
        // No latest column => recreate table
        err = sqlite3_exec_nobusy(jobDB, "CREATE TABLE IF NOT EXISTS jobs_new(" JOBS_COLUMNS ", UNIQUE(id))", NULL, NULL, NULL);   
        if(err != SQLITE_OK) {
          handleError(NULL, err);
          tearDown();
          throw SQLiteException(IString("Unable to create jobs_new table in data base (%s)", name).str(), err);
        }
        err = sqlite3_exec_nobusy(jobDB, "INSERT INTO jobs_new (" JOBS_COLUMNS_OLD ") SELECT " JOBS_COLUMNS_OLD " FROM jobs", NULL, NULL, NULL);   
        if(err != SQLITE_OK) {
          handleError(NULL, err);
          tearDown();
          throw SQLiteException(IString("Unable to transfer from jobs to jobs_new in data base (%s)", name).str(), err);
        }
        err = sqlite3_exec_nobusy(jobDB, "DROP TABLE jobs", NULL, NULL, NULL);   
        if(err != SQLITE_OK) {
          handleError(NULL, err);
          tearDown();
          throw SQLiteException(IString("Unable to transfer drop jobs in data base (%s)", name).str(), err);
        }
        err = sqlite3_exec_nobusy(jobDB, "ALTER TABLE jobs_new RENAME TO jobs", NULL, NULL, NULL);   
        if(err != SQLITE_OK) {
          handleError(NULL, err);
          tearDown();
          throw SQLiteException(IString("Unable to rename jobs table in data base (%s)", name).str(), err);
        }
      }

      err = sqlite3_exec_nobusy(jobDB,
          "CREATE INDEX IF NOT EXISTS serviceinformationhost ON jobs(serviceinformationhost)",
           NULL, NULL, NULL);   
      if(err != SQLITE_OK) {
        handleError(NULL, err);
        tearDown();
        throw SQLiteException(IString("Unable to create index for jobs table in data base (%s)", name).str(), err);
      }
    } else {
      // SQLite opens database in lazy way. But we still want to know if it is good database.
      err = sqlite3_exec_nobusy(jobDB, "PRAGMA schema_version;", NULL, NULL, NULL);
      if(err != SQLITE_OK) {
        handleError(NULL, err);
        tearDown();
        throw SQLiteException(IString("Failed checking database (%s)", name).str(), err);
      }
      JobInformationStorageSQLite::logger.msg(DEBUG, "Job database connection established successfully (%s)", name);
    }
  }

  void JobInformationStorageSQLite::JobDB::tearDown() {
    if (jobDB) {
      (void)sqlite3_close(jobDB);
      jobDB = NULL;
    }
  }

  JobInformationStorageSQLite::JobDB::~JobDB() {
    tearDown();
  }

  void JobInformationStorageSQLite::JobDB::handleError(const char* errpfx, int err) {
#ifdef HAVE_SQLITE3_ERRSTR
    std::string msg = sqlite3_errstr(err);
#else
    std::string msg = "error code "+Arc::tostring(err);
#endif
    if (errpfx) {
      JobInformationStorageSQLite::logger.msg(DEBUG, "Error from SQLite: %s: %s", errpfx, msg);
    }
    else {
      JobInformationStorageSQLite::logger.msg(DEBUG, "Error from SQLite: %s", msg);
    }
  }

  JobInformationStorageSQLite::SQLiteException::SQLiteException(const std::string& msg, int ret, bool writeLogMessage) throw() : message(msg), returnvalue(ret) {
    if (writeLogMessage) {
      JobInformationStorageSQLite::logger.msg(VERBOSE, msg);
      JobInformationStorageSQLite::logErrorMessage(ret);
    }
  }


  JobInformationStorageSQLite::JobInformationStorageSQLite(const std::string& name, unsigned nTries, unsigned tryInterval)
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
    } catch (const SQLiteException& e) {
      isValid = false;
      return;
    }
    isStorageExisting = isValid = true;
  }

  struct ListJobsCallbackArg {
    std::list<std::string>& ids;
    ListJobsCallbackArg(std::list<std::string>& ids):ids(ids) { };
  };

  static int ListJobsCallback(void* arg, int colnum, char** texts, char** names) {
    ListJobsCallbackArg& carg = *reinterpret_cast<ListJobsCallbackArg*>(arg);
    for(int n = 0; n < colnum; ++n) {
      if(names[n] && texts[n]) {
        if(strcmp(names[n], "id") == 0) {
          carg.ids.push_back(texts[n]);
        }
      }
    }
    return 0;
  }

  bool JobInformationStorageSQLite::Write(const std::list<Job>& jobs, const std::set<std::string>& prunedServices, std::list<const Job*>& newJobs) {
    if (!isValid) {
      return false;
    }
    if (jobs.empty()) return true;
    
    try {
      JobDB db(name, true);
      // Identify jobs to remove
      std::list<std::string> prunedIds;
      ListJobsCallbackArg prunedArg(prunedIds);
      for (std::set<std::string>::const_iterator itPruned = prunedServices.begin();
           itPruned != prunedServices.end(); ++itPruned) {
        std::string sqlcmd = "SELECT id FROM jobs WHERE (serviceinformationhost = '" + *itPruned + "')";
        (void)sqlite3_exec_nobusy(db.handle(), sqlcmd.c_str(), &ListJobsCallback, &prunedArg, NULL);
      }
      // Filter out jobs to be modified
      if(!jobs.empty()) {
        for(std::list<std::string>::iterator itId = prunedIds.begin(); itId != prunedIds.end();) {
          bool found = false;
          for (std::list<Job>::const_iterator it = jobs.begin(); it != jobs.end(); ++it) {
            if(it->JobID == *itId) {
              found = true;
              break;
            }
          }
          if(found) {
            itId = prunedIds.erase(itId);
          } else {
            ++itId;
          }
        }
      }
      // Remove identified jobs
      for(std::list<std::string>::iterator itId = prunedIds.begin(); itId != prunedIds.end(); ++itId) {
        std::string sqlcmd = "DELETE FROM jobs WHERE (id = '" + *itId + "')";
        (void)sqlite3_exec_nobusy(db.handle(), sqlcmd.c_str(), &ListJobsCallback, &prunedArg, NULL);
      }
      // Add new jobs
      for (std::list<Job>::const_iterator it = jobs.begin();
           it != jobs.end(); ++it) {
        std::string sqlvalues = "jobs("
          JOBS_COLUMNS
          ") VALUES ('"+
             sql_escape(it->JobID)+"', '"+
             sql_escape(it->IDFromEndpoint)+"', '"+
             sql_escape(it->Name)+"', '"+
             sql_escape(it->JobStatusInterfaceName)+"', '"+
             sql_escape(it->JobStatusURL.fullstr())+"', '"+
             sql_escape(it->JobManagementInterfaceName)+"', '"+
             sql_escape(it->JobManagementURL.fullstr())+"', '"+
             sql_escape(it->ServiceInformationInterfaceName)+"', '"+
             sql_escape(it->ServiceInformationURL.fullstr())+"', '"+
             sql_escape(it->ServiceInformationURL.Host())+"', '"+
             sql_escape(it->SessionDir.fullstr())+"', '"+
             sql_escape(it->StageInDir.fullstr())+"', '"+
             sql_escape(it->StageOutDir.fullstr())+"', '"+
             sql_escape(it->JobDescriptionDocument)+"', '"+
             sql_escape(tostring(it->LocalSubmissionTime.GetTime()))+"', '"+
             sql_escape(it->DelegationID)+"', '"+
             // attributes available after code update
             sql_escape(it->Type)+"', '"+
             sql_escape(it->LocalIDFromManager)+"', '"+
             sql_escape(it->JobDescription)+"', '"+
             sql_escape(it->State.GetGeneralState())+"', '"+
             sql_escape(it->RestartState.GetGeneralState())+"', '"+
             sql_escape(it->ExitCode)+"', '"+
             sql_escape(it->ComputingManagerExitCode)+"', '"+
             sql_escape(it->Error)+"', '"+
             sql_escape(it->WaitingPosition)+"', '"+
             sql_escape(it->UserDomain)+"', '"+
             sql_escape(it->Owner)+"', '"+
             sql_escape(it->LocalOwner)+"', '"+
             sql_escape(it->RequestedTotalWallTime)+"', '"+
             sql_escape(it->RequestedTotalCPUTime)+"', '"+
             sql_escape(it->RequestedSlots)+"', '"+
             sql_escape(it->RequestedApplicationEnvironment)+"', '"+
             sql_escape(it->StdIn)+"', '"+
             sql_escape(it->StdOut)+"', '"+
             sql_escape(it->StdErr)+"', '"+
             sql_escape(it->LogDir)+"', '"+
             sql_escape(it->ExecutionNode)+"', '"+
             sql_escape(it->Queue)+"', '"+
             sql_escape(it->UsedTotalWallTime)+"', '"+
             sql_escape(it->UsedTotalCPUTime)+"', '"+
             sql_escape(it->UsedMainMemory)+"', '"+
             sql_escape(it->SubmissionTime)+"', '"+
             sql_escape(it->ComputingManagerSubmissionTime)+"', '"+
             sql_escape(it->StartTime)+"', '"+
             sql_escape(it->ComputingManagerEndTime)+"', '"+
             sql_escape(it->EndTime)+"', '"+
             sql_escape(it->WorkingAreaEraseTime)+"', '"+
             sql_escape(it->ProxyExpirationTime)+"', '"+
             sql_escape(it->SubmissionHost)+"', '"+
             sql_escape(it->SubmissionClientName)+"', '"+
             sql_escape(it->OtherMessages)+"', '"+
             sql_escape(it->ActivityOldID)+"')";
        bool new_job = true;
        int err = sqlite3_exec_nobusy(db.handle(), ("INSERT OR IGNORE INTO " + sqlvalues).c_str(), NULL, NULL, NULL);
        if(err != SQLITE_OK) {
          logger.msg(VERBOSE, "Unable to write records into job database (%s): Id \"%s\"", name, it->JobID);
          logErrorMessage(err);
          return false;
        }
        if(sqlite3_changes(db.handle()) == 0) {
          err = sqlite3_exec_nobusy(db.handle(), ("REPLACE INTO " + sqlvalues).c_str(), NULL, NULL, NULL);
          if(err != SQLITE_OK) {
            logger.msg(VERBOSE, "Unable to write records into job database (%s): Id \"%s\"", name, it->JobID);
            logErrorMessage(err);
            return false;
          }
          new_job = false;
        }
        if(sqlite3_changes(db.handle()) != 1) {
          logger.msg(VERBOSE, "Unable to write records into job database (%s): Id \"%s\"", name, it->JobID);
          logErrorMessage(err);
          return false;
        }
        if(new_job) newJobs.push_back(&(*it));
      }
    } catch (const SQLiteException& e) {
      return false;
    }

    return true;
  }

  struct ReadJobsCallbackArg {
    std::list<Job>& jobs;
    std::list<std::string>* jobIdentifiers;
    const std::list<std::string>* endpoints;
    const std::list<std::string>* rejectEndpoints;
    std::list<std::string> jobIdentifiersMatched;
    ReadJobsCallbackArg(std::list<Job>& jobs, 
                        std::list<std::string>* jobIdentifiers,
                        const std::list<std::string>* endpoints,
                        const std::list<std::string>* rejectEndpoints):
       jobs(jobs), jobIdentifiers(jobIdentifiers), endpoints(endpoints), rejectEndpoints(rejectEndpoints) {};
  };

  static int ReadJobsCallback(void* arg, int colnum, char** texts, char** names) {
    ReadJobsCallbackArg& carg = *reinterpret_cast<ReadJobsCallbackArg*>(arg);
    carg.jobs.push_back(Job());
    bool accept = false;
    bool drop = false;
    for(int n = 0; n < colnum; ++n) {
      if(names[n] && texts[n]) {
        if(strcmp(names[n], "id") == 0) {
          carg.jobs.back().JobID = sql_unescape(texts[n]);
          if(carg.jobIdentifiers) {
            for(std::list<std::string>::iterator it = carg.jobIdentifiers->begin();
                           it != carg.jobIdentifiers->end(); ++it) {
              if(*it == carg.jobs.back().JobID) {
                accept = true;
                carg.jobIdentifiersMatched.push_back(*it);
                break;
              }
            }
          } else {
            accept = true;
          }
        } else if(strcmp(names[n], "idfromendpoint") == 0) {
          carg.jobs.back().IDFromEndpoint = sql_unescape(texts[n]);
        } else if(strcmp(names[n], "name") == 0) {
          carg.jobs.back().Name = sql_unescape(texts[n]);
          if(carg.jobIdentifiers) {
            for(std::list<std::string>::iterator it = carg.jobIdentifiers->begin();
                           it != carg.jobIdentifiers->end(); ++it) {
              if(*it == carg.jobs.back().Name) {
                accept = true;
                carg.jobIdentifiersMatched.push_back(*it);
                break;
              }
            }
          } else {
            accept = true;
          }
        } else if(strcmp(names[n], "statusinterface") == 0) {
          carg.jobs.back().JobStatusInterfaceName = sql_unescape(texts[n]);
        } else if(strcmp(names[n], "statusurl") == 0) {
          carg.jobs.back().JobStatusURL = sql_unescape(texts[n]);
        } else if(strcmp(names[n], "managementinterfacename") == 0) {
          carg.jobs.back().JobManagementInterfaceName = sql_unescape(texts[n]);
        } else if(strcmp(names[n], "managementurl") == 0) {
          carg.jobs.back().JobManagementURL = sql_unescape(texts[n]);
          if(carg.rejectEndpoints) {
            for (std::list<std::string>::const_iterator it = carg.rejectEndpoints->begin();
                     it != carg.rejectEndpoints->end(); ++it) {
              if (carg.jobs.back().JobManagementURL.StringMatches(*it)) {
                drop = true;
                break;
              }
            }
          }
          if(carg.endpoints) {
            for (std::list<std::string>::const_iterator it = carg.endpoints->begin();
                     it != carg.endpoints->end(); ++it) {
              if (carg.jobs.back().JobManagementURL.StringMatches(*it)) {
                accept = true;
                break;
              }
            }
          }
        } else if(strcmp(names[n], "serviceinformationinterfacename") == 0) {
          carg.jobs.back().ServiceInformationInterfaceName = sql_unescape(texts[n]);
        } else if(strcmp(names[n], "serviceinformationurl") == 0) {
          carg.jobs.back().ServiceInformationURL = sql_unescape(texts[n]);
        } else if(strcmp(names[n], "sessiondir") == 0) {
          carg.jobs.back().SessionDir = sql_unescape(texts[n]);
        } else if(strcmp(names[n], "stageindir") == 0) {
          carg.jobs.back().StageInDir = sql_unescape(texts[n]);
        } else if(strcmp(names[n], "stageoutdir") == 0) {
          carg.jobs.back().StageOutDir = sql_unescape(texts[n]);
        } else if(strcmp(names[n], "descriptiondocument") == 0) {
          carg.jobs.back().JobDescriptionDocument = sql_unescape(texts[n]);
        } else if(strcmp(names[n], "localsubmissiontime") == 0) {
          carg.jobs.back().LocalSubmissionTime.SetTime(stringtoi(sql_unescape(texts[n])));
        } else if(strcmp(names[n], "delegationid") == 0) {
          carg.jobs.back().DelegationID.push_back(sql_unescape(texts[n]));
        // attributs available after code update
        } else if(strcmp(names[n], "type") == 0) {
          carg.jobs.back().Type = sql_unescape(texts[n]);
        } else if(strcmp(names[n], "localidfrommanager") == 0) {
          carg.jobs.back().LocalIDFromManager = sql_unescape(texts[n]);
        } else if(strcmp(names[n], "description") == 0) {
          carg.jobs.back().JobDescription = sql_unescape(texts[n]);
        } else if(strcmp(names[n], "state") == 0) {
          carg.jobs.back().State = sql_unescape(texts[n]);
        } else if(strcmp(names[n], "restartstate") == 0) {
          carg.jobs.back().RestartState = sql_unescape(texts[n]);
        } else if(strcmp(names[n], "exitcode") == 0) {
          sql_unescape(texts[n], carg.jobs.back().ExitCode);
        } else if(strcmp(names[n], "computingmanagerexitcode") == 0) {
          sql_unescape(texts[n], carg.jobs.back().ComputingManagerExitCode);
        } else if(strcmp(names[n], "error") == 0) {
          sql_unescape(texts[n], carg.jobs.back().Error);
        } else if(strcmp(names[n], "waitingposition") == 0) {
          sql_unescape(texts[n], carg.jobs.back().WaitingPosition);
        } else if(strcmp(names[n], "userdomain") == 0) {
          carg.jobs.back().UserDomain = sql_unescape(texts[n]);
        } else if(strcmp(names[n], "owner") == 0) {
          carg.jobs.back().Owner = sql_unescape(texts[n]);
        } else if(strcmp(names[n], "localowner") == 0) {
          carg.jobs.back().LocalOwner = sql_unescape(texts[n]);
        } else if(strcmp(names[n], "requestedtotalwallltime") == 0) {
          sql_unescape(texts[n], carg.jobs.back().RequestedTotalWallTime);
        } else if(strcmp(names[n], "requestedtotalcputime") == 0) {
          sql_unescape(texts[n], carg.jobs.back().RequestedTotalCPUTime);
        } else if(strcmp(names[n], "requestedslots") == 0) {
          sql_unescape(texts[n], carg.jobs.back().RequestedSlots);
        } else if(strcmp(names[n], "requestedapplicationenvironment") == 0) {
          sql_unescape(texts[n], carg.jobs.back().RequestedApplicationEnvironment);
        } else if(strcmp(names[n], "stdin") == 0) {
          carg.jobs.back().StdIn = sql_unescape(texts[n]);
        } else if(strcmp(names[n], "stdout") == 0) {
          carg.jobs.back().StdOut = sql_unescape(texts[n]);
        } else if(strcmp(names[n], "stderr") == 0) {
          carg.jobs.back().StdErr = sql_unescape(texts[n]);
        } else if(strcmp(names[n], "logdir") == 0) {
          carg.jobs.back().LogDir = sql_unescape(texts[n]);
        } else if(strcmp(names[n], "executionnode") == 0) {
          sql_unescape(texts[n], carg.jobs.back().ExecutionNode);
        } else if(strcmp(names[n], "queue") == 0) {
          carg.jobs.back().Queue = sql_unescape(texts[n]);
        } else if(strcmp(names[n], "usedtotalwallltime") == 0) {
          carg.jobs.back().UsedTotalWallTime = sql_unescape(texts[n]);
        } else if(strcmp(names[n], "usedtotalcputime") == 0) {
          carg.jobs.back().UsedTotalCPUTime = sql_unescape(texts[n]);
        } else if(strcmp(names[n], "usedmainmemory") == 0) {
          sql_unescape(texts[n], carg.jobs.back().UsedMainMemory);
        } else if(strcmp(names[n], "submissiontime") == 0) {
          sql_unescape(texts[n], carg.jobs.back().SubmissionTime);
        } else if(strcmp(names[n], "computingmanagersubmissiontime") == 0) {
          sql_unescape(texts[n], carg.jobs.back().ComputingManagerSubmissionTime);
        } else if(strcmp(names[n], "starttime") == 0) {
          sql_unescape(texts[n], carg.jobs.back().StartTime);
        } else if(strcmp(names[n], "computingmanagerendtime") == 0) {
          sql_unescape(texts[n], carg.jobs.back().ComputingManagerEndTime);
        } else if(strcmp(names[n], "endtime") == 0) {
          sql_unescape(texts[n], carg.jobs.back().EndTime);
        } else if(strcmp(names[n], "workingareaerasetime") == 0) {
          sql_unescape(texts[n], carg.jobs.back().WorkingAreaEraseTime);
        } else if(strcmp(names[n], "proxyexpirationtime") == 0) {
          sql_unescape(texts[n], carg.jobs.back().ProxyExpirationTime);
        } else if(strcmp(names[n], "submissionhost") == 0) {
          carg.jobs.back().SubmissionHost = sql_unescape(texts[n]);
        } else if(strcmp(names[n], "submissionclienttime") == 0) {
          carg.jobs.back().SubmissionClientName = sql_unescape(texts[n]);
        } else if(strcmp(names[n], "othermessages") == 0) {
          sql_unescape(texts[n], carg.jobs.back().OtherMessages);
        } else if(strcmp(names[n], "activityoldid") == 0) {
          sql_unescape(texts[n], carg.jobs.back().ActivityOldID);
        }
      }
    }
    if(drop || !accept) {
      carg.jobs.pop_back();
    }
    return 0;
  }

  bool JobInformationStorageSQLite::ReadAll(std::list<Job>& jobs, const std::list<std::string>& rejectEndpoints) {
    if (!isValid) {
      return false;
    }
    jobs.clear();

    try {
      JobDB db(name);
      std::string sqlcmd = "SELECT * FROM jobs";
      ReadJobsCallbackArg carg(jobs, NULL, NULL, &rejectEndpoints);
      int err = sqlite3_exec_nobusy(db.handle(), sqlcmd.c_str(), &ReadJobsCallback, &carg, NULL);
      if(err != SQLITE_OK) {
        // handle error ??
        return false;
      }
    } catch (const SQLiteException& e) {
      return false;
    }
    
    return true;
  }
  
  bool JobInformationStorageSQLite::Read(std::list<Job>& jobs, std::list<std::string>& jobIdentifiers,
                                      const std::list<std::string>& endpoints,
                                      const std::list<std::string>& rejectEndpoints) {
    if (!isValid) {
      return false;
    }
    jobs.clear();
    
    try {
      JobDB db(name);
      std::string sqlcmd = "SELECT * FROM jobs";
      ReadJobsCallbackArg carg(jobs, &jobIdentifiers, &endpoints, &rejectEndpoints);
      int err = sqlite3_exec_nobusy(db.handle(), sqlcmd.c_str(), &ReadJobsCallback, &carg, NULL);
      if(err != SQLITE_OK) {
        // handle error ??
        return false;
      }
      carg.jobIdentifiersMatched.sort();
      carg.jobIdentifiersMatched.unique();
      for(std::list<std::string>::iterator itMatched = carg.jobIdentifiersMatched.begin();
                        itMatched != carg.jobIdentifiersMatched.end(); ++itMatched) {
        jobIdentifiers.remove(*itMatched);
      }
    } catch (const SQLiteException& e) {
      return false;
    }

    return true;
  }

  bool JobInformationStorageSQLite::Clean() {
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
  
  bool JobInformationStorageSQLite::Remove(const std::list<std::string>& jobids) {
    if (!isValid) {
      return false;
    }

    try {
      JobDB db(name, true);
      for (std::list<std::string>::const_iterator it = jobids.begin();
           it != jobids.end(); ++it) {
        std::string sqlcmd = "DELETE FROM jobs WHERE (id = '"+sql_escape(*it)+"')";
        int err = sqlite3_exec_nobusy(db.handle(), sqlcmd.c_str(), NULL, NULL, NULL); 
        if(err != SQLITE_OK) {
        } else if(sqlite3_changes(db.handle()) < 1) {
        }
      }
    } catch (const SQLiteException& e) {
      return false;
    }
    
    return true;
  }
  
  void JobInformationStorageSQLite::logErrorMessage(int err) {
    switch (err) {
    default:
      logger.msg(DEBUG, "Unable to determine error (%d)", err);
    }
  }

} // namespace Arc
