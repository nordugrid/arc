// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_DATAPOINTGFAL_H__
#define __ARC_DATAPOINTGFAL_H__

#include <list>

#include <arc/Thread.h>
#include <arc/data/DataPointDirect.h>

namespace Arc {

  /**
   *
   * This class is a loadable module and cannot be used directly. The DataHandle
   * class loads modules at runtime and should be used instead of this.
   */
  class DataPointGFAL
    : public DataPointDirect {
  public:
    DataPointGFAL(const URL& url, const UserConfig& usercfg, PluginArgument* parg);
    virtual ~DataPointGFAL();
    static Plugin* Instance(PluginArgument *arg);
    virtual DataStatus StartReading(DataBuffer& buffer);
    virtual DataStatus StopReading();
    virtual DataStatus StartWriting(DataBuffer& buffer, DataCallback *space_cb = NULL);
    virtual DataStatus StopWriting();
    virtual DataStatus Check();
    virtual DataStatus Stat(FileInfo& file, DataPointInfoType verb = INFO_TYPE_ALL);
    virtual DataStatus List(std::list<FileInfo>& files, DataPointInfoType verb = INFO_TYPE_ALL);
    virtual DataStatus Remove();
    virtual DataStatus CreateDirectory(bool with_parents=false);
  private:
    static void read_file_start(void *object);
    void read_file();
    static void write_file_start(void *object);
    void write_file();
    static Logger logger;
    int fd;
    bool reading;
    bool writing;
    SimpleCondition transfer_condition;
  };

} // namespace Arc

#endif // __ARC_DATAPOINTGFAL_H__
