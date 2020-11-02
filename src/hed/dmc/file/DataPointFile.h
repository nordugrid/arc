// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_DATAPOINTFILE_H__
#define __ARC_DATAPOINTFILE_H__

#include <list>

#include <arc/Thread.h>
#include <arc/data/DataPointDirect.h>

namespace ArcDMCFile {

  using namespace Arc;

  /**
   * This class allows access to the regular local filesystem through the
   * same interface as is used for remote storage on the grid.
   *
   * This class is a loadable module and cannot be used directly. The DataHandle
   * class loads modules at runtime and should be used instead of this.
   */
  class DataPointFile
    : public DataPointDirect {
  public:
    DataPointFile(const URL& url, const UserConfig& usercfg, PluginArgument* parg);
    virtual ~DataPointFile();
    static Plugin* Instance(PluginArgument *arg);
    virtual DataStatus StartReading(DataBuffer& buffer);
    virtual DataStatus StartWriting(DataBuffer& buffer,
                                    DataCallback *space_cb = NULL);
    virtual DataStatus StopReading();
    virtual DataStatus StopWriting();
    virtual DataStatus Check(bool check_meta);
    virtual DataStatus Stat(FileInfo& file, DataPointInfoType verb = INFO_TYPE_ALL);
    virtual DataStatus List(std::list<FileInfo>& files, DataPointInfoType verb = INFO_TYPE_ALL);
    virtual DataStatus Remove();
    virtual DataStatus CreateDirectory(bool with_parents=false);
    virtual DataStatus Rename(const URL& newurl);
    virtual bool WriteOutOfOrder() const;
    virtual bool RequiresCredentials() const { return false; };
  private:
    SimpleCounter transfers_started;
    int open_channel();
    static void read_file_start(void* arg);
    static void write_file_start(void* arg);
    void read_file();
    void write_file();
    bool reading;
    bool writing;
    int fd;
    FileAccess* fa;
    bool is_channel;
    unsigned int channel_num;
    static Logger logger;
  };

} // namespace Arc

#endif // __ARC_DATAPOINTFILE_H__
