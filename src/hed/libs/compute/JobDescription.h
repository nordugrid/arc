#ifndef __ARC_JOBDESCRIPTION_H__
#define __ARC_JOBDESCRIPTION_H__

/** \file
 * \brief Classes related to creating JobDescription objects.
 */

#include <list>
#include <vector>
#include <string>

#include <arc/DateTime.h>
#include <arc/XMLNode.h>
#include <arc/URL.h>
#include <arc/compute/Software.h>


namespace Arc {

  /**
   * \defgroup jobdescription JobDescription related classes
   * This list of classes is used to make up the structure of the JobDescription
   * class.
   * \ingroup compute
   */

  /**
   * \mapdef jobdescription_attributes_mapping Job description attributes mapping
   * On this page the mapping of job description attributes of different
   * job description languages to those defined by the libarccompute library in
   * the \ref jobdescription "JobDescription" group is documented.
   **/


  class JobDescriptionParserPluginLoader;
  class ExecutionTarget;

  /**
   * \ingroup jobdescription
   * \headerfile JobDescription.h arc/compute/JobDescription.h
   */
  template<class T>
  class OptIn {
  public:
    OptIn<T>() : optIn(false) {}
    OptIn<T>(const T& t) : v(t), optIn(false) {}
    OptIn<T>(const T& t, bool o) : v(t), optIn(o) {}
    OptIn<T>(const OptIn<T>& ot) : v(ot.v), optIn(ot.optIn) {}

    OptIn<T>& operator=(const OptIn<T>& ot) { v = ot.v; optIn = ot.optIn; return *this; }
    OptIn<T>& operator=(const T& t) { v = t; optIn = false; return *this; }

    operator T() const { return v; }

    T v;
    bool optIn;
  };

  /**
   * \ingroup jobdescription
   * \headerfile JobDescription.h arc/compute/JobDescription.h
   */
  template<class T>
  class Range {
  public:
    Range<T>() : min(0), max(0) {}
    Range<T>(const T& t) : min(t), max(t) {}
    operator T(void) const { return max; }

    Range<T>& operator=(const Range<T>& t) { min = t.min; max = t.max; return *this; };
    Range<T>& operator=(const T& t) { max = t; return *this; };

    T min;
    T max;
  };

  /**
   * \ingroup jobdescription
   * \headerfile JobDescription.h arc/compute/JobDescription.h
   */
  template<class T>
  class ScalableTime {
  public:
    ScalableTime<T>() : benchmark("", -1.) {}
    ScalableTime<T>(const T& t) : range(t) {}

    std::pair<std::string, double> benchmark;
    Range<T> range;
  };

  /**
   * \ingroup jobdescription
   * \headerfile JobDescription.h arc/compute/JobDescription.h
   */
  template<>
  class ScalableTime<int> {
  public:
    ScalableTime<int>() : benchmark("", -1.) {}
    ScalableTime<int>(const int& t) : range(t) {}

    std::pair<std::string, double> benchmark;
    Range<int> range;

    int scaleMin(double s) const { return (int)(range.min*benchmark.second/s); }
    int scaleMax(double s) const { return (int)(range.max*benchmark.second/s); }
  };

  /// Job identification
  /**
   * This class serves to provide human readable information about a job
   * description. Some of this information might also be passed to the
   * execution service for providing information about the job created from
   * this job description. An object of this class is part of the
   * JobDescription class as the Identification public member.
   * 
   * \ingroup jobdescription
   * \headerfile JobDescription.h arc/compute/JobDescription.h
   **/
  class JobIdentificationType {
  public:
    JobIdentificationType() :
      JobName(""), Description(""), Type("") {}
    /// Name of job
    /**
     * The JobName string is used to specify a name of the job description, and
     * it will most likely also be the name given to the job when created at the
     * execution service.
     * 
     * \mapdefattr JobName Arc::JobIdentificationType
     **/
    std::string JobName;

    /// Human readable description
    /**
     * The Description string can be used to provide a human readable
     * description of e.g. the task which should be performed when processing
     * the job description.
     * 
     * \mapdefattr Description Arc::JobIdentificationType
     **/
    std::string Description;

