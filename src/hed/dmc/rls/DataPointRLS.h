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

  class DataPointRLS
    : public DataPointIndex {
  public:
    DataPointRLS(const URL& url);
    ~DataPointRLS();
    virtual DataStatus Resolve(bool source);
    virtual DataStatus PreRegister(bool replication, bool force = false);
    virtual DataStatus PostRegister(bool replication);
    virtual DataStatus PreUnregister(bool replication);
    virtual DataStatus Unregister(bool all);
    virtual DataStatus ListFiles(std::list<FileInfo>& files,
				 bool resolve = true);
    bool ResolveCallback(globus_rls_handle_t *h, const URL& url, void *arg);
    bool ListFilesCallback(globus_rls_handle_t *h, const URL& url, void *arg);
    bool UnregisterCallback(globus_rls_handle_t *h, const URL& url, void *arg);
  protected:
    static Logger logger;
    bool guid_enabled;
    std::string pfn_path;
  };

} // namespace Arc

#endif // __ARC_DATAPOINTRLS_H__
