#ifndef __ARC_DATAPOINTRLS_H__
#define __ARC_DATAPOINTRLS_H__

#include <string>
#include <list>

#include "data/DataPoint.h"

extern "C" {
#include <globus_rls_client.h>
}

namespace Arc {
  class Logger;
  class URL;

  class DataPointRLS : public DataPointIndex {
   public:
    DataPointRLS(const URL& url);
    ~DataPointRLS() {};
    virtual bool meta_resolve(bool source);
    virtual bool meta_preregister(bool replication, bool force = false);
    virtual bool meta_postregister(bool replication, bool failure);
    virtual bool meta_preunregister(bool replication);
    virtual bool meta_unregister(bool all);
    virtual bool list_files(std::list<FileInfo> &files, bool resolve = true);
   protected:
    static Logger logger;
    bool guid_enabled;
    std::string pfn_path;
    static bool meta_resolve_callback(globus_rls_handle_t *h,
				      const URL& url, void *arg);
    static bool list_files_callback(globus_rls_handle_t *h,
				    const URL& url, void *arg);
    static bool meta_unregister_callback(globus_rls_handle_t *h,
					 const URL& url,void *arg);
  };

} // namespace Arc

#endif // __ARC_DATAPOINTRLS_H__