    /// Job type
    /**
     * The Type string specifies a classification of the activity in
     * compliance with GLUE2. The possible values should follow those defined in
     * the ComputingActivityType_t enumeration of GLUE2.
     * 
     * \mapdefattr JobIdentificationType::Type Arc
     **/
    std::string Type;

    /// Annotation
    /**
     * The Annotation list is used for human readable comments, tags for free
     * grouping or identifying different activities.
     * 
     * \mapdefattr Annotation Arc::JobIdentificationType
     **/
    std::list<std::string> Annotation;

    /// ID of old activity
    /**
     * The ActivityOldID object is used to store a list of IDs corresponding to
     * activities which were performed from this job description. This
     * information is not intended to used by the execution service, but rather
     * used for keeping track of activities, e.g. when doing a job resubmission
     * the old activity ID is appended to this list.
     * 
     * \mapdefattr ActivityOldID Arc::JobIdentificationType
     **/
    std::list<std::string> ActivityOldID;
  };

  /// Executable
  /**
   * The ExecutableType class is used to specify path to an executable,
   * arguments to pass to it when invoked and the exit code for successful
   * execution.
   *
   * \note The Name string member has been renamed to Path.
   * 
   * \ingroup jobdescription
   * \headerfile JobDescription.h arc/compute/JobDescription.h
   **/
  class ExecutableType {
  public:
    ExecutableType() : Path(""), SuccessExitCode(false, 0) {}

    /// Path to executable
    /**
     * The Path string should specify the path to an executable. Note that some
     * implementations might only accept a relative path, while others might
     * also accept a absolute one.
     * 
     * \mapdefattr ExecutableType::Path Arc
     **/
    std::string Path;

    /// List of arguments to executable
    /**
     * The Argument list is used to specify arguments which should be passed to
     * the executable upon invocation.
     * 
     * \mapdefattr ExecutableType::Argument Arc
     **/
    std::list<std::string> Argument;

    /// Exit code at successful execution
    /**
     * The SuccessExitCode pair is used to specify the exit code returned by the
     * executable in case of successful execution. For some scenarios the exit
     * code returned by the executable should be ignored, which is specified by
     * setting the first member of this object to false. If the exit code should
     * be used for validation at the execution service, the first member of pair
     * must be set to true, while the second member should be the exit code returned
     * at successful execution.
     * 
     * \mapdefattr ExecutableType::SuccessExitCode Arc
     **/
    std::pair<bool, int> SuccessExitCode;
  };

  /// Remote logging
  /**
   * This class is used to specify a service which should be used to report
   * logging information to, such as job resource usage.
   * 
   * \ingroup jobdescription
   * \headerfile JobDescription.h arc/compute/JobDescription.h
   **/
  class RemoteLoggingType {
  public:
    RemoteLoggingType() : optional(false) {}

    /// Type of logging service
    /**
     * The ServiceType string specifies the type of logging service. Some
     * examples are "SGAS" (http://www.sgas.se) and "APEL"
     * (https://wiki.egi.eu/wiki/APEL), however please refer to the particular
     * execution service for a list of supported logging service types.
     * 
     * \mapdefattr RemoteLoggingType::ServiceType Arc
     **/
    std::string ServiceType;

    /// URL of logging service
    /**
     * The Location URL specifies the URL of the service which job logging
     * information should be sent to.
     * 
     * \mapdefattr RemoteLoggingType::Location Arc
     **/
    URL Location;

    /// Requirement satisfaction switch
    /**
     * The optional boolean specifies whether the requirement specified in the
     * particular object is mandatory for job execution, or whether it be
     * ignored.
     * 
     * \mapdefattr RemoteLoggingType::optional Arc
     **/
     bool optional;
  };

  /**
   * \ingroup jobdescription
   * \headerfile JobDescription.h arc/compute/JobDescription.h
   */
  class NotificationType {
  public:
    NotificationType() {}
    
    /**
     * \mapdefattr NotificationType::Email Arc
     **/
    std::string Email;
    
