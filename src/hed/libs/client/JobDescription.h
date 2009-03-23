// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_JOBDESCRIPTION_H__
#define __ARC_JOBDESCRIPTION_H__

#include <list>
#include <string>

#include <arc/DateTime.h>
#include <arc/XMLNode.h>
#include <arc/URL.h>

namespace Arc {

  class Logger;

  struct ReferenceTimeType {
    std::string benchmark;
    double value;
    Period time;
  };

  struct EnvironmentType {
    std::string name_attribute;
    std::string value;
  };

  struct XLogueType {
    std::string Name;
    std::list<std::string> Arguments;
  };

  struct RunTimeEnvironmentType {
    std::string Name;
    std::list<std::string> Version;
  };

  struct NotificationType {
    std::list<std::string> Address;
    std::list<std::string> State;
  };

  struct SourceType {
    URL URI;
    int Threads;
  };

  struct TargetType {
    URL URI;
    int Threads;
    bool Mandatory;
    int NeededReplicas;
  };

  struct FileType {
    std::string Name;
    std::list<SourceType> Source;
    std::list<TargetType> Target;
    bool KeepData;
    bool IsExecutable;
    URL DataIndexingService;
    bool DownloadToCache;
  };

  struct DirectoryType {
    std::string Name;
    std::list<SourceType> Source;
    std::list<TargetType> Target;
    bool KeepData;
    bool IsExecutable;
    URL DataIndexingService;
    bool DownloadToCache;
  };

  struct OptionalElementType {
    std::string Name;
    std::string Path;
    std::string Value;
  };

  class JobDescription {
  public:
    JobDescription();

    ~JobDescription();

    operator bool() const {
      return (!Executable.empty());
    }

    // Try to parse the source string and store it.
    bool Parse(const std::string& source);

    // Transform the inner job description representation into a given format,
    // if it's known as a parser (JSDL as default)
    // If there is some error during this method, then return empty string.
    std::string UnParse(const std::string& format = "POSIXJSDL") const;

    // Returns with the original job descriptions format as a string. Right now, this value is one of the following:
    // "jsdl", "jdl", "xrsl". If there is an other parser written for another language, then this set can be extended.
    bool getSourceFormat(std::string& _sourceFormat) const;

    // Returns with true and the uploadable local files list, if the source
    // has been set up and is valid, else return with false.
    bool getUploadableFiles(std::list<std::pair<std::string, std::string> >&
                            sourceFiles) const;

    // Add an URL to OldJobIDs
    bool addOldJobID(const URL& oldjobid);

    // Print all value to the standard output.
    void Print(bool longlist = false) const;

    std::string sourceFormat;

    // Meta information
    std::string Author;
    Time DocumentExpiration;
    std::string Rank;
    bool FuzzyRank;

    // JobIdentification information
    std::string JobName;
    std::string Description;
    std::string JobProject;
    std::string JobType;
    std::string JobCategory;
    std::list<std::string> UserTag;

    // Application information
    std::string Executable;
    std::list<std::string> Argument;
    std::string Input;
    std::string Output;
    std::string Error;
    std::string LogDir;
    URL RemoteLogging;
    std::list<EnvironmentType> Environment;
    int LRMSReRun;
    XLogueType Prologue;
    XLogueType Epilogue;
    Period SessionLifeTime;
    XMLNode AccessControl;
    Time ProcessingStartTime;
    std::list<NotificationType> Notification;
    URL CredentialService;
    bool Join;

    // Resource information
    Period TotalCPUTime;
    Period IndividualCPUTime;
    Period TotalWallTime;
    Period IndividualWallTime;
    std::list<ReferenceTimeType> ReferenceTime;
    bool ExclusiveExecution;
    std::string NetworkInfo;
    std::string OSFamily;
    std::string OSName;
    std::string OSVersion;
    std::string Platform;
    int IndividualPhysicalMemory;
    int IndividualVirtualMemory;
    int DiskSpace;
    int CacheDiskSpace;         //DiskSpace
    int SessionDiskSpace;       //DiskSpace
    int IndividualDiskSpace;    //DiskSpace
    std::string Alias;          //CandidateTarget
    URL EndPointURL;            //CandidateTarget
    std::string QueueName;      //CandidateTarget
    std::string Country;        //Location
    std::string Place;          //Location
    std::string PostCode;       //Location
    std::string Latitude;       //Location
    std::string Longitude;      //Location
    std::string CEType;
    int Slots;
    int NumberOfProcesses;      //Slots
    int ProcessPerHost;         //Slots
    int ThreadPerProcesses;     //Slots
    std::string SPMDVariation;  //Slots
    std::list<RunTimeEnvironmentType> RunTimeEnvironment;
    bool Homogeneous;
    bool InBound;               //NodeAccess
    bool OutBound;              //NodeAccess

    // DataStaging information
    std::list<FileType> File;
    std::list<DirectoryType> Directory;

    //other specified elements
    std::map<std::string, std::string> XRSL_elements;
    std::map<std::string, std::string> JDL_elements;
    bool cached;                // It used the XRSL parsing method.
    std::list<OptionalElementType> OptionalElement;
    std::list<URL> OldJobIDs;

    static Logger logger;
  };

} // namespace Arc

#endif // __ARC_JOBDESCRIPTION_H__
