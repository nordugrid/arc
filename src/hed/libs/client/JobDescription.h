#ifndef __ARC_JOBDESCRIPTION_H__
#define __ARC_JOBDESCRIPTION_H__

#include <list>
#include <vector>
#include <string>

#include <arc/DateTime.h>
#include <arc/XMLNode.h>
#include <arc/URL.h>
#include <arc/client/Software.h>

/** JobDescription
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
 */


namespace Arc {

  class JobDescriptionParserLoader;

  template<class T>
  class Range {
  public:
    Range<T>() {}
    Range<T>(const T& t) : min(t), max(t) {}
    operator T(void) const { return max; }

    Range<T>& operator=(const Range<T>& t) { min = t.min; max = t.max; return *this; };
    Range<T>& operator=(const T& t) { max = t; return *this; };

    T min;
    T max;
  };

  template<class T>
  class ScalableTime {
  public:
    ScalableTime<T>() : benchmark("", -1.) {}
    ScalableTime<T>(const T& t) : range(t) {}

    std::pair<std::string, double> benchmark;
    Range<T> range;
  };

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

  enum ComputingActivityType {
    SINGLE = 0,
    COLLECTIONELEMENT = 1,
    PARALLELELEMENT = 2,
    WORKFLOWNODE = 3
  };

  class JobIdentificationType {
  public:
    JobIdentificationType() :
      JobType(SINGLE) {}
    std::string JobName;
    std::string Description;
    std::string JobVOName;
    ComputingActivityType JobType;
    std::list<std::string> UserTag;
    std::list<std::string> ActivityOldId;
  };

  class ExecutableType {
  public:
    ExecutableType() : Name(), Argument() {}
    std::string Name;
    std::list<std::string> Argument;
  };

  class NotificationType {
  public:
    NotificationType() {}
    std::string Email;
    std::list<std::string> States;
  };

  class ApplicationType {
  public:
    ApplicationType() :
      Join(false),
      Rerun(-1),
      ExpiryTime(-1),
      ProcessingStartTime(-1) {}
    ExecutableType Executable;
    std::string Input;
    std::string Output;
    std::string Error;
    bool Join;
    std::list< std::pair<std::string, std::string> > Environment;
    ExecutableType Prologue;
    ExecutableType Epilogue;
    std::string LogDir;
    std::list<URL> RemoteLogging;
    int Rerun;
    Time ExpiryTime;
    Time ProcessingStartTime;
    std::list<NotificationType> Notification;
    std::list<URL> CredentialService;
    XMLNode AccessControl;
  };

  class ResourceSlotType {
  public:
    ResourceSlotType() :
      NumberOfSlots(-1),
      ProcessPerHost(-1),
      ThreadsPerProcesses(-1) {}
    Range<int> NumberOfSlots;
    Range<int> ProcessPerHost;
    Range<int> ThreadsPerProcesses;
    std::string SPMDVariation;
  };

  class DiskSpaceRequirementType {
  public:
    DiskSpaceRequirementType() :
      DiskSpace(-1),
      CacheDiskSpace(-1),
      SessionDiskSpace(-1) {}
    Range<int64_t> DiskSpace;
    int64_t CacheDiskSpace;
    int64_t SessionDiskSpace;
  };

  class ResourceTargetType {
  public:
    ResourceTargetType() :
    UseQueue(true) {}
    URL EndPointURL;
    std::string QueueName;
    bool UseQueue;
  };

  enum SessionDirectoryAccessMode {
    SDAM_NONE = 0,
    SDAM_RO = 1,
    SDAM_RW = 2
  };

