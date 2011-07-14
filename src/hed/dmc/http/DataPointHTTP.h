// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_DATAPOINTHTTP_H__
#define __ARC_DATAPOINTHTTP_H__

#include <arc/Thread.h>
#include <arc/data/DataPointDirect.h>

namespace Arc {

  class ChunkControl;

  /**
   * This class allows access through HTTP to remote resources. HTTP over SSL
   * (HTTPS) and HTTP over GSI (HTTPG) are also supported.
   */
  class DataPointHTTP
    : public DataPointDirect {
  public:
    DataPointHTTP(const URL& url, const UserConfig& usercfg);
    virtual ~DataPointHTTP();
    static Plugin* Instance(PluginArgument *arg);
    virtual bool SetURL(const URL& url);
    virtual DataStatus Check();
    virtual DataStatus Remove();
    virtual DataStatus Stat(FileInfo& file, DataPointInfoType verb = INFO_TYPE_ALL);
    virtual DataStatus List(std::list<FileInfo>& files, DataPointInfoType verb = INFO_TYPE_ALL);
    virtual DataStatus StartReading(DataBuffer& buffer);
    virtual DataStatus StartWriting(DataBuffer& buffer, DataCallback *space_cb = NULL);
    virtual DataStatus StopReading();
    virtual DataStatus StopWriting();
  private:
    static void read_thread(void *arg);
    static void write_thread(void *arg);
    static Logger logger;
    ChunkControl *chunks;
    SimpleCounter transfers_started;
    int transfers_tofinish;
    Glib::Mutex transfer_lock;
  };

} // namespace Arc

#endif // __ARC_DATAPOINTHTTP_H__
