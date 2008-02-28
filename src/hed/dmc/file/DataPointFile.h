#ifndef __ARC_DATAPOINTFILE_H__
#define __ARC_DATAPOINTFILE_H__

#include <string>
#include <list>

#include <arc/Thread.h>

#include <arc/data/DataPointDirect.h>

namespace Arc {

  class DataPointFile : public DataPointDirect {
   private:
    int fd;
    bool start_reading_file(DataBufferPar& buffer);
    bool start_writing_file(DataBufferPar& buffer, DataCallback *space_cb);
    bool stop_reading_file(void);
    bool stop_writing_file(void);
    bool check_file(void);
    bool remove_file(void);
    void read_file();
    void write_file();
    SimpleCondition file_thread_exited;
    bool is_channel;
    static Logger logger;
   protected:
    virtual bool init_handle(void);
    virtual bool deinit_handle(void);
   public:
    DataPointFile(const URL& url);
    virtual ~DataPointFile(void);
    virtual bool start_reading(DataBufferPar& buffer);
    virtual bool start_writing(DataBufferPar& buffer,
                               DataCallback *space_cb = NULL);
    virtual bool stop_reading(void);
    virtual bool stop_writing(void);
    virtual bool analyze(analyze_t& arg);
    virtual bool check(void);
    virtual bool remove(void);
    virtual bool list_files(std::list<FileInfo>& files, bool resolve = true);
    virtual bool out_of_order(void);
  };

} // namespace Arc

#endif // __ARC_DATAPOINTFILE_H__
