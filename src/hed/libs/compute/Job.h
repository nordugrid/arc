// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_JOB_H__
#define __ARC_JOB_H__

#include <string>
#include <set>

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
    /**
     * This member is not a part of GLUE2.
     * \since Added in 4.1.0.
     **/
    std::list<std::string> DelegationID;


    enum ResourceType {
      STDIN,
      STDOUT,
      STDERR,
      STAGEINDIR,
      STAGEOUTDIR,
      SESSIONDIR,
      LOGDIR,
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
    void SaveToStreamJSON(std::ostream& out, bool longlist) const;

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
     * @param jobids is a list of URL objects to be written to file
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

} // namespace Arc

#endif // __ARC_JOB_H__
