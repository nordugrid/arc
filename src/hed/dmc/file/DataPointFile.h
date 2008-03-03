#ifndef __ARC_DATAPOINTFILE_H__
#define __ARC_DATAPOINTFILE_H__

#include <list>

#include <arc/data/DataPointDirect.h>

namespace Arc {

  class DataPointFile : public DataPointDirect {
   public:
    DataPointFile(const URL& url);
    virtual ~DataPointFile();
    virtual bool start_reading(DataBufferPar& buffer);
    virtual bool start_writing(DataBufferPar& buffer,
                               DataCallback *space_cb = NULL);
    virtual bool stop_reading();
    virtual bool stop_writing();
    virtual bool check();
    virtual bool remove();
    virtual bool list_files(std::list<FileInfo>& files, bool resolve = true);
    virtual bool out_of_order();
   private:
    void read_file();
    void write_file();
    int fd;
    bool is_channel;
    static Logger logger;
  };

} // namespace Arc

#endif // __ARC_DATAPOINTFILE_H__