    /**
     * \mapdefattr NotificationType::States Arc
     **/
    std::list<std::string> States;
  };

  /**
   * \ingroup jobdescription
   * \headerfile JobDescription.h arc/compute/JobDescription.h
   */
  class ApplicationType {
  public:
    ApplicationType() :
      Rerun(-1),
      ExpirationTime(-1),
      ProcessingStartTime(-1),
      Priority (-1),
      DryRun(false)
      {}
    ApplicationType& operator=(const ApplicationType&);
    /// Main executable to be run
    /**
     * The Executable object specifies the main executable which should be run
     * by the job created by the job description enclosing this object. Note
     * that in some job description languages specifying a main executable is
     * not essential.
     **/
    ExecutableType Executable;

    /// Standard input
    /**
     * The Input string specifies the relative path to the job session directory
     * of the file to be used for standard input for the job.
     * 
     * \mapdefattr Input Arc::ApplicationType
     **/
    std::string Input;

    /// Standard output
    /**
     * The Output string specifies the relative path to the job session
     * directory of the file which standard output of the job should be written
     * to.
     * 
     * \mapdefattr Output Arc::ApplicationType
     **/
    std::string Output;

    /// Standard error
    /**
     * The Error string specifies the relative path to the job session directory
     * of the file which standard error of the job should be written to.
     * 
     * \mapdefattr Error Arc::ApplicationType
     **/
    std::string Error;

    /**
     * \mapdefattr Environment Arc::ApplicationType
     **/
    std::list< std::pair<std::string, std::string> > Environment;

    /// Executables to be run before the main executable
    /**
     * The PreExecutable object specifies a number of executables which should
     * be executed before invoking the main application, where the main
     * application is either the main executable (Executable) or the specified
     * run time environment (RunTimeEnvironment in the ResourcesType class).
     * 
     * \mapdefattr PreExecutable Arc::ApplicationType
     **/
    std::list<ExecutableType> PreExecutable;

    /// Executables to be run after the main executable
    /**
     * The PostExecutable object specifies a number of executables which should
     * be executed after invoking the main application, where the main
     * application is either the main executable (Executable) or the specified
     * run time environment (RunTimeEnvironment in the ResourcesType class).
     * 
     * \mapdefattr PostExecutable Arc::ApplicationType
     **/
    std::list<ExecutableType> PostExecutable;

    /// Name of logging directory
    /**
     * The LogDir string specifies the name of the logging directory at the
     * execution service which should be used to access log files for the job.
     * 
     * \mapdefattr LogDir Arc::ApplicationType
     **/
    std::string LogDir;

    /// Remote logging services
    /**
     * The RemoteLogging list specifies the services to use for logging job
     * information. See the RemoteLoggingType class for more details.
     **/
    std::list<RemoteLoggingType> RemoteLogging;

    /**
     * \mapdefattr Rerun Arc::ApplicationType
     **/
    int Rerun;
    
    /**
     * \mapdefattr ExpirationTime Arc::ApplicationType
     **/
    Time ExpirationTime;
    
    /**
     * \mapdefattr ProcessingStartTime Arc::ApplicationType
     **/
    Time ProcessingStartTime;
    
    /**
     * \mapdefattr Priority Arc::ApplicationType
     **/
    int Priority;
    
    std::list<NotificationType> Notification;
    
    /**
     * \mapdefattr CredentialService Arc::ApplicationType
     **/
    std::list<URL> CredentialService;
    
    /**
     * \mapdefattr AccessControl Arc::ApplicationType
     **/
    XMLNode AccessControl;
    
    /**
     * \mapdefattr DryRun Arc::ApplicationType
     **/
    bool DryRun;
  };

  /**
   * \ingroup jobdescription
   * \headerfile JobDescription.h arc/compute/JobDescription.h
   */
  class SlotRequirementType {
  public:
    SlotRequirementType() :
      NumberOfSlots(-1),
      SlotsPerHost(-1),
      ExclusiveExecution(EE_DEFAULT) {}
    /**
     * \mapdefattr NumberOfSlots Arc::SlotRequirementType
     **/
    int NumberOfSlots; // Range?
    
