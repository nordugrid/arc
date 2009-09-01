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
    ScalableTime<T>() : benchmark("", -1) {}
    ScalableTime<T>(const T& t) : range(t) {}

    std::pair<std::string, int> benchmark;
    Range<T> range;
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
    std::string Name;
    std::list<std::string> Argument;
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
    std::list<std::string> Notification;
    std::list<URL> CredentialService;
    XMLNode AccessControl;
  };

  class ResourceSlotType {
  public:
    ResourceSlotType() :
      NumberOfProcesses(-1),
      ProcessPerHost(-1),
      ThreadsPerProcesses(-1) {}
    Range<int> NumberOfProcesses;
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
    URL EndPointURL;
    std::string QueueName;
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

  class FileType : public DataType {};
  class DirectoryType : public DataType {};

  class DataStagingType {
  public:
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
    JobDescription() {};

    // Language wrapper constructor
    JobDescription(const long int& ptraddr);

    operator bool() const {
      return (!Application.Executable.Name.empty());
    }

    // Try to parse the source string and store it.
    bool Parse(const std::string& source);
    bool Parse(const XMLNode& xmlSource);

    // Transform the job description representation into a given format,
    // if it's known as a parser (JSDL as default)
    // If there is some error during this method, then return empty string.
    std::string UnParse(const std::string& format = "ARCJSDL") const;

    // Returns with the original job descriptions format as a string. Right now, this value is one of the following:
    // "jsdl", "jdl", "xrsl". If there is an other parser written for another language, then this set can be extended.
    bool getSourceFormat(std::string& _sourceFormat) const;

    // Print all value to the standard output.
    void Print(bool longlist = false) const;

    JobIdentificationType Identification;
    ApplicationType Application;
    ResourcesType Resources;
    DataStagingType DataStaging;
    JobMetaType JobMeta;

    // Other elements which is needed for submission using the respective languages.
    std::map<std::string, std::string> XRSL_elements;
    std::map<std::string, std::string> JDL_elements;

  private:
    std::string sourceFormat;

    static Logger logger;
  };

} // namespace Arc

#endif // __ARC_JOBDESCRIPTION_H__