  enum NodeAccessType {
    NAT_NONE = 0,
    NAT_INBOUND = 1,
    NAT_OUTBOUND = 2,
    NAT_INOUTBOUND = 3
  };

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
      TotalWallTime(-1),
      NodeAccess(NAT_NONE) {}
    SoftwareRequirement OperatingSystem;
    std::string Platform;
    std::string NetworkInfo;
    Range<int64_t> IndividualPhysicalMemory;
    Range<int64_t> IndividualVirtualMemory;
    DiskSpaceRequirementType DiskSpaceRequirement;
    Period SessionLifeTime;
    SessionDirectoryAccessMode SessionDirectoryAccess;
    ScalableTime<int> IndividualCPUTime;
    ScalableTime<int> TotalCPUTime;
    ScalableTime<int> IndividualWallTime;
    ScalableTime<int> TotalWallTime;
    NodeAccessType NodeAccess;
    SoftwareRequirement CEType;
    ResourceSlotType SlotRequirement;
    std::list<ResourceTargetType> CandidateTarget;
    SoftwareRequirement RunTimeEnvironment;
  };

  class DataSourceType {
  public:
    DataSourceType() :
      Threads(-1) {}
    URL URI;
    int Threads;
  };

  class DataTargetType {
  public:
    DataTargetType() :
      Threads(-1),
      Mandatory(false),
      NeededReplica(-1) {}
    URL URI;
    int Threads;
    bool Mandatory;
    int NeededReplica;
  };

  class DataType {
  public:
    DataType() :
      KeepData(false),
      IsExecutable(false),
      DownloadToCache(false) {}
    std::string Name;
    bool KeepData;
    bool IsExecutable;
    bool DownloadToCache;
    std::list<URL> DataIndexingService;
    std::list<DataSourceType> Source;
    std::list<DataTargetType> Target;
  };

  class FileType : public DataType {
  public:
    FileType() {}
  };
  class DirectoryType : public DataType {
  public:
    DirectoryType() {}
  };

  class DataStagingType {
  public:
    DataStagingType() : File(), Directory() {}
    std::list<FileType> File;
    std::list<DirectoryType> Directory;
  };

  class JobMetaType {
  public:
    JobMetaType() :
      DocumentExpiration(-1),
      FuzzyRank(false) {}
    std::string Author;
    Time DocumentExpiration;
    std::string Rank;
    bool FuzzyRank;
  };

  class JobDescription {
  public:
    friend class JobDescriptionParser;
    JobDescription() : alternatives(), current(alternatives.begin()) {};

    JobDescription(const JobDescription& j, bool withAlternatives = true);

    JobDescription& operator=(const JobDescription& j);

    // Language wrapper constructor
    JobDescription(const long int& ptraddr);

    ~JobDescription() {}

    void AddAlternative(const JobDescription& j);

    bool HasAlternatives() const { return !alternatives.empty(); }
    const std::list<JobDescription>& GetAlternatives() const { return alternatives; }
    std::list<JobDescription>& GetAlternatives() { return alternatives; }
    std::list<JobDescription>  GetAlternativesCopy() const { return alternatives; }
    bool UseAlternative();
    void UseOriginal();

    void RemoveAlternatives();

    /// DEPRECATED: Check whether JobDescription is valid.
    /**
     * The JobDescription class itself is not able to tell whether its objects
     * are valid or not. Instead when parsing/outputting, JobDescriptionParser
     * classes checks the validity. Thus the Parse and UnParse methods should be
     * used for this purpose.
     **/
    operator bool() const;

    /// Parse string into JobDescription objects
    /**
     * The passed string will be tried parsed into the list of JobDescription
     * objects. The available specialized JobDesciptionParser classes will be
     * tried one by one, parsing the string, and if one succeeds the list of
     * JobDescription objects is filled with the parsed contents and true is
     * returned, otherwise false is returned. If no language specified, each
     * JobDescriptionParser will try all its supported languages. On the other
     * hand if a language is specified, only the JobDescriptionParser supporting
     * that language will be tried. A dialect can also be specified, which only
     * has an effect on the parsing if the JobDescriptionParser supports that
     * dialect.
     *
     * @param source
     * @param jobdescs
     * @param language
     * @param dialect
     * @return true if the passed string can be parsed successfully by any of
     *   the available parsers.
     **/
    static bool Parse(const std::string& source, std::list<JobDescription>& jobdescs, const std::string& language = "", const std::string& dialect = "");

    /// DEPRECATED: Parse source string
    /**
     * This method is deprecated, use the Parse(const std::string&, std::list<JobDescription>&, const std::string&, const std::string&)
     * method instead.
     **/
    bool Parse(const std::string& source, const std::string& language = "", const std::string& dialect = "");

    /// DEPRECATED: Parse source string
    /**
     * This method is deprecated, use the Parse(const std::string&, std::list<JobDescription>&, const std::string&, const std::string&)
     * method instead.
     **/
    bool Parse(const XMLNode& xmlSource);

    /// DEPRECATED: Output contents in the specified language
    /**
     * This method is deprecated, use the UnParse(std::string&, std::string, const std::string&)
     * method instead.
     **/
    std::string UnParse(const std::string& language = "nordugrid:jsdl") const;

    /// Output contents in the specified language
    /**
     *
     * @param product
     * @param language
     * @param dialect
     * @return
     **/
    bool UnParse(std::string& product, std::string language, const std::string& dialect = "") const;


    /// Get input source language
    /**
     * If this object was created by a JobDescriptionParser, then this method
     * returns a string which indicates the job description language of the
     * parsed source. If not created by a JobDescripionParser the string
     * returned is empty.
     *
     * @return const std::string& source langauge of parsed input source.
     **/
    const std::string& GetSourceLanguage() const { return sourceLanguage; }

    /// DEPRECATED: Print all values to standard output.
    /**
     * This method is DEPRECATED, use the SaveToStream method instead.
     *
     * @param longlist
     * @see SaveToStream
     */
    void Print(bool longlist = false) const;

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
    bool SaveToStream(std::ostream& out, const std::string& format) const;

    JobIdentificationType Identification;
    ApplicationType Application;
    ResourcesType Resources;
    DataStagingType DataStaging;
    JobMetaType JobMeta;

    /// Holds attributes not fitting into this class
    /**
     * This member is used by JobDescriptionParser classes to store
     * attribute/value pairs not fitting into attributes stored in this class.
     * The form of the attribute (the key in the map) should be as follows:
     * <language>;<attribute-name>
     * E.g.: "nordugrid:xrsl;hostname".
     **/
    std::map<std::string, std::string> OtherAttributes;

  private:
    void Set(const JobDescription& j);

    std::string sourceLanguage;

    std::list<JobDescription> alternatives;
    std::list<JobDescription>::iterator current;

    static Glib::Mutex jdpl_lock;
    static JobDescriptionParserLoader jdpl;

    static Logger logger;
  };

} // namespace Arc

#endif // __ARC_JOBDESCRIPTION_H__