    /**
     * \mapdefattr SlotsPerHost Arc::SlotRequirementType
     **/
    int SlotsPerHost; // Range?
    
    /**
     * \mapdefattr ExclusiveExecution Arc::SlotRequirementType
     **/
    enum ExclusiveExecutionType {
      EE_DEFAULT,
      EE_TRUE,
      EE_FALSE
    } ExclusiveExecution;
  };

  /**
   * \ingroup jobdescription
   * \headerfile JobDescription.h arc/compute/JobDescription.h
   */
  class DiskSpaceRequirementType {
  public:
    DiskSpaceRequirementType() :
      DiskSpace(-1),
      CacheDiskSpace(-1),
      SessionDiskSpace(-1) {}
    /** Specifies the required size of disk space which must be available to
     * the job in mega-bytes (MB). A negative value undefines this attribute
     * 
     * \mapdefattr DiskSpace Arc::DiskSpaceRequirementType
     **/
    Range<int> DiskSpace;
    /** Specifies the required size of cache which must be available
     * to the job in mega-bytes (MB). A negative value undefines this
     * attribute
     * 
     * \mapdefattr CacheDiskSpace Arc::DiskSpaceRequirementType
     */
    int CacheDiskSpace;
    /** Specifies the required size of job session disk space which must be
     * available to the job in mega-byte (MB). A negative value undefines
     * this attribute.
     * 
     * \mapdefattr SessionDiskSpace Arc::DiskSpaceRequirementType
     */
    int SessionDiskSpace;
  };

  /**
   * \ingroup jobdescription
   * \headerfile JobDescription.h arc/compute/JobDescription.h
   */
  enum SessionDirectoryAccessMode {
    SDAM_NONE = 0,
    SDAM_RO = 1,
    SDAM_RW = 2
  };

  /**
   * \ingroup jobdescription
   * \headerfile JobDescription.h arc/compute/JobDescription.h
   */
  enum NodeAccessType {
    NAT_NONE = 0,
    NAT_INBOUND = 1,
    NAT_OUTBOUND = 2,
    NAT_INOUTBOUND = 3
  };

  /**
   * \ingroup jobdescription
   * \headerfile JobDescription.h arc/compute/JobDescription.h
   */
  class ParallelEnvironmentType {
  public:
    ParallelEnvironmentType() :
      ProcessesPerSlot(-1),
      ThreadsPerProcess(-1) {}
      
    /**
     * \mapdefattr ParallelEnvironmentType::Type Arc
     **/
    std::string Type;
    
    /**
     * \mapdefattr ParallelEnvironmentType::Version Arc
     **/
    std::string Version;
    
    /**
     * \mapdefattr ParallelEnvironmentType::ProcessesPerSlot Arc
     **/
    int ProcessesPerSlot; // Range?
    
    /**
     * \mapdefattr ParallelEnvironmentType::ThreadsPerProcess Arc
     **/
    int ThreadsPerProcess; // Range?
    
    /**
     * \mapdefattr ParallelEnvironmentType::Options Arc
     **/
    std::multimap<std::string, std::string> Options;
  };

  /// Resource requirements structure
  /**
   * 
   * \ingroup jobdescription
   * \headerfile JobDescription.h arc/compute/JobDescription.h
   */
  class ResourcesType {
  public:
    ResourcesType() :
      IndividualPhysicalMemory(-1),
      IndividualVirtualMemory(-1),
      SessionLifeTime(-1),
      SessionDirectoryAccess(SDAM_NONE),
      IndividualCPUTime(-1),
      TotalCPUTime(-1),
      IndividualWallTime(-1),
      TotalWallTime(IndividualWallTime),
      NodeAccess(NAT_NONE) {}
    ResourcesType& operator=(const ResourcesType&);

    /// Specifies operating system which apllication should be executed at
    /**
     * \mapdefattr OperatingSystem Arc::ResourcesType
     **/
    SoftwareRequirement OperatingSystem;
    
