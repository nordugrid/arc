// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_DATAPOINTLFC_H__
#define __ARC_DATAPOINTLFC_H__

#include <list>

#include <arc/data/DataPointIndex.h>

namespace Arc {
  class Logger;
  class URL;

  class DataPointLFC
    : public DataPointIndex {
  public:
    DataPointLFC(const URL& url);
    ~DataPointLFC();
    virtual DataStatus Resolve(bool source);
    virtual DataStatus PreRegister(bool replication, bool force = false);
    virtual DataStatus PostRegister(bool replication);
    virtual DataStatus PreUnregister(bool replication);
    virtual DataStatus Unregister(bool all);
    virtual DataStatus ListFiles(std::list<FileInfo>& files, bool long_list = false, bool resolve = false, bool metadata = false);
  protected:
    static Logger logger;
    std::string guid;
  private:
    bool resolveGUIDToLFN();
  };

} // namespace Arc

#endif // __ARC_DATAPOINTRLS_H__
