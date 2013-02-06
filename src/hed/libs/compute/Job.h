// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_JOB_H__
#define __ARC_JOB_H__

#include <string>
#include <set>

#include <db_cxx.h>

#include <arc/DateTime.h>
#include <arc/URL.h>
#include <arc/compute/JobState.h>

namespace Arc {

  class DataHandle;
  class JobControllerPlugin;
  class JobControllerPluginLoader;
  class JobSupervisor;
  class Logger;
  class UserConfig;
  class XMLNode;

  /// Job
  /**
   * This class describe a Grid job. The class contains public accessible
   * member attributes and methods for dealing with a Grid job. Most of the
   * member attributes contained in this class are directly linked to the
   * ComputingActivity defined in the GLUE Specification v. 2.0 (GFD-R-P.147).
   * 
   * \ingroup compute
   * \headerfile Job.h arc/compute/Job.h 
   */
  class Job {
  friend class JobSupervisor;
  public:

    /// Create a Job object
    /**
     * Default constructor. Takes no arguments.
     */
    Job();
    ~Job();

    Job(const Job& job);
    Job(XMLNode job);

    // Proposed mandatory attributes for ARC 3.0
    std::string JobID;
    std::string Name;
    URL ServiceInformationURL;
    std::string ServiceInformationInterfaceName;
    URL JobStatusURL;
    std::string JobStatusInterfaceName;
    URL JobManagementURL;
    std::string JobManagementInterfaceName;
    
    URL StageInDir;
    URL StageOutDir;
    URL SessionDir;

    std::string Type;
    std::string IDFromEndpoint;

    std::string LocalIDFromManager;
    /// Language of job description describing job
    /**
     * Equivalent to the GLUE2 ComputingActivity entity JobDescription (open
     * enumeration), which here is represented by a string.
     */
    std::string JobDescription;
    
    /// Job description document describing job
    /**
     * No GLUE2 entity equivalent. Should hold the job description document
     * which was submitted to the computing service for this job.
     */
    std::string JobDescriptionDocument;
    
    JobState State;
    JobState RestartState;
    int ExitCode;
    std::string ComputingManagerExitCode;
    std::list<std::string> Error;
    int WaitingPosition;
    std::string UserDomain;
    std::string Owner;
    std::string LocalOwner;
    Period RequestedTotalWallTime;
    Period RequestedTotalCPUTime;
    int RequestedSlots;
    std::list<std::string> RequestedApplicationEnvironment;
    std::string StdIn;
    std::string StdOut;
    std::string StdErr;
    std::string LogDir;
    std::list<std::string> ExecutionNode;
    std::string Queue;
    Period UsedTotalWallTime;
    Period UsedTotalCPUTime;
    int UsedMainMemory;
    Time LocalSubmissionTime;
    Time SubmissionTime;
    Time ComputingManagerSubmissionTime;
    Time StartTime;
    Time ComputingManagerEndTime;
    Time EndTime;
    Time WorkingAreaEraseTime;
    Time ProxyExpirationTime;
    std::string SubmissionHost;
    std::string SubmissionClientName;
    Time CreationTime;
    Period Validity;
    std::list<std::string> OtherMessages;
    //Associations
    std::list<std::string>  ActivityOldID;
    std::map<std::string, std::string> LocalInputFiles;


    enum ResourceType {
      STDIN,
      STDOUT,
      STDERR,
      STAGEINDIR,
      STAGEOUTDIR,
      SESSIONDIR,
      JOBLOG,
      JOBDESCRIPTION
    };
  
    /// Write job information to a std::ostream object
    /**
     * This method will write job information to the passed std::ostream object.
     * The longlist boolean specifies whether more (true) or less (false)
     * information should be printed.
     *
     * @param out is the std::ostream object to print the attributes to.
     * @param longlist is a boolean for switching on long listing (more
     *        details).
     **/
    void SaveToStream(std::ostream& out, bool longlist) const;

    /// Set Job attributes from a XMLNode
    /**
     * The attributes of the Job object is set to the values specified in the
     * XMLNode. The XMLNode should be a ComputingActivity type using the GLUE2
     * XML hierarchical rendering, see
     * http://forge.gridforum.org/sf/wiki/do/viewPage/projects.glue-wg/wiki/GLUE2XMLSchema
     * for more information. Note that associations are not parsed.
     *
     * @param job is a XMLNode of GLUE2 ComputingActivity type.
     * @see ToXML
     **/
    Job& operator=(XMLNode job);

    Job& operator=(const Job& job);

    int operator==(const Job& other);