    /**
     * \mapdefattr Platform Arc::ResourcesType
     **/
    std::string Platform;
    
    /**
     * \mapdefattr NetworkInfo Arc::ResourcesType
     **/
    std::string NetworkInfo;
    
    /// Specifies amount of requested RAM in MB
    /**
     * \mapdefattr IndividualPhysicalMemory Arc::ResourcesType
     **/
    Range<int> IndividualPhysicalMemory;
    
    /// Specifies amount of requested virtual memory in MB
    /**
     * \mapdefattr IndividualVirtualMemory Arc::ResourcesType
     **/
    Range<int> IndividualVirtualMemory;
    
    DiskSpaceRequirementType DiskSpaceRequirement;
    
    /**
     * \mapdefattr SessionLifeTime Arc::ResourcesType
     **/
    Period SessionLifeTime;
    
    /**
     * \mapdefattr SessionDirectoryAccess Arc::ResourcesType
     **/
    SessionDirectoryAccessMode SessionDirectoryAccess;
    
    /**
     * \mapdefattr IndividualCPUTime Arc::ResourcesType
     **/
    ScalableTime<int> IndividualCPUTime;
    
    /**
     * \mapdefattr TotalCPUTime Arc::ResourcesType
     **/
    ScalableTime<int> TotalCPUTime;
    
    /**
     * \mapdefattr IndividualWallTime Arc::ResourcesType
     **/
    ScalableTime<int> IndividualWallTime;
    
    ScalableTime<int>& TotalWallTime;
    
    /**
     * \mapdefattr NodeAccess Arc::ResourcesType
     **/
    NodeAccessType NodeAccess;
    
    /// CE Middleware
    /**
     * Specifies the middleware which the CE should use.
     * 
     * \mapdefattr CEType Arc::ResourcesType
     **/
    SoftwareRequirement CEType;
    
    SlotRequirementType SlotRequirement;
    ParallelEnvironmentType ParallelEnvironment;
    
    /**
     * \mapdefattr Coprocessor Arc::ResourcesType
     **/
    OptIn<std::string> Coprocessor;
    
    /// Name of queue to use
    /**
     * \mapdefattr QueueName Arc::ResourcesType
     **/
    std::string QueueName;
    
    /// Runtime environment
    /**
     * Specifies which runtime environments should be available to the
     * application.
     * 
     * \mapdefattr RunTimeEnvironment Arc::ResourcesType
     **/
    SoftwareRequirement RunTimeEnvironment;
  };

  /**
   * \ingroup jobdescription
   * \headerfile JobDescription.h arc/compute/JobDescription.h
   */
  class SourceType: public URL {
  public:
    SourceType() {};
    SourceType(const URL& u):URL(u) {};
    SourceType(const std::string& s):URL(s) {};
    SourceType& operator=(const URL& u) { URL::operator=(u); return *this; };
    SourceType& operator=(const std::string& s) { URL::operator=(s); return *this; };
    std::string DelegationID;
  };

  /// Represent an output file destination 
  /**
   * \ingroup jobdescription
   * \headerfile JobDescription.h arc/compute/JobDescription.h
   */
  class TargetType: public URL {
  public:
    /// Default constructor
    /**
     * Creation flag is set to CFE_DEFAULT, UserIfFailure is set to false,
     * UserIfCancel is set to false, UserIfSuccess is set to true and
     * DelegationID is empty. Default URL constructor is used.
     **/
    TargetType() :
      CreationFlag(CFE_DEFAULT),
      UseIfFailure(false),
      UseIfCancel(false),
      UseIfSuccess(true) {};
    /// Constructor destination from URL
    /**
     * Uses same defaults as TargetType(). Passes argument to URL constructor.
     **/
    TargetType(const URL& u) :
      URL(u),
      CreationFlag(CFE_DEFAULT),
      UseIfFailure(false),
      UseIfCancel(false),
      UseIfSuccess(true) {};
    /// Constructor destination from string
    /**
     * Uses same defaults as TargetType(). Passes argument to URL constructor.
     **/
    TargetType(const std::string& s) :
      URL(s),
      CreationFlag(CFE_DEFAULT),
      UseIfFailure(false),
      UseIfCancel(false),
      UseIfSuccess(true) {};
    
