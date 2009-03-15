// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_DATAPOINTHTTP_H__
#define __ARC_DATAPOINTHTTP_H__

#include <list>
#include <string>

#include <arc/Thread.h>
#include <arc/data/DataPointDirect.h>

namespace Arc {

  class URL;
  class ChunkControl;

  class DataPointHTTP
    : public DataPointDirect {
  private:
    static Logger logger;
    unsigned int transfer_chunk_size;
    ChunkControl *chunks;
    int transfers_started;
    int transfers_finished;
    Glib::Mutex transfer_lock;
    static void read_thread(void *arg);
    static void write_thread(void *arg);
  public:
    DataPointHTTP(const URL& url);
    virtual ~DataPointHTTP();
    DataStatus Check();
    DataStatus Remove();
    DataStatus ListFiles(std::list<FileInfo>& files, bool long_list = false, bool resolve = false);
    DataStatus StartReading(DataBuffer& buffer);
    DataStatus StartWriting(DataBuffer& buffer,
                            DataCallback *space_cb = NULL);
    DataStatus StopReading();
    DataStatus StopWriting();
  };

} // namespace Arc

#endif // __ARC_DATAPOINTHTTP_H__