    /// Set Job attributes from a XMLNode representing GLUE2 ComputingActivity
    /**
     * Because job XML representation follows GLUE2 model this method is
     * similar to operator=(XMLNode). But it only covers job attributes which
     * are part of GLUE2 computing activity. Also it treats Job object as
     * being iextended with information provided by XMLNode. Contrary operator=(XMLNode)
     * fully reinitializes Job, hence removing any associations to other objects.
     **/
    void SetFromXML(XMLNode job);

    /// Add job information to a XMLNode
    /**
     * Child nodes of GLUE ComputingActivity type containing job information of
     * this object will be added to the passed XMLNode.
     *
     * @param job is the XMLNode to add job information to in form of
     *        GLUE2 ComputingActivity type child nodes.
     * @see operator=
     **/
    void ToXML(XMLNode job) const;

    bool PrepareHandler(const UserConfig& uc);
    bool Update();
    
    bool Clean();
    
    bool Cancel();
  
    bool Resume();
  
    bool Renew();

    bool GetURLToResource(ResourceType resource, URL& url) const;
    
    bool Retrieve(const UserConfig& uc, const URL& destination, bool force) const;
    
    static bool CopyJobFile(const UserConfig& uc, const URL& src, const URL& dst);
    static bool ListFilesRecursive(const UserConfig& uc, const URL& dir, std::list<std::string>& files) { files.clear(); return ListFilesRecursive(uc, dir, files, ""); }
    
    static bool CompareJobID(const Job& a, const Job& b) { return a.JobID.compare(b.JobID) < 0; }
    static bool CompareSubmissionTime(const Job& a, const Job& b) { return a.SubmissionTime < b.SubmissionTime; }
    static bool CompareJobName(const Job& a, const Job& b) { return a.Name.compare(b.Name) < 0; }

    /// Read a list of Job IDs from a file, and append them to a list
    /**
     * This static method will read job IDs from the given file, and append the
     * strings to the string list given as parameter. File locking will be done
     * as described for the ReadAllJobsFromFile method. It returns false if the
     * file was not readable, true otherwise, even if there were no IDs in the
     * file. The lines of the file will be trimmed, and lines starting with #
     * will be ignored.
     *
     * @param filename is the filename of the jobidfile
     * @param jobids is a list of strings, to which the IDs read from the file
     *  will be appended
     * @param nTries specifies the maximal number of times the method will try
     *  to acquire a lock on file to read.
     * @param tryInterval specifies the interval (in micro seconds) between each
     *  attempt to acquire a lock.
     * @return true in case of success, otherwise false.
     **/
    static bool ReadJobIDsFromFile(const std::string& filename, std::list<std::string>& jobids, unsigned nTries = 10, unsigned tryInterval = 500000);

    /// Append a jobID to a file
    /**
     * This static method will put the ID represented by a URL object, and
     * append it to the given file. File locking will be done as described for
     * the ReadAllJobsFromFile method. It returns false if the file is not
     * writable, true otherwise.
     *
     * @param jobid is a jobID as a URL object
     * @param filename is the filename of the jobidfile, where the jobID will be
     *  appended
     * @param nTries specifies the maximal number of times the method will try
     *  to acquire a lock on file to read.
     * @param tryInterval specifies the interval (in micro seconds) between each
     *  attempt to acquire a lock.
     * @return true in case of success, otherwise false.
     **/
    static bool WriteJobIDToFile(const std::string& jobid, const std::string& filename, unsigned nTries = 10, unsigned tryInterval = 500000);

    /// Append list of URLs to a file
    /**
     * This static method will put the ID given as a string, and append it to
     * the given file. File locking will be done as described for the
     * ReadAllJobsFromFile method. It returns false if the file was not
     * writable, true otherwise.
     *
     * @param jobid is a list of URL objects to be written to file
     * @param filename is the filename of file, where the URL objects will be
     *  appended to.
     * @param nTries specifies the maximal number of times the method will try
     *  to acquire a lock on file to read.
     * @param tryInterval specifies the interval (in micro seconds) between each
     *  attempt to acquire a lock.
     * @return true in case of success, otherwise false.
     **/
    static bool WriteJobIDsToFile(const std::list<std::string>& jobids, const std::string& filename, unsigned nTries = 10, unsigned tryInterval = 500000);

    static bool WriteJobIDsToFile(const std::list<Job>& jobs, const std::string& filename, unsigned nTries = 10, unsigned tryInterval = 500000);

  private:
    static bool ListFilesRecursive(const UserConfig& uc, const URL& dir, std::list<std::string>& files, const std::string& prefix);

    JobControllerPlugin* jc;

    static JobControllerPluginLoader& getLoader();

