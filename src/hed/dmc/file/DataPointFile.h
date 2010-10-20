// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_DATAPOINTFILE_H__
#define __ARC_DATAPOINTFILE_H__

#include <list>

#include <arc/Thread.h>
#include <arc/data/DataPointDirect.h>

namespace Arc {

  class DataPointFile
    : public DataPointDirect {
  public:
    DataPointFile(const URL& url, const UserConfig& usercfg);
    virtual ~DataPointFile();
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
    virtual bool WriteOutOfOrder();
  private:
    SimpleCondition transfer_cond;
    static void read_file_start(void* arg);
    static void write_file_start(void* arg);
    void read_file();
    void write_file();
    bool reading;
    bool writing;
    int fd;
    bool is_channel;
    static Logger logger;
  };

} // namespace Arc

#endif // __ARC_DATAPOINTFILE_H__
