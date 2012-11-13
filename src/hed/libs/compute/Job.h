// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_JOB_H__
#define __ARC_JOB_H__

#include <arc/DateTime.h>
#include <arc/URL.h>
#include <arc/compute/JobState.h>
#include <string>
#include <set>

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
    std::string ID;
    std::string IDOnService;
    std::string Name;
    URL ServiceInformationURL;
    std::string ServiceInformationInterfaceName;
    URL JobStatusURL;
    std::string JobStatusInterfaceName;
    URL JobManagementURL;
    std::string JobManagementInterfaceName;

    // Attributes not part of ComputingActivity entity in GLUE2
    // These are used for central functionality in the library
    URL JobID;
    std::string InterfaceName;


    std::string Type;
    std::string IDFromEndpoint;
    std::string LocalIDFromManager;
    std::string JobDescription;
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
    //ExecutionEnvironment (condensed)
    bool VirtualMachine;
    std::string UsedCPUType;
    std::string UsedOSFamily;
    std::string UsedPlatform;


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
    void Update(XMLNode job);

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
    
    static bool CompareJobID(const Job& a, const Job& b) { return a.JobID.fullstr().compare(b.JobID.fullstr()) < 0; }
    static bool CompareSubmissionTime(const Job& a, const Job& b) { return a.SubmissionTime < b.SubmissionTime; }
    static bool CompareJobName(const Job& a, const Job& b) { return a.Name.compare(b.Name) < 0; }

    /// Read all jobs from file
    /**
     * This static method will read jobs (in XML format) from the specified
     * file, and they will be stored in the referenced list of jobs. The XML
     * element in the file representing a job should be named "Job", and have
     * the same format as accepted by the operator=(XMLNode) method.
     *
     * File locking:
     * To avoid simultaneous use (writing and reading) of the file, reading will
     * not be initiated before a lock on the file has been acquired. For this
     * purpose the FileLock class is used. nTries specifies the maximal number
     * of times the method will try to acquire a lock on the file, with an
     * interval of tryInterval micro seconds between each attempt. If a lock is
     * not acquired* this method returns false.
     *
     * The method will also return false if the content of file is not in XML
     * format. Otherwise it returns true.
     *
     * @param filename is the filename of the job list to read jobs from.
     * @param jobs is a reference to a list of Job objects, which will be filled
     *  with the jobs read from file (cleared before use).
     * @param nTries specifies the maximal number of times the method will try
     *  to acquire a lock on file to read.
     * @param tryInterval specifies the interval (in micro seconds) between each
     *  attempt to acquire a lock.
     * @return true in case of success, otherwise false.
     * @see operator=(XMLNode)
     * @see ReadJobsFromFile
     * @see WriteJobsToTruncatedFile
     * @see WriteJobsToFile
     * @see RemoveJobsFromFile
     * @see FileLock
     * @see XMLNode::ReadFromFile
     **/
    static bool ReadAllJobsFromFile(const std::string& filename, std::list<Job>& jobs, unsigned nTries = 10, unsigned tryInterval = 500000);

    /// Read specified jobs from file
    /**
     * Extract job information for jobs specified by job identifiers and/or
     * endpoints from job list file. The method read all jobs from specified
     * job list file, using the ReadAllJobsFromFile method. If the all argument
     * is false, jobs will only be put into the list of Job objects (jobs) if
     * the IDFromEndpoint or Name attributes of the Job object matches one of
     * the entries in the jobIdentifiers list argument or if the Cluster
     * attribute of the Job object matches one of the entries in
     * the endpoints list argument (if specified), using the URL::StringMatches
     * method. If the all argument is true, none of those matchings is carried
     * out, instead all jobs are put into the list of Job objects. For both
     * values of the all argument, the entries in the jobIdentifiers list will
     * be removed if corresponding to a job in the jobs list. In the end, if the
     * rejectEndpoints list is non-empty, the jobs list will be filtered by
     * removing Job objects for which the Cluster attribute
     * matches those in the rejectEndpoints list, using the URL::StringMatches
     * method. This method returns true, except when the ReadAllJobsFromFile
     * method returns false.
     *
     * @param filename is the filename of the job list to read jobs from.
     * @param jobs is a reference to a list of Job objects, which will be filled
     *  with the jobs read from file (cleared before use).
     * @param jobIdentifiers specifies the job IDs and names of jobs to be put
     *  into the jobs list. Entries in this list is removed if found among the
     *  jobs in the job list file.
     * @param all specifies whether all jobs from the jobs list should be put
     *  into the jobs list.
     * @param endpoints is a list of strings resembling endpoints for which Job
     *  objects having matching Cluster attribute should be
     *  added to the jobs list.
     * @param rejectEndpoints is a list of strings resembling endpoint for which
     *  Job objects having matching Cluster attribute should be
     *  removed from the jobs list. Overides jobIdentifiers, all and endpoints.
     * @param nTries will be passed to the ReadAllJobsFromFile method
     * @param tryInterval will be passed to the ReadAllJobsFromFile method
     * @return true in case of success, otherwise false.
     * @see ReadAllJobsFromFile
     * @see URL::StringMatches
     * @see WriteJobsToTruncatedFile
     * @see WriteJobsToFile
     * @see RemoveJobsFromFile
     **/
    static bool ReadJobsFromFile(const std::string& filename, std::list<Job>& jobs, std::list<std::string>& jobIdentifiers, bool all = false, const std::list<std::string>& endpoints = std::list<std::string>(), const std::list<std::string>& rejectEndpoints = std::list<std::string>(), unsigned nTries = 10, unsigned tryInterval = 500000);

    /// Truncate file and write jobs to it
    /**
     * This static method will write the passed list of jobs to the specified
     * file, but before writing the file will be truncated. Jobs will be written
     * in XML format as returned by the ToXML method, and each job will be
     * contained in a element named "Job". If the passed list of jobs contains
     * two identical jobs (i.e. IDFromEndpoint identical), only the latter
     * Job object is stored. File locking will be done as described
     * for the ReadAllJobsFromFile method. The method will return false if
     * writing jobs to the file fails. Otherwise it returns true.
     *
     * @param filename is the filename of the job list to write jobs to.
     * @param jobs is the list of Job objects which should be written to file.
     * @param nTries specifies the maximal number of times the method will try
     *  to acquire a lock on file to read.
     * @param tryInterval specifies the interval (in micro seconds) between each
     *  attempt to acquire a lock.
     * @return true in case of success, otherwise false.
     * @see ToXML
     * @see ReadAllJobsFromFile
     * @see WriteJobsToFile
     * @see RemoveJobsFromFile
     * @see FileLock
     * @see XMLNode::SaveToFile
     **/
    static bool WriteJobsToTruncatedFile(const std::string& filename, const std::list<Job>& jobs, unsigned nTries = 10, unsigned tryInterval = 500000);

    /// Write jobs to file
    /**
     * This method is in all respects identical to the WriteJobsToFile(const std::string&, const std::list<Job>&, std::list<const Job*>&, unsigned, unsigned)
     * method, except for the information about new jobs which is disregarded.
     *
     * @see WriteJobsToFile(const std::string&, const std::list<Job>&, std::list<const Job*>&, unsigned, unsigned)
     */
    static bool WriteJobsToFile(const std::string& filename, const std::list<Job>& jobs, unsigned nTries = 10, unsigned tryInterval = 500000);

    /// Write jobs to file
    /**
     * This method invokes
     * WriteJobsToFile(const std::string&, const std::list<Job>&, const std::set<std::string>&, std::list<const Job*>&, unsigned, unsigned)
     * with an empty set as the 3rd argument, meaning that all existing jobs
     * which are not replaced, will be kept.
     *
     * @see WriteJobsToFile(const std::string&, const std::list<Job>&, const std::set<std::string>&, std::list<const Job*>&, unsigned, unsigned)
     */
    static bool WriteJobsToFile(const std::string& filename, const std::list<Job>& jobs, std::list<const Job*>& newJobs, unsigned nTries = 10, unsigned tryInterval = 500000);

    /// Write jobs to file
    /**
     * This static method will write (append) the passed list of jobs to the
     * specified file. Jobs will be written in XML format as returned by the
     * ToXML method, and each job will be contained in a element named "Job".
     * If the passed list of jobs contains two identical jobs (i.e.
     * IDFromEndpoint identical), only the latter Job object is stored. If
     * a job in the list is identical to one in file, the one in file will be
     * replaced with the one from the list. A pointer (no memory allocation) to
     * those jobs from the list which are not in the file will be added to the
     * newJobs list, thus these pointers goes out of scope when 'jobs' list goes
     * out of scope. File locking will be done as described for the
     * ReadAllJobsFromFile method. The method will return false if writing jobs
     * to the file fails. Otherwise it returns true.
     *
     * @param filename is the filename of the job list to write jobs to.
     * @param jobs is the list of Job objects which should be written to file.
     * @param prunedServices is a set of service names whose jobs should be
     *  removed if not replaced.  This is typically the list of service names
     *  for which at least one endpoint was successfully queried.  Passing the
     *  empty set, all existing jobs are kept, even if outdated.
     * @param newJobs is a reference to a list of pointers to Job objects which
     *  are not duplicates.
     * @param nTries specifies the maximal number of times the method will try
     *  to acquire a lock on file to read.
     * @param tryInterval specifies the interval (in micro seconds) between each
     *  attempt to acquire a lock.
     * @return true in case of success, otherwise false.
     * @see ToXML
     * @see ReadAllJobsFromFile
     * @see WriteJobsToTruncatedFile
     * @see RemoveJobsFromFile
     * @see FileLock
     * @see XMLNode::SaveToFile
     **/
    static bool WriteJobsToFile(const std::string& filename, const std::list<Job>& jobs, const std::set<std::string>& prunedServices, std::list<const Job*>& newJobs, unsigned nTries = 10, unsigned tryInterval = 500000);

    /// Remove job from file
    /**
     * This static method will remove the jobs having IDFromEndpoint identical
     * to any of those in the passed list jobids. File locking will be done as
     * described for the ReadAllJobsFromFile method. The method will return
     * false if reading from or writing jobs to the file fails. Otherwise it
     * returns true.
     *
     * @param filename is the filename of the job list to write jobs to.
     * @param jobids is a list of URL objects which specifies which jobs from
     *  the file to remove.
     * @param nTries specifies the maximal number of times the method will try
     *  to acquire a lock on file to read.
     * @param tryInterval specifies the interval (in micro seconds) between each
     *  attempt to acquire a lock.
     * @return true in case of success, otherwise false.
     * @see ReadAllJobsFromFile
     * @see WriteJobsToTruncatedFile
     * @see WriteJobsToFile
     * @see FileLock
     * @see XMLNode::ReadFromFile
     * @see XMLNode::SaveToFile
     **/
    static bool RemoveJobsFromFile(const std::string& filename, const std::list<URL>& jobids, unsigned nTries = 10, unsigned tryInterval = 500000);

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
    static bool WriteJobIDToFile(const URL& jobid, const std::string& filename, unsigned nTries = 10, unsigned tryInterval = 500000);

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
    static bool WriteJobIDsToFile(const std::list<URL>& jobids, const std::string& filename, unsigned nTries = 10, unsigned tryInterval = 500000);

    static bool WriteJobIDsToFile(const std::list<Job>& jobs, const std::string& filename, unsigned nTries = 10, unsigned tryInterval = 500000);

  private:
    static bool ListFilesRecursive(const UserConfig& uc, const URL& dir, std::list<std::string>& files, const std::string& prefix);

    JobControllerPlugin* jc;

    static JobControllerPluginLoader loader;

    // Objects might be pointing to allocated memory upon termination, leave it as garbage.
    static DataHandle *data_source, *data_destination;
    
    static Logger logger;
  };

} // namespace Arc

#endif // __ARC_JOB_H__
