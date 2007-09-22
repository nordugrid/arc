#ifndef __ARC_DATAPOINTLFC_H__
#define __ARC_DATAPOINTLFC_H__

#include <list>

#include <arc/data/DataPointIndex.h>

namespace Arc {
  class Logger;
  class URL;

  class DataPointLFC : public DataPointIndex {
   public:
    DataPointLFC(const URL& url);
    ~DataPointLFC() {};
    virtual bool meta_resolve(bool source);
    virtual bool meta_preregister(bool replication, bool force = false);
    virtual bool meta_postregister(bool replication);
    virtual bool meta_preunregister(bool replication);
    virtual bool meta_unregister(bool all);
    virtual bool list_files(std::list<FileInfo> &files, bool resolve = true);
   protected:
    static Logger logger;
    std::string guid;
  };

} // namespace Arc

#endif // __ARC_DATAPOINTRLS_H__
