#ifndef __ARC_DATAPOINTXROOTD_H__
#define __ARC_DATAPOINTXROOTD_H__

#include <list>

#include <arc/data/DataPointDirect.h>

namespace Arc {

  /**
   * xrootd is a protocol for data access across large scale storage clusters.
   * More information can be found at http://xrootd.slac.stanford.edu/
   */
  class DataPointXrootd
    : public DataPointDirect {

   public:
    DataPointXrootd(const URL& url, const UserConfig& usercfg);
    virtual ~DataPointXrootd();
    static Plugin* Instance(PluginArgument *arg);
    virtual DataStatus StartReading(DataBuffer& buffer);
    virtual DataStatus StartWriting(DataBuffer& buffer,
                                    DataCallback *space_cb = NULL);
    virtual DataStatus StopReading();
    virtual DataStatus StopWriting();
    virtual DataStatus Check();
    virtual DataStatus Stat(FileInfo& file, DataPointInfoType verb = INFO_TYPE_ALL);
    virtual DataStatus List(std::list<FileInfo>& files, DataPointInfoType verb = INFO_TYPE_ALL);
    virtual DataStatus Remove();

   private:
    /// thread functions for async read/write
    static void read_file_start(void* arg);
    static void write_file_start(void* arg);
    void read_file();
    void write_file();

    /// must be called everytime a new XrdClient is created
    void set_log_level();

    SimpleCondition transfer_cond;
    XrdClient* client;
    bool reading;
    bool writing;
    static Logger logger;
  };

} // namespace Arc

#endif /* __ARC_DATAPOINTXROOTD_H__ */
