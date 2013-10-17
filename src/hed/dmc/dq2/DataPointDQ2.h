#ifndef __ARC_DATAPOINTDQ2_H__
#define __ARC_DATAPOINTDQ2_H__

#include <list>
#include <string>

#include <arc/Thread.h>
#include <arc/data/DataPointIndex.h>
#include <arc/data/DataHandle.h>

namespace ArcDMCDQ2 {

  using namespace Arc;

  /**
   * This index DataPoint resolves a DQ2 dataset and filename to a set of physical
   * replicas. The format of the URL for these files is dq2://cataloghost:port/dataset/lfn
   *
   * This class is a loadable module and cannot be used directly. The DataHandle
   * class loads modules at runtime and should be used instead of this.
   */
  class DataPointDQ2
    : public DataPointIndex {
  private:
    /// Cached DQ2 information to avoid look up
    class DQ2Cache {
    public:
      /// Map caching dataset -> duid information
      std::map<std::string, std::string> dataset_duid;
      /// Map caching dataset -> location information
      std::map<std::string, std::list<std::string> > dataset_locations;
      /// Validity period of location info
      const Arc::Period location_validity;
      /// Expiration time of location info. After this time the map is wiped
      Arc::Time location_expiration;
      DQ2Cache() : location_validity(24*60*60), location_expiration(Arc::Time() + location_validity) {}
    };
    /// global cache information
    static DQ2Cache dq2_cache;
    /// Lock for the cache in dq2_cache
    static Glib::Mutex dq2_cache_lock;
    static Logger logger;
    /// DQ2 catalog URL
    std::string catalog;
    std::string dataset;
    std::string scope;
    std::string lfn;
    /// Call DQ2 or use cache to get dataset locations
    DataStatus resolveLocations(std::list<std::string>& sites);
    /// Return json-formatted info from DQ2
    Arc::DataStatus queryDQ2(std::string& content, const std::string& method, const std::string& path, const std::string& post_data="") const;
    /// Make deterministic paths and add locations to DataPoint
    void makePaths(const std::list<std::string>& endpoints);
  public:
    DataPointDQ2(const URL& url, const UserConfig& usercfg, PluginArgument* parg);
    virtual ~DataPointDQ2();
    static Plugin* Instance(PluginArgument *arg);
    virtual Arc::DataStatus Resolve(bool source);
    virtual Arc::DataStatus Resolve(bool source, const std::list<DataPoint*>& urls);
    virtual Arc::DataStatus Check(bool check_meta);
    virtual Arc::DataStatus PreRegister(bool replication, bool force = false) { return DataStatus(DataStatus::PreRegisterError, EOPNOTSUPP); };
    virtual Arc::DataStatus PostRegister(bool replication) { return DataStatus(DataStatus::PostRegisterError, EOPNOTSUPP); };
    virtual Arc::DataStatus PreUnregister(bool replication) { return DataStatus(DataStatus::UnregisterError, EOPNOTSUPP); };
    virtual Arc::DataStatus Unregister(bool all) { return DataStatus(DataStatus::UnregisterError, EOPNOTSUPP); };
    virtual Arc::DataStatus Stat(Arc::FileInfo& file, Arc::DataPoint::DataPointInfoType verb = INFO_TYPE_ALL);
    virtual Arc::DataStatus Stat(std::list<Arc::FileInfo>& files,
                                 const std::list<Arc::DataPoint*>& urls,
                                 Arc::DataPoint::DataPointInfoType verb = INFO_TYPE_ALL);
    virtual Arc::DataStatus List(std::list<Arc::FileInfo>& files, Arc::DataPoint::DataPointInfoType verb = INFO_TYPE_ALL) { return DataStatus(DataStatus::ListError, EOPNOTSUPP); };
    virtual Arc::DataStatus CreateDirectory(bool with_parents=false) { return DataStatus(DataStatus::CreateDirectoryError, EOPNOTSUPP); };
    virtual Arc::DataStatus Rename(const Arc::URL& newurl) { return DataStatus(DataStatus::RenameError, EOPNOTSUPP); };
    virtual const std::string DefaultCheckSum() const { return "adler32"; }
    virtual bool RequiresCredentialsInFile() const { return false; }
  };

  /// Class to handle loading AGIS information.
  /** This class is a singleton so that only one global instance can exist. On
   * creation of the instance all AGIS information is downloaded to memory.
   * Whenever the instance is accessed the timestamp of the cached information
   * is checked, and if it is past the expiry time new information is downloaded.
   * During this time a lock is held to allow only one thread to download at
   * once. */
  class AGISInfo {
  private:
    /// Single instance of this class
    static AGISInfo* instance;
    /// File to save and read information from. If empty data is only in memory
    std::string cache_file;
    /// The information held about storage endpoints. site -> endpoint
    // TODO: only highest priority endpoint is used, should use all?
    std::map<std::string, std::string> endpoint_info;
    /// Non-deterministic sites, which will not be used
    std::list<std::string> nondeterministic_sites;
    /// Lock for protecting mutli-threaded access
    static Glib::Mutex lock;
    /// Time at which the current information expires
    Arc::Time expiry_time;
    /// Timeout (seconds) for downloading info from AGIS server
    int timeout;
    /// Whether instance is valid (contains any information)
    bool valid;
    /// Logger
    static Logger logger;
    /// Length of time for which new information will be valid
    static const Arc::Period info_lifetime;
    /// Private constructor
    AGISInfo(int timeout, const std::string& cache_file);
    /// Get info from AGIS and parse it
    bool getAGISInfo();
    /// Download info in json format from AGIS and return it
    std::string downloadAGISInfo();
    /// Parse the json info into endpoint_info
    bool parseAGISInfo(const std::string& json);
  public:
    /// Deletes singleton instance on destruction
    ~AGISInfo();
    /// Get singleton instance. If necessary downloads new information. If
    /// cache_file is defined then read/save information to file
    static AGISInfo* getInstance(int timeout, const std::string& cache_file="");
    /// Return a list of storage endpoints for the given sites
    std::list<std::string> getStorageEndpoints(const std::list<std::string>& site);
  };


} // namespace ArcDMCDQ2

#endif // __ARC_DATAPOINTDQ2_H__