    // Objects might be pointing to allocated memory upon termination, leave it as garbage.
    static DataHandle *data_source, *data_destination;
    
    static Logger logger;
  };

  /// Abstract class for storing job information
  /**
   * This abstract class provides an interface which can be used to store job
   * information, which can then later be used to initialise Job objects from
   * the stored information.
   * 
   * \note This class is abstract. All functionality is provided by specialised
   *  child classes.
   * 
   * \headerfile Job.h arc/compute/Job.h
   * \ingroup compute
   **/
  class JobInformationStorage {
  public:
    /// Constructor
    /**
     * Construct a JobInformationStorage object with name \c name. The name
     * could be a file name or maybe a database, that is implemention specific.
     * The \c nTries argument specifies the number times a lock on the storage
     * should be tried obtained for each method invocation. The constructor it
     * self should not acquire a lock through-out the object lifetime.
     * \c tryInterval is the waiting period in micro seconds between each
     * locking attemp.
     * 
     * @param name name of the storage.
     * @param nTries specifies the maximal number of times try
     *  to acquire a lock on file to read.
     * @param tryInterval specifies the interval (in micro seconds) between each
     *  attempt to acquire a lock.
     **/
    JobInformationStorage(const std::string& name, unsigned nTries = 10, unsigned tryInterval = 500000)
      : name(name), nTries(nTries), tryInterval(tryInterval) {}
    virtual ~JobInformationStorage() {}
    
    /// Read all jobs from storage
    /**
     * Read all jobs contained in storage, except those managed by a service at
     * an endpoint which matches any of those in the \c rejectEndpoints list
     * parameter. The read jobs are added to the list of Job objects referenced
     * by the \c jobs parameter. The algorithm used for matching should be
     * equivalent to that used in the URL::StringMatches method.
     *
     * \note This method is abstract and an implementation must be provided by
     *  specialised classes.
     * 
     * @param jobs is a reference to a list of Job objects, which will be filled
     *  with the jobs read from file (cleared before use).
     * @param rejectEndpoints is a list of strings specifying endpoints for
     *  which Job objects with JobManagementURL matching any of those endpoints
     *  will not be part of the retrieved jobs. The algorithm used for matching
     *  should be equivalent to that used in the URL::StringMatches method.
     * @return \c true is returned if all jobs contained in the storage was
     *  retrieved (except those rejected, if any), otherwise false.
     **/
    virtual bool ReadAll(std::list<Job>& jobs, const std::list<std::string>& rejectEndpoints = std::list<std::string>()) = 0;
    
    /// Read specified jobs
    /**
     * Read jobs specified by job identifiers and/or endpoints from storage.
     * Only jobs which has a JobID or a Name attribute matching any of the items
     * in the \c identifiers list parameter, and also jobs for which the
     * \c JobManagementURL attribute matches any of those endpoints specified in
     * the \c endpoints list parameter, will be added to the
     * list of Job objects reference to by the \c jobs parameter, except those
     * jobs for which the \c JobManagementURL attribute matches any of those
     * endpoints specified in the \c rejectEndpoints list parameter. Identifiers
     * specified in the \c jobIdentifiers list parameter which matches a job in
     * the storage will be removed from the referenced list. The algorithm used
     * for matching should be equivalent to that used in the URL::StringMatches
     * method.
     *
     * \note This method is abstract and an implementation must be provided by
     *  specialised classes.
     * 
     * @param jobs reference to list of Job objects which will be filled with
     *  matching jobs.
     * @param jobIdentifiers specifies the job IDs and names of jobs to be added
     *  to the job list. Entries in this list is removed if they match a job
     *  from the storage.
     * @param endpoints is a list of strings specifying endpoints for
     *  which Job objects with the JobManagementURL attribute matching any of
     *  those endpoints will added to the job list. The algorithm used for
     *  matching should be equivalent to that used in the URL::StringMatches
     *  method.
     * @param rejectEndpoints is a list of strings specifying endpoints for
     *  which Job objects with the JobManagementURL attribute matching any of
     *  those endpoints will not be part of the retrieved jobs. The algorithm
     *  used for matching should be equivalent to that used in the
     *  URL::StringMatches method.
     * @return \c false is returned in case a job failed to be read from
     *  storage, otherwise \c true is returned. This method will also return in
     *  case an identifier does not match any jobs in the storage.
     **/
    virtual bool Read(std::list<Job>& jobs, std::list<std::string>& jobIdentifiers,
                      const std::list<std::string>& endpoints = std::list<std::string>(),
                      const std::list<std::string>& rejectEndpoints = std::list<std::string>()) = 0;

