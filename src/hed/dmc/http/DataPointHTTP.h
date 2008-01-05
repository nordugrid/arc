#ifndef __ARC_DATAPOINTHTTP_H__
#define __ARC_DATAPOINTHTTP_H__

#include <string>
#include <list>

#include <arc/Thread.h>
#include <arc/data/DataPointDirect.h>

namespace Arc {

  class URL;
  class ChunkControl;

  class DataPointHTTP : public DataPointDirect {
   private:
    static Logger logger;
    unsigned int transfer_chunk_size;
    ChunkControl* chunks;
    int transfer_threads;
    Glib::Mutex transfer_lock;
    static void read_thread(void *arg);
    static void write_thread(void *arg);
   public:
    DataPointHTTP(const URL& url);
    virtual ~DataPointHTTP();
    virtual bool list_files(std::list<FileInfo>& files, bool resolve = true);
    virtual bool start_reading(DataBufferPar& buffer);
    virtual bool start_writing(DataBufferPar& buffer,
                               DataCallback *space_cb = NULL);
    virtual bool stop_reading(void);
    virtual bool stop_writing(void);
  };

} // namespace Arc

#endif // __ARC_DATAPOINTHTTP_H__