    /// Delegation ID to use
    /**
     * Specifies the delegation ID to use when accessing this destination.
     **/
    std::string DelegationID;
    
    enum CreationFlagEnumeration {
      CFE_DEFAULT, /**< Default action should be used (default w.r.t. the service).*/
      CFE_OVERWRITE, /**< Overwrite an existing file. */
      CFE_APPEND, /**< Append file to an possible existing file. */
      CFE_DONTOVERWRITE /**< Don't overwrite an existing file. */
    };
    
    /// Output file creation flag
    /**
     * Specifies what action should be taken when creating output file at this
     * destination.
     **/
    CreationFlagEnumeration CreationFlag;
    
    /// Action in case job failed
    /**
     * Specifies whether this destination should used in case job failed
     * (JobState::FAILED).
     **/
    bool UseIfFailure;
    
    /// Action in case job was cancelled
    /**
     * Specifies whether this destination should be used in case job was
     * cancelled (JobState::KILLED).
     **/
    bool UseIfCancel;

    /// Action in case job succeeded
    /**
     * Specifies whether this destination should be used in case job succeeded
     * (JobState::FINISHED).
     **/
    bool UseIfSuccess;
  };

  /**
   * \ingroup jobdescription
   * \headerfile JobDescription.h arc/compute/JobDescription.h
   */
  class InputFileType {
  public:
    InputFileType() : Name(""), IsExecutable(false), FileSize(-1) {};
    std::string Name;
    /**
     * Testing
     * 
     * \mapdefattr InputFileType::IsExecutable Arc
     **/
    bool IsExecutable;
    
    long FileSize;
    /// CRC32 checksum of file
    /**
     * The Checksum attribute specifies the textural representation of CRC32
     * checksum of file in base 10.
     **/
    std::string Checksum;
    std::list<SourceType> Sources;
  };

  /// An output file
  /**
   * \ingroup jobdescription
   * \headerfile JobDescription.h arc/compute/JobDescription.h
   */
  class OutputFileType {
  public:
    OutputFileType() : Name("") {};
    /// Name of output file
    std::string Name;
    /// List of destinations for which the output file should be copied
    std::list<TargetType> Targets;
  };

  /// Simple structure for in- and output files.
  /**
   * \ingroup jobdescription
   * \headerfile JobDescription.h arc/compute/JobDescription.h
   */
  class DataStagingType {
  public:
    DataStagingType() {};
    
    /// List of inputfiles
    std::list<InputFileType> InputFiles;
    /// List of outputfiles
    std::list<OutputFileType> OutputFiles;
    /// Delegation for all data staging operations
    std::string DelegationID;
  };


  /// Job description parsing or assembly result
  /**
   * This structure holds boolean value and a string. The boolean indicates
   * whether parsing or assembling a job description was successful, while the
   * string gives an explanation in human readable text.
   * 
   * \ingroup compute
   * \ingroup jobdescription
   * \headerfile JobDescription.h arc/compute/JobDescription.h
   */
  class JobDescriptionResult {
  public:
    JobDescriptionResult(bool r):res(r) { };
    JobDescriptionResult(bool r, const std::string& s):res(r),desc(s) { };
    /// Get result as bool
    operator bool(void) { return res; };
    /// Get negated result
    bool operator!(void) { return !res; };
    /// Get explanation of result as string
    const std::string& str(void) { return desc; };
  private:
    bool res;
    std::string desc;
  };