    /// Write jobs
    /**
     * Add jobs to storage. If there already exist a job with a specific job ID
     * in the storage, and a job with the same job ID is tried added to the
     * storage then the existing job will be overwritten. 
     *
     * A specialised implementaion does not necessarily need to be provided. If
     * not provided Write(const std::list<Job>&, std::set<std::string>&, std::list<const Job*>&)
     * will be used.
     *
     * @param jobs is the list of Job objects which should be added to the
     *  storage.
     * @return \c true is returned if all jobs in the \c jobs list are written
     *  to to storage, otherwise \c false is returned.
     * @see Write(const std::list<Job>&, std::set<std::string>&, std::list<const Job*>&)
     */
    virtual bool Write(const std::list<Job>& jobs)  { std::list<const Job*> newJobs; std::set<std::string> prunedServices; return Write(jobs, prunedServices, newJobs); }

    /// Write jobs
    /**
     * Add jobs to storage. If there already exist a job with a specific job ID
     * in the storage, and a job with the same job ID is tried added to the
     * storage then the existing job will be overwritten. For jobs in the
     * storage with a ServiceEndpointURL attribute where the host name is equal
     * to any of the entries in the set referenced by the \c prunedServices
     * parameter, is removed from the storage, if they are not among the list of
     * jobs referenced by the \c jobs parameter. A pointer to jobs in the job
     * list (\c jobs) which does not already exist in the storage will be added
     * to the list of Job object pointers referenced by the \c newJobs
     * parameter.
     * 
     * \note This method is abstract and an implementation must be provided by
     *  specialised classes.
     * 
     * @param jobs is the list of Job objects which should be added to the
     *  storage.
     * @param prunedServices is a set of host names of services whose jobs
     *  should be removed if not replaced. This is typically the list of
     *  host names for which at least one endpoint was successfully queried.
     *  By passing an empty set, all existing jobs are kept, even if jobs are
     *  outdated.
     * @param newJobs is a reference to a list of pointers to Job objects which
     *  are not duplicates.
     * @return \c true is returned if all jobs in the \c jobs list are written
     *  to to storage, otherwise \c false is returned.
     **/
    virtual bool Write(const std::list<Job>& jobs, const std::set<std::string>& prunedServices, std::list<const Job*>& newJobs) = 0;

    /// Clean storage
    /**
     * Invoking this method causes the storage to be cleaned of any jobs it
     * holds.
     * 
     * \note This method is abstract and an implementation must be provided by
     *  specialised classes.
     * 
     * @return \c true is returned if the storage was successfully cleaned,
     *  otherwise \c false is returned.
     **/
    virtual bool Clean() = 0;

    /// Remove jobs
    /**
     * The jobs with matching job IDs (Job::JobID attribute) as specified with
     * the list of job IDs (\c jobids parameter) will be remove from the
     * storage.
     *
     * \note This method is abstract and an implementation must be provided by
     *  specialised classes.
     * 
     * @param jobids list job IDs for which matching jobs should be remove from
     *  storage. 
     * @return \c is returned if any of the matching jobs failed to be removed
     *  from the storage, otherwise \c true is returned.
     **/
    virtual bool Remove(const std::list<std::string>& jobids) = 0;
    
    /// Get name
    /**
     * @return Returns the name of the storage.
     **/
    const std::string& GetName() const { return name; }

  protected:
    const std::string name;
    unsigned nTries;
    unsigned tryInterval;
  };

  class JobInformationStorageXML : public JobInformationStorage {
  public:
    JobInformationStorageXML(const std::string& name, unsigned nTries = 10, unsigned tryInterval = 500000)
      : JobInformationStorage(name, nTries, tryInterval) {}
    virtual ~JobInformationStorageXML() {}
    
    bool ReadAll(std::list<Job>& jobs, const std::list<std::string>& rejectEndpoints = std::list<std::string>());
    bool Read(std::list<Job>& jobs, std::list<std::string>& jobIdentifiers,
                      const std::list<std::string>& endpoints = std::list<std::string>(),
                      const std::list<std::string>& rejectEndpoints = std::list<std::string>());
    using JobInformationStorage::Write;
    bool Write(const std::list<Job>& jobs, const std::set<std::string>& prunedServices, std::list<const Job*>& newJobs);
    bool Clean();
    bool Remove(const std::list<std::string>& jobids);
    
  private:
    static Logger logger;
  };

  class JobInformationStorageBDB : public JobInformationStorage {
  public:
    JobInformationStorageBDB(const std::string& name, unsigned nTries = 10, unsigned tryInterval = 500000)
      : JobInformationStorage(name, nTries, tryInterval) {}
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
  };

} // namespace Arc

#endif // __ARC_JOB_H__
