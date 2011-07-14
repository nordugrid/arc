// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_DATAPOINTRLS_H__
#define __ARC_DATAPOINTRLS_H__

#include <list>
#include <string>

extern "C" {
#include <globus_rls_client.h>
}

#include <arc/data/DataPointIndex.h>

namespace Arc {
  class Logger;
  class URL;

  /**
   * The Replica Location Service (RLS) is a replica catalog developed by
   * Globus. It maps filenames in a flat namespace to one or more physical
   * locations, and can also store meta-information on each file. This class
   * uses the Globus Toolkit libraries for accessing RLS.
   */
  class DataPointRLS
    : public DataPointIndex {
  public:
    DataPointRLS(const URL& url, const UserConfig& usercfg);
    ~DataPointRLS();
    static Plugin* Instance(PluginArgument *arg);
    virtual DataStatus Resolve(bool source);
    virtual DataStatus Check();
    virtual DataStatus PreRegister(bool replication, bool force = false);
    virtual DataStatus PostRegister(bool replication);
    virtual DataStatus PreUnregister(bool replication);
    virtual DataStatus Unregister(bool all);
    virtual DataStatus Stat(FileInfo& file, DataPointInfoType verb = INFO_TYPE_ALL);
    virtual DataStatus List(std::list<FileInfo>& files, DataPointInfoType verb = INFO_TYPE_ALL);
    bool ResolveCallback(globus_rls_handle_t *h, const URL& url, void *arg);
    bool ListFilesCallback(globus_rls_handle_t *h, const URL& url, void *arg);
    bool UnregisterCallback(globus_rls_handle_t *h, const URL& url, void *arg);
  private:
    static Logger logger;
    bool guid_enabled;
    std::string pfn_path;
    URL AddPFN(const URL& url, bool source);
  };

} // namespace Arc

#endif // __ARC_DATAPOINTRLS_H__
