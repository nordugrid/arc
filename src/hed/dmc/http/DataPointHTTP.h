// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_DATAPOINTHTTP_H__
#define __ARC_DATAPOINTHTTP_H__

#include <arc/Thread.h>
#include <arc/data/DataPointDirect.h>

namespace Arc {

  class ChunkControl;

  class DataPointHTTP
    : public DataPointDirect {
  public:
    DataPointHTTP(const URL& url, const UserConfig& usercfg);
    virtual ~DataPointHTTP();
    static Plugin* Instance(PluginArgument *arg);
    DataStatus Check();
    DataStatus Remove();
    DataStatus Stat(FileInfo& file, DataPointInfoType verb = INFO_TYPE_ALL);
    DataStatus List(std::list<FileInfo>& files, DataPointInfoType verb = INFO_TYPE_ALL);
    DataStatus StartReading(DataBuffer& buffer);
    DataStatus StartWriting(DataBuffer& buffer, DataCallback *space_cb = NULL);
    DataStatus StopReading();
    DataStatus StopWriting();
  private:
    static void read_thread(void *arg);
    static void write_thread(void *arg);
    static Logger logger;
    ChunkControl *chunks;
    int transfers_started;
    int transfers_finished;
    Glib::Mutex transfer_lock;
    Glib::Cond transfer_cond;
  };

} // namespace Arc

#endif // __ARC_DATAPOINTHTTP_H__