  /**
   * The JobDescription class is the internal representation of a job description
   * in the ARC-lib. It is structured into a number of other classes/objects which
   * should strictly follow the description given in the job description document
   * <http://svn.nordugrid.org/trac/nordugrid/browser/arc1/trunk/doc/tech_doc/client/job_description.odt>.
   *
   * The class consist of a parsing method JobDescription::Parse which tries to
   * parse the passed source using a number of different parsers. The parser
   * method is complemented by the JobDescription::UnParse method, a method to
   * generate a job description document in one of the supported formats.
   * Additionally the internal representation is contained in public members which
   * makes it directly accessible and modifiable from outside the scope of the
   * class.
   * 
   * \ingroup compute
   * \ingroup jobdescription
   * \headerfile JobDescription.h arc/compute/JobDescription.h
   */
  class JobDescription {
  public:
    friend class JobDescriptionParserPlugin;
    JobDescription() : alternatives(), current(alternatives.begin()), X509Delegation(true), TokenDelegation(false) {};

    JobDescription(const JobDescription& j, bool withAlternatives = true);

    // Language wrapper constructor
    JobDescription(const long int& ptraddr);

    ~JobDescription() {}

    /// Testing
    JobDescription& operator=(const JobDescription& j);

    /// Add alternative job description
    /**
     * \param[in] j JobDescription object to add as an alternative job
     *  description to this.
     **/
    void AddAlternative(const JobDescription& j);
    /// Has alternative job descriptions
    /**
     * \return true if this job description has any alternative job descriptions
     **/
    bool HasAlternatives() const { return !alternatives.empty(); }
    /// Get list of alternative job descriptions
    /**
     * \return A const reference to the list of alternative job descriptions is
     *  returned.
     **/
    const std::list<JobDescription>& GetAlternatives() const { return alternatives; }
    /// Get list of alternative job descriptions (modifiable)
    /**
     * \return A reference to the list of alternative job descriptions is
     *  returned.
     **/
    std::list<JobDescription>& GetAlternatives() { return alternatives; }
    /// Get a copy of the list of alternative job descriptions
    /**
     * \return A copy of the list of alternative job descriptions is returned.
     **/
    std::list<JobDescription> GetAlternativesCopy() const { return alternatives; }
    /// Fill next alternative into this object
    /**
     * The next alternative job description is filled into this object.
     * 
     * \return If there is a next alternative then it is filled into this object
     *  and true is returned. If there is no next false is returned.
     **/
    bool UseAlternative();
    void UseOriginal();

    void RemoveAlternatives();

    /// Parse string into JobDescription objects
    /**
     * The passed string will be tried parsed into the list of JobDescription
     * objects. The available specialized JobDesciptionParser classes will be
     * tried one by one, parsing the string, and if one succeeds the list of
     * JobDescription objects is filled with the parsed contents and true is
     * returned, otherwise false is returned. If no language specified, each
     * JobDescriptionParserPlugin will try all its supported languages. On the other
     * hand if a language is specified, only the JobDescriptionParserPlugin supporting
     * that language will be tried. A dialect can also be specified, which only
     * has an effect on the parsing if the JobDescriptionParserPlugin supports that
     * dialect.
     *
     * @param source
     * @param jobdescs
     * @param language
     * @param dialect
     * @return true if the passed string can be parsed successfully by any of
     *   the available parsers.
     **/
    static JobDescriptionResult Parse(const std::string& source, std::list<JobDescription>& jobdescs, const std::string& language = "", const std::string& dialect = "");

    static JobDescriptionResult ParseFromFile(const std::string& filename, std::list<JobDescription>& jobdescs, const std::string& language = "", const std::string& dialect = "");

    /// Output contents in the specified language
    /**
     *
     * @param product
     * @param language
     * @param dialect
     * @return
     **/
    JobDescriptionResult UnParse(std::string& product, std::string language, const std::string& dialect = "") const;

    /// Get input source language
    /**
     * If this object was created by a JobDescriptionParserPlugin, then this method
     * returns a string which indicates the job description language of the
     * parsed source. If not created by a JobDescripionParser the string
     * returned is empty.
     *
     * @return const std::string& source langauge of parsed input source.
     **/
    const std::string& GetSourceLanguage() const { return sourceLanguage; }

