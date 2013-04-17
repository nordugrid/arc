// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_DATAPOINTLFC_H__
#define __ARC_DATAPOINTLFC_H__

#include <list>

#include <arc/Logger.h>
#include <arc/URL.h>
#include <arc/data/DataPointIndex.h>

namespace ArcDMCLFC {

  /**
   * The LCG File Catalog (LFC) is a replica catalog developed by CERN. It
   * consists of a hierarchical namespace of grid files and each filename
   * can be associated with one or more physical locations.
   *
   * This class is a loadable module and cannot be used directly. The DataHandle
   * class loads modules at runtime and should be used instead of this.
   */
  class DataPointLFC
    : public Arc::DataPointIndex {
  public:
    DataPointLFC(const Arc::URL& url, const Arc::UserConfig& usercfg, Arc::PluginArgument* parg);
    ~DataPointLFC();
    static Plugin* Instance(Arc::PluginArgument *arg);
    virtual Arc::DataStatus Resolve(bool source);
    virtual Arc::DataStatus Resolve(bool source, const std::list<DataPoint*>& urls);
    virtual Arc::DataStatus Check(bool check_meta);
    virtual Arc::DataStatus PreRegister(bool replication, bool force = false);
    virtual Arc::DataStatus PostRegister(bool replication);
    virtual Arc::DataStatus PreUnregister(bool replication);
    virtual Arc::DataStatus Unregister(bool all);
    virtual Arc::DataStatus Stat(Arc::FileInfo& file, Arc::DataPoint::DataPointInfoType verb = INFO_TYPE_ALL);
    virtual Arc::DataStatus Stat(std::list<Arc::FileInfo>& files,
                            const std::list<Arc::DataPoint*>& urls,
                            Arc::DataPoint::DataPointInfoType verb = INFO_TYPE_ALL);
    virtual Arc::DataStatus List(std::list<Arc::FileInfo>& files, Arc::DataPoint::DataPointInfoType verb = INFO_TYPE_ALL);
    virtual Arc::DataStatus CreateDirectory(bool with_parents=false);
    virtual Arc::DataStatus Rename(const Arc::URL& newurl);
    //virtual Arc::DataStatus ListFiles(std::list<FileInfo>& files, bool long_list = false, bool resolve = false, bool metadata = false);
    virtual const std::string DefaultCheckSum() const;
    virtual std::string str() const;
    virtual bool RequiresCredentialsInFile() const;
  protected:
    static Arc::Logger logger;
    std::string guid;
    std::string path_for_guid;
  private:
    std::string ResolveGUIDToLFN();
    int error_no;
    /// Convert serrno to Arc::DataStatus errno
    int lfc2errno() const;
    /// Convert serrno to string, only if it is not a regular errno.
    std::string lfcerr2str() const;
    Arc::DataStatus ListFiles(std::list<Arc::FileInfo>& files, Arc::DataPoint::DataPointInfoType verb, bool listdir);

  };

} // namespace Arc

#endif // __ARC_DATAPOINTLFC_H__
