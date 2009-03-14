#ifndef __ARC_JOBINNERREPRESENTATION_H__
#define __ARC_JOBINNERREPRESENTATION_H__

#include <string>
#include <map>

#include <arc/DateTime.h>
#include <arc/URL.h>
#include <arc/XMLNode.h>
#include <arc/ws-addressing/WSA.h>

namespace Arc {

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
    std::list<Arc::SourceType> Source;
    std::list<Arc::TargetType> Target;
    bool KeepData;
    bool IsExecutable;
    URL DataIndexingService;
    bool DownloadToCache;
};

struct DirectoryType {
    std::string Name;
    std::list<Arc::SourceType> Source;
    std::list<Arc::TargetType> Target;
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

  class JobInnerRepresentation {

  public:

    JobInnerRepresentation();
    //~JobInnerRepresentation();

    // Copy constructor
    JobInnerRepresentation(const JobInnerRepresentation&);

    // Special language wrapper constructor
    JobInnerRepresentation(const long int ptraddr);

    // Print all value to the standard output.
    void Print(bool longlist) const;
    void Reset();

    // Export the inner representation into the jobTree XMLNode.
    bool getXML( Arc::XMLNode& jobTree) const;

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
    std::list<Arc::EnvironmentType> Environment;
    int LRMSReRun;
    Arc::XLogueType Prologue;
    Arc::XLogueType Epilogue;
    Period SessionLifeTime;
    Arc::XMLNode AccessControl;
    Time ProcessingStartTime;
    std::list<Arc::NotificationType> Notification;
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
    int ProcessPerHost;	        //Slots
    int ThreadPerProcesses;     //Slots
    std::string SPMDVariation;  //Slots
    std::list<Arc::RunTimeEnvironmentType> RunTimeEnvironment;
    bool Homogeneous;
    bool InBound;               //NodeAccess
    bool OutBound;              //NodeAccess

    // DataStaging information
    std::list<Arc::FileType> File;
    std::list<Arc::DirectoryType> Directory;

    //other specified elements
    std::map<std::string, std::string> XRSL_elements;
    std::map<std::string, std::string> JDL_elements;
    bool cached;                // It used the XRSL parsing method.
    std::list<Arc::OptionalElementType> OptionalElement;
    std::list<URL> OldJobIDs;
  };

} // namespace Arc

#endif // __ARC_JOBINNERREPRESENTATION_H__