    /// Print job description to a std::ostream object.
    /**
     * The job description will be written to the passed std::ostream object
     * out in the format indicated by the format parameter. The format parameter
     * should specify the format of one of the job description languages
     * supported by the library. Or by specifying the special "user" or
     * "userlong" format the job description will be written as a
     * attribute/value pair list with respectively less or more attributes.
     *
     * The mote
     *
     * @return true if writing the job description to the out object succeeds,
     *              otherwise false.
     * @param out a std::ostream reference specifying the ostream to write the
     *            job description to.
     * @param format specifies the format the job description should written in.
     */
    JobDescriptionResult SaveToStream(std::ostream& out, const std::string& format) const;

    /// Prepare for submission to target
    /**
     * The Prepare method, is used to check and adapt the JobDescription object
     * to the passed ExecutionTarget object before submitting the job
     * description to the target. This method is normally called by SubmitterPlugin
     * plugin classes, before submitting the job description.
     * First the method checks the DataStaging.InputFiles list, for identical
     * file names, and non-existent local input files. If any of such files are
     * found, the method returns false. Then if the Application.Executable and
     * Application.Input objects are specified as local input files, and they
     * are not among the files in the DataStaging.InputFiles list a existence
     * check will be done and if not found, false will be returned, otherwise
     * they will be added to the list. Likewise if the Application.Output,
     * Application.Error and Application.LogDir attributes have been specified,
     * and is not among the files in the DataStaging.OutputFiles list, they will
     * be added to this list.
     * After the file check, the Resources.RunTimeEnvironment, Resources.CEType
     * and Resources.OperatingSystem SoftwareRequirement objects are
     * respectively resolved against the
     * ExecutionTarget::ApplicationEnvironments, ExecutionTarget::Implementation
     * and ExecutionTarget::OperatingSystem Software objects using the
     * SoftwareRequirement::selectSoftware method. If that method returns false
     * i.e. unable to resolve the requirements false will be returned.
     * After resolving software requirements, the value of the
     * Resources.QueueName attribute will be set to that of the
     * ExecutionTarget::ComputingShareName attribute, and then true is returned.
     *
     * @param et ExecutionTarget object which to resolve software requirements
     *  against, and to pick up queue name from.
     * @return false is returned is file checks fails, or if unable to resolve
     *  software requirements.
     */
    bool Prepare(const ExecutionTarget& et) { return Prepare(&et); }

    bool Prepare() { return Prepare(NULL); }

    static bool GetTestJob(int testid, JobDescription& jobdescription);

    /// Structure for identification
    /**
     * This member object stores information which can be used to identify the
     * job description.
     **/
    JobIdentificationType Identification;
    
    /// Structure for apllication options
    /**
     * All options relating to the application is stored in this structure.
     **/
    ApplicationType Application;
    
    /// Structure for resource requirements
    /**
     * This structure specifies requirements which should be satisfied before
     * application can be started.
     **/
    ResourcesType Resources;
    
    /// Structure for data staging
    /**
     * Input files requirements, and destinations for output files can be
     * specified using this structure.
     **/
    DataStagingType DataStaging;

    /// Holds attributes not fitting into this class
    /**
     * This member is used by JobDescriptionParserPlugin classes to store
     * attribute/value pairs not fitting into attributes stored in this class.
     * The form of the attribute (the key in the map) should be as follows:
       \verbatim
       <language>;<attribute-name>
       \endverbatim
     * E.g.: "nordugrid:xrsl;hostname".
     * 
     * \mapdefattr OtherAttributes Arc::JobDescription
     **/
    std::map<std::string, std::string> OtherAttributes;

    /// Special purpose attribute. If set to true delegation should be performed for job. Default is true.
    bool X509Delegation;
    /// Special purpose attribute. If set to true delegation should be performed for job. Default is false.
    bool TokenDelegation;

  private:
    bool Prepare(const ExecutionTarget* et);
  
    void Set(const JobDescription& j);

    std::string sourceLanguage;

    std::list<JobDescription> alternatives;
    std::list<JobDescription>::iterator current;

    static Glib::Mutex jdpl_lock;
    static JobDescriptionParserPluginLoader *jdpl;

    static Logger logger;
  };

} // namespace Arc

#endif // __ARC_JOBDESCRIPTION_H__
